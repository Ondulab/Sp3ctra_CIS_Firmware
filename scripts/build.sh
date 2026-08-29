#!/bin/bash

# Script de build pour STM32H745 (CM4 & CM7)
# Usage: ./scripts/build.sh [cm4|cm7|all] [debug|release]

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
    echo "Targets:"
    echo "  bootloader : Build CM7 bootloader only"
    echo "  cm4        : Build CM4 firmware only"
    echo "  cm7        : Build CM7 firmware only"
    echo "  all        : Build bootloader + CM7 + CM4 (full system)"
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

# Conversion de la config en CamelCase pour correspondre aux dossiers STM32CubeIDE
if [[ "$CONFIG_LOW" == "debug" ]]; then
    CONFIG="Debug"
elif [[ "$CONFIG_LOW" == "release" ]]; then
    CONFIG="Release"
else
    CONFIG="$CONFIG_ARG"
fi

# --- Détection de la Toolchain ---
# Les makefiles de Debug/ et Release/ sont générés par STM32CubeIDE pour SA toolchain
# ("GNU Tools for STM32" : options ST comme -fcyclomatic-complexity, inconnues du GCC ARM
# upstream). On la privilégie donc si elle est installée, même si un autre arm-none-eabi-gcc
# est dans le PATH. Surcharge possible : ARM_TOOLCHAIN_BIN=/chemin/vers/bin
CUBE_IDE_PATH="/Applications/STM32CubeIDE.app/Contents/Eclipse/plugins"
if [ -n "$ARM_TOOLCHAIN_BIN" ] && [ -x "$ARM_TOOLCHAIN_BIN/arm-none-eabi-gcc" ]; then
    echo "Using toolchain from ARM_TOOLCHAIN_BIN: $ARM_TOOLCHAIN_BIN"
    export PATH="$ARM_TOOLCHAIN_BIN:$PATH"
elif [ -d "$CUBE_IDE_PATH" ]; then
    TOOLCHAIN_DIR=$(ls -d "$CUBE_IDE_PATH"/com.st.stm32cube.ide.mcu.externaltools.gnu-tools-for-stm32.*/tools/bin 2>/dev/null | tail -n 1)
    if [ -n "$TOOLCHAIN_DIR" ] && [ -x "$TOOLCHAIN_DIR/arm-none-eabi-gcc" ]; then
        echo "Using STM32CubeIDE toolchain: $TOOLCHAIN_DIR"
        export PATH="$TOOLCHAIN_DIR:$PATH"
    fi
fi

if ! command -v arm-none-eabi-gcc >/dev/null 2>&1; then
    echo "Error: arm-none-eabi-gcc not found. Install STM32CubeIDE in /Applications or set ARM_TOOLCHAIN_BIN."
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

build_bootloader() {
    local bootloader_path=$1
    local config=$2
    echo "================================================"
    echo "Building Bootloader (CM7) in $config mode..."
    echo "================================================"

    if [ ! -d "$bootloader_path/$config" ]; then
        echo "Error: Directory $bootloader_path/$config does not exist."
        echo "Please generate the bootloader project configuration in STM32CubeIDE first."
        return 1
    fi

    cd "$bootloader_path/$config" || return 1
    make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4) all
    local status=$?
    cd "$PROJECT_ROOT" || return 1

    if [ $status -eq 0 ]; then
        echo "Bootloader build successful!"
    else
        echo "Bootloader build failed!"
        return 1
    fi
}

case "$TARGET" in
    bootloader)
        build_bootloader "CM7_Bootloader/CM7" "$CONFIG"
        ;;
    cm4)
        build_core "CM4" "$CONFIG"
        ;;
    cm7)
        build_core "CM7" "$CONFIG"
        ;;
    all)
        build_bootloader "CM7_Bootloader/CM7" "$CONFIG" && \
        build_core "CM7" "$CONFIG" && \
        build_core "CM4" "$CONFIG"
        ;;
    *)
        show_help
        exit 1
        ;;
esac
