#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
TOOL_DIR="${ROOT_DIR}/scripts/makefsdata"

cd "${TOOL_DIR}"

rm -f makefsdata fsdata.c fsdata.tmp fshdr.tmp

echo "[makefsdata] Cleaned."
