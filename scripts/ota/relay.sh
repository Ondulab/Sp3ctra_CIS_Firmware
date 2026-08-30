#!/bin/bash
# Execute une commande via Terminal.app et rend sa sortie.
#
# macOS refuse l'acces au reseau local aux processus dont le verdict TCC a ete
# mis en cache avant que l'autorisation ne soit accordee ; le verdict ne se
# rafraichit qu'au relancement de l'application. Terminal.app, lui, dispose de
# l'autorisation : on lui delegue donc l'execution et on relit son journal.
#
# Usage: scripts/ota/relay.sh <timeout_s> <commande...>

set -uo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
RELAY="$ROOT/build/ota/relay"
mkdir -p "$RELAY"

TIMEOUT="${1:?timeout manquant}"; shift

# Un identifiant par invocation : avec des chemins fixes, une sonde lancee
# pendant qu'un test tourne encore ecrase son journal et son code de sortie.
# C'est arrive -- le resultat d'un T12 en cours a ete perdu de cette facon.
ID="$$-$(date +%s)"
LOG="$RELAY/relay-$ID.log"
DONE="$RELAY/relay-$ID.done"
CMD="$RELAY/run-$ID.command"

{
    echo "#!/bin/bash"
    echo "cd '$ROOT'"
    printf '%s\n' "$* > '$LOG' 2>&1"
    echo "echo \$? > '$DONE'"
    echo "rm -f '$CMD'"
    echo "exit 0"
} > "$CMD"
chmod +x "$CMD"

open -a Terminal "$CMD"

for ((i = 0; i < TIMEOUT; i++)); do
    [ -f "$DONE" ] && break
    sleep 1
done

[ -f "$LOG" ] && cat "$LOG"
if [ -f "$DONE" ]; then
    echo "--- code de sortie : $(cat "$DONE") ---"
else
    echo "--- toujours en cours apres ${TIMEOUT}s ---"
fi
