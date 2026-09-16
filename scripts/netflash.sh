#!/bin/bash

# Flash par Ethernet, sans ST-Link, via le flasheur reseau du bootloader.
# Meme interface que scripts/flash.sh (docs/NETBOOT.md pour le detail).
# Usage: ./scripts/netflash.sh [cm4|cm7|all] [debug|release] [--host IP] [--no-boot] [--show-log] [--relay]

show_help() {
    echo "Usage: $0 [target] [config] [options]"
    echo ""
    echo "Arguments:"
    echo "  target  : cm4, cm7, all (= cm7 + cm4) or bootloader (default: all)"
    echo "            bootloader: writes the inactive bootloader slot (A/B) and switches BOOT_ADD0"
    echo "  config  : debug or release (default: release)"
    echo ""
    echo "Options:"
    echo "  --host IP  : device address (default: 192.168.100.1)"
    echo "  --no-boot  : leave the device in network flash mode afterwards"
    echo "  --show-log : print the bootloader trace while flashing"
    echo "  --relay    : run through Terminal.app (scripts/ota/relay.sh) when this terminal"
    echo "               lacks the macOS 'Local Network' permission ('No route to host')"
    echo "  --help     : Show this help message"
    echo ""
    echo "The device is rebooted into its bootloader (POST /netboot), the images are"
    echo "erased, written and verified over UDP, then it reboots and the script waits"
    echo "for the application to answer. If the application is not reachable, power"
    echo "the device on with the two outer buttons held to enter the flash mode."
    echo ""
    echo "Example:"
    echo "  $0 all release"
    echo "  $0 cm7 debug"
    echo "  $0 cm4 release --host 192.168.100.1"
}

TARGET_ARG="all"
CONFIG_ARG="Release"
HOST="192.168.100.1"
EXTRA=()
RELAY=0
POSITIONAL=0

while [ $# -gt 0 ]; do
    case "$1" in
        --help|-h) show_help; exit 0 ;;
        --host) HOST="$2"; shift ;;
        --host=*) HOST="${1#--host=}" ;;
        --no-boot|--show-log|--no-verify|--no-erase) EXTRA+=("$1") ;;
        --relay) RELAY=1 ;;
        --*) echo "Unknown option: $1"; show_help; exit 1 ;;
        *)
            if [ $POSITIONAL -eq 0 ]; then TARGET_ARG="$1"
            elif [ $POSITIONAL -eq 1 ]; then CONFIG_ARG="$1"
            else echo "Too many arguments"; show_help; exit 1; fi
            POSITIONAL=$((POSITIONAL + 1)) ;;
    esac
    shift
done

# Conversion en minuscule compatible macOS/Bash 3.2
TARGET=$(echo "$TARGET_ARG" | tr '[:upper:]' '[:lower:]')
CONFIG_LOW=$(echo "$CONFIG_ARG" | tr '[:upper:]' '[:lower:]')

if [[ "$CONFIG_LOW" == "debug" ]]; then
    CONFIG="Debug"
elif [[ "$CONFIG_LOW" == "release" ]]; then
    CONFIG="Release"
else
    CONFIG="$CONFIG_ARG"
fi

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
NETFLASH="$SCRIPT_DIR/netboot/netflash.py"

# Image d'un coeur : le .bin s'il existe, sinon le .elf (converti par objcopy).
# Les images *_SLOT_B sont liees pour le slot B, jamais demarre : exclues.
find_image() {
    local core_dir="$1"
    local img
    img=$(find "$ROOT/$core_dir/$CONFIG" -maxdepth 1 -name "*.bin" ! -name "*SLOT_B*" 2>/dev/null | head -n 1)
    if [ -z "$img" ]; then
        img=$(find "$ROOT/$core_dir/$CONFIG" -maxdepth 1 -name "*.elf" ! -name "*SLOT_B*" 2>/dev/null | head -n 1)
    fi
    echo "$img"
}

ARGS=()
case "$TARGET" in
    cm7|cm4|all) ;;
    bootloader)
        BL_DIR="$ROOT/CM7_Bootloader/CM7/$CONFIG"
        BL_ELF=$(find "$BL_DIR" -maxdepth 1 -name "*.elf" ! -name "*SLOT_B*" 2>/dev/null | head -n 1)
        if [ -z "$BL_ELF" ]; then
            echo "Error: bootloader not found in CM7_Bootloader/CM7/$CONFIG. Run ./scripts/build.sh bootloader $CONFIG_LOW first."
            exit 1
        fi
        BL_A="${BL_ELF%.elf}.bin"
        BL_B="${BL_ELF%.elf}_SLOT_B.bin"
        # Les deux images sortent des memes objets : le slot B est (re)lie si absent ou perime.
        if [ ! -f "$BL_B" ] || [ ! -f "$BL_A" ] || [ "$BL_ELF" -nt "$BL_B" ]; then
            "$SCRIPT_DIR/ota/link_slot_b.sh" bootloader "$CONFIG" || exit 1
        fi
        ARGS+=(--bl-a "$BL_A" --bl-b "$BL_B") ;;
    *) show_help; exit 1 ;;
esac

if [[ "$TARGET" == "cm7" || "$TARGET" == "all" ]]; then
    CM7_IMG=$(find_image CM7)
    if [ -z "$CM7_IMG" ]; then
        echo "Error: CM7 image not found in CM7/$CONFIG. Run ./scripts/build.sh cm7 $CONFIG_LOW first."
        exit 1
    fi
    ARGS+=(--cm7 "$CM7_IMG")
fi
if [[ "$TARGET" == "cm4" || "$TARGET" == "all" ]]; then
    CM4_IMG=$(find_image CM4)
    if [ -z "$CM4_IMG" ]; then
        echo "Error: CM4 image not found in CM4/$CONFIG. Run ./scripts/build.sh cm4 $CONFIG_LOW first."
        exit 1
    fi
    ARGS+=(--cm4 "$CM4_IMG")
fi

echo "================================================"
echo "Network flash: $TARGET ($CONFIG) -> $HOST"
echo "================================================"
if [ "$RELAY" -eq 1 ]; then
    # Terminal.app has the Local Network permission; relay.sh captures the output
    # and prints it when the command ends (no live progress).
    exec "$SCRIPT_DIR/ota/relay.sh" 300 python3 "$NETFLASH" --host "$HOST" flash "${ARGS[@]}" "${EXTRA[@]}"
fi
exec python3 "$NETFLASH" --host "$HOST" flash "${ARGS[@]}" "${EXTRA[@]}"
