#!/bin/bash
# Compile le test unitaire et le simulateur du flasheur reseau sur l'hote.
set -e
HERE="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$HERE/../../.." && pwd)"
INC="-I$ROOT/Common/Inc -I$ROOT/CM7_Bootloader/CM7/Application/Inc"
CORE="$ROOT/CM7_Bootloader/CM7/Application/Src/netboot_core.c"
CFLAGS="-std=c11 -O1 -g -Wall -Wextra -Wno-unused-parameter $INC"
cc $CFLAGS -o "$HERE/test_core" "$HERE/test_core.c" "$CORE" -lz
cc $CFLAGS -o "$HERE/netboot_sim" "$HERE/netboot_sim.c" "$CORE" -lz
echo "built: $HERE/test_core $HERE/netboot_sim"
