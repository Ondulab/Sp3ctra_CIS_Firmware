#!/usr/bin/env python3
"""Resynchronise the STM32CubeIDE-generated subdir.mk files with the sources on disk.

CubeIDE writes one subdir.mk per source folder (Release/<dir>/subdir.mk) listing
every .c file explicitly. Adding, renaming or deleting a source therefore
breaks `scripts/build.sh` until the IDE regenerates the makefiles. This script
rebuilds the C_SRCS / OBJS / C_DEPS lists and the clean rule from the actual
directory contents, keeping the per-file rules (custom optimisation levels)
and the generic pattern rule untouched. Idempotent.

Usage: scripts/sync_subdir_mk.py [--check] [--config Release|Debug] [cm7|cm4|bootloader|all]
"""
import os
import re
import sys
import glob

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CORES = {"cm7": "CM7", "cm4": "CM4", "bootloader": "CM7_Bootloader"}


def source_dir_of(mk_path, c_srcs):
    """The directory the listed sources live in (relative ../X/Y or absolute)."""
    if not c_srcs:
        return None
    first = c_srcs[0]
    d = os.path.dirname(first)
    if os.path.isabs(d):
        return d, True
    # relative paths are relative to the build root (the folder holding `makefile`)
    root = os.path.dirname(mk_path)
    while root and not os.path.isfile(os.path.join(root, "makefile")):
        parent = os.path.dirname(root)
        if parent == root:
            return None
        root = parent
    return os.path.normpath(os.path.join(root, d)), False


def sync(mk_path, check=False):
    with open(mk_path, "r", encoding="utf-8", errors="replace") as f:
        text = f.read()

    m = re.search(r"C_SRCS \+= \\\n((?:.*\\\n)*.*)\n", text)
    if not m:
        return None  # not a C source subdir (startup .s, HAL aggregates, ...)
    listed = [l.strip().rstrip("\\").strip() for l in m.group(1).split("\n")]
    listed = [l for l in listed if l]
    if any(not l.endswith(".c") for l in listed):
        return None

    info = source_dir_of(mk_path, listed)
    if info is None:
        return None
    src_dir, absolute = info
    if not os.path.isdir(src_dir):
        return None

    actual = sorted(glob.glob(os.path.join(src_dir, "*.c")))
    if absolute:
        srcs = actual
    else:
        rel_prefix = os.path.dirname(listed[0])  # e.g. ../Application/Src
        srcs = [rel_prefix + "/" + os.path.basename(p) for p in actual]

    if srcs == listed:
        return False  # already in sync

    # object dir prefix, e.g. ./Application/Src or ./Common/Src
    om = re.search(r"OBJS \+= \\\n(.*)", text)
    obj_prefix = os.path.dirname(om.group(1).strip().rstrip("\\").strip()) if om else "./" + os.path.basename(os.path.dirname(mk_path))
    stems = [os.path.splitext(os.path.basename(p))[0] for p in srcs]

    def block(var, items):
        return var + " += \\\n" + " \\\n".join(items) + " \n"

    new_lists = (block("C_SRCS", srcs) + "\n"
                 + block("OBJS", [f"{obj_prefix}/{s}.o" for s in stems]) + "\n"
                 + block("C_DEPS", [f"{obj_prefix}/{s}.d" for s in stems]) + "\n")

    text2 = re.sub(r"C_SRCS \+= \\\n(?:.*\n)*?C_DEPS \+= \\\n(?:.*\\\n)*.*\n\n", new_lists, text, count=1)

    # linked folders (absolute sources) have NO pattern rule: every file needs an
    # explicit rule -> clone the first existing one for the newcomers
    if absolute:
        rule_re = re.compile(r"(" + re.escape(obj_prefix.lstrip("./")) + r"/([\w\-]+)\.o): [^\n]*\n\t[^\n]*\n")
        rules = {m.group(2): m.group(0) for m in rule_re.finditer(text2)}
        if rules:
            tmpl_stem, tmpl = next(iter(rules.items()))
            missing = [st for st in stems if st not in rules]
            if missing:
                last = list(rule_re.finditer(text2))[-1]
                cloned = "".join(re.sub(r"(?<=/)" + re.escape(tmpl_stem) + r"(?=\.[A-Za-z]+)", st, tmpl) for st in missing)
                text2 = text2[:last.end()] + cloned + text2[last.end():]

    # drop explicit rules of files that no longer exist
    def keep_rule(mo):
        target = mo.group(1)
        stem = os.path.splitext(os.path.basename(target))[0]
        return mo.group(0) if stem in stems else ""
    text2 = re.sub(r"(" + re.escape(obj_prefix.lstrip("./")) + r"/[\w\-]+\.o): [^\n]*\n\t[^\n]*\n", keep_rule, text2)

    # clean rule
    clean_items = " ".join(f"{obj_prefix}/{s}.cyclo {obj_prefix}/{s}.d {obj_prefix}/{s}.o {obj_prefix}/{s}.su" for s in stems)
    text2 = re.sub(r"(\n\t-\$\(RM\) )[^\n]*", r"\g<1>" + clean_items.replace("\\", "\\\\"), text2, count=1)

    if check:
        print(f"[stale]  {os.path.relpath(mk_path, ROOT)}")
        return True
    with open(mk_path, "w", encoding="utf-8") as f:
        f.write(text2)
    added = [s for s in srcs if s not in listed]
    removed = [s for s in listed if s not in srcs]
    print(f"[synced] {os.path.relpath(mk_path, ROOT)}  +{len(added)} -{len(removed)}")
    for a in added: print(f"           + {os.path.basename(a)}")
    for r in removed: print(f"           - {os.path.basename(r)}")
    return True


def sync_objects_list(build_root, check=False):
    """objects.list feeds the final link (@"objects.list"): keep it equal to the union of OBJS."""
    path = os.path.join(build_root, "objects.list")
    if not os.path.isfile(path):
        return False
    wanted = []
    for mk in sorted(glob.glob(os.path.join(build_root, "**", "subdir.mk"), recursive=True)):
        with open(mk, "r", encoding="utf-8", errors="replace") as f:
            m = re.search(r"OBJS \+= \\\n((?:.*\\\n)*.*)\n", f.read())
        if m:
            wanted += [l.strip().rstrip("\\").strip() for l in m.group(1).split("\n") if l.strip()]
    with open(path, "r", encoding="utf-8") as f:
        current = [l.strip().strip('"') for l in f if l.strip()]
    wanted_set = set(wanted)
    kept = [o for o in current if o in wanted_set]
    missing = [o for o in wanted if o not in set(current)]
    # insert newcomers after the last object of the same directory (keeps CubeIDE's grouping)
    for o in missing:
        d = os.path.dirname(o)
        idx = max((i for i, k in enumerate(kept) if os.path.dirname(k) == d), default=None)
        kept.insert(idx + 1 if idx is not None else len(kept), o)
    if kept == current:
        return False
    if check:
        print(f"[stale]  {os.path.relpath(path, ROOT)}")
        return True
    with open(path, "w", encoding="utf-8") as f:
        f.write("".join(f'"{o}"\n' for o in kept))
    removed = [o for o in current if o not in wanted_set]
    print(f"[synced] {os.path.relpath(path, ROOT)}  +{len(missing)} -{len(removed)}")
    return True


def main(argv):
    check = "--check" in argv
    config = "Release"
    if "--config" in argv:
        config = argv[argv.index("--config") + 1]
    targets = [a for a in argv if a in CORES or a == "all"] or ["all"]
    cores = list(CORES.values()) if "all" in targets else [CORES[t] for t in targets]

    changed = 0
    for core in cores:
        base = os.path.join(ROOT, core, config)
        if not os.path.isdir(base):
            print(f"[skip]   {core}/{config} (not generated by CubeIDE yet)")
            continue
        for mk in sorted(glob.glob(os.path.join(base, "**", "subdir.mk"), recursive=True)):
            if "Drivers" in mk or "Middlewares" in mk or "Startup" in mk:
                continue
            r = sync(mk, check)
            if r:
                changed += 1
        if sync_objects_list(base, check):
            changed += 1
    if check and changed:
        print(f"{changed} subdir.mk out of date -> run without --check")
        return 1
    if not changed:
        print("all subdir.mk in sync")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
