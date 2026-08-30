#!/usr/bin/env python3
"""Televerse un paquet de mise a jour sur un CIS et affiche sa reponse.

Sockets brutes plutot que requests : le banc de test a besoin de couper la
connexion au milieu du transfert (--abort-at) pour verifier que l'appareil
repart d'un etat propre, ce qu'une bibliotheque HTTP ne permet pas.

    scripts/ota/ota_upload.py build/ota/cis_package_4.0.0.bin
    scripts/ota/ota_upload.py --host 192.168.100.1 <paquet>
    scripts/ota/ota_upload.py --abort-at 50 <paquet>   # coupe a 50 %

Note macOS : le terminal doit avoir l'autorisation "Reseau local"
(Reglages Systeme > Confidentialite et securite) pour joindre l'appareil.
"""

import argparse
import os
import socket
import sys
import time

DEFAULT_HOST = "192.168.100.1"
BOUNDARY = "----Sp3ctraOtaBoundary7d91f4"


def build_request(host, path):
    """En-tete HTTP et corps multipart attendus par fwupdate_multipart_state_machine().

    L'analyseur embarque est litteral : il cherche exactement
    'Content-Disposition: form-data; name="firmware"; filename="..."' puis
    'application/octet-stream\\r\\n\\r\\n'. L'ordre et les espaces comptent.
    """
    with open(path, "rb") as f:
        payload = f.read()

    filename = os.path.basename(path)

    prologue = (
        "--%s\r\n"
        'Content-Disposition: form-data; name="firmware"; filename="%s"\r\n'
        "Content-Type: application/octet-stream\r\n"
        "\r\n" % (BOUNDARY, filename)
    ).encode()

    epilogue = ("\r\n--%s--\r\n" % BOUNDARY).encode()
    body = prologue + payload + epilogue

    header = (
        "POST /upload HTTP/1.1\r\n"
        "Host: %s\r\n"
        "Content-Type: multipart/form-data; boundary=%s\r\n"
        "Content-Length: %d\r\n"
        "Connection: close\r\n"
        "\r\n" % (host, BOUNDARY, len(body))
    ).encode()

    return header, body


def upload(args):
    header, body = build_request(args.host, args.package)
    total = len(body)

    print("paquet   : %s (%d o)" % (args.package, os.path.getsize(args.package)))
    print("cible    : %s:%d" % (args.host, args.port))

    sock = socket.create_connection((args.host, args.port), timeout=args.timeout)
    sock.settimeout(args.timeout)

    try:
        sock.sendall(header)

        cut = total * args.abort_at // 100 if args.abort_at is not None else None
        sent = 0
        chunk = 1460  # une MSS Ethernet : au plus pres du comportement du navigateur

        while sent < total:
            if cut is not None and sent >= cut:
                print("coupure volontaire a %d %% (%d/%d o)" % (args.abort_at, sent, total))
                sock.close()
                return 2

            end = min(sent + chunk, total)
            sock.sendall(body[sent:end])
            sent = end

            if args.throttle:
                time.sleep(args.throttle)

        print("envoye   : %d o, attente de la reponse" % sent)

        response = b""
        while True:
            try:
                data = sock.recv(4096)
            except socket.timeout:
                break
            if not data:
                break
            response += data

    finally:
        try:
            sock.close()
        except OSError:
            pass

    if not response:
        print("aucune reponse (l'appareil a peut-etre redemarre avant de repondre)")
        return 1

    text = response.decode("utf-8", errors="replace")
    print("--- reponse ---")
    print(text.strip())

    status = text.split("\r\n", 1)[0]
    return 0 if " 200 " in status else 1


def main(argv):
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("package", help="paquet a televerser")
    parser.add_argument("--host", default=DEFAULT_HOST)
    parser.add_argument("--port", type=int, default=80)
    parser.add_argument("--timeout", type=float, default=30.0)
    parser.add_argument("--abort-at", type=int, metavar="PCT",
                        help="fermer la connexion apres PCT %% du corps")
    parser.add_argument("--throttle", type=float, default=0.0,
                        help="pause en secondes entre deux blocs")
    args = parser.parse_args(argv)

    if not os.path.isfile(args.package):
        print("paquet introuvable : %s" % args.package, file=sys.stderr)
        return 1

    try:
        return upload(args)
    except OSError as exc:
        print("erreur reseau : %s" % exc, file=sys.stderr)
        print("Sur macOS, verifier l'autorisation \"Reseau local\" du terminal.", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
