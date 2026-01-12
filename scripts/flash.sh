#!/bin/bash

# Script de flash pour STM32H745 (CM4 & CM7)
# Usage: ./scripts/flash.sh [bootloader|cm4|cm7|all] [debug|release]

show_help() {
    echo "Usage: $0 [target] [config]"
    echo ""
    echo "Arguments:"
    echo "  target  : bootloader, cm4, cm7, or all (default: all)"
    echo "  config  : debug or release (default: release)"
    echo ""
    echo "Options:"
    echo "  --help  : Show this help message"
    echo ""
    echo "Example:"
    echo "  $0 all release"
    echo "  $0 cm7 debug"
    echo "  $0 bootloader release"
}

if [[ "$1" == "--help" ]]; then
    show_help
    exit 0
fi

TARGET_ARG=${1:-all}
CONFIG_ARG=${2:-Release}

# Conversion en minuscule compatible macOS/Bash 3.2
TARGET=$(echo "$TARGET_ARG" | tr '[:upper:]' '[:lower:]')
CONFIG_LOW=$(echo "$CONFIG_ARG" | tr '[:upper:]' '[:lower:]')

# Conversion de la config en CamelCase
if [[ "$CONFIG_LOW" == "debug" ]]; then
    CONFIG="Debug"
elif [[ "$CONFIG_LOW" == "release" ]]; then
    CONFIG="Release"
else
    CONFIG="$CONFIG_ARG"
fi

# --- Détection de STM32_Programmer_CLI ---
if ! command -v STM32_Programmer_CLI >/dev/null 2>&1; then
    echo "STM32_Programmer_CLI not found in PATH, searching in STM32CubeIDE paths..."
    # Chemins typiques sur macOS
    CUBE_IDE_PATH="/Applications/STM32CubeIDE.app/Contents/Eclipse/plugins"
    if [ -d "$CUBE_IDE_PATH" ]; then
        PROG_BIN=$(find "$CUBE_IDE_PATH" -name "STM32_Programmer_CLI" -type f | head -n 1)
        if [ -n "$PROG_BIN" ]; then
            PROG_DIR=$(dirname "$PROG_BIN")
            echo "Found programmer at: $PROG_DIR"
            export PATH="$PROG_DIR:$PATH"
        fi
    fi
fi

if ! command -v STM32_Programmer_CLI >/dev/null 2>&1; then
    echo "Error: STM32_Programmer_CLI not found. Please install STM32CubeProgrammer or ensure STM32CubeIDE is in /Applications."
    exit 1
fi
# -----------------------------------------

# Chemins des binaires (ELF)
CM7_ELF=$(find CM7/$CONFIG -name "*.elf" | head -n 1)
CM4_ELF=$(find CM4/$CONFIG -name "*.elf" | head -n 1)
BOOTLOADER_ELF=$(find CM7_Bootloader/CM7/$CONFIG -name "*.elf" | head -n 1)

flash_elf() {
    local elf=$1
    local core_name=$2

    if [ -z "$elf" ] || [ ! -f "$elf" ]; then
        echo "Error: Binary for $core_name not found in $CONFIG mode."
        echo "Please run ./scripts/build.sh $core_name $CONFIG first."
        return 1
    fi

    echo "Flashing $core_name ($elf)..."
    # Utilisation de STM32_Programmer_CLI
    STM32_Programmer_CLI -c port=SWD -w "$elf" -v -rst
    return $?
}

case "$TARGET" in
    bootloader)
        flash_elf "$BOOTLOADER_ELF" "Bootloader"
        ;;
    cm4)
        flash_elf "$CM4_ELF" "CM4"
        ;;
    cm7)
        flash_elf "$CM7_ELF" "CM7"
        ;;
    all)
        flash_elf "$BOOTLOADER_ELF" "Bootloader" && \
        flash_elf "$CM7_ELF" "CM7" && \
        flash_elf "$CM4_ELF" "CM4"
        ;;
    *)
        show_help
        exit 1
        ;;
esac
