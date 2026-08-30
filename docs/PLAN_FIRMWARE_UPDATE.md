# Plan — Mise à jour firmware CIS : robustesse et rollback

État : étape 0 implémentée et **validée sur cible** (rollback prouvé), matrice réseau restante
Dernière révision : 2026-08-30
Périmètre : `CM7_Bootloader/`, `CM7/Application/Src/http_server.c`, `Common/`, `scripts/ota/`

---

## 1. Mécanisme actuel

### 1.1 Chaîne complète

| Maillon | Fichier | Rôle |
|---|---|---|
| Générateur | `UpdateFileGen/updateFileGen.py` | assemble `cis_package_<version>.bin` |
| Transport | `CM7/Application/Src/http_server.c:354` | `POST /upload` multipart → `0:/firmware/` (NOR QSPI 16 Mo, FatFs) |
| Drapeau | `CM7/Peripheral/Src/stm32_flash.c:111` | un mot de 32 bits en secteur 1 bank1 |
| Flasheur | `CM7_Bootloader/CM7/Application/Src/update.c` | CRC → backup → erase → flash |
| Séquenceur | `CM7_Bootloader/CM7/Core/Src/main.c:250-400` | machine à états 5 valeurs |

Format du paquet : en-tête 24 o (`"BOOT"`, cm7_size, cm4_size, external_size, version[8]),
puis CM7.bin, CM4.bin, `External_MAX8.tar.gz`, puis un footer CRC-32 (zlib) sur tout ce qui précède.

### 1.2 Séquence nominale (trois reboots)

```
App    upload OK ──> écrit RECEIVED ──> reset
BL     RECEIVED  ──> CRC, backup CM7+CM4 vers la NOR, erase, flash, écrit TO_TEST ──> reset
BL     TO_TEST   ──> écrit TESTING ──> saute dans l'app
App    état != NONE ──> écrit DONE ──> reset
BL     DONE      ──> écrit NONE ──> reset
BL     NONE      ──> saute dans l'app
```

### 1.3 Carte flash actuelle (2 Mo) et occupation réelle

| Zone | Adresse | Réservé (`boot_config.h`) | `.ld` | Utilisé |
|---|---|---|---|---|
| Bootloader | 0x08000000 | 128 Ko | 128 Ko | **112 088 o (87 %)** |
| État persistant | 0x08020000 | 128 Ko | — | 4 o |
| CM4 | 0x08040000 | 640 Ko | **768 Ko** | 67 208 o |
| CM7 | 0x08100000 | 896 Ko | **1024 Ko** | 388 592 o |

Les colonnes « Réservé » et « `.ld` » divergent : c'est le défaut **I** ci-dessous.

### 1.4 Ce qui fonctionne déjà bien

- Le CRC est vérifié **avant** tout effacement.
- Le backup passe par un `.tmp` + `f_rename`, et il est sauté s'il existe déjà : une coupure
  secteur pendant le flash est rattrapée, le bootloader relit `RECEIVED` et recommence proprement.
- L'OLED affiche une progression en 8 étapes (`progress.c` + `update_gui.c`).

---

## 2. Défauts identifiés

Classés par gravité. Les lettres servent de référence dans le suivi et les tests.

### Brique

**A — Rien ne détecte un firmware qui ne démarre pas, ni un firmware vivant mais cassé.**
`USE_WDG` est commenté (`Common/Inc/config.h:52`), l'IWDG n'est jamais armé. Le rollback n'est
déclenché que par un *reset* pendant l'état `TESTING`. Un firmware qui plante ou boucle sans
se réinitialiser laisse la machine morte jusqu'à une coupure secteur manuelle.

Second volet, découvert en préparant T3 : le chien de garde ne rattrape qu'une image *figée*.
Une image qui démarre, tourne, mais n'atteint jamais ses critères de santé — serveur HTTP cassé,
typiquement — ne provoque aucun reset, donc le compteur d'essais n'avance pas et le rollback ne
se déclenche jamais. D'où `OTA_TRIAL_DEADLINE_MS` (180 s) : passé ce délai sans confirmation,
l'image se réinitialise elle-même pour laisser le bootloader compter la tentative. La valeur doit
rester au-dessus du plus long chemin légitime vers la confirmation (jusqu'à 60 s d'attente du lien
réseau + initialisation + `OTA_CONFIRM_SETTLE_MS`).

**B — La validation est un placebo.**
`FW_UPDATE_DONE` est écrit à `freertos.c:203`, **avant** `http_serverInit()`. Le critère de succès
est « FreeRTOS a démarré et LwIP s'est initialisé ». Une version qui casse le serveur HTTP est
déclarée bonne : pas de rollback, et le seul canal de mise à jour est perdu. C'est le scénario
de brique le plus probable en exploitation.

**C — Boucle de restauration infinie.**
Si `update_restoreBackupFirmwares()` échoue (`main.c:297`), on reboote avec l'état toujours à
`TESTING` : la restauration est retentée indéfiniment, sans compteur ni échappatoire.
Or `delete_old_firmware()` supprime `backup_cm7.bin` / `backup_cm4.bin` à chaque upload
(`http_server.c:318`), donc la fenêtre sans backup est réelle.

**D — `FW_UPDATE_DONE = 0xFFFFFFFF`** (`CM7/Peripheral/Inc/stm32_flash.h:35`) est la valeur d'une
flash effacée. Une coupure entre l'erase et le program du secteur d'état vaut « DONE » : la mise
à jour est silencieusement abandonnée et l'OLED affiche *update success*. Ni CRC sur l'état, ni
redondance, ni compteur de tentatives.

### Sécurité

**E — Aucune authenticité.** CRC-32 = intégrité seulement. Le serveur HTTP n'a aucune
authentification et le FTP anonyme est ouvert en écriture. N'importe qui sur le réseau pousse le
firmware qu'il veut.

**H — Aucune borne sur les tailles de l'en-tête.** `update.c:715` efface `cm7_size` octets sans
vérifier `cm7_size <= FW_CM7_MAX_SIZE`. Le CRC couvrant l'en-tête, il suffit de le recalculer pour
faire effacer des secteurs hors zone.

### Correction

**F — Machine à états multipart jamais réinitialisée.** `static fwupdate_t fwupdate`
(`http_server.c:240`) est appelée sur *chaque* requête. Upload interrompu ⇒ l'état reste
`DOWNLOAD_STREAM`, le `FIL` reste ouvert, et la requête suivante — même un GET — est écrite comme
données firmware jusqu'à atteindre `file_length`, ce qui lève `RECEIVED` et reboote sur un paquet
corrompu. Ni timeout ni reset sur fermeture de connexion.

**G — Underflow `size_t`.** `for (size_t i = 0; i <= bytes_read - boundary_len; i++)`
(`http_server.c:621` et `:650`) : fichier plus court que le boundary ⇒ 4 milliards d'itérations
sur un buffer de 512 o.

**I — Carte mémoire incohérente** entre `boot_config.h` et les linker scripts (§1.3). Une app qui
grandit déborde sur des secteurs que le backup ne couvre pas → restauration partielle silencieuse.

**K — `updateFileGen.py` est cassé.** Il cherche `#define FW_VERSION "x.y.z"` alors que
`config.h:32-37` construit `FW_VERSION` par concaténation de macros. Il embarque aussi l'external
MAX8 hérité dans chaque paquet.

### Bloquant — découvert au banc le 2026-08-30

**M — Le téléversement de firmware est cassé depuis `907ad74`.**

`http_assembleRequest()` (`http_server.c:78`), ajouté par
`907ad74 fix(rtos): complete CMSIS-RTOS v2 port and lwIP 2.2.1 tuning` pour
recoller les POST XHR de Safari envoyés en deux segments, recopie **toute**
requête dans `http_reqbuf`, un tampon de 2048 octets, et tronque
silencieusement le surplus :

```c
if ((u32_t)total + l > HTTP_REQ_BUF_SIZE - 1) { l = (u16_t)(HTTP_REQ_BUF_SIZE - 1 - total); }
```

Pour un paquet de 550 Ko, l'assembleur consomme deux `netbuf`, en tronque le
second, puis rend la main. `fwupdate.accum_length` n'atteint donc jamais
`file_length` : le téléversement ne se termine pas, aucune réponse HTTP n'est
émise, et le client reste en attente jusqu'au délai de garde. Constaté sur
cible — la trace s'arrête net après :

```
@ fwupdate - Scanning HEADER
@ fwupdate - Found content length = 550827
```

Ce défaut **précède l'étape 0** et bloque toute la matrice réseau (T1, T3, T4,
T5, T9, T12 exigent tous un téléversement abouti). T6 passe, lui, précisément
parce qu'il n'attend aucun aboutissement.

**Corrigé le 2026-08-30 — un gestionnaire dédié qui possède la connexion.**
Une rustine ne suffit pas : la chaîne de `netbuf` doit être consommée par un
seul propriétaire, et mélanger l'assembleur avec une lecture de fragments bruts
produit une boucle d'inondation (essayé, annulé). Le téléversement doit donc
être détourné avant l'assembleur :

```c
if (fwupdate_isUploadRequest(first, firstlen)) {
    reboot = fwupdate_handleUpload(conn, inbuf);   /* possede la connexion */
    close  = true;
    break;
}
buflen = http_assembleRequest(conn, inbuf);        /* inchangé pour le reste */
```

`fwupdate_handleUpload()` accumule les en-têtes dans `http_reqbuf` — terminés
par un NUL, donc les `strstr()` restent bornés — jusqu'à la balise de flux, qui
marque la fin des en-têtes multipart. Cela couvre le cas des en-têtes étalés sur
plusieurs segments, ce pour quoi l'assembleur existait. Passé ce point, chaque
fragment part directement au fichier : la machine à états est alors en
`DOWNLOAD_STREAM`, où elle ne fait aucune opération de chaîne. L'assembleur
n'est plus jamais sollicité pour un téléversement, et le correctif Safari reste
intact.

Un premier essai consistant à court-circuiter l'assembleur pour les fragments de
flux a échoué : il faisait consommer la même chaîne de `netbuf` par deux
propriétaires, et la machine s'est mise à inonder la liaison série. La chaîne
doit avoir un propriétaire unique — d'où le gestionnaire qui prend la connexion.

### Ergonomie

**J — Backup surdimensionné** : 896 + 640 Ko copiés vers la NOR pour sauvegarder 455 Ko de code réel.

**L — Trois reboots** par mise à jour, sans retour de progression côté navigateur.

---

## 3. Cible : A/B slots + manifeste signé

### 3.1 Carte flash proposée

| Zone | Adresse | Taille |
|---|---|---|
| BL-A | 0x08000000 | 128 Ko |
| BL-B | 0x08020000 | 128 Ko |
| Méta A / Méta B | 0x08040000 / 0x08060000 | 2 × 128 Ko |
| CM4-A / CM4-B | 0x08080000 / 0x080A0000 | 2 × 128 Ko |
| Réserve | 0x080C0000 | 256 Ko |
| CM7-A / CM7-B | 0x08100000 / 0x08180000 | 2 × 512 Ko |

Le slot inactif **est** le backup : plus de copie de 1,5 Mo vers la NOR, rollback instantané.
Marges : CM7 24 %, CM4 48 %.

### 3.2 Manifeste signé

Remplace l'en-tête 24 o :

```
magic "SP3U", format_version, header_len
device   : product_id, hw_rev, min_bl_version
release  : version sémantique, compteur anti-rollback
images[] : { type = BL | CM7 | CM4, offset, size, load_addr, sha256 }
signature: Ed25519 sur l'ensemble du manifeste (64 o)
```

Clé publique en dur dans le bootloader. Vérification **deux fois** : par l'app à l'upload (échec
immédiat, réponse HTTP explicite, le drapeau n'est jamais levé sur un mauvais paquet), puis par le
bootloader avant tout effacement. Le H745 n'a pas d'accélérateur HASH/CRYP (contrairement au H755) ;
SHA-256 logiciel sur 450 Ko coûte ~50 ms à 480 MHz, Ed25519 ~10 ms.

### 3.3 CM4 et les option bytes

L'adresse de boot du CM4 est un option byte (granularité 64 Ko). Le basculement A/B lui coûte une
reprogrammation d'OB + un reset — la logique existe déjà dans `configureBootConfiguration()`.
On rend les **métadonnées autoritaires** et l'OB devient un cache que le bootloader répare au boot
suivant : auto-cicatrisant, aucune fenêtre où les deux se contredisent durablement.

---

## 4. OTA du bootloader

**Faisable et raisonnablement sûr**, parce que le vecteur de reset du CM7 est l'option byte `BOOT_ADD0`.

Contrainte : le bootloader occupe **87 % de ses 128 Ko**. Pas question d'y mettre LwIP — et ce
n'est pas nécessaire : l'application télécharge, et **le bootloader actif écrit dans le slot
bootloader inactif**. Aucun code ne s'auto-efface.

```
BL-A  vérifie la signature de l'image BL du paquet
      efface le secteur 1 bank1, écrit BL-B
      RELIT et vérifie le SHA-256 en place
      écrit méta { bl_essai = B }
      programme BOOT_ADD0 = 0x0802, HAL_FLASH_OB_Launch() ──> reset
BL-B  démarre, atteint f_mount + OLED, écrit méta { bl_actif = B }
```

Une coupure secteur avant le `OB_Launch` laisse `BOOT_ADD0` sur A : la machine redémarre
normalement. La seule fenêtre irréductible est le `OB_Launch` lui-même (option bytes stockées avec
complément redondant ; ce risque est déjà pris aujourd'hui au premier boot).

**Trou restant** : un BL-B *valide mais buggé* n'est rattrapé par rien. Le filet naturel est la
broche BOOT0, qui sélectionne `BOOT_ADD1` — resté à `0x1FF0`, soit le bootloader système ST (DFU/USART).

> **Constat matériel** — dans `Sp3ctra_CIS_Hardware/CIS/Electronique/…/CIS.net`, BOOT0 (U2 pin C5)
> est tirée à la masse par R4 (10k, montée) et le pull-up **R52 est marqué NC**. Il n'y a donc
> aujourd'hui aucune échappatoire matérielle : il faut souder R52 ou passer par le SWD.
>
> **Portée réelle, à ne pas surestimer** : un cavalier sous le capot sert en usine et en SAV,
> pas à quelqu'un devant sa machine. Il ne supprime pas la brique du point de vue de
> l'utilisateur.
>
> Ce qui serait vraiment accessible à l'utilisateur : une combinaison de boutons à la mise sous
> tension. Le bootloader tourne toujours, même application morte, et dispose déjà de FatFs, de la
> NOR et de l'écran ; « maintenir deux boutons au démarrage → restaurer la version précédente »
> réutiliserait `update_restoreBackupFirmwares()` tel quel. À garder en réserve : depuis
> l'étape 0, le rollback automatique couvre les plantages, les figeages et les images
> vivantes-mais-cassées, si bien que ce recours ne servirait que si la restauration elle-même
> échoue.

Sans ce pontet, l'OTA bootloader reste envisageable mais doit être réservé aux mises à jour
réellement nécessaires, et jamais combiné à une mise à jour applicative.

---

## 5. Étapes

### Étape 0 — Rendre le rollback réel (validée sur cible le 2026-08-30)

Sans changer l'architecture. Supprime **A, B, C, D, F, G, H, I, K**.

| # | Action | Défaut |
|---|---|---|
| 0.1 | Journal OTA append-only en secteur 1 bank1 : enregistrements de 32 o `{magic, version, phase, trial_attempts, rollback_attempts, seq, crc32}`, on append sans effacer, le dernier enregistrement valide gagne | D |
| 0.2 | CRC-32 logiciel partagé (`Common/Src/ota_crc32.c`) au lieu du périphérique CRC configuré différemment dans l'app et le bootloader | — |
| 0.3 | Compteur de tentatives : `TRIAL` au-delà de 3 boots ⇒ `ROLLBACK` ; `ROLLBACK` au-delà de 3 ⇒ `IDLE` + erreur permanente à l'écran (fin de la boucle infinie) | C |
| 0.4 | IWDG armé par le bootloader **uniquement avant un saut en essai** ; rafraîchi par une tâche dédiée de l'app et par `progress_update()` côté bootloader | A |
| 0.5 | Confirmation applicative déplacée à `BOOT_STAGE_READY` + `systemFullyInitialized`, plus un délai de garde | B |
| 0.6 | Reset de la machine à états multipart à chaque nouvelle connexion, `FIL` refermé | F |
| 0.7 | Garde `bytes_read >= boundary_len` | G |
| 0.8 | Bornes `cm7_size`/`cm4_size` et cohérence de l'en-tête avant tout effacement | H |
| 0.9 | Vérification complète du paquet à l'upload, avant de lever le drapeau | E (partiel) |
| 0.10 | Linker scripts alignés sur `boot_config.h` | I |
| 0.11 | Trace de la cause de reset (`RCC_CSR`) au démarrage du bootloader | instrumentation |
| 0.12 | `scripts/ota/make_package.py` remplace `updateFileGen.py` | K |
| 0.13 | Injection de fautes + banc de test (§6) | — |

Toutes ces lignes sont écrites. Fichiers ajoutés :

| Fichier | Rôle |
|---|---|
| `Common/Inc/ota.h` | enregistrement de journal, phases, compteurs, CRC-32 |
| `Common/Src/ota_crc32.c` | CRC-32 logiciel (identique à `zlib.crc32`), scellement des enregistrements |
| `Common/Src/ota_journal.c` | journal append-only en secteur 1 bank1 |
| `Common/Inc/ota_fault_inject.h` | injection de fautes, inactive par défaut |
| `CM7_Bootloader/CM7/Application/{Inc,Src}/ota_boot.*` | séquenceur, chien de garde, saut, cause de reset |
| `CM7/Application/{Inc,Src}/ota_app.*` | tâche de garde, contrôle de santé, vérification du paquet |
| `scripts/ota/make_package.py` | générateur de paquets (+ variantes corrompues) |
| `scripts/ota/build_broken_fw.sh` | construit une image empoisonnée puis restaure les sources |
| `scripts/ota/ota_upload.py` | téléversement HTTP, avec coupure volontaire |
| `scripts/ota/ota_test.py` | déroulé de la matrice de test |

#### Séquence obtenue

```
App    paquet reçu ──> vérifié (en-tête + CRC) ──> journal PENDING ──> reset
                       si invalide : HTTP 400 avec le motif, aucun reboot
BL     PENDING   ──> pending_attempts+1, applique, journal TRIAL(0) ──> reset
BL     TRIAL(n)  ──> n >= 3 ? journal ROLLBACK : journal TRIAL(n+1)
                     + ARME L'IWDG ──> saute dans l'application
App    tâche de garde : recharge l'IWDG, attend CONFIG+HTTP pendant 30 s
                        ──> journal IDLE
BL     ROLLBACK  ──> restaure, journal IDLE ──> reset
                     au-delà de 3 essais : journal FAILED, message permanent,
                     démarrage de ce qui est en place (plus aucun effacement)
```

#### Budget mémoire après l'étape 0

| Image | Occupation | Capacité | Marge |
|---|---|---|---|
| bootloader | 115 648 o | 131 072 o | 15 424 o (88 %) |
| CM7 | 391 928 o | 917 504 o | 525 576 o |
| CM4 | 67 044 o | 655 360 o | 588 316 o |

#### Migration : un flashage SWD obligatoire

Le format de l'état persistant change. Une machine portant l'ancien bootloader
qui recevrait la nouvelle application par la voie habituelle serait **annulée à
tort** : le nouveau CM7 écrit dans le journal, que l'ancien bootloader ne sait
pas lire ; il resterait en `TESTING` et restaurerait la version précédente.

Le passage à l'étape 0 impose donc, une fois par machine :

```bash
./scripts/build.sh all release
./scripts/flash.sh all release       # bootloader + CM7 + CM4 par SWD
```

Après quoi les mises à jour réseau reprennent normalement.

#### Reste à faire sur l'étape 0

- **0.14** — Fenêtre ECC : sur STM32H7, relire un mot de flash dont la
  programmation a été coupée lève une double erreur ECC, donc une faute de bus.
  Le journal réduit cette fenêtre de l'effacement d'un secteur de 128 Ko
  (~1 s, l'ancien comportement) à la programmation d'un mot de 32 octets
  (~100 µs), mais ne la supprime pas. La fermer demande un gestionnaire de
  faute qui marque le journal comme suspect dans une zone RAM `NOLOAD`
  conservée au reset, puis l'efface au démarrage suivant. À faire après la
  validation matérielle.
- Vérifier sur cible si l'IWDG survit à un reset système (la trace
  `Reset cause: ... IWDG` du scénario T2 donnera la réponse). Le code est écrit
  pour être correct dans les deux cas.

### Étape 1 — Authentifier le canal, pas l'image (E)

**Révisé.** Le projet est open source : signer les images verrouillerait le
propriétaire hors de son propre matériel, ce qui est un contresens ici, et
imposerait de garder une clé privée pour la vie du produit.

Or la menace réelle n'est pas « quelqu'un fait tourner son propre firmware sur
sa machine » — c'est une fonctionnalité — mais « quelqu'un sur le même réseau
pousse un firmware sur MA machine sans mon accord ». Le remède proportionné est
donc d'authentifier le canal :

- mot de passe administrateur sur `POST /upload` et sur les POST de
  configuration ;
- ou restriction de `/upload` à l'hôte actuellement lié en session SLP ;
- fermeture de l'écriture FTP anonyme, aujourd'hui grande ouverte.

Nettement moins cher qu'Ed25519 + SHA-256 + gestion de clé, et sans rien retirer
au propriétaire. La signature garde du sens pour prouver qu'un paquet vient bien
d'Ondulab, mais c'est un problème de distribution, traitable par une signature
détachée vérifiée côté hôte — pas par le bootloader.

**Réalisé le 2026-08-30.** Mot de passe de 12 caractères tiré au sort par le RNG
au premier démarrage (aucune valeur d'usine), alphabet sans caractères ambigus
puisqu'il se lit sur l'OLED et se retape à la main. Affiché sur l'écran de
démarrage jusqu'à son premier usage, jamais servi par le réseau. Un factory
reset formate la NOR, donc régénère et réaffiche.

**Le FTP fermait la boucle.** `cmd_user()` et `cmd_pass()` ignoraient purement
leurs arguments et répondaient toujours « connecté » ; comme `CONFIG.TXT` réside
sur la même NOR et porte désormais `ADMIN_PASSWORD`, un seul `GET` FTP anonyme
suffisait à récupérer le mot de passe et à contourner toute l'authentification
HTTP. Le répartiteur de commandes n'appliquait par ailleurs aucun filtre d'état,
donc vérifier le mot de passe sans filtrer les commandes n'aurait rien changé.
Les deux sont corrigés, le même secret garde les deux services.

C'est le genre de défaut qui survit à une relecture : le code d'authentification
HTTP est correct isolément, et sa vacuité n'apparaît qu'en regardant **où le
secret est rangé et qui d'autre peut lire cet endroit**.

Vérifié sur cible :

| Requête | Sans identifiants | Mauvais mdp | Bon mdp |
|---|---|---|---|
| `GET /getFirmwareVersion` | 200 | — | — |
| `POST /setDPI` | 401 | 401 | 200 |
| `POST /factoryReset` | 401 | — | — |
| `POST /upload` | 401 avant toute écriture NOR | 401 | 200, mise à jour complète |
| FTP | refusé | refusé | `230`, `PWD` opérationnel |

Scénarios T14 et T15 ajoutés à la matrice.
### Étape 2 — Slots A/B (§3)

**À faire maintenant.** Le parc compte 5 machines, toutes sous contrôle : la
passe de reflashage SWD qu'impose la migration est gratuite aujourd'hui et ne le
sera plus jamais autant. C'est le dernier moment économique, avant les premières
livraisons.

Ce qu'on y gagne : rollback instantané (plus d'aller-retour de 1,5 Mo vers la
NOR), mise à jour environ deux fois plus rapide, recouvrement qui ne dépend plus
du système de fichiers de la NOR, et le préalable indispensable à l'étape 3.

#### Deux images liées par cœur

Une image Cortex-M contient des adresses absolues — table de vecteurs, pools de
littéraux, table d'initialisation de `.data`. Elle ne peut donc pas s'exécuter
depuis deux emplacements sans être **liée deux fois**. Le paquet embarque en
conséquence quatre images : `CM7-A`, `CM7-B`, `CM4-A`, `CM4-B`. Il passe
d'environ 550 Ko à 1 Mo, ce qui est sans conséquence sur une NOR de 16 Mo, et la
compilation double.

Les deux autres voies ont été écartées : recopier vers une adresse d'exécution
unique conserve la fenêtre d'écriture sur l'image active, c'est-à-dire le défaut
même qu'A/B doit supprimer ; et compiler en ROPI/RWPI supposerait que la HAL ST,
lwIP et FreeRTOS soient propres à ce régime, ce que rien ne garantit.

#### Carte flash cible

| Zone | Adresse | Taille | Occupation |
|---|---|---|---|
| Bootloader | 0x08000000 | 128 Ko | 116 Ko |
| Journal OTA | 0x08020000 | 128 Ko | quelques octets |
| CM4-A | 0x08040000 | 128 Ko | 67 Ko |
| CM4-B | 0x08060000 | 128 Ko | 67 Ko |
| Réserve | 0x08080000 | 384 Ko | — |
| BL-B (étape 3) | 0x080E0000 | 128 Ko | — |
| CM7-A | 0x08100000 | 512 Ko | 392 Ko |
| CM7-B | 0x08180000 | 512 Ko | 392 Ko |

Les deux adresses CM4 sont alignées sur 64 Ko, granularité de l'option byte
`CM4BootAddr0` : la bascule de slot le reprogramme, et le bootloader répare
l'écart au démarrage suivant s'il ne correspond pas au journal.

#### Ce que devient le rollback

Le journal porte `active_slot` (confirmé) et `trial_slot` (à l'essai). En phase
`TRIAL` le bootloader démarre `trial_slot`, sinon `active_slot`. Une image
confirmée fait `active_slot = trial_slot` ; une annulation ne fait **rien** —
elle se contente de continuer à démarrer l'ancien slot. Plus d'effacement, plus
de restauration, plus de dépendance au système de fichiers : le rollback devient
instantané et ne peut plus échouer à mi-parcours.
### Étape 3 — OTA bootloader via `BOOT_ADD0` (§4)
### Étape 4 — OTA via SLP depuis le VST (opcodes `0x30-0x7F` libres)

---

## 6. Banc de test du rollback

Objectif : prouver que le rollback se déclenche pour chaque famille de panne, sans SWD.

### 6.1 Injection de fautes

`Common/Inc/ota_fault_inject.h` — inactif par défaut, activé par `SP3CTRA_OTA_FAULT`.
`scripts/ota/build_broken_fw.sh N` construit un paquet empoisonné et le restaure ensuite.

| Faute | Comportement injecté | Ce qui doit sauver la machine |
|---|---|---|
| 1 | HardFault dès l'entrée de `main()` | reset immédiat → compteur de tentatives |
| 2 | `while(1)` avant le démarrage du noyau | **IWDG** |
| 3 | boot complet mais `http_serverInit()` échoue | health-check réel (0.5) |
| 4 | boot complet, HardFault 45 s plus tard | délai de garde avant confirmation |
| 5 | débordement de pile dans une tâche | `vApplicationStackOverflowHook` → reset |

Côté bootloader, `SP3CTRA_OTA_ABORT_AT_STEP=n` provoque un `NVIC_SystemReset()` au milieu de
l'étape *n* : coupure secteur déterministe et rejouable, sans banc électrique.

### 6.2 Résultats sur cible (2026-08-30, Sp3ctra-77DD)

Les trois images ont été flashées par SWD, puis la phase d'essai a été armée en
écrivant directement un enregistrement `TRIAL` dans le journal — ce qui permet
d'exercer toute la mécanique **sans réseau**.

**Migration depuis l'ancien format.** Le secteur d'état contenait bien les
28 octets à zéro de l'ancien `FW_UPDATE_NONE`. Le journal les a lus comme
invalides, conclu `IDLE` et sauté directement dans l'application, sans rien
écrire. Vérifié aussi qu'un démarrage nominal laisse le secteur intact.

**Essai puis confirmation** (image saine, un seul reset) :

```
OTA: running under trial (attempt 1/3)
OTA: health criteria met, confirming in 30000 ms
OTA journal: IDLE (trial 0, rollback 0, pending 0, seq 3, slot 2)
OTA: image confirmed
```

Trois enregistrements exactement, un seul passage bootloader : **aucun reset
parasite**, donc aucune annulation à tort d'une image saine.

**Chien de garde et rollback** (faute 5, tâche de garde absente) :

```
Reset cause: 0x04460000 PIN IWDG
OTA phase: TRIAL (trial 1/3, ...) -> journal TRIAL(2) -> Watchdog armed (~10000 ms)
Reset cause: 0x04460000 PIN IWDG
OTA phase: TRIAL (trial 2/3, ...) -> journal TRIAL(3)
Reset cause: 0x04460000 PIN IWDG
OTA phase: TRIAL (trial 3/3, ...)
OTA: trial image exhausted its attempts, rolling back
OTA: restoring the previous firmware        (x3, sans sauvegarde sur la NOR)
OTA: rollback failed 3 times, giving up
OTA journal: FAILED (...)
OTA: RECOVERY FAILED | SERVICE REQUIRED (SWD)
```

Ce qui est prouvé :

| Défaut | Preuve |
|---|---|
| **A** | l'IWDG coupe effectivement une image figée, `Reset cause ... IWDG` |
| **A** | le compteur est incrémenté *avant* le saut : une image qui ne revient jamais consomme son quota (3 essais exactement) |
| **B** | la confirmation attend 30 s de santé réelle, pas le début de l'init |
| **C** | après 3 restaurations ratées : `FAILED`, message permanent, démarrage de ce qui est en place — **plus aucune boucle infinie ni effacement** |
| **D** | journal ajouté à la suite (slots 2→8, seq 3→9), tous CRC valides, aucun effacement de secteur |

`update_restoreBackupFirmwares()` sort **avant tout effacement** quand la
sauvegarde manque : la zone applicative n'a jamais été abîmée pendant les trois
tentatives.

### 6.3 Résultats réseau (2026-08-30)

Accès réseau obtenu via `scripts/ota/relay.sh`, qui délègue à Terminal.app : le
verdict TCC « Réseau local » est mis en cache par processus et ne se rafraîchit
qu'au relancement de l'application, ce qui aurait coûté la session.

| Test | Résultat |
|---|---|
| T5 — paquet à CRC faux | `HTTP/1.1 400 Firmware rejected: checksum mismatch`, aucun redémarrage |
| T9 — `cm7_size` aberrant | `HTTP/1.1 400 Firmware rejected: CM7 image size out of range`, refusé avant tout effacement |
| T6 — téléversement coupé à 50 % | interruption absorbée, machine nominale, requête suivante intacte |
| T12 — mise à jour saine | 36 s, **deux reboots** au lieu de trois, `image confirmed` |
| T3 — serveur HTTP cassé | rollback **entièrement autonome**, retour en 4.0.0, image restaurée identique octet pour octet à la compilation saine |
| T1 — HardFault immédiat | rollback, 3 resets IWDG |
| T13 — chien de garde jamais rechargé | rollback, 3 resets IWDG |
| T4 — panne avant la fin du délai | rollback, 3 resets IWDG ; l'oracle réseau montre 4.0.99 / injoignable ×3 puis 4.0.0 |
| T7 — coupure au milieu du flash CM7 | reprise au redémarrage, mise à jour menée à terme, `image confirmed` |
| T8 — coupure au milieu de la sauvegarde CM7 | sauvegarde refaite depuis zéro, mise à jour menée à terme, `image confirmed` |

**Répartition des mécanismes sur T1, T13 et T4** : 9 resets `IWDG`, 0 par délai
d'essai. C'est la bonne orthogonalité — le chien de garde couvre les plantages
et les figeages, le délai couvre le seul cas qu'il ne voit pas, l'image vivante
mais jamais saine (T3). Les deux filets se complètent sans se recouvrir.

Séquence complète de T12, telle que tracée :

```
OTA: package verified (CM7 392344 B, CM4 67472 B, external 91238 B)
OTA journal: PENDING -> 200 OK -> reboot
BL  PENDING -> pending 1 compté AVANT de toucher la flash
    CRC verified -> backup CM7/CM4 -> erase -> flash -> journal TRIAL(0) -> reboot
BL  TRIAL(0) -> TRIAL(1) -> Watchdog armed (~10000 ms) -> saut
App running under trial (attempt 1/3) -> health criteria met -> IDLE -> image confirmed
```

T3 exerce le scénario de brique le plus probable : une version qui casse le
serveur HTTP emporte avec elle le seul moyen d'en envoyer une autre. Cycle
complet, sans la moindre intervention SWD :

```
OTA: running under trial (attempt 1/3)
OTA: trial deadline elapsed without confirmation (health 0x1B), resetting
OTA: running under trial (attempt 2/3)
OTA: trial deadline elapsed without confirmation (health 0x1B), resetting
OTA: running under trial (attempt 3/3)
OTA: trial deadline elapsed without confirmation (health 0x1B), resetting
OTA: trial image exhausted its attempts, rolling back
```

`health 0x1B` = `CONFIG | NETWORK | LINK | CIS` levés, seul `OTA_HEALTH_HTTP`
(0x04) manquant : la machine tourne, le réseau marche, le capteur scanne, mais
le canal de mise à jour est mort — et c'est bien ce seul critère qui décide.

Les coupures secteur simulées (`SP3CTRA_OTA_ABORT_AT_STEP`, un
`NVIC_SystemReset()` au milieu de l'étape visée, uniquement à la première
tentative) :

```
OTA journal: PENDING (pending 1)
Step 6: Flash new CM7 firmware
OTA: simulating a power cut in the middle of step 6
------- START BOOTLOADER -------
OTA journal: PENDING (pending 2)      <- reprise, tentative comptée
Step 1..8 -> package applied -> TRIAL -> image confirmed
```

T8 vise l'étape 2 : la sauvegarde interrompue ne laisse qu'un `.tmp`, jamais un
`backup_cm7.bin` tronqué, et la reprise la refait entièrement. C'est le point
qui pouvait être vicieux — une sauvegarde à moitié écrite mais tenue pour bonne
aurait rendu tout rollback ultérieur destructeur.

Et la restauration, avec une sauvegarde réelle sur la NOR :

```
Step 1/2: Erasing CM7 / CM4 region
Step 3: Restoring CM7 backup -> Successfully restored 0:/firmware/backup_cm7.bin
Step 4: Restoring CM4 backup -> Successfully restored 0:/firmware/backup_cm4.bin
OTA: previous firmware restored -> journal IDLE -> reboot -> OTA phase: IDLE
```

### 6.4 Matrice de test

| # | Scénario | Attendu |
|---|---|---|
| T1 | Faute 1 | 3 boots en échec, rollback, ancienne version opérationnelle |
| T2 | Faute 2 | reset par IWDG ×3, rollback |
| T3 | Faute 3 | pas de confirmation, rollback au boot suivant |
| T4 | Faute 4 | pas de confirmation, rollback |
| T5 | Paquet à CRC faux | refusé à l'upload, **aucun reboot** — **PASSÉ** (`400 checksum mismatch`) |
| T6 | Upload interrompu à 50 % | refusé, app toujours joignable, requête suivante non corrompue — **PASSÉ** |
| T7 | `ABORT_AT_STEP=6` (flash CM7) | reprise au reboot, mise à jour menée à terme — **PASSÉ** |
| T8 | `ABORT_AT_STEP=2` (backup CM7) | reprise, backup reconstruit — **PASSÉ** |
| T9 | En-tête avec `cm7_size` aberrant | refusé sans effacement — **PASSÉ** (`400 CM7 image size out of range`) |
| T10 | Coupure secteur réelle pendant le flash | idem T7 |
| T11 | Backup absent + firmware cassé | `ROLLBACK` ×3 puis erreur permanente, **pas de boucle infinie** |
| T12 | Mise à jour saine | un seul passage, confirmation, `IDLE` — **PASSÉ** (36 s, deux reboots) |

### 6.5 Déroulé sur la machine

```bash
# 1. Point de départ propre : les trois images par SWD
./scripts/build.sh all release && ./scripts/flash.sh all release

# 2. Trace UART dans un second terminal — c'est l'oracle
./scripts/uart_trace.sh | tee build/ota/trace.log

# 3. Scénarios rapides, sans reconstruction (T5, T9, T6)
python3 scripts/ota/ota_test.py

# 4. Un rollback complet, en observant la trace
./scripts/ota/build_broken_fw.sh 2      # boucle infinie -> chien de garde
python3 scripts/ota/ota_test.py T2

# 5. Retour à une image saine
./scripts/build.sh all release && python3 scripts/ota/make_package.py
python3 scripts/ota/ota_test.py T12
```

L'image empoisonnée porte le patch 99 : `GET /getFirmwareVersion` suffit à
constater le rollback, sans avoir à interpréter la trace.

### 6.6 Outils

- `scripts/ota/make_package.py` — génère un paquet (option `--corrupt`, `--truncate`, `--bad-size`)
- `scripts/ota/ota_upload.py` — upload HTTP + suivi
- `scripts/ota/ota_test.py` — déroule la matrice, corrèle avec la trace UART.
  Reconstruit un paquet empoisonné dès que les sources sont plus récentes que
  lui : réutiliser un paquet périmé teste une image qui n'est plus celle du
  dépôt et produit un vert trompeur. C'est arrivé — un paquet `fault3` antérieur
  à `OTA_TRIAL_DEADLINE_MS` a été réutilisé et le scénario est resté bloqué sur
  l'essai 1/3.
- `scripts/ota/relay.sh` — délègue une commande à Terminal.app pour contourner
  le verdict TCC « Réseau local » mis en cache. Chaque invocation a son propre
  identifiant de journal : avec des chemins fixes, une sonde lancée pendant
  qu'un test tourne écrase son résultat, et l'échec ressemble à un problème
  firmware.
- `scripts/ota/build_abort_bl.sh <0..8>` — bootloader à coupure secteur simulée,
  à flasher par SWD (le bootloader n'est pas transporté par les paquets)
- `scripts/uart_trace.sh` — capture existante, sert d'oracle

Lignes à retrouver dans la trace pour un rollback réussi :

```
OTA phase: TRIAL (trial 1/3, ...)      puis 2/3, puis 3/3
Reset cause: 0x........ IWDG           (scénarios T2 et T13)
OTA: trial image exhausted its attempts, rolling back
OTA: restoring the previous firmware
OTA: previous firmware restored
OTA journal: IDLE (...)
```

---

## 7. Pièges de banc constatés

Trois pièges venaient du **harnais lui-même**, tous du même genre : faire croire
qu'on teste ce qu'on croit tester.

1. Un paquet empoisonné périmé était réutilisé — T3 est resté bloqué sur
   l'essai 1/3 avec une image antérieure à `OTA_TRIAL_DEADLINE_MS`.
   Corrigé : reconstruction dès que les sources sont plus récentes.
2. `build_broken_fw.sh` restaurait les sources mais laissait les `.bin`
   empoisonnés dans `Release/`, **plus récents que les sources**, donc
   indétectables par comparaison de dates. Un T7 a flashé une image fault-4 en
   la croyant saine. Corrigé : le `trap` supprime les binaires, et `ota_test.py`
   reconstruit avant d'empaqueter.
3. `relay.sh` utilisait des chemins de journal fixes : une sonde lancée pendant
   un test écrasait son résultat, et la perte ressemblait à une panne firmware.
   Corrigé : un identifiant par invocation.

Dans les trois cas, c'est le **journal OTA en flash** qui a tranché — seul
témoin que ni le réseau, ni le relais, ni la trace UART ne peuvent falsifier.

- **La trace UART et le programmateur SWD ne peuvent pas coexister** sur le
  STLINK-V3MODS : `stlink_uart_trace.py` tient `/dev/cu.usbmodem*` en exclusivité
  et `STM32_Programmer_CLI` échoue en `DEV_CONNECT_ERR`. Pire, une invocation
  passée pendant ce conflit a rapporté « File download complete » tout en
  écrivant **32 octets erronés** en flash, et les relectures renvoyaient des
  valeurs fantaisistes. Toujours arrêter la trace avant tout accès SWD.
- `mode=UR` **laisse la cible sous reset en sortie** : après une écriture il faut
  un `-rst` explicite, sinon rien ne redémarre (symptôme : trace UART muette).
- Écrire un enregistrement de journal par SWD est le moyen le plus rapide
  d'exercer la séquence d'essai sans réseau :
  `STM32_Programmer_CLI -c port=SWD mode=UR -d journal_trial.bin 0x08020000 --skipErase`
- Le VPN (NordVPN ici) bloque le réseau local : l'ARP répond mais tout paquet IP
  part en `EHOSTUNREACH`. Désactiver « Invisibility on LAN » ou se déconnecter.

## 8. Notes d'outillage

- `scripts/sync_subdir_mk.py` ne couvre pas le bootloader : `CORES["bootloader"]` vaut
  `CM7_Bootloader` alors que la racine de build est `CM7_Bootloader/CM7`. À corriger avant
  d'ajouter des sources au bootloader.
- `CM7/Peripheral/{Src,Inc}/stm32_flash.*` et leurs copies `CM7_Bootloader/CM7/Peripheral/…` sont
  **identiques** : toute modification doit être répercutée dans les deux.
- `Common/Src/*.c` est compilé par les deux projets via des chemins absolus.
- Marge bootloader : 15 Ko après l'étape 0. Toute addition de code doit être mesurée.
- Les deux `CM7/Core/Src/main.c` et `CM7_Bootloader/CM7/Core/Src/main.c` sont en
  **CRLF** : un outil qui les réécrit en LF produit un diff de fichier entier.
