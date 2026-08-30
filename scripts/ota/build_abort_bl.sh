#!/bin/bash
# Construit un bootloader qui simule une coupure secteur au milieu d'une etape
# de la mise a jour, puis restaure la source. Le bootloader doit ensuite etre
# flashe par SWD (il n'est pas transporte par les paquets).
#
# Usage: scripts/ota/build_abort_bl.sh <1..8|0>
#
#   1 CRC   2 sauvegarde CM7   3 sauvegarde CM4   4 effacement CM7
#   5 effacement CM4   6 flash CM7   7 flash CM4   8 donnees externes
#   0 remet un bootloader normal
#
# La coupure ne frappe qu'a la premiere tentative d'application : la reprise est
# ce qu'on veut mettre a l'epreuve, pas le compteur de tentatives.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
HEADER="$ROOT/Common/Inc/ota_fault_inject.h"

STEP="${1:-}"
if ! [[ "$STEP" =~ ^[0-8]$ ]]; then
    sed -n '/Coupure secteur simulee/,/NE JAMAIS COMMITTER/p' "$HEADER"
    echo "Usage: $0 <0..8>"
    exit 1
fi

restore() {
    sed -i '' "s/^#define SP3CTRA_OTA_ABORT_AT_STEP .*/#define SP3CTRA_OTA_ABORT_AT_STEP 0/" "$HEADER"
    echo "SP3CTRA_OTA_ABORT_AT_STEP remis a 0"
}
trap restore EXIT INT TERM

cd "$ROOT"

if [ "$STEP" = "0" ]; then
    echo "=== Bootloader normal ==="
else
    echo "=== Coupure simulee au milieu de l'etape $STEP ==="
    sed -i '' "s/^#define SP3CTRA_OTA_ABORT_AT_STEP .*/#define SP3CTRA_OTA_ABORT_AT_STEP $STEP/" "$HEADER"
    grep -n "^#define SP3CTRA_OTA_ABORT_AT_STEP" "$HEADER"
fi

./scripts/build.sh bootloader release

echo
echo "Flasher par SWD (arreter la trace UART d'abord) :"
echo "  pkill -f stlink_uart_trace.py"
echo "  STM32_Programmer_CLI -c port=SWD mode=UR -w CM7_Bootloader/CM7/Release/Sp3ctra_CIS_Bootloader_CM7.elf"
