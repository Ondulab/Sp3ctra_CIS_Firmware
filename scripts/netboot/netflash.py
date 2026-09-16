#!/usr/bin/env python3
"""Flashe le CIS par Ethernet, sans ST-Link, via le bootloader (docs/NETBOOT.md).

    scripts/netboot/netflash.py discover                     # qui est en mode flasheur ?
    scripts/netboot/netflash.py info --host 192.168.100.1
    scripts/netboot/netflash.py flash --cm7 CM7/Release/Sp3ctra_CIS_Firmware_CM7.bin \
                                      --cm4 CM4/Release/Sp3ctra_CIS_Firmware_CM4.bin
    scripts/netboot/netflash.py flash --at 0x080E0000 bootloader_b.bin --no-boot
    scripts/netboot/netflash.py log --follow                 # trace bootloader/CM7/CM4
    scripts/netboot/netflash.py boot

`flash` enchaine : POST /netboot a l'application (sauf --no-enter), attente du
bootloader, effacement secteur par secteur, ecriture, CRC relu en place, puis
BOOT (sauf --no-boot) et attente du retour de l'application (--wait-app).

Note macOS : le terminal doit avoir l'autorisation "Reseau local".
"""

import argparse
import http.client
import os
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from netboot import (NetBoot, NetBootError, Timeout, IMAGES, PHASE, PHASE_BY_NAME, SECTOR,  # noqa: E402
                     BL_SLOTS, image_bytes, sectors_covering)

DEFAULT_HOST = "192.168.100.1"


def log(msg):
    print(msg, flush=True)


def http_post(host, path, timeout=3.0):
    """POST sans corps ; retourne (statut, texte) ou None si injoignable."""
    try:
        c = http.client.HTTPConnection(host, 80, timeout=timeout)
        c.request("POST", path, body=b"", headers={"Content-Length": "0"})
        r = c.getresponse()
        return r.status, r.read().decode("utf-8", "replace")
    except (OSError, http.client.HTTPException):
        return None


def http_get(host, path, timeout=2.0):
    try:
        c = http.client.HTTPConnection(host, 80, timeout=timeout)
        c.request("GET", path)
        r = c.getresponse()
        return r.status, r.read().decode("utf-8", "replace")
    except (OSError, http.client.HTTPException):
        return None


def wait_netboot(nb, host, wait_s):
    """Attend qu'un bootloader reponde en unicast a DISCOVER."""
    deadline = time.time() + wait_s
    while time.time() < deadline:
        found = nb.discover(broadcast=host, wait=0.5)
        if found:
            return found[0]
    return None


def enter_netboot(nb, host, wait_s=20.0, probe_s=12.0):
    """Amene l'appareil en mode flasheur, quel que soit son etat de depart.

    Sonde l'UDP (bootloader) et le HTTP (application) en alternance pendant
    probe_s : une machine en train de redemarrer ne repond a rien pendant
    quelques secondes, ce n'est pas une raison d'abandonner."""
    t0 = time.time()
    deadline = t0 + probe_s
    r = None
    told = False
    while True:
        if wait_netboot(nb, host, 0.6):
            log("device already in network flash mode")
            return
        r = http_post(host, "/netboot")
        if r is not None:
            break
        if time.time() >= deadline:
            raise NetBootError("no application at http://%s and no bootloader answering on UDP %d "
                               "for %.0f s.\nIf the device is off or rebooting, try again; hold the two "
                               "outer buttons while powering on to force the flash mode."
                               % (host, nb.port, probe_s))
        if not told:
            log("device silent on HTTP and UDP, retrying for up to %.0f s..." % probe_s)
            told = True
        time.sleep(0.5)
    status, text = r
    if status != 202:
        raise NetBootError("application refused /netboot: HTTP %d %s (firmware too old?)" % (status, text.strip()))
    log("application acknowledged, waiting for the bootloader...")
    info = wait_netboot(nb, host, wait_s)
    if info is None:
        raise NetBootError("bootloader did not show up within %.0f s" % wait_s)
    log("bootloader ready after %.1f s: %s" % (time.time() - t0, info))


def show_new_log(nb, state, src=0, prefix=""):
    """Affiche ce que l'appareil a journalise depuis le dernier appel."""
    try:
        nxt, text = nb.log_all(src=src, since=state.get(src, 0))
    except NetBootError:
        return
    state[src] = nxt
    for line in text.splitlines():
        log(prefix + line)


def cmd_discover(args):
    nb = NetBoot(timeout=args.timeout)
    found = nb.discover(broadcast=args.broadcast, wait=args.wait)
    if not found:
        log("no device in network flash mode (broadcast %s)" % args.broadcast)
        return 1
    for i in found:
        log(str(i))
    return 0


def cmd_info(args):
    nb = NetBoot(args.host, timeout=args.timeout).release_at_exit()
    log(str(nb.info()))
    return 0


def cmd_enter(args):
    nb = NetBoot(args.host, timeout=args.timeout).release_at_exit()
    enter_netboot(nb, args.host)
    return 0


def cmd_flash(args):
    images = []
    if args.cm7:
        images.append(("CM7", IMAGES["cm7"], args.cm7))
    if args.cm4:
        images.append(("CM4", IMAGES["cm4"], args.cm4))
    for addr, path in args.at or []:
        images.append(("raw", int(addr, 0), path))
    bl = None
    if args.bl_a or args.bl_b:
        if not (args.bl_a and args.bl_b):
            log("the bootloader needs both images: --bl-a (linked at 0x08000000) and --bl-b (0x080E0000)")
            return 2
        bl = (args.bl_a, args.bl_b)
        images += [("BL", None, args.bl_a), ("BL", None, args.bl_b)]
    if not images:
        log("nothing to flash: give --cm7, --cm4, --at ADDR FILE or --bl-a/--bl-b")
        return 2
    for _, _, p in images:
        if not os.path.isfile(p):
            log("missing file: %s" % p)
            return 2

    nb = NetBoot(args.host, timeout=args.timeout, verbose=args.verbose).release_at_exit()
    t_all = time.time()
    if not args.no_enter:
        enter_netboot(nb, args.host)
    info = nb.info()
    log(str(info))
    logstate = {}
    if args.show_log:
        show_new_log(nb, logstate, prefix="    | ")

    # Bootloader : on ecrit l'AUTRE slot que celui qui tourne, puis on bascule BOOT_ADD0.
    bl_target = None
    if bl is not None:
        target_slot = 1 - info.boot_slot
        bl_target = BL_SLOTS[target_slot]
        images = [(n, a, p) for n, a, p in images if n != "BL"]
        images.append(("BL slot %s" % ("B" if target_slot else "A"), bl_target, bl[target_slot]))
        log("bootloader %s runs from slot %s -> writing slot %s" % (info.bl_version, "B" if info.boot_slot else "A",
                                                                "B" if target_slot else "A"))

    for name, addr, path in images:
        data = image_bytes(path)
        log("%s: %s -> 0x%08X (%d bytes, %d sector(s))"
            % (name, path, addr, len(data), len(sectors_covering(addr, len(data)))))
        t = nb.flash_image(addr, data, log=log, verify=not args.no_verify, erase=not args.no_erase)
        log("  %s done: %s" % (name, ", ".join("%s %.2f s" % kv for kv in t.items())))
        if args.show_log:
            show_new_log(nb, logstate, prefix="    | ")

    if bl_target is not None:
        nb.boot_select(bl_target)
        log("BOOT_ADD0 -> 0x%08X (takes effect at the next reset)" % bl_target)

    if args.journal:
        phase = PHASE_BY_NAME[args.journal]
        nb.journal(phase, args.trial, 0, 0)
        log("journal: %s" % PHASE.get(phase, args.journal))

    if args.no_boot:
        log("staying in network flash mode (--no-boot)")
        return 0

    nb.boot()
    log("boot requested after %.1f s" % (time.time() - t_all))
    if args.wait_app:
        deadline = time.time() + args.wait_app
        while time.time() < deadline:
            r = http_get(args.host, "/getFirmwareVersion", timeout=1.0)
            if r and r[0] == 200:
                log("application back after %.1f s total, firmware %s" % (time.time() - t_all, r[1].strip()))
                if bl_target is not None:
                    # Le nouveau bootloader a demarre l'application : on le fait parler.
                    log("checking the new bootloader ...")
                    enter_netboot(nb, args.host)
                    i2 = nb.info()
                    log(str(i2))
                    nb.boot()
                    ok = BL_SLOTS.get(i2.boot_slot) == bl_target
                    log("bootloader now %s from slot %s%s" % (i2.bl_version, "B" if i2.boot_slot else "A",
                                                          "" if ok else "  (UNEXPECTED: BOOT_ADD0 did not switch)"))
                    return 0 if ok else 1
                return 0
            time.sleep(0.3)
        log("application not reachable after %.0f s" % args.wait_app)
        return 1
    return 0


def cmd_erase(args):
    nb = NetBoot(args.host, timeout=args.timeout).release_at_exit()
    addr, length = int(args.addr, 0), int(args.len, 0)
    for s in sectors_covering(addr, length):
        ms = nb.erase(s, SECTOR)
        log("erased 0x%08X in %.2f s" % (s, ms / 1000.0))
    return 0


def cmd_write(args):
    nb = NetBoot(args.host, timeout=args.timeout).release_at_exit()
    data = image_bytes(args.file)
    addr = int(args.addr, 0)
    t = nb.flash_image(addr, data, log=log, verify=not args.no_verify, erase=not args.no_erase)
    log("done: %s" % ", ".join("%s %.2f s" % kv for kv in t.items()))
    return 0


def cmd_read(args):
    nb = NetBoot(args.host, timeout=args.timeout).release_at_exit()
    addr, length = int(args.addr, 0), int(args.len, 0)
    data = nb.read(addr, length)
    if args.output:
        with open(args.output, "wb") as f:
            f.write(data)
        log("%d bytes from 0x%08X -> %s" % (len(data), addr, args.output))
    else:
        for off in range(0, len(data), 16):
            chunk = data[off:off + 16]
            log("%08X  %-48s %s" % (addr + off, " ".join("%02X" % b for b in chunk),
                                    "".join(chr(b) if 32 <= b < 127 else "." for b in chunk)))
    return 0


def cmd_crc(args):
    nb = NetBoot(args.host, timeout=args.timeout).release_at_exit()
    addr, length = int(args.addr, 0), int(args.len, 0)
    log("CRC-32 0x%08X over 0x%08X + %d" % (nb.crc(addr, length), addr, length))
    return 0


def cmd_log(args):
    nb = NetBoot(args.host, timeout=args.timeout).release_at_exit()
    src = 1 if args.src == "cm4" else 0
    since = args.since
    while True:
        nxt, text = nb.log_all(src=src, since=since)
        if text:
            sys.stdout.write(text)
            sys.stdout.flush()
        since = nxt
        if not args.follow:
            return 0
        time.sleep(args.interval)


def cmd_journal(args):
    nb = NetBoot(args.host, timeout=args.timeout).release_at_exit()
    phase = PHASE_BY_NAME[args.phase]
    nb.journal(phase, args.trial, args.rollback, args.pending)
    log("journal %s" % ("cleared" if phase == 0 else "written: " + PHASE.get(phase, args.phase)))
    return 0


def cmd_boot(args):
    nb = NetBoot(args.host, timeout=args.timeout).release_at_exit()
    nb.boot()
    log("boot requested")
    return 0


def cmd_ping(args):
    nb = NetBoot(args.host, timeout=args.timeout).release_at_exit()
    t0 = time.time()
    nb.ping()
    log("pong in %.1f ms" % ((time.time() - t0) * 1000))
    return 0


def main(argv):
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("--host", default=DEFAULT_HOST, help="IP de l'appareil (defaut %s)" % DEFAULT_HOST)
    p.add_argument("--timeout", type=float, default=1.0, help="delai par requete (s)")
    p.add_argument("-v", "--verbose", action="store_true")
    sub = p.add_subparsers(dest="cmd", required=True)

    s = sub.add_parser("discover", help="cherche les appareils en mode flasheur")
    s.add_argument("--broadcast", default="255.255.255.255")
    s.add_argument("--wait", type=float, default=1.5)
    s.set_defaults(fn=cmd_discover)

    sub.add_parser("info", help="etat de l'appareil").set_defaults(fn=cmd_info)
    sub.add_parser("enter", help="fait passer l'application en mode flasheur").set_defaults(fn=cmd_enter)
    sub.add_parser("ping").set_defaults(fn=cmd_ping)
    sub.add_parser("boot", help="quitte le mode flasheur (reset)").set_defaults(fn=cmd_boot)

    s = sub.add_parser("flash", help="efface, ecrit, verifie, redemarre")
    s.add_argument("--cm7", metavar="FILE", help=".bin ou .elf du CM7 -> 0x%08X" % IMAGES["cm7"])
    s.add_argument("--cm4", metavar="FILE", help=".bin ou .elf du CM4 -> 0x%08X" % IMAGES["cm4"])
    s.add_argument("--at", nargs=2, action="append", metavar=("ADDR", "FILE"), help="image brute a une adresse")
    s.add_argument("--bl-a", metavar="FILE", help="bootloader lie pour le slot A (0x08000000)")
    s.add_argument("--bl-b", metavar="FILE", help="bootloader lie pour le slot B (0x080E0000) ; l'outil ecrit le slot inactif et bascule BOOT_ADD0")
    s.add_argument("--no-enter", action="store_true", help="ne pas passer par POST /netboot")
    s.add_argument("--no-boot", action="store_true", help="rester en mode flasheur a la fin")
    s.add_argument("--no-erase", action="store_true")
    s.add_argument("--no-verify", action="store_true")
    s.add_argument("--journal", choices=sorted(PHASE_BY_NAME), help="enregistrement de journal a ecrire apres le flash")
    s.add_argument("--trial", type=int, default=0)
    s.add_argument("--show-log", action="store_true", help="affiche la trace du bootloader pendant l'operation")
    s.add_argument("--wait-app", type=float, default=30.0, metavar="S", help="attend le retour HTTP de l'application (0 = non)")
    s.set_defaults(fn=cmd_flash)

    s = sub.add_parser("erase")
    s.add_argument("--addr", required=True)
    s.add_argument("--len", required=True)
    s.set_defaults(fn=cmd_erase)

    s = sub.add_parser("write", help="ecrit une image a une adresse")
    s.add_argument("--addr", required=True)
    s.add_argument("file")
    s.add_argument("--no-erase", action="store_true")
    s.add_argument("--no-verify", action="store_true")
    s.set_defaults(fn=cmd_write)

    s = sub.add_parser("read", help="lit la flash ou la RAM")
    s.add_argument("--addr", required=True)
    s.add_argument("--len", required=True)
    s.add_argument("-o", "--output")
    s.set_defaults(fn=cmd_read)

    s = sub.add_parser("crc")
    s.add_argument("--addr", required=True)
    s.add_argument("--len", required=True)
    s.set_defaults(fn=cmd_crc)

    s = sub.add_parser("log", help="anneau de logs (bootloader + CM7, ou CM4)")
    s.add_argument("--src", choices=["cm7", "cm4"], default="cm7")
    s.add_argument("--since", type=int, default=0)
    s.add_argument("--follow", "-f", action="store_true")
    s.add_argument("--interval", type=float, default=0.2)
    s.set_defaults(fn=cmd_log)

    s = sub.add_parser("journal", help="ecrit un enregistrement du journal OTA")
    s.add_argument("phase", choices=sorted(PHASE_BY_NAME))
    s.add_argument("--trial", type=int, default=0)
    s.add_argument("--rollback", type=int, default=0)
    s.add_argument("--pending", type=int, default=0)
    s.set_defaults(fn=cmd_journal)

    args = p.parse_args(argv)
    try:
        return args.fn(args)
    except Timeout as e:
        log("timeout: %s" % e)
        return 3
    except NetBootError as e:
        log("error: %s" % e)
        return 1
    except KeyboardInterrupt:
        return 130


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
