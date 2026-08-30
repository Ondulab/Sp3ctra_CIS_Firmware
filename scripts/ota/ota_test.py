#!/usr/bin/env python3
"""Banc de test du rollback de mise a jour du CIS.

Chaque scenario envoie un paquet, puis observe l'appareil par HTTP jusqu'a ce
qu'il reponde a nouveau. L'oracle est GET /getFirmwareVersion : les images
empoisonnees construites par build_broken_fw.sh portent le patch 99, donc un
retour a la version d'origine prouve que la restauration a eu lieu.

    scripts/ota/ota_test.py --list
    scripts/ota/ota_test.py T5 T6 T9        # scenarios rapides, sans reflash
    scripts/ota/ota_test.py T1              # rollback complet, plusieurs minutes
    scripts/ota/ota_test.py --all

Ce que HTTP ne voit pas -- l'ecran OLED, l'ordre des phases, la cause des
resets -- se lit sur la trace UART, a capturer en parallele :

    ./scripts/uart_trace.sh | tee build/ota/trace_T1.log

Lignes a y retrouver pour un rollback reussi :
    OTA phase: TRIAL (trial 1/3 ...) ... (2/3) ... (3/3)
    Reset cause: 0x......  IWDG                 [fautes 2 et 5]
    OTA: trial image exhausted its attempts, rolling back
    OTA: restoring the previous firmware
    OTA: previous firmware restored
    OTA journal: IDLE

Note macOS : le terminal doit avoir l'autorisation "Reseau local".
"""

import argparse
import os
import subprocess
import sys
import time
import urllib.error
import urllib.request

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
OTA_DIR = os.path.join(ROOT, "build", "ota")
MAKE_PACKAGE = os.path.join(ROOT, "scripts", "ota", "make_package.py")
BUILD_BROKEN = os.path.join(ROOT, "scripts", "ota", "build_broken_fw.sh")
UPLOAD = os.path.join(ROOT, "scripts", "ota", "ota_upload.py")

DEFAULT_HOST = "192.168.100.1"

# Mot de passe d'administration, exige par tout ce qui modifie l'appareil. Il
# s'affiche sur l'ecran de demarrage et sur la trace UART jusqu'a son premier
# usage : il ne transite jamais par le reseau, sinon la protection ne servirait
# a rien. Renseigne par --password.
PASSWORD = None

# Une restauration reecrit 896 Ko + 640 Ko en flash interne depuis la NOR, et
# une image a l'essai consomme trois demarrages avant d'etre abandonnee.
ROLLBACK_TIMEOUT_S = 900
QUICK_TIMEOUT_S = 60


class Scenario:
    def __init__(self, key, title, kind, fault=None, package_args=None, expect=None,
                 status="400", password=""):
        self.key = key
        self.title = title
        self.kind = kind  # "rollback" | "reject" | "abort" | "accept"
        self.fault = fault
        self.package_args = package_args or []
        self.expect = expect
        self.status = status
        # "" = le mot de passe global ; None = aucun identifiant ; sinon celui-ci.
        self.password = password


SCENARIOS = [
    Scenario("T1", "HardFault immediat", "rollback", fault=1),
    Scenario("T2", "Boucle infinie au boot (chien de garde)", "rollback", fault=2),
    Scenario("T3", "Serveur HTTP en echec (controle de sante)", "rollback", fault=3),
    Scenario("T4", "Panne avant la fin du delai de confirmation", "rollback", fault=4),
    Scenario("T5", "Paquet a CRC faux", "reject", package_args=["--corrupt"],
             expect="checksum mismatch"),
    Scenario("T6", "Televersement interrompu a 50 %", "abort"),
    Scenario("T9", "Taille CM7 aberrante dans l'en-tete", "reject",
             package_args=["--bad-size", "0x200000"],
             expect="CM7 image size out of range"),
    Scenario("T12", "Mise a jour saine", "accept"),
    Scenario("T13", "Chien de garde jamais recharge", "rollback", fault=5),
    Scenario("T14", "Televersement sans identifiants", "reject", status="401",
             expect="Administrator credentials required", password=None),
    Scenario("T15", "Televersement avec mauvais mot de passe", "reject", status="401",
             expect="Administrator credentials required", password="WRONGWRONG12"),
]

BY_KEY = {s.key: s for s in SCENARIOS}


def http_get(host, path, timeout=5.0):
    try:
        with urllib.request.urlopen("http://%s%s" % (host, path), timeout=timeout) as r:
            return r.read().decode("utf-8", errors="replace").strip()
    except (urllib.error.URLError, OSError, TimeoutError):
        return None


def device_version(host, timeout=5.0):
    return http_get(host, "/getFirmwareVersion", timeout)


def wait_for_version(host, predicate, timeout_s, label):
    """Attend qu'une version satisfaisant le predicat soit servie."""
    deadline = time.time() + timeout_s
    last = None
    print("  attente : %s (%d s max)" % (label, timeout_s))

    while time.time() < deadline:
        version = device_version(host, timeout=3.0)
        if version != last:
            elapsed = int(timeout_s - (deadline - time.time()))
            print("    t+%-4ds  %s" % (elapsed, version if version else "(injoignable)"))
            last = version
        if version and predicate(version):
            return version
        time.sleep(3.0)

    return None


def newest_source_mtime():
    """Date de modification de la source la plus recente du firmware."""
    newest = 0.0
    for tree in ("CM7", "CM4", "Common"):
        for root, dirs, files in os.walk(os.path.join(ROOT, tree)):
            dirs[:] = [d for d in dirs if d not in ("Release", "Debug", "Drivers", "Middlewares", "DSP")]
            for f in files:
                if f.endswith((".c", ".h")):
                    newest = max(newest, os.path.getmtime(os.path.join(root, f)))
    return newest


def ensure_package(scenario):
    """Construit le paquet du scenario, en le reconstruisant s'il a vieilli.

    Un paquet empoisonne reutilise alors que les sources ont change teste une
    image qui n'est plus celle du depot -- et rend un vert trompeur. C'est
    arrive : un paquet fault3 anterieur a l'ajout de OTA_TRIAL_DEADLINE_MS a ete
    reutilise, et le scenario est reste bloque sur l'essai 1/3.
    """
    os.makedirs(OTA_DIR, exist_ok=True)

    if scenario.fault:
        existing = [f for f in os.listdir(OTA_DIR) if f.endswith("_fault%d.bin" % scenario.fault)]
        if existing:
            newest = os.path.join(OTA_DIR, sorted(existing)[-1])
            if os.path.getmtime(newest) >= newest_source_mtime():
                return newest
            print("  paquet %s anterieur aux sources, reconstruction" % os.path.basename(newest))
            for f in existing:
                os.unlink(os.path.join(OTA_DIR, f))

        print("  construction de l'image empoisonnee (faute %d), plusieurs minutes..." % scenario.fault)
        subprocess.run([BUILD_BROKEN, str(scenario.fault)], cwd=ROOT, check=True)

        existing = [f for f in os.listdir(OTA_DIR) if f.endswith("_fault%d.bin" % scenario.fault)]
        if not existing:
            raise RuntimeError("le paquet de la faute %d n'a pas ete produit" % scenario.fault)
        return os.path.join(OTA_DIR, sorted(existing)[-1])

    # Reconstruction systematique avant d'empaqueter une image saine : make est
    # incremental, donc c'est quasi gratuit quand rien n'a bouge, et cela evite
    # d'embarquer les binaires laisses par un scenario a faute.
    subprocess.run([os.path.join(ROOT, "scripts", "build.sh"), "all", "release"],
                   cwd=ROOT, check=True, capture_output=True, text=True)

    result = subprocess.run([sys.executable, MAKE_PACKAGE] + scenario.package_args,
                            cwd=ROOT, check=True, capture_output=True, text=True)
    for line in result.stdout.splitlines():
        if line.startswith("paquet"):
            return line.split(":", 1)[1].split("(")[0].strip()
    raise RuntimeError("chemin du paquet introuvable dans la sortie de make_package.py")


def run(scenario, host):  # noqa: D401
    print("\n=== %s : %s ===" % (scenario.key, scenario.title))

    baseline = device_version(host)  # GET libre, sans identifiants
    if baseline is None:
        print("  ECHEC : appareil injoignable avant le test")
        return False
    print("  version initiale : %s" % baseline)

    package = ensure_package(scenario)
    print("  paquet : %s" % os.path.basename(package))

    cmd = [sys.executable, UPLOAD, "--host", host, package]
    password = PASSWORD if scenario.password == "" else scenario.password
    if password:
        cmd += ["--password", password]
    if scenario.kind == "abort":
        cmd += ["--abort-at", "50"]

    upload = subprocess.run(cmd, cwd=ROOT, capture_output=True, text=True)
    output = upload.stdout + upload.stderr
    print("  " + "\n  ".join(l for l in output.strip().splitlines() if l))

    if scenario.kind == "reject":
        if scenario.status not in output:
            print("  ECHEC : le paquet aurait du etre refuse avec un code %s" % scenario.status)
            return False
        if scenario.expect and scenario.expect not in output:
            print("  ECHEC : motif de refus attendu %r" % scenario.expect)
            return False
        # Un refus ne doit provoquer aucun redemarrage.
        time.sleep(2.0)
        version = device_version(host)
        if version != baseline:
            print("  ECHEC : l'appareil a redemarre alors que le paquet etait refuse (%s)" % version)
            return False
        print("  OK : refuse sans redemarrage, toujours en %s" % version)
        return True

    if scenario.kind == "abort":
        time.sleep(2.0)
        version = device_version(host)
        if version != baseline:
            print("  ECHEC : l'appareil a redemarre apres un televersement interrompu (%s)" % version)
            return False
        # Le point du test : la requete suivante ne doit pas etre avalee par la
        # machine a etats restee en cours de televersement.
        if http_get(host, "/getDPI") is None:
            print("  ECHEC : l'appareil ne repond plus normalement apres l'interruption")
            return False
        print("  OK : interruption absorbee, appareil toujours nominal en %s" % version)
        return True

    if scenario.kind == "accept":
        version = wait_for_version(host, lambda v: True, ROLLBACK_TIMEOUT_S,
                                   "retour de l'appareil apres la mise a jour")
        if version is None:
            print("  ECHEC : l'appareil n'est pas revenu")
            return False
        print("  OK : revenu en %s (verifier 'OTA: image confirmed' sur l'UART)" % version)
        return True

    # rollback
    version = wait_for_version(host, lambda v: v == baseline, ROLLBACK_TIMEOUT_S,
                               "retour a la version %s apres restauration" % baseline)
    if version is None:
        current = device_version(host)
        print("  ECHEC : pas de retour a %s (version courante : %s)" % (baseline, current))
        return False

    print("  OK : restauration effectuee, de retour en %s" % version)
    return True


def main(argv):
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("cases", nargs="*", help="scenarios a jouer (defaut : les rapides)")
    parser.add_argument("--host", default=DEFAULT_HOST)
    parser.add_argument("--password", help="mot de passe d'administration")
    parser.add_argument("--all", action="store_true", help="jouer tous les scenarios")
    parser.add_argument("--list", action="store_true", help="lister les scenarios")
    args = parser.parse_args(argv)

    global PASSWORD
    PASSWORD = args.password

    if args.list:
        for s in SCENARIOS:
            print("%-4s %-10s %s" % (s.key, s.kind, s.title))
        return 0

    if args.all:
        cases = [s.key for s in SCENARIOS]
    elif args.cases:
        cases = args.cases
    else:
        cases = ["T5", "T9", "T6"]
        print("Scenarios rapides par defaut (aucune reconstruction) : %s" % ", ".join(cases))
        print("Utiliser --all pour la matrice complete, ou --list pour la detailler.\n")

    unknown = [c for c in cases if c not in BY_KEY]
    if unknown:
        print("scenario inconnu : %s" % ", ".join(unknown), file=sys.stderr)
        return 1

    results = {}
    for key in cases:
        try:
            results[key] = run(BY_KEY[key], args.host)
        except (subprocess.CalledProcessError, RuntimeError, OSError) as exc:
            print("  ERREUR : %s" % exc)
            results[key] = False

    print("\n=== Bilan ===")
    for key in cases:
        print("%-4s %s  %s" % (key, "OK   " if results[key] else "ECHEC", BY_KEY[key].title))

    return 0 if all(results.values()) else 1


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
