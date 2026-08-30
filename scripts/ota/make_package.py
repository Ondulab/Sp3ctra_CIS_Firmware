#!/usr/bin/env python3
"""Assemble un paquet de mise a jour CIS (cis_package_<version>.bin).

Remplace UpdateFileGen/updateFileGen.py, casse depuis que Common/Inc/config.h
construit FW_VERSION par concatenation de macros au lieu d'une chaine litterale.

Format (inchange, le bootloader deploye doit pouvoir le lire) :

    offset 0   : "BOOT"                    4 o
    offset 4   : taille de l'image CM7     4 o, little-endian
    offset 8   : taille de l'image CM4     4 o
    offset 12  : taille de la charge externe 4 o
    offset 16  : version, complete de zeros 8 o
    offset 24  : image CM7, image CM4, charge externe
    fin - 4    : CRC-32 (zlib) de tout ce qui precede

Les options de corruption servent au banc de test du rollback : elles
fabriquent des paquets que le firmware doit refuser.

    scripts/ota/make_package.py
    scripts/ota/make_package.py --corrupt      # un octet retourne dans le corps
    scripts/ota/make_package.py --truncate 50  # fichier coupe a 50 %
    scripts/ota/make_package.py --bad-size     # taille CM7 aberrante, CRC juste
"""

import argparse
import os
import re
import struct
import sys
import zlib

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

CM7_BIN = os.path.join(ROOT, "CM7", "Release", "Sp3ctra_CIS_Firmware_CM7.bin")
CM4_BIN = os.path.join(ROOT, "CM4", "Release", "Sp3ctra_CIS_Firmware_CM4.bin")
EXTERNAL = os.path.join(ROOT, "UpdateFileGen", "External_MAX8.tar.gz")
CONFIG_H = os.path.join(ROOT, "Common", "Inc", "config.h")
BOOT_CONFIG_H = os.path.join(ROOT, "Common", "Inc", "boot_config.h")
FAULT_H = os.path.join(ROOT, "Common", "Inc", "ota_fault_inject.h")

HEADER_STRUCT = "<4sIII8s"
HEADER_SIZE = 24


def read_version(path=CONFIG_H):
    """FW_VERSION, reconstruite depuis les trois macros de config.h.

    config.h definit FW_VERSION comme la concatenation de FW_VERSION_MAJOR,
    _MINOR et _PATCH ; l'ancien generateur cherchait une chaine litterale et
    echouait donc systematiquement.
    """
    with open(path, "r", encoding="utf-8", errors="replace") as f:
        text = f.read()

    parts = []
    for name in ("MAJOR", "MINOR", "PATCH"):
        m = re.search(r"#define\s+FW_VERSION_%s\s+(\d+)" % name, text)
        if not m:
            raise ValueError("FW_VERSION_%s introuvable dans %s" % (name, path))
        parts.append(m.group(1))

    version = ".".join(parts)
    if len(version.encode()) > 8:
        raise ValueError("version %r trop longue pour le champ de 8 octets" % version)
    return version


def read_max_sizes(path=BOOT_CONFIG_H):
    """(FW_CM7_MAX_SIZE, FW_CM4_MAX_SIZE) en octets, depuis boot_config.h."""
    with open(path, "r", encoding="utf-8", errors="replace") as f:
        text = f.read()

    sectors = dict(
        re.findall(r"#define\s+(ADDR_FLASH_SECTOR_\d+_BANK\d+)\s+\(\(uint32_t\)(0x[0-9A-Fa-f]+)\)", text)
    )
    if not sectors:
        raise ValueError("adresses de secteurs introuvables dans %s" % path)

    def resolve(macro):
        m = re.search(r"#define\s+%s\s+\(([A-Z0-9_]+)\s*-\s*([A-Z0-9_]+)\)" % macro, text)
        if not m:
            raise ValueError("%s introuvable dans %s" % (macro, path))
        end, start = m.group(1), m.group(2)
        start_macro = re.search(r"#define\s+%s\s+\(([A-Z0-9_]+)\)" % start, text)
        if start_macro:
            start = start_macro.group(1)
        return int(sectors[end], 16) - int(sectors[start], 16)

    return resolve("FW_CM7_MAX_SIZE"), resolve("FW_CM4_MAX_SIZE")


def active_fault(path=FAULT_H):
    """Valeur de SP3CTRA_OTA_FAULT, 0 quand aucune faute n'est injectee."""
    with open(path, "r", encoding="utf-8", errors="replace") as f:
        text = f.read()

    m = re.search(r"#define\s+SP3CTRA_OTA_FAULT\s+(\S+)", text)
    if not m:
        raise ValueError("SP3CTRA_OTA_FAULT introuvable dans %s" % path)

    token = m.group(1)
    if token in ("0", "OTA_FAULT_NONE"):
        return 0

    named = re.search(r"#define\s+%s\s+(\d+)" % re.escape(token), text)
    return int(named.group(1)) if named else -1


def read_file(path, what):
    if not os.path.isfile(path):
        raise FileNotFoundError("%s introuvable : %s\nConstruire d'abord avec ./scripts/build.sh all release" % (what, path))
    with open(path, "rb") as f:
        return f.read()


def build(args):
    version = read_version()
    cm7 = read_file(CM7_BIN, "image CM7")
    cm4 = read_file(CM4_BIN, "image CM4")

    external = b""
    if os.path.isfile(EXTERNAL) and not args.no_external:
        with open(EXTERNAL, "rb") as f:
            external = f.read()

    max_cm7, max_cm4 = read_max_sizes()
    if len(cm7) > max_cm7:
        raise ValueError("image CM7 de %d o au-dela de FW_CM7_MAX_SIZE (%d o)" % (len(cm7), max_cm7))
    if len(cm4) > max_cm4:
        raise ValueError("image CM4 de %d o au-dela de FW_CM4_MAX_SIZE (%d o)" % (len(cm4), max_cm4))

    fault = active_fault()
    if fault != 0 and not args.allow_fault:
        raise SystemExit(
            "SP3CTRA_OTA_FAULT vaut %s dans Common/Inc/ota_fault_inject.h.\n"
            "Cette image est une image de test : elle ne demarrera pas correctement.\n"
            "Utiliser --allow-fault pour l'empaqueter volontairement, ou remettre la macro a 0."
            % fault
        )

    announced_cm7 = args.bad_size if args.bad_size else len(cm7)

    header = struct.pack(
        HEADER_STRUCT,
        b"BOOT",
        announced_cm7,
        len(cm4),
        len(external),
        version.encode("utf-8")[:8],
    )

    body = header + cm7 + cm4 + external
    package = body + struct.pack("<I", zlib.crc32(body) & 0xFFFFFFFF)

    if args.corrupt:
        # Un octet retourne au milieu de l'image CM7 : le CRC du pied ne
        # correspond plus, le paquet doit etre refuse avant tout effacement.
        offset = HEADER_SIZE + len(cm7) // 2
        mutated = bytearray(package)
        mutated[offset] ^= 0xFF
        package = bytes(mutated)

    if args.truncate is not None:
        keep = max(1, len(package) * args.truncate // 100)
        package = package[:keep]

    suffix = ""
    if fault:
        suffix += "_fault%d" % fault
    if args.corrupt:
        suffix += "_corrupt"
    if args.truncate is not None:
        suffix += "_trunc%d" % args.truncate
    if args.bad_size:
        suffix += "_badsize"

    out = args.out or os.path.join(args.out_dir, "cis_package_%s%s.bin" % (version, suffix))
    os.makedirs(os.path.dirname(os.path.abspath(out)), exist_ok=True)
    with open(out, "wb") as f:
        f.write(package)

    print("version        : %s" % version)
    print("CM7            : %d o (annonce %d)" % (len(cm7), announced_cm7))
    print("CM4            : %d o" % len(cm4))
    print("charge externe : %d o" % len(external))
    print("faute injectee : %s" % (fault if fault else "aucune"))
    print("CRC-32         : 0x%08X" % (zlib.crc32(body) & 0xFFFFFFFF))
    print("paquet         : %s (%d o)" % (out, len(package)))
    return out


def main(argv):
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--out", help="chemin du paquet produit")
    parser.add_argument("--out-dir", default=os.path.join(ROOT, "build", "ota"), help="dossier de sortie")
    parser.add_argument("--no-external", action="store_true", help="omettre la charge externe MAX8")
    parser.add_argument("--allow-fault", action="store_true", help="empaqueter malgre une faute injectee")
    parser.add_argument("--corrupt", action="store_true", help="retourner un octet du corps (CRC faux)")
    parser.add_argument("--truncate", type=int, metavar="PCT", help="tronquer le paquet a PCT %% de sa taille")
    parser.add_argument("--bad-size", type=lambda v: int(v, 0), metavar="N",
                        help="annoncer N comme taille CM7, avec un CRC correct")
    args = parser.parse_args(argv)

    try:
        build(args)
    except (FileNotFoundError, ValueError) as exc:
        print("erreur : %s" % exc, file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
