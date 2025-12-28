#!/bin/bash

# Script de build pour STM32H745 (CM4 & CM7)
# Usage: ./scripts/build.sh [cm4|cm7|all] [debug|release]

show_help() {
    echo "Usage: $0 [target] [config]"
    echo ""
    echo "Arguments:"
    echo "  target  : cm4, cm7, or all (default: all)"
    echo "  config  : debug or release (default: release)"
    echo ""
    echo "Options:"
    echo "  --help  : Show this help message"
    echo ""
    echo "Example:"
    echo "  $0 all release"
    echo "  $0 cm7 debug"
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

# Conversion de la config en CamelCase pour correspondre aux dossiers STM32CubeIDE
if [[ "$CONFIG_LOW" == "debug" ]]; then
    CONFIG="Debug"
elif [[ "$CONFIG_LOW" == "release" ]]; then
    CONFIG="Release"
else
    CONFIG="$CONFIG_ARG"
fi

# --- Détection de la Toolchain ---
if ! command -v arm-none-eabi-gcc >/dev/null 2>&1; then
    echo "arm-none-eabi-gcc not found in PATH, searching in STM32CubeIDE paths..."
    # Chemins typiques sur macOS
    CUBE_IDE_PATH="/Applications/STM32CubeIDE.app/Contents/Eclipse/plugins"
    if [ -d "$CUBE_IDE_PATH" ]; then
        TOOLCHAIN_BIN=$(find "$CUBE_IDE_PATH" -name "arm-none-eabi-gcc" -type f | head -n 1)
        if [ -n "$TOOLCHAIN_BIN" ]; then
            TOOLCHAIN_DIR=$(dirname "$TOOLCHAIN_BIN")
            echo "Found toolchain at: $TOOLCHAIN_DIR"
            export PATH="$TOOLCHAIN_DIR:$PATH"
        fi
    fi
fi

if ! command -v arm-none-eabi-gcc >/dev/null 2>&1; then
    echo "Error: arm-none-eabi-gcc not found. Please install it or ensure STM32CubeIDE is in /Applications."
    exit 1
fi
# ---------------------------------

PROJECT_ROOT=$(pwd)

build_core() {
    local core=$1
    local config=$2
    echo "------------------------------------------------"
    echo "Building $core in $config mode..."
    echo "------------------------------------------------"

    if [ ! -d "$core/$config" ]; then
        echo "Error: Directory $core/$config does not exist."
        echo "Please generate the project configuration in STM32CubeIDE first."
        return 1
    fi

    cd "$core/$config" || return 1
    make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4) all
    local status=$?
    cd "$PROJECT_ROOT" || return 1

    if [ $status -eq 0 ]; then
        echo "$core build successful!"
    else
        echo "$core build failed!"
        return 1
    fi
}

case "$TARGET" in
    cm4)
        build_core "CM4" "$CONFIG"
        ;;
    cm7)
        build_core "CM7" "$CONFIG"
        ;;
    all)
        build_core "CM7" "$CONFIG" && build_core "CM4" "$CONFIG"
        ;;
    *)
        show_help
        exit 1
        ;;
esac
