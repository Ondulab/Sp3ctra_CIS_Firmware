#!/usr/bin/env python3
"""Suit la trace du CIS par le reseau, sans UART ni ST-Link.

Lit l'anneau de logs en RAM retenue par HTTP (GET /log) quand l'application
tourne, et par le flasheur reseau (UDP) quand c'est le bootloader qui tourne.
Rien ne circule entre deux lectures : c'est l'outil qui tire.

    scripts/netboot/netlog.py                  # suit le CM7 (bootloader + application)
    scripts/netboot/netlog.py --src cm7,cm4    # les deux coeurs, entrelaces
    scripts/netboot/netlog.py --once           # vide l'anneau et sort

Chaque ligne est horodatee par l'appareil : "[7  12.345] " = CM7 a 12,345 s,
'B' = bootloader, '4' = CM4. Les positions sont conservees a travers les
reboots ; une coupure secteur remet le compteur a zero et l'outil repart du debut.

Note macOS : le terminal doit avoir l'autorisation "Reseau local".
"""

import argparse
import http.client
import os
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from netboot import NetBoot, NetBootError  # noqa: E402

DEFAULT_HOST = "192.168.100.1"


def http_log(host, src, since, maxlen, timeout):
    """(next, head, texte) ou None si l'application ne repond pas."""
    try:
        c = http.client.HTTPConnection(host, 80, timeout=timeout)
        c.request("GET", "/log?src=%s&since=%d&max=%d" % (src, since, maxlen))
        r = c.getresponse()
        body = r.read()
        if r.status != 200:
            return None
        nxt = int(r.getheader("X-Log-Next", "0"))
        head = int(r.getheader("X-Log-Head", "0"))
        return nxt, head, body.decode("utf-8", "replace")
    except (OSError, ValueError, http.client.HTTPException):
        return None


def main(argv):
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("--host", default=DEFAULT_HOST)
    p.add_argument("--src", default="cm7", help="cm7, cm4 ou cm7,cm4")
    p.add_argument("--interval", type=float, default=0.2, help="periode de tirage (s)")
    p.add_argument("--once", action="store_true", help="une lecture puis sortie")
    p.add_argument("--from-start", action="store_true", help="commence au plus ancien octet conserve")
    p.add_argument("--tail", type=int, default=4096, help="octets a rejouer au depart (defaut 4096)")
    args = p.parse_args(argv)

    srcs = [s.strip() for s in args.src.split(",") if s.strip()]
    pos = {s: None for s in srcs}      # None = premiere lecture (fin de l'anneau moins --tail)
    nb = NetBoot(args.host, timeout=0.3, retries=0)
    via = None
    try:
        while True:
            got_any = False
            for s in srcs:
                since = pos[s]
                # --- application (HTTP) ---
                r = http_log(args.host, s, 0 if since is None else since,
                             args.tail if since is None else 4096, timeout=1.0)
                if r is not None:
                    nxt, head, text = r
                    if since is None and not args.from_start:
                        # premiere lecture : GET /log sans since renvoie la fin ; ici on a
                        # demande since=0 avec max=tail, donc rejouer seulement la fin
                        r = http_log(args.host, s, max(0, head - args.tail), args.tail, timeout=1.0)
                        if r is None:
                            continue
                        nxt, head, text = r
                    if nxt < (since or 0):
                        text = "[netlog] device log counter reset (power cycle)\n" + text
                    pos[s] = nxt
                    if via != "http":
                        via = "http"
                        sys.stdout.write("[netlog] reading %s via HTTP\n" % args.host)
                    if text:
                        sys.stdout.write(text)
                        got_any = True
                    continue
                # --- bootloader (UDP) ---
                try:
                    nxt, text = nb.log_all(src=1 if s == "cm4" else 0, since=0 if since is None else since)
                except NetBootError:
                    continue
                if since is None and len(text) > args.tail:
                    text = text[-args.tail:]
                if nxt < (since or 0):
                    text = "[netlog] device log counter reset (power cycle)\n" + text
                pos[s] = nxt
                if via != "udp":
                    via = "udp"
                    sys.stdout.write("[netlog] reading %s via the bootloader (UDP)\n" % args.host)
                if text:
                    sys.stdout.write(text)
                    got_any = True
            sys.stdout.flush()
            if args.once:
                return 0
            time.sleep(args.interval if got_any else max(args.interval, 0.5))
    except KeyboardInterrupt:
        return 130


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
