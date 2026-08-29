#!/usr/bin/env bash

# Régénère les pages web embarquées du serveur HTTP lwIP :
#   Middlewares/Third_Party/LwIP/src/apps/http/fs/  ->  .../http/fsdata.c
# Équivalent macOS/Linux de makeFSdata.exe (même source lwIP, même sortie).
# Compile l'outil hôte (scripts/makefsdata/makefsdata) si besoin.
#
# Usage: ./scripts/makefsdata.sh [--rebuild] [--check] [-- <options makefsdata>]

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TOOL_DIR="${ROOT_DIR}/scripts/makefsdata"
TOOL_BIN="${TOOL_DIR}/makefsdata"
TOOL_SRC="${TOOL_DIR}/makefsdata.c"

HTTP_DIR="${ROOT_DIR}/Middlewares/Third_Party/LwIP/src/apps/http"
FS_DIR="${HTTP_DIR}/fs"
OUT_FILE="${HTTP_DIR}/fsdata.c"

# Extensions ignorées (fichiers parasites macOS / Windows dans fs/)
EXCLUDE_EXT_LIST="${EXCLUDE_EXT_LIST:-DS_Store,db}"

show_help() {
    echo "Usage: $0 [--rebuild] [--check] [-- <options makefsdata>]"
    echo ""
    echo "Génère ${OUT_FILE#${ROOT_DIR}/}"
    echo "à partir de ${FS_DIR#${ROOT_DIR}/}"
    echo ""
    echo "Options:"
    echo "  --rebuild : force la recompilation de l'outil hôte (scripts/makefsdata/makefsdata)"
    echo "  --check   : ne modifie rien, retourne 1 si fsdata.c n'est pas à jour"
    echo "  --help    : affiche cette aide"
    echo "  --        : les options suivantes sont passées telles quelles à makefsdata"
    echo "              (ex. -11, -e, -c, -m, -svr:<name>, -x:<ext>, -xc:<ext>, -nossi, -ssi:<file>)"
    echo ""
    echo "Variables:"
    echo "  EXCLUDE_EXT_LIST : extensions exclues (défaut: ${EXCLUDE_EXT_LIST})"
    echo "  CC               : compilateur hôte pour l'outil (défaut: clang)"
    echo ""
    echo "Exemple:"
    echo "  $0"
    echo "  $0 --check"
    echo "  $0 -- -11 -m"
}

REBUILD=0
CHECK=0
EXTRA=()
while [[ $# -gt 0 ]]; do
    case "$1" in
        --help|-h) show_help; exit 0 ;;
        --rebuild) REBUILD=1 ;;
        --check)   CHECK=1 ;;
        --)        shift; EXTRA+=("$@"); break ;;
        -*)        EXTRA+=("$1") ;;
        *)         echo "[makefsdata] Argument inconnu: $1" >&2; show_help; exit 1 ;;
    esac
    shift
done

if [[ ! -d "${FS_DIR}" ]]; then
    echo "[makefsdata] Dossier source introuvable: ${FS_DIR}" >&2
    exit 1
fi

# --- 1. Outil hôte : compiler si absent, obsolète, forcé ou d'une autre architecture ---
needs_build() {
    [[ "${REBUILD}" == 1 ]] && return 0
    [[ ! -x "${TOOL_BIN}" ]] && return 0
    [[ "${TOOL_SRC}" -nt "${TOOL_BIN}" ]] && return 0
    file -b "${TOOL_BIN}" | grep -q "$(uname -m)" || return 0
    return 1
}

if needs_build; then
    "${TOOL_DIR}/build.sh"
fi

# --- 2. Génération dans un dossier temporaire (makefsdata écrit fsdata.tmp/fshdr.tmp dans le cwd) ---
WORK="$(mktemp -d "${TMPDIR:-/tmp}/makefsdata.XXXXXX")"
trap 'rm -rf "${WORK}"' EXIT
NEW_FILE="${WORK}/fsdata.c"

echo "[makefsdata] Source  : ${FS_DIR}"
echo "[makefsdata] Cible   : ${OUT_FILE}"
echo "[makefsdata] Exclus  : ${EXCLUDE_EXT_LIST}"

(
    cd "${WORK}"
    "${TOOL_BIN}" "${FS_DIR}" -f:"${NEW_FILE}" -x:"${EXCLUDE_EXT_LIST}" ${EXTRA[@]+"${EXTRA[@]}"}
)

if [[ ! -s "${NEW_FILE}" ]]; then
    echo "[makefsdata] Échec : aucun fichier généré." >&2
    exit 1
fi

# --- 3. Comparaison / installation ---
if [[ -f "${OUT_FILE}" ]] && cmp -s "${NEW_FILE}" "${OUT_FILE}"; then
    echo "[makefsdata] fsdata.c inchangé."
    exit 0
fi

if [[ "${CHECK}" == 1 ]]; then
    if [[ -f "${OUT_FILE}" ]]; then
        echo "[makefsdata] fsdata.c N'EST PAS à jour (relancer sans --check)."
    else
        echo "[makefsdata] fsdata.c absent (relancer sans --check)."
    fi
    exit 1
fi

cp "${NEW_FILE}" "${OUT_FILE}"
echo "[makefsdata] fsdata.c mis à jour ($(wc -c < "${OUT_FILE}" | tr -d ' ') octets)."
