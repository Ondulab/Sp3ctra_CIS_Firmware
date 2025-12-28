#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
TOOL_DIR="${ROOT_DIR}/scripts/makefsdata"

HTTP_DIR="${ROOT_DIR}/Middlewares/Third_Party/LwIP/src/apps/http"
FS_DIR="${HTTP_DIR}/fs"
OUT_FILE="${HTTP_DIR}/fsdata.c"

MAKEFSDATA_BIN="${TOOL_DIR}/makefsdata"

if [[ ! -x "${MAKEFSDATA_BIN}" ]]; then
  echo "[makefsdata] Binary not found: ${MAKEFSDATA_BIN}"
  echo "[makefsdata] Build it first: (cd ${TOOL_DIR} && ./build.sh)"
  exit 1
fi

if [[ ! -d "${FS_DIR}" ]]; then
  echo "[makefsdata] Input directory not found: ${FS_DIR}"
  exit 1
fi

EXCLUDE_EXT_LIST="${EXCLUDE_EXT_LIST:-DS_Store,db}"

echo "[makefsdata] Generating: ${OUT_FILE}"
echo "[makefsdata] From:       ${FS_DIR}"
echo "[makefsdata] Excluding:  ${EXCLUDE_EXT_LIST}"

cd "${TOOL_DIR}"

"${MAKEFSDATA_BIN}" "${FS_DIR}" -f:"${OUT_FILE}" -x:"${EXCLUDE_EXT_LIST}" "$@"

echo "[makefsdata] Done."
