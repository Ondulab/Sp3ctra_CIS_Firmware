#!/bin/bash
#
# post_cubemx_restore.sh
#
# Réapplique les réglages « à la main » que STM32CubeMX écrase à chaque
# régénération de code (HAL / .ioc). À lancer systématiquement après un
# « Generate Code » dans STM32CubeIDE, avant de builder.
#
# Usage:
#   ./scripts/post_cubemx_restore.sh              # applique les patchs
#   ./scripts/post_cubemx_restore.sh --check      # n'écrit rien, rapporte l'état
#
# Chaque patch est idempotent : relancer le script ne fait rien de plus.
# Note : les sources générées par CubeMX sont en CRLF -> les motifs
# multi-lignes utilisent \R (saut de ligne générique) et jamais \n.
#

set -u

DRY_RUN=0
FW_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

while [ $# -gt 0 ]; do
    case "$1" in
        --check|-n) DRY_RUN=1; shift ;;
        --help|-h)
            awk 'NR>1 && /^#/ {sub(/^# ?/, ""); print; next} NR>1 {exit}' "${BASH_SOURCE[0]}"
            exit 0 ;;
        *) echo "Argument inconnu : $1 (voir --help)"; exit 1 ;;
    esac
done

RED=$'\033[31m'; GRN=$'\033[32m'; YEL=$'\033[33m'; DIM=$'\033[2m'; RST=$'\033[0m'
N_APPLIED=0; N_OK=0; N_MISSING=0

section() { echo; echo "${DIM}────────────────────────────────────────────────────────${RST}"; echo "  $1"; echo "${DIM}────────────────────────────────────────────────────────${RST}"; }

# patch <libellé> <fichier> <regex-perl-de-recherche> <programme-perl-de-substitution>
#   - si <regex-de-recherche> ne matche pas -> patch déjà appliqué (ou motif disparu)
#   - le programme perl travaille sur le fichier entier en slurp ($_)
patch_file() {
    local label="$1" file="$2" probe="$3" prog="$4"
    if [ ! -f "$file" ]; then
        printf '  %s[absent]%s  %s %s(%s)%s\n' "$RED" "$RST" "$label" "$DIM" "${file#$FW_ROOT/}" "$RST"
        N_MISSING=$((N_MISSING + 1)); return
    fi
    # motif et programme passés par l'environnement : ni le shell ni le
    # délimiteur de perl ne peuvent alors casser sur / ( ) " ' contenus dedans.
    if ! PROBE="$probe" perl -0777 -ne 'exit($_ =~ /$ENV{PROBE}/sm ? 0 : 1)' "$file"; then
        printf '  %s[déjà ok]%s %s\n' "$GRN" "$RST" "$label"
        N_OK=$((N_OK + 1)); return
    fi
    if [ "$DRY_RUN" -eq 1 ]; then
        printf '  %s[à faire]%s %s\n' "$YEL" "$RST" "$label"
    else
        PROG="$prog" perl -0777 -i -pe 'eval $ENV{PROG}; die $@ if $@' "$file" || {
            printf '  %s[ÉCHEC]%s   %s\n' "$RED" "$RST" "$label"
            N_MISSING=$((N_MISSING + 1)); return
        }
        printf '  %s[appliqué]%s %s\n' "$YEL" "$RST" "$label"
    fi
    N_APPLIED=$((N_APPLIED + 1))
}

# ═══════════════════════════════════════════════════════════════════════
section "FIRMWARE — ADC overclocké (CM7/Core/Src/adc.c)"
# Résolution, overrun et temps d'échantillonnage sont pilotés par le .ioc.
# Restent ici ce que CubeMX ne sait pas produire : le prescaler PCLK/2 (son
# validateur le refuse au-delà de l'horloge nominale), la forme de MX_ADC3_Init
# et le triplet de fronts.
# ═══════════════════════════════════════════════════════════════════════
ADC="$FW_ROOT/CM7/Core/Src/adc.c"

patch_file "ClockPrescaler PCLK_DIV4 -> PCLK_DIV2 (ADC1/2/3)" "$ADC" \
    'ADC_CLOCK_SYNC_PCLK_DIV4' \
    's/ADC_CLOCK_SYNC_PCLK_DIV4/ADC_CLOCK_SYNC_PCLK_DIV2/g'

# CubeMX sort Resolution de la structure d'ADC3 et l'applique via un SECOND
# HAL_ADC_Init() (défaut de son modèle ADC3, cf. README). On remonte le champ
# dans la structure et on supprime le doublon, sans présumer de la valeur.
patch_file "ADC3 : Resolution remise dans la structure (2e HAL_ADC_Init supprimé)" "$ADC" \
    '\}\R  hadc3\.Init\.Resolution = ADC_RESOLUTION_\w+;\R  if \(HAL_ADC_Init\(&hadc3\)' \
    's/(hadc3\.Init\.ClockPrescaler = \w+;)(\R)(.*?  if \(HAL_ADC_Init\(&hadc3\) != HAL_OK\)\R  \{\R    Error_Handler\(\);\R  \}\R)  hadc3\.Init\.(Resolution = ADC_RESOLUTION_\w+;)\R  if \(HAL_ADC_Init\(&hadc3\) != HAL_OK\)\R  \{\R    Error_Handler\(\);\R  \}\R/${1}${2}  hadc3.Init.${4}${2}${3}/s'

# Fronts de TIM1_CC1 : ADC1 et ADC3 sur le montant, ADC2 sur le descendant
# (entrelacement du scan CIS). Une modification du front dans l'IHM CubeMX se
# propage aux trois instances -> on réimpose le triplet.
patch_file "ADC1 : front de déclenchement RISING" "$ADC" \
    'hadc1\.Init\.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_FALLING' \
    's/(hadc1\.Init\.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_)FALLING/${1}RISING/'

patch_file "ADC2 : front de déclenchement FALLING" "$ADC" \
    'hadc2\.Init\.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_RISING' \
    's/(hadc2\.Init\.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_)RISING/${1}FALLING/'

patch_file "ADC3 : front de déclenchement RISING" "$ADC" \
    'hadc3\.Init\.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_FALLING' \
    's/(hadc3\.Init\.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_)FALLING/${1}RISING/'

# ═══════════════════════════════════════════════════════════════════════
section "FIRMWARE — pile LwIP"
# ═══════════════════════════════════════════════════════════════════════
# fsdata.c (pages du serveur HTTP) est SUPPRIMÉ par la régénération LwIP,
# alors que fs.c l'inclut via HTTPD_FSDATA_FILE -> le build casse sur fs.o.
FSDATA="$FW_ROOT/Middlewares/Third_Party/LwIP/src/apps/http/fsdata.c"
if [ -f "$FSDATA" ]; then
    printf '  %s[déjà ok]%s fsdata.c présent (pages du serveur HTTP)\n' "$GRN" "$RST"
    N_OK=$((N_OK + 1))
elif [ "$DRY_RUN" -eq 1 ]; then
    printf '  %s[à faire]%s fsdata.c supprimé -> ./scripts/makefsdata.sh\n' "$YEL" "$RST"
    N_APPLIED=$((N_APPLIED + 1))
else
    if "$FW_ROOT/scripts/makefsdata.sh" >/dev/null 2>&1 && [ -f "$FSDATA" ]; then
        printf '  %s[appliqué]%s fsdata.c régénéré (makefsdata.sh)\n' "$YEL" "$RST"
        N_APPLIED=$((N_APPLIED + 1))
    else
        printf '  %s[ÉCHEC]%s   fsdata.c : lancer ./scripts/makefsdata.sh à la main\n' "$RED" "$RST"
        N_MISSING=$((N_MISSING + 1))
    fi
fi

# ═══════════════════════════════════════════════════════════════════════
section "FIRMWARE — adresse MAC par unité (CM7/LWIP/Target/ethernetif.c)"
# CubeMX régénère low_level_init() avec la MAC codée en dur 00:80:E1:00:00:00,
# identique sur toutes les unités. Le firmware dérive une MAC administrée
# localement (02:53:33:xx:xx:xx) de l'UID du MCU (Common/Src/sys_identity.c).
# ═══════════════════════════════════════════════════════════════════════
ETHIF="$FW_ROOT/CM7/LWIP/Target/ethernetif.c"

patch_file "ethernetif.c : include sys_identity.h" "$ETHIF" \
    '#include "ethernetif\.h"\R(?!#include "sys_identity\.h")' \
    's/(#include "ethernetif\.h")(\R)/${1}${2}#include "sys_identity.h"${2}/'

patch_file "ethernetif.c : MAC dérivée de l'UID (sys_identity_mac)" "$ETHIF" \
    'MACAddr\[0\] = 0x00;\R\s*MACAddr\[1\] = 0x80;' \
    's/(\s*)MACAddr\[0\] = 0x00;\R\s*MACAddr\[1\] = 0x80;\R\s*MACAddr\[2\] = 0xE1;\R\s*MACAddr\[3\] = 0x00;\R\s*MACAddr\[4\] = 0x00;\R\s*MACAddr\[5\] = 0x00;/${1}sys_identity_mac(MACAddr);   \/* per-unit locally administered MAC (see post_cubemx_restore.sh) *\//'

# ═══════════════════════════════════════════════════════════════════════
section "FIRMWARE — HAL fournisseur (écrasé à chaque mise à jour du HAL)"
# ═══════════════════════════════════════════════════════════════════════
# HAL_Init() configure l'ART accelerator du Cortex-M4 (son SEUL cache
# d'instructions). ST y met en dur 0x08100000 = Flash Bank 2, valable pour la
# répartition standard des cœurs. Ici les banques sont permutées : le firmware
# CM4 est linké en 0x08040000 (Bank 1). Avec l'adresse ST, l'ART cache la page
# où réside le firmware CM7 -> taux de hit nul, le CM4 subit la latence Flash
# sur chaque fetch et rame, sans le moindre message d'erreur.
patch_file "HAL_Init : base ART du CM4 -> 0x08040000 (Bank 1)" \
    "$FW_ROOT/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal.c" \
    '__HAL_ART_CONFIG_BASE_ADDRESS\(0x08100000UL\)' \
    's/__HAL_ART_CONFIG_BASE_ADDRESS\(0x08100000UL\);(\s*)\/\* Configure the Cortex-M4 ART Base address to the Flash Bank 2 :/__HAL_ART_CONFIG_BASE_ADDRESS(0x08040000UL);${1}\/* Configure the Cortex-M4 ART Base address to the Flash Bank 1 :/'

# ═══════════════════════════════════════════════════════════════════════
section "Bilan"
# ═══════════════════════════════════════════════════════════════════════
if [ "$DRY_RUN" -eq 1 ]; then
    echo "  Mode --check : rien n'a été écrit."
    echo "  $N_APPLIED patch(s) à appliquer, $N_OK déjà en place, $N_MISSING alerte(s)."
else
    echo "  $N_APPLIED patch(s) appliqué(s), $N_OK déjà en place, $N_MISSING alerte(s)."
    [ "$N_APPLIED" -gt 0 ] && echo "  -> rebuild : ./scripts/build.sh all release"
fi
[ "$N_MISSING" -gt 0 ] && exit 2
exit 0
