# Flash et logs par Ethernet, sans ST-Link — Sp3ctra Net Boot (SNB)

État : **validé sur cible** (Sp3ctra-77DD, 2026-09-16) : flash CM7 + CM4 par le
réseau, mise à jour du bootloader par ses deux slots, entrée par `POST /netboot`
et par journal `FAILED`, logs HTTP et UDP, OTA HTTP classique (T12) rejouée sur
le bootloader en -Os. Reste à jouer : entrée par les boutons, coupure au milieu
d'un `WRITE`.
Dernière révision : 2026-09-16
Périmètre : `CM7_Bootloader/`, `Common/{Inc,Src}/{log_ring,boot_mailbox,netboot_protocol}`,
`CM7/Application/Src/http_server.c`, `CM7/LWIP/Target/ethernetif.c`, `scripts/netboot/`

---

## 1. Ce que ça remplace, et pourquoi le bootloader

Le ST-Link servait à deux choses : programmer la flash par SWD, et lire la
trace UART par son pont USB-série. Les deux passent désormais par le port
Ethernet, avec le **bootloader** comme flasheur. Il tient ce rôle mieux que
l'application :

- il ne s'exécute pas depuis les zones qu'il écrit, donc aucun gel du cœur
  pendant l'effacement (le H7 bloque toute lecture d'une bank en cours
  d'écriture) ;
- il tourne en nu, sans RTOS ni lwIP : une boucle, une trame, une réponse ;
- il marche avec une application morte, un journal OTA incohérent, ou après un
  `FAILED` — les cas où seul le SWD sauvait la machine ;
- il écrit n'importe quelle zone : les deux cœurs, les deux slots, le journal,
  et l'autre slot du bootloader lui-même (§2.5).

Le temps de flash est celui de la physique, identique au ST-Link (datasheet
STM32H743, parallélisme x32) :

| Opération | Durée typique |
|---|---|
| Effacement d'un secteur de 128 Ko | 1,5 à 2 s |
| Programmation d'un mot de 32 o | 130 µs |
| Image CM7 de 392 Ko : 3 secteurs + programmation | ~5 s + 1,6 s |
| Image CM4 de 67 Ko : 1 secteur + programmation | ~1,7 s + 0,3 s |

Mesuré sur cible le 2026-09-16 (carte V3, `netflash.py` depuis le Mac) :

| Étape | Mesure |
|---|---|
| `POST /netboot` → bootloader joignable (dont ~1,6 s de renégociation PHY) | 5,5 s |
| Effacement d'un secteur de 128 Ko | 1,0 s |
| Écriture CM7 (402 Ko, blocs de 1 Ko, stop-and-wait) | 1,7 s |
| Écriture CM4 (80 Ko) | 0,3 s |
| CRC relu en place | 0,04 s |
| `BOOT` → application de retour en HTTP | 3,9 s |
| **CM7 seul, depuis l'application : total** | **15,2 s** |
| **CM7 + CM4, bootloader déjà en mode flasheur** | **11,0 s** |
| Référence ST-Link (`flash.sh`), CM7 : téléchargement + vérification, hors connexion | 6,6 s |

Les 5,5 s d'entrée se décomposent en un reset, la renégociation du lien (le
PHY est remis à zéro par le reset du MCU sur cette carte, voir §6) et le pas
de scrutation de l'outil. Un pré-effacement pendant la compilation et un
pull-up sur le reset du PHY ramèneraient la boucle sous les 10 s.

## 2. Architecture

```
hôte                              CIS
netflash.py ── POST /netboot ──▶ application : boîte aux lettres RAM + reset
            ◀─ INFO ──────────── bootloader  : mode flasheur (UDP 55152)
            ── ERASE/WRITE/CRC ▶              écrit la flash, relit, répond
            ── BOOT ───────────▶              reset → démarrage normal
netlog.py   ── GET /log ───────▶ application : anneau de logs (RAM retenue)
            ── LOG (UDP) ──────▶ bootloader  : même anneau, pendant le flash
```

### 2.1 Bootloader

| Fichier | Rôle |
|---|---|
| `Application/Src/netboot.c` | entrée (boîte aux lettres, boutons, `FAILED`), boucle, portage : flash, journal, logs, identité |
| `Application/Src/netboot_core.c` | pile ARP/IPv4/ICMP/UDP minimale + protocole SNB, **sans HAL** (compilable sur l'hôte) |
| `Application/Src/netboot_eth.c` | HAL ETH en scrutation, PHY LAN8742, pool de tampons en D2 (`.netboot_eth` du `.ld`) |
| `Peripheral/Src/lan8742.c` | copie du pilote BSP du firmware |
| `Drivers/.../stm32h7xx_hal_eth*.c` | copie du HAL ETH (même version 1.11.6 que le firmware) |

Entrée dans le mode flasheur, testée dans `main.c` juste après
`otaBoot_logResetCause()`, avant l'étape précoce :

1. **boîte aux lettres** (`Common/Inc/boot_mailbox.h`, 64 o en RAM retenue,
   scellée par CRC-32) : l'application y écrit `NETB` puis fait un reset
   logiciel. Le bootloader ne l'honore que si la cause de reset est bien
   `SOFT`, pour qu'une boîte laissée par un plantage ne piège pas la machine ;
   la demande est effacée dès qu'elle est lue ;
2. **deux boutons extérieurs** (PE13 + PE15) maintenus à la mise sous tension,
   anti-rebond 200 ms ;
3. **journal OTA en `FAILED`** (restauration impossible) : remplace l'ancien
   message « SERVICE REQUIRED (SWD) ».

Dans ce mode : D-cache coupé (le DMA Ethernet lit et écrit la RAM sans
maintenance de cohérence, la flash borne de toute façon la vitesse), IWDG
rafraîchi dans la boucle, écran OLED « NETWORK FLASH MODE » + IP, lien PHY
scruté toutes les 200 ms, MAC démarré ou arrêté en conséquence. Sans hôte
pendant 120 s, une entrée par boîte ou boutons rend la main au démarrage
normal ; une entrée sur `FAILED` reste.

L'adresse IP est celle que l'application utilisait (transmise par la boîte),
sinon `192.168.100.1/24`. `DISCOVER` en broadcast répond quelle que soit
l'adresse.

### 2.2 Application (CM7) et page UPDATE

- `POST /netboot` → réponse `202`, fermeture de la connexion, boîte aux
  lettres remplie avec IP et masque courants, `System_SafeReset()` 150 ms plus
  tard (pas les 3 s de l'upload historique) ;
- `GET /log?src=cm7|cm4&since=N&max=M` → texte brut, en-têtes `X-Log-Next`
  (position à redemander) et `X-Log-Head` ; sans `since`, les `M` derniers
  octets (4096 par défaut) ;
- la page **UPDATE** du serveur web gagne deux sections : « NETWORK FLASH »
  (bouton `Enter flash mode` → `POST /netboot`, puis attente du retour de
  l'application) et « DEVICE LOG » (source CM7/CM4, Refresh, Follow à 500 ms,
  lecture incrémentale par `X-Log-Next`) ;
- le PHY n'est plus remis à zéro au démarrage : `gpio.c` le laisse relâché
  (`PinState=GPIO_PIN_SET`, désormais dans le `.ioc`), `main.c` ne pulse plus,
  et `ethernetif.c` ne le réinitialise que s'il ne répond pas sur le MDIO. Le
  lien survit ainsi aux resets logiciels : flash réseau, changement de DPI.

### 2.3 Anneau de logs (`Common/Inc/log_ring.h`)

Moitié haute de la SRAM4 (D3), retirée des trois linker scripts
(`RAM_D3`/`RAM_D4` : 64 K → 32 K) :

| Adresse | Taille | Contenu |
|---|---|---|
| `0x38008000` | 64 o | boîte aux lettres de démarrage |
| `0x38008040` | 32 o + 20 Ko | anneau CM7 : bootloader **et** application, à la suite |
| `0x3800D060` | 32 o + 11,9 Ko | anneau CM4 |

`_write()` (donc tout `printf`) y écrit sur les trois images, avec un préfixe
par ligne `[T ssss.mmm] ` où `T` vaut `B` (bootloader), `7` ou `4`. Le contenu
survit aux resets logiciels et IWDG : la trace du bootloader (cause de reset,
phase OTA) se lit après coup. Le CM7 nettoie ses lignes de cache après chaque
écriture et invalide avant de lire l'anneau du CM4 ; le CM4 n'a pas de cache.
La recopie sur l'UART reste active (`LOG_RING_UART_ECHO 1`), donc la trace
série est désormais horodatée elle aussi. Le CM4, qui n'initialise aucun UART,
devient lisible pour la première fois.

## 3. Protocole SNB

UDP, port **55152**, tous les champs petit-boutiens, contrat dans
`Common/Inc/netboot_protocol.h` (recopié en Python dans `scripts/netboot/netboot.py`).
En-tête de 8 o : `magic "SNB1"`, `type`, `flags`, `seq`. Une requête par
datagramme, l'hôte retransmet sur silence, la réponse recopie `seq`.

| Requête | Charge | Réponse |
|---|---|---|
| `DISCOVER 0x01` | — (broadcast accepté, ne lie pas de session) | `INFO 0x81` : MAC, IP, UID, nom, version BL, journal, cause de reset, raison d'entrée |
| `ERASE 0x02` | `addr, len` (aligné secteur) | `ACK` avec la durée mesurée |
| `WRITE 0x03` | `addr, len` + données (multiples de 32, ≤ 1024) | `ACK` |
| `READ 0x04` | `addr, len` (≤ 1024) | `DATA 0x84` |
| `CRC 0x05` | `addr, len` | `CRC_REPLY 0x85` (CRC-32 zlib relu en place) |
| `LOG 0x06` | `since, max, src` | `LOG_REPLY 0x86` : `next` + texte |
| `JOURNAL 0x07` | `phase, trial, rollback, pending` (`phase` 0 = effacer) | `ACK` |
| `BOOT 0x08` | `mode` (0) | `ACK`, puis reset 100 ms plus tard |
| `PING 0x09` | — | `ACK` |
| `BOOTSEL 0x0A` | `addr` (slot A ou B du bootloader) | `ACK` ; `BOOT_ADD0` bascule au prochain reset |

Codes d'`ACK` : `OK`, `ARG`, `RANGE`, `FLASH`, `BUSY`, `VERIFY`, `NOT_ERASED`,
`UNKNOWN`. Règles :

- zone accessible en effacement et écriture : toute la flash sauf le secteur
  d'où le bootloader s'exécute (slot A ou B, décidé à l'exécution) ; le journal
  OTA (secteur 1) est inclus, ce qui permet sa remise à zéro ;
- `WRITE` est **idempotent** : un mot déjà porteur des données est ignoré (une
  retransmission ne casse rien), un mot non vierge et différent est refusé ;
  chaque mot est relu après programmation ;
- **session exclusive** : le premier hôte qui envoie autre chose que
  `DISCOVER` la prend ; un autre hôte reçoit `BUSY` pendant 10 s de silence ;
  le même hôte depuis un autre port (nouvelle invocation de l'outil) reprend la
  main ; le drapeau `RELEASE` (0x01) libère la session à la fin d'une requête,
  l'outil l'envoie en sortie de processus.

### 2.5 Mise à jour du bootloader : deux slots et `BOOT_ADD0`

Le bootloader ne peut pas écrire le secteur d'où il s'exécute, mais le H7
choisit son adresse de démarrage par l'option byte `BOOT_ADD0`. Il existe donc
deux slots, A en `0x08000000` et B en `0x080E0000` (secteur 7 de la bank 1,
réservé de longue date), et deux images liées, produites par
`scripts/ota/link_slot_b.sh bootloader` à partir des mêmes objets. Séquence de
`./scripts/netflash.sh bootloader release` :

1. `INFO` dit quel slot tourne (`boot_slot`, déduit de `VTOR`, que le
   bootloader pose sur sa propre table des vecteurs dès l'entrée de `main`) ;
2. l'image liée pour l'**autre** slot y est effacée, écrite, vérifiée ;
3. `BOOTSEL` : le bootloader vérifie la table des vecteurs du slot cible
   (pointeur de pile en RAM, vecteur de reset dans le slot, bit Thumb), puis
   programme `BOOT_ADD0` par `HAL_FLASHEx_OBProgram` + `OB_Launch` ;
4. `BOOT` : le nouveau bootloader démarre l'application ; l'outil rentre alors
   une seconde fois en mode flasheur pour lire `INFO` et confirmer version et
   slot, puis rend l'application.

L'ancien slot reste intact : si le nouveau bootloader ne démarrait pas, seul le
SWD ramènerait `BOOT_ADD0`, la broche BOOT0 étant câblée à la masse. C'est le
même risque que `flash.sh bootloader`, avec en plus la vérification d'image.
Chaque mise à jour alterne A → B → A ; le flasheur refuse toujours le secteur
qu'il occupe, l'outil aussi.

### 2.4 VST

La page SETUP de la source Sp3ctra (`Sp3ctra_VST`, `SourceSetupPanel`) gagne
« Flash via Network (bootloader) » à côté de « Upload Firmware » : même fichier
(paquet `cis_package_x.y.z.bin`, ou image CM7 brute), mais écrit par le client
SNB natif `communication/device/Sp3ctraNetFlash.cpp` : `POST /netboot`, attente
du bootloader, effacement, écriture, CRC, `BOOT`, attente de l'application, avec
la progression dans la barre existante. Un paquet est vérifié (taille, CRC-32)
avant tout envoi.

## 4. Utilisation

```bash
# boucle de développement : compiler, puis flasher les deux cœurs et attendre l'app
./scripts/build.sh cm7 release && ./scripts/build.sh cm4 release
./scripts/netflash.sh all release          # même interface que flash.sh
./scripts/netflash.sh cm7 release --show-log

# l'outil sous-jacent, avec des images choisies
scripts/netboot/netflash.py flash --cm7 CM7/Release/Sp3ctra_CIS_Firmware_CM7.bin \
                                  --cm4 CM4/Release/Sp3ctra_CIS_Firmware_CM4.bin

# un seul cœur, en gardant le bootloader en mode flasheur pour enchaîner
scripts/netboot/netflash.py flash --cm7 CM7/Release/Sp3ctra_CIS_Firmware_CM7.elf --no-boot
scripts/netboot/netflash.py boot

# mettre à jour le bootloader lui-même (écrit l'autre slot, bascule BOOT_ADD0)
./scripts/build.sh bootloader release && ./scripts/netflash.sh bootloader release

# qui est en mode flasheur sur le réseau ? état d'un appareil ?
scripts/netboot/netflash.py discover
scripts/netboot/netflash.py info --host 192.168.100.1

# lecture mémoire, CRC, journal
scripts/netboot/netflash.py read --addr 0x08020000 --len 64
scripts/netboot/netflash.py crc --addr 0x08100000 --len 392344
scripts/netboot/netflash.py journal clear

# trace en continu, application ou bootloader, sans UART (remplace uart_trace.sh)
./scripts/netlog.sh                        # CM7 (bootloader + application)
./scripts/netlog.sh all --terminal         # les deux cœurs, dans sa propre fenêtre
```

`flash` enchaîne : `POST /netboot` (sauf `--no-enter`), attente du bootloader,
effacement secteur par secteur avec la durée mesurée, écriture avec
progression, CRC comparé au fichier, `BOOT`, puis attente du retour HTTP de
l'application (`--wait-app`, 30 s par défaut) avec le temps total. Les `.elf`
sont convertis par `arm-none-eabi-objcopy` (celui de STM32CubeIDE si aucun
n'est dans le PATH). `--show-log` intercale la trace du bootloader.

Si l'application ne répond pas en HTTP et qu'aucun bootloader ne répond en
UDP : mise sous tension avec **les deux boutons extérieurs maintenus**.

Note macOS : le terminal doit avoir l'autorisation « Réseau local ». Symptôme
sinon : `No route to host` dès le premier envoi, HTTP compris, alors que le
broadcast passe. Réglages > Confidentialité et sécurité > Réseau local, activer
l'application du terminal, puis la **relancer** : le verdict est mis en cache par
processus. `./scripts/netflash.sh … --relay` délègue à Terminal.app entre-temps
(`scripts/ota/relay.sh`).

## 5. Mise en service

Le bootloader **1.3.0** (flasheur + slots A/B) se flashe une fois par SWD, c'est
le dernier passage de la sonde : les versions suivantes passeront par
`netflash.sh bootloader` (§2.5).

```bash
./scripts/build.sh bootloader release
./scripts/flash.sh bootloader release
```

Puis l'application par le réseau. L'ordre a de l'importance : une application
antérieure à ce chantier ne connaît pas `POST /netboot`, il faut alors entrer
par les boutons ; un bootloader antérieur ignore la boîte aux lettres et
démarre normalement.

Le bootloader est maintenant compilé en **-Os** (`.cproject`, Release) : en -O0
il ne tenait plus avec l'Ethernet. Mesures :

| Image | Flash occupée | Marge sur 128 Ko |
|---|---|---|
| bootloader -O0, avant | 114,6 Ko | 13,4 Ko |
| bootloader -Os + Ethernet + flasheur | 89,8 Ko | 38,3 Ko |

## 6. Validation sur cible

Joué le 2026-09-16 sur Sp3ctra-77DD, carte V3 (✔ = passé, ○ = reste à jouer) :

1. ✔ **-Os** : T12 (mise à jour HTTP saine) passé, retour en 24 s, `image
   confirmed`. Rejouer le reste de la matrice OTA (`docs/PLAN_FIRMWARE_UPDATE.md` §6), c'est
   la première fois que le bootloader tourne autrement qu'en -O0 ; le saut
   vers l'application est passé en assembleur (`msr msp` + `bx`) pour ne plus
   dépendre de l'allocation des locales ;
2. ✔ **Ethernet dans le bootloader** : `POST /netboot` → écran « NETWORK FLASH
   MODE » → `netflash.py discover` répond → `ping`, `read` du journal, `flash`
   d'un CM7 puis `boot`, application de retour ; chronométrer ;
3. ✔ mesuré, **non persistant sur la V3** : `NETBOOT: link down` puis `link up`
   1,6 s après chaque reset, car le reset interne du MCU sort sur NRST et la
   ligne PC14 flotte pendant ce temps. La LED du connecteur clignote à chaque
   reset. Sur la carte V4, `R11` (10 k) tire `nRST` du PHY à la
   masse : pendant le reset du MCU la broche PK5 flotte et le PHY se remet à
   zéro seul. Passer R11 en pull-up règle le cas ; sur V3 la netlist n'est pas
   dans l'espace de travail, mesurer ;
4. ✔ **logs** : `GET /log` rend la trace du bootloader après le démarrage de
   l'application, `--src cm4` rend « CM4 BOOT » ; `netlog.py` avant, pendant et après un flash ; vérifier que la
   trace du bootloader apparaît dans `GET /log` une fois l'application
   démarrée, et que les lignes du CM4 sortent sur `--src cm4` ;
5. ○ **boutons** : mise sous tension avec PE13 + PE15 tenus → mode flasheur ;
   relâchés → démarrage normal ;
6. ✔ **`FAILED`** : `netflash.py journal failed`, `boot` → le bootloader entre
   seul en mode flasheur (`reason: OTA journal in FAILED state`), `journal clear`
   + `boot` rend l'application ;
7. ✔ **délai sans hôte** : `POST /netboot` puis ne rien faire → retour à
   l'application après 120 s ;
8. ○ **coupure au milieu d'un `WRITE`** : relancer la même commande, les mots
   déjà écrits sont ignorés et le CRC final passe ;
9. ○ **IWDG** : entrer en mode flasheur depuis une image en essai (chien de garde
   armé) et y rester plus de 10 s ;
10. ✔ **bootloader A → B → A** : `netflash.sh bootloader release` deux fois de
    suite (2026-09-16). Écriture du slot 0,4 s, `BOOT_ADD0` basculé, application
    de retour 11 s après l'entrée, `INFO` rapporte `1.3.0 (slot B)` puis
    `(slot A)`. Le bootloader se met désormais à jour sans sonde.

## 7. Pièges connus

- `CM7_Bootloader/CM7/Core/Inc/stm32h7xx_hal_conf.h` (`HAL_ETH_MODULE_ENABLED`,
  `ETH_RX_DESC_CNT 8`) est généré par CubeMX et le `.ioc` du bootloader ne
  déclare pas l'ETH : une régénération les perdra. À rejouer à la main, comme
  les patchs de `scripts/post_cubemx_restore.sh` côté firmware.
- Le projet CubeIDE du bootloader lie chaque source du HAL une par une dans
  son `.project` : `stm32h7xx_hal_eth.c` et `_eth_ex.c` y sont déclarés, sans
  quoi une régénération des makefiles les oublie et l'édition de liens échoue.
- `scripts/sync_subdir_mk.py` ignore les dossiers `Drivers` ; les deux fichiers
  HAL ETH ont été ajoutés au `subdir.mk` par un appel direct de sa fonction
  `sync()` (voir l'historique). Toute régénération CubeIDE les redécouvre seule.
- La trace UART porte maintenant le préfixe `[T ssss.mmm] ` : les outils qui
  la lisent par sous-chaîne (`ota_test.py`) ne sont pas affectés, ceux qui
  compareraient un début de ligne le seraient.
- `_write()` de `Common/Src/log_ring.c` remplace la version faible de
  `syscalls.c` sur les trois images ; ne pas en redéfinir une autre.
- Le mode flasheur coupe le D-cache et ne le rallume pas : tout retour à
  l'application passe par un reset, jamais par un saut.
- Le broadcast `DISCOVER` répond même si l'IP configurée ne correspond pas au
  sous-réseau de l'hôte ; l'unicast qui suit exige, lui, une route.
- `ACK.elapsed_ms` vaut 0 pour une opération en bank 1 (CM4, journal) : le
  bootloader s'exécute depuis cette bank et le cœur est gelé pendant
  l'effacement, tick compris. L'outil mesure aussi côté hôte.
- Aucune authentification : réseau local de confiance, comme le reste des
  services de l'appareil (décision du 2026-09-02).

## 8. Tests sur l'hôte

```bash
scripts/netboot/sim/build.sh          # cc natif, zlib
scripts/netboot/sim/test_core         # ARP, ICMP, DISCOVER, WRITE, session, CRC, LOG, BOOT
scripts/netboot/sim/netboot_sim &     # le vrai cœur derrière un tunnel UDP local
scripts/netboot/netflash.py --host 127.0.0.1 flash --cm7 fw.bin --no-enter --wait-app 0
```

Le simulateur (`scripts/netboot/sim/netboot_sim.c`) lie `netboot_core.c` tel
quel à une flash en RAM, 100 ms par secteur effacé et 130 µs par mot écrit,
et enveloppe chaque datagramme dans une trame Ethernet/IP/UDP factice : ce qui
y passe passe aussi sur cible, aux pilotes près.
