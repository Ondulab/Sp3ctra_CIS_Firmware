# Scripts de Build et Flash STM32H745

Ce dossier contient des scripts pour automatiser la compilation et le déploiement du firmware pour le STM32H745 (Dual Core CM4/CM7).

## Prérequis

- `arm-none-eabi-gcc` (GNU Arm Embedded Toolchain)
- `make`
- `STM32_Programmer_CLI` (partie de STM32CubeProgrammer)
- Les dossiers de build (`CM4/Release`, `CM7/Release`, etc.) doivent avoir été générés au moins une fois par STM32CubeIDE (ou contenir un Makefile valide).

## Utilisation

Tous les scripts acceptent deux arguments optionnels :
1. **Target** : `cm4`, `cm7` ou `all` (défaut: `all`)
2. **Config** : `debug` ou `release` (défaut: `release`)

### Compilation

```bash
./scripts/build.sh [target] [config]
```
Exemple : `./scripts/build.sh all release`
Utilisez `./scripts/build.sh --help` pour plus de détails.

### Flashage (via ST-Link SWD)

```bash
./scripts/flash.sh [target] [config]
```
Exemple : `./scripts/flash.sh all release`
Vous pouvez flasher un seul cœur : `./scripts/flash.sh cm7`
Utilisez `./scripts/flash.sh --help` pour plus de détails.

### Nettoyage

```bash
./scripts/clean.sh [target] [config]
```
Exemple : `./scripts/clean.sh all`

## Notes sur le Dual Core

Le script de flashage déploie d'abord le binaire CM7, puis le CM4. Le CM7 est responsable du démarrage du CM4 sur cette architecture.

## Après une régénération STM32CubeMX

CubeMX réécrit les fichiers `Core/Src/*.c` à chaque « Generate Code » et efface les
réglages qu'il ne sait pas exprimer (notamment l'ADC overclocké, que son validateur
refuse). Relancer systématiquement :

```bash
./scripts/post_cubemx_restore.sh
```

Le script est idempotent et réapplique, en vérifiant chaque motif :

| Fichier | Réglage restauré | Pourquoi CubeMX l'écrase |
|---|---|---|
| `CM7/Core/Src/adc.c` | `ClockPrescaler = PCLK_DIV2` | son validateur interdit `PCLK_DIV2` au-delà de l'horloge nominale |
| `CM7/Core/Src/adc.c` | fronts TIM1_CC1 : ADC1 **montant**, ADC2 **descendant**, ADC3 **montant** | changer le front dans l'IHM le propage aux trois instances |
| `CM7/Core/Src/adc.c` | ADC3 : `Resolution` remise dans la structure, 2ᵉ `HAL_ADC_Init()` supprimé (indépendant de la valeur) | défaut du modèle CubeMX pour ADC3 (voir plus bas) |
| `Drivers/.../Src/stm32h7xx_hal.c` | base ART du CM4 = `0x08040000` (Bank 1) | ST code en dur `0x08100000` ; fichier fournisseur, écrasé à chaque MAJ du HAL |
| `Middlewares/.../http/fsdata.c` | régénéré via `makefsdata.sh` s'il a disparu | la régénération LwIP le supprime, `fs.c` l'inclut → build cassé |



Plusieurs réglages autrefois repatchés sont désormais définitifs et sortis du script :
côté ADC, **résolution (10 bits), overrun (`DATA_PRESERVED`) et temps d'échantillonnage
(1,5 cycle)** sont portés par le `.ioc` et régénérés correctement — seul le prescaler y
échappe ; `MEMP_NUM_UDP_PCB=8` est dans le `.ioc` (CubeMX le régénère), les sections ETH du
`CM7/STM32H745IIKX_FLASH.ld` acceptent les deux orthographes `.Rx/TxDe[s]cripSection`
(le `.ld` n'est jamais régénéré), le **MDMA** (`TransferTriggerMode=MDMA_FULL_TRANSFER` et
`SourceBlockAddressOffset=source_offset` sur les canaux 1/2/3 — CubeMX accepte le symbole),
et le **DMA2D**, passé en « Do Not Generate Function Call » côté CM7 pour que `MX_DMA2D_Init()`
ne soit plus appelé depuis `main()`.


### ART accelerator du CM4

Le Cortex-M4 n'a pas de cache ; son seul accélérateur est l'**ART** (1 Ko, 64 lignes,
direct-mapped) qui cache ses fetch d'instructions depuis la Flash AXI. Il est configuré
par `HAL_Init()` — donc dans un fichier **fournisseur**, réécrit à chaque mise à jour du HAL :

```c
#if defined(DUAL_CORE) && defined(CORE_CM4)
   __HAL_RCC_ART_CLK_ENABLE();
   __HAL_ART_CONFIG_BASE_ADDRESS(0x08040000UL);   /* Flash Bank 1 — PAS le 0x08100000 de ST */
   __HAL_ART_ENABLE();
#endif
```

`ART_CTR.PCACHEADDR` ne retient que les bits [31:20] : la fenêtre cachée fait **1 Mo alignée**.
Ce projet permute les banques, le firmware CM4 est linké en `0x08040000` (Bank 1) alors que ST
code en dur `0x08100000` (Bank 2). Avec la valeur ST, l'ART cache la page où réside le firmware
**CM7** : taux de hit nul, le CM4 subit la latence Flash sur chaque fetch et devient très lent —
sans aucun message d'erreur, ce qui rend le symptôme difficile à rattacher à sa cause.

Contrôle après build : `arm-none-eabi-objdump -d` sur `HAL_Init` doit montrer `orr #0x8000`
(PCACHEADDR = 0x080) puis `orr #1`, soit `ART->CTR = 0x8001`.

### Le double `HAL_ADC_Init(&hadc3)`

CubeMX sort `Resolution` de `hadc3.Init` et l'applique via un second `HAL_ADC_Init()`.
Ce n'est pas un contournement d'errata ST : sur STM32H745 (`ADC_VER_V5_X`), ADC3 emprunte
exactement le même chemin que ADC1/ADC2 dans `HAL_ADC_Init()` — le traitement particulier
d'ADC3 (`__LL_ADC12_RESOLUTION_TO_ADC3`, `Init.DataAlign`) est encadré par `ADC_VER_V5_V90`,
qui n'existe que sur STM32H72x/H73x. Le `.ioc` n'est pas en cause non plus : `ADC3.Resolution`
y figure comme pour ADC1/2. C'est le modèle CubeMX d'ADC3 (paramètres `ClockPrescalerADC3`,
`ResolutionADC3`, surcharges invisibles sur `$IpNumber=3`), écrit pour l'ADC3 différent des
H72x/H73x, qui déborde sur le H745. Effet réel : le premier init configure ADC3 en 16 bits
(`hadc3` étant global donc à zéro, et `ADC_RESOLUTION_16B == 0`), le second applique la bonne
résolution. Sans conséquence puisque aucune conversion n'a démarré, mais le script rétablit
la forme à un seul init.

Option `--check` : rapporte l'état sans rien écrire.

## Traces UART via STLINK-V3 (USB Serial)

Le STLINK-V3 expose un endpoint USB CDC (type modem) qui apparaît sur macOS en `/dev/cu.usbmodemXXXX` (le suffixe peut changer).

### Prérequis

Créer le virtualenv local et installer `pyserial` (une seule fois) :

```bash
python3 -m venv scripts/.venv
scripts/.venv/bin/python -m pip install -U pip
scripts/.venv/bin/python -m pip install pyserial
```

### Lire les traces (2 000 000 bauds) avec reconnexion auto

Utilise le wrapper `.sh` (il crée le venv et installe `pyserial` automatiquement si besoin) :

```bash
./scripts/uart_trace.sh -v
```

Forcer le matching par numéro de série USB (recommandé si plusieurs sondes) :

```bash
./scripts/uart_trace.sh -v --serial 004600263432511130343838
```

Options utiles :
- `--baudrate 2000000` : changer le baudrate
- `--no-follow` : désactiver la reconnexion auto
- `--retry-s 1.0` : délai entre tentatives de reconnexion

Note : par défaut le script préfère `/dev/cu.*` (recommandé sur macOS). Tu peux autoriser `/dev/tty.*` avec `--allow-tty`.

## Pages web embarquées (fsdata.c)

Les pages du serveur HTTP lwIP (`Middlewares/Third_Party/LwIP/src/apps/http/fs/`) sont
compilées dans `fsdata.c`, inclus par `fs.c`. Après toute modification de `fs/`
(HTML, images…), régénérer ce fichier — sans passer par Windows ni `makeFSdata.exe` :

```bash
./scripts/makefsdata.sh
```

Le script compile l'outil hôte (`scripts/makefsdata/makefsdata`, source lwIP officielle,
avec `clang`) s'il est absent ou obsolète, puis génère `fsdata.c` en excluant les fichiers
parasites (`.DS_Store`, `Thumbs.db`). Le fichier n'est réécrit que s'il change.

Options utiles :
- `--check` : vérifie que `fsdata.c` est à jour sans le modifier (retourne 1 sinon)
- `--rebuild` : force la recompilation de l'outil hôte
- `-- <options>` : options passées à `makefsdata` (`-11`, `-e`, `-c`, `-m`, `-svr:<nom>`, `-xc:<ext>`…)

`fsdata.c` est versionné : penser à le committer avec les pages modifiées.
