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
