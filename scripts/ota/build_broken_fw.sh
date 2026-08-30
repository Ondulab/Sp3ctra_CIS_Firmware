#!/bin/bash
# Construit un paquet de mise a jour volontairement defaillant, pour valider le
# rollback. La macro SP3CTRA_OTA_FAULT est positionnee le temps de la
# compilation puis systematiquement remise a zero, y compris si le build
# echoue ou si le script est interrompu.
#
# Usage: scripts/ota/build_broken_fw.sh <1..5>
#
#   1  HardFault des l'entree de main()
#   2  boucle infinie des l'entree de main()
#   3  serveur HTTP en echec
#   4  HardFault avant la fin du delai de confirmation
#   5  chien de garde jamais recharge

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
HEADER="$ROOT/Common/Inc/ota_fault_inject.h"
CONFIG="$ROOT/Common/Inc/config.h"

# L'image empoisonnee porte un numero de version distinct : GET /getFirmwareVersion
# suffit alors a constater le rollback, sans avoir a lire la trace UART.
BROKEN_PATCH=99

FAULT="${1:-}"
case "$FAULT" in
    1) NAME="OTA_FAULT_HARDFAULT_BOOT" ;;
    2) NAME="OTA_FAULT_HANG_BOOT" ;;
    3) NAME="OTA_FAULT_NO_HTTP" ;;
    4) NAME="OTA_FAULT_LATE_CRASH" ;;
    5) NAME="OTA_FAULT_NO_GUARD" ;;
    *)
        sed -n '/Familles de panne/,/^ \*\//p' "$HEADER"
        echo "Usage: $0 <1..5>"
        exit 1
        ;;
esac

ORIGINAL_PATCH="$(sed -n 's/^#define FW_VERSION_PATCH \([0-9]*\).*/\1/p' "$CONFIG")"

restore() {
    # Filet : ni la macro de faute ni la version de test ne doivent rester dans
    # l'arbre de sources, meme si le build echoue ou si on interrompt le script.
    sed -i '' "s/^#define SP3CTRA_OTA_FAULT .*/#define SP3CTRA_OTA_FAULT OTA_FAULT_NONE/" "$HEADER"
    sed -i '' "s/^#define FW_VERSION_PATCH .*/#define FW_VERSION_PATCH $ORIGINAL_PATCH/" "$CONFIG"

    # Les binaires empoisonnes restent dans Release/ et sont PLUS RECENTS que
    # les sources restaurees : aucune comparaison de dates ne peut les demasquer.
    # Un empaquetage ulterieur les embarquerait en les croyant sains -- c'est
    # arrive. On les supprime donc, ce qui force une reconstruction explicite.
    rm -f "$ROOT"/CM7/Release/*.bin "$ROOT"/CM7/Release/*.elf \
          "$ROOT"/CM4/Release/*.bin "$ROOT"/CM4/Release/*.elf
    echo "Sources restaurees, binaires empoisonnes supprimes (reconstruire avant tout empaquetage)"
}
trap restore EXIT INT TERM

echo "=== Injection de $NAME (faute $FAULT) ==="
sed -i '' "s/^#define SP3CTRA_OTA_FAULT .*/#define SP3CTRA_OTA_FAULT $NAME/" "$HEADER"
sed -i '' "s/^#define FW_VERSION_PATCH .*/#define FW_VERSION_PATCH $BROKEN_PATCH/" "$CONFIG"
grep -n "^#define SP3CTRA_OTA_FAULT" "$HEADER"
grep -n "^#define FW_VERSION_PATCH" "$CONFIG"

cd "$ROOT"
./scripts/build.sh cm7 release
./scripts/build.sh cm4 release

python3 scripts/ota/make_package.py --allow-fault

echo
echo "Paquet empoisonne pret."
echo "Version portee par l'image cassee : ${ORIGINAL_PATCH:+}$(sed -n 's/^#define FW_VERSION_MAJOR \([0-9]*\).*/\1/p' "$CONFIG").$(sed -n 's/^#define FW_VERSION_MINOR \([0-9]*\).*/\1/p' "$CONFIG").$BROKEN_PATCH"
echo "Apres rollback, GET /getFirmwareVersion doit a nouveau repondre en .$ORIGINAL_PATCH"
echo
echo "Penser a reconstruire une image saine ensuite :"
echo "  ./scripts/build.sh all release && python3 scripts/ota/make_package.py"
