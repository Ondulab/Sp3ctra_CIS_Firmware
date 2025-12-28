#!/bin/zsh
set -euo pipefail

# UART traces via STLINK-V3 (USB CDC serial) wrapper.
# - Creates/uses a local venv in scripts/.venv
# - Ensures pyserial is installed in the venv
# - Runs scripts/stlink_uart_trace.py (2,000,000 baud by default)

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
VENV_DIR="$SCRIPT_DIR/.venv"
PY="$VENV_DIR/bin/python"
PIP="$PY -m pip"

ensure_venv() {
  if [[ ! -x "$PY" ]]; then
    echo "[uart_trace] Creating venv at $VENV_DIR" >&2
    python3 -m venv "$VENV_DIR"
  fi
}

ensure_pyserial() {
  if ! "$PY" -c "import serial" >/dev/null 2>&1; then
    echo "[uart_trace] Installing pyserial into venv" >&2
    "$PIP" install -U pip >/dev/null
    "$PIP" install pyserial
  fi
}

main() {
  ensure_venv
  ensure_pyserial

  # Forward all args to the python script.
  exec "$PY" "$SCRIPT_DIR/stlink_uart_trace.py" "$@"
}

main "$@"
