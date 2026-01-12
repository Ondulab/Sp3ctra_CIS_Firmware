#!/bin/bash

# Script de nettoyage pour STM32H745 (CM4 & CM7)
# Usage: ./scripts/clean.sh [bootloader|cm4|cm7|all] [debug|release]

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
    echo "  $0 all"
    echo "  $0 cm4 debug"
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

PROJECT_ROOT=$(pwd)

clean_core() {
    local core=$1
    local config=$2
    echo "Cleaning $core in $config mode..."

    if [ -d "$core/$config" ]; then
        cd "$core/$config" || return 1
        make clean
        cd "$PROJECT_ROOT" || return 1
    else
        echo "Directory $core/$config not found, skipping."
    fi
}

clean_bootloader() {
    local config=$1
    local bootloader_path="CM7_Bootloader/CM7"

    echo "Cleaning Bootloader (CM7) in $config mode..."

    if [ -d "$bootloader_path/$config" ]; then
        cd "$bootloader_path/$config" || return 1
        make clean
        cd "$PROJECT_ROOT" || return 1
    else
        echo "Directory $bootloader_path/$config not found, skipping."
    fi
}

case "$TARGET" in
    bootloader)
        clean_bootloader "$CONFIG"
        ;;
    cm4)
        clean_core "CM4" "$CONFIG"
        ;;
    cm7)
        clean_core "CM7" "$CONFIG"
        ;;
    all)
        clean_bootloader "$CONFIG"
        clean_core "CM7" "$CONFIG"
        clean_core "CM4" "$CONFIG"
        ;;
    *)
        show_help
        exit 1
        ;;
esac
