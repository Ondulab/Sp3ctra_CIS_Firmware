#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
TOOL_DIR="${ROOT_DIR}/scripts/makefsdata"

cd "${TOOL_DIR}"

CC="${CC:-clang}"
CFLAGS=(
  -O2
  -Wall
  -Wextra
  -Wno-unused-parameter
  -Wno-unused-function
  -Wno-macro-redefined
  -I.
  -I"${ROOT_DIR}/Middlewares/Third_Party/LwIP/src/include"
  -I"${ROOT_DIR}/Middlewares/Third_Party/LwIP/src/core"
  -I"${ROOT_DIR}/Middlewares/Third_Party/LwIP/src"
  -DHTTPD_SERVER_AGENT="\"lwIP\""
)

echo "[makefsdata] Building with ${CC}..."
"${CC}" "${CFLAGS[@]}" -o makefsdata makefsdata.c

echo "[makefsdata] OK -> ${TOOL_DIR}/makefsdata"
