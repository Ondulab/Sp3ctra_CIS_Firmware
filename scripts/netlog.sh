#!/bin/bash

# Trace de l'appareil par Ethernet, sans UART ni ST-Link : remplace uart_trace.sh.
# Lit l'anneau de logs par HTTP (application) ou UDP (bootloader), voir docs/NETBOOT.md.
# Usage: ./scripts/netlog.sh [cm7|cm4|all] [--host IP] [--once] [--from-start] [--terminal]

show_help() {
    echo "Usage: $0 [source] [options]"
    echo ""
    echo "Arguments:"
    echo "  source  : cm7 (bootloader + application), cm4, or all (default: cm7)"
    echo ""
    echo "Options:"
    echo "  --host IP     : device address (default: 192.168.100.1)"
    echo "  --once        : one read, then exit (default: follow, Ctrl-C to stop)"
    echo "  --from-start  : replay the whole retained ring first"
    echo "  --terminal    : open the follower in a Terminal.app window (use it when this"
    echo "                  terminal lacks the macOS 'Local Network' permission)"
    echo "  --help        : Show this help message"
    echo ""
    echo "Lines are stamped by the device: [B 12.345] bootloader, [7 ...] CM7, [4 ...] CM4,"
    echo "seconds since each boot. Nothing is sent by the device between two reads."
    echo ""
    echo "Example:"
    echo "  $0                 # follow the CM7 trace"
    echo "  $0 all --terminal  # both cores, in its own window"
}

SRC="cm7"
HOST="192.168.100.1"
EXTRA=()
TERMINAL=0
POSITIONAL=0

while [ $# -gt 0 ]; do
    case "$1" in
        --help|-h) show_help; exit 0 ;;
        --host) HOST="$2"; shift ;;
        --host=*) HOST="${1#--host=}" ;;
        --once|--from-start) EXTRA+=("$1") ;;
        --terminal) TERMINAL=1 ;;
        --*) echo "Unknown option: $1"; show_help; exit 1 ;;
        *)
            if [ $POSITIONAL -eq 0 ]; then SRC="$1"
            else echo "Too many arguments"; show_help; exit 1; fi
            POSITIONAL=$((POSITIONAL + 1)) ;;
    esac
    shift
done

case "$(echo "$SRC" | tr '[:upper:]' '[:lower:]')" in
    cm7) SRC_ARG="cm7" ;;
    cm4) SRC_ARG="cm4" ;;
    all) SRC_ARG="cm7,cm4" ;;
    *) show_help; exit 1 ;;
esac

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
NETLOG="$SCRIPT_DIR/netboot/netlog.py"

if [ "$TERMINAL" -eq 1 ]; then
    # Terminal.app a la permission « Reseau local » ; la fenetre reste ouverte.
    RUN="$ROOT/build/ota/relay"
    mkdir -p "$RUN"
    CMD="$RUN/netlog-$$.command"
    {
        echo "#!/bin/bash"
        echo "cd '$ROOT'"
        printf 'python3 %q --host %q --src %q' "$NETLOG" "$HOST" "$SRC_ARG"
        for e in "${EXTRA[@]}"; do printf ' %q' "$e"; done
        echo
        echo "rm -f '$CMD'"
    } > "$CMD"
    chmod +x "$CMD"
    open -a Terminal "$CMD"
    echo "netlog opened in Terminal.app ($SRC_ARG @ $HOST)"
    exit 0
fi

exec python3 "$NETLOG" --host "$HOST" --src "$SRC_ARG" "${EXTRA[@]}"
