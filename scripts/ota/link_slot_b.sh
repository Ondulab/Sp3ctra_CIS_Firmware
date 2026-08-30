#!/bin/bash
# Produit l'image du slot B en RELIANT les objets deja compiles.
#
# Seule l'edition de liens distingue les deux slots : les .o sont identiques,
# c'est le placement qui change. Recompiler serait donc du gaspillage -- un
# second lien coute deux secondes la ou une compilation complete en coute
# quatre-vingt-dix.
#
# La commande de lien est extraite du makefile engendre par STM32CubeIDE plutot
# que recopiee ici : elle suit ainsi toute evolution des options sans risque de
# desynchronisation.
#
# Usage: scripts/ota/link_slot_b.sh <cm7|cm4> [Release|Debug]

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
CORE_ARG="${1:?core manquant (cm7 ou cm4)}"
CONFIG="${2:-Release}"

case "$(echo "$CORE_ARG" | tr '[:upper:]' '[:lower:]')" in
    cm7) CORE=CM7; SLOT_B_ADDR=0x08180000 ;;
    cm4) CORE=CM4; SLOT_B_ADDR=0x08060000 ;;
    *)   echo "core inconnu : $CORE_ARG (attendu cm7 ou cm4)"; exit 1 ;;
esac

BUILD_DIR="$ROOT/$CORE/$CONFIG"
LD_A="$ROOT/$CORE/STM32H745IIKX_FLASH.ld"
LD_B="$ROOT/$CORE/STM32H745IIKX_FLASH_SLOT_B.ld"

[ -d "$BUILD_DIR" ] || { echo "Build $CORE/$CONFIG absent : lancer ./scripts/build.sh $CORE_ARG $CONFIG"; exit 1; }
[ -f "$BUILD_DIR/objects.list" ] || { echo "objects.list absent dans $BUILD_DIR"; exit 1; }

# --- Toolchain, meme detection que build.sh -------------------------------
CUBE_IDE_PATH="/Applications/STM32CubeIDE.app/Contents/Eclipse/plugins"
if [ -n "${ARM_TOOLCHAIN_BIN:-}" ]; then
    export PATH="$ARM_TOOLCHAIN_BIN:$PATH"
elif [ -d "$CUBE_IDE_PATH" ]; then
    TC=$(ls -d "$CUBE_IDE_PATH"/com.st.stm32cube.ide.mcu.externaltools.gnu-tools-for-stm32.*/tools/bin 2>/dev/null | tail -n 1)
    [ -n "$TC" ] && export PATH="$TC:$PATH"
fi
command -v arm-none-eabi-gcc >/dev/null || { echo "arm-none-eabi-gcc introuvable"; exit 1; }

# --- Script de lien du slot B, engendre depuis celui du slot A ------------
# Un seul fichier fait autorite : dupliquer le .ld a la main garantirait qu'un
# jour les deux divergent sur une section, et le slot B tomberait en marche.
# Le sed de macOS ne connait pas \s : classes POSIX obligatoires.
sed -e "s|^\([[:space:]]*FLASH (rx)[[:space:]]*:[[:space:]]*ORIGIN = \)[^,]*|\1$SLOT_B_ADDR|" \
    -e "s|/\* Slot A\.|/* Slot B, ENGENDRE par scripts/ota/link_slot_b.sh -- ne pas editer.|" \
    "$LD_A" > "$LD_B"

grep -q "ORIGIN = $SLOT_B_ADDR" "$LD_B" || { echo "Echec de la reecriture de ORIGIN dans $LD_B"; exit 1; }

# --- Rejeu de la commande de lien, avec le .ld et les noms du slot B -------
ARTIFACT=$(sed -n 's/^BUILD_ARTIFACT_NAME := //p' "$BUILD_DIR/makefile")
LINK_CMD=$(grep -m1 "^	arm-none-eabi-gcc -o \"$ARTIFACT.elf\"" "$BUILD_DIR/makefile" | sed 's/^\t//')
[ -n "$LINK_CMD" ] || { echo "Commande de lien introuvable dans $BUILD_DIR/makefile"; exit 1; }

LINK_CMD=${LINK_CMD//$LD_A/$LD_B}
LINK_CMD=${LINK_CMD//$ARTIFACT.elf/${ARTIFACT}_SLOT_B.elf}
LINK_CMD=${LINK_CMD//$ARTIFACT.map/${ARTIFACT}_SLOT_B.map}

cd "$BUILD_DIR"
eval "$LINK_CMD"
arm-none-eabi-objcopy -O binary "${ARTIFACT}_SLOT_B.elf" "${ARTIFACT}_SLOT_B.bin"

echo "Slot B lie : $CORE/$CONFIG/${ARTIFACT}_SLOT_B.bin ($(wc -c < "${ARTIFACT}_SLOT_B.bin" | tr -d ' ') octets, base $SLOT_B_ADDR)"
