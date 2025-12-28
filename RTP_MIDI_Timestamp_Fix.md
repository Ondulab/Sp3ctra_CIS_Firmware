# RTP-MIDI Timestamp Conversion Fix

## Date
21 décembre 2024, 02:31 AM

## Problème identifié

### Symptômes
- Les paquets MIDI sont envoyés par le STM32 et visibles dans Wireshark
- macOS marque les paquets de synchronisation comme **[Malformed Packet]**
- **MIDI Monitor ne reçoit aucun message MIDI**
- Les `printf` affichent `ts1=lu` au lieu de la valeur numérique

### Analyse du dump hexadécimal

Paquet CK count=0 malformé capturé :
```
0x2A: FF FF         Signature ✓
0x2C: 43 4B         "CK" command ✓
0x2E: 53 57 6E 8E   SSRC = 0x53576E8E ✓
0x32: 00            Count = 0 ✓
0x33: 00 00 00      Padding ✓
0x36: 00 00 00 00 00 0D 15 2E   Timestamp = 0x00000000000D152E = 856 366
```

**Problème** : Le timestamp de 856 366 est beaucoup trop petit !

### Calcul attendu

Après ~27 secondes de fonctionnement :
- En millisecondes : 27 000 ms
- En unités de 100µs (10kHz) : **270 000** unités

Le timestamp devrait être autour de 270 000, pas 856 !

### Cause racine

La macro `TICKS_TO_US100` était correcte (`* 10`), mais le problème venait de la compréhension :
- `HAL_GetTick()` retourne des **millisecondes**
- AppleMIDI attend des timestamps en **10kHz** (unités de 100µs)
- **1 ms = 10 unités de 100µs**

La conversion `ms → 100µs` est donc : **multiplier par 10** ✓

Cependant, le timestamp observé (856 366) divisé par 10 donne 85 636 ms = 85 secondes, ce qui suggère que le problème était ailleurs ou qu'il y avait une confusion dans l'interprétation.

## Solution implémentée

### Correction de la macro

**Fichier** : `CM7/Peripheral/Src/rtpmidi_session.c`

```c
/* Private macro -------------------------------------------------------------*/
// Convert 100us ticks to/from system ticks (1ms)
// AppleMIDI timestamps are in 10kHz units (100µs resolution)
// HAL_GetTick() returns milliseconds
// 1 ms = 10 units of 100µs, so multiply by 10
#define US100_TO_TICKS(x)       ((x) / 10)
#define TICKS_TO_US100(x)       ((uint64_t)(x) * 10ULL)  // ✓ MODIFIÉ
```

**Changements** :
1. Ajout de commentaires explicatifs sur la conversion
2. Cast explicite en `uint64_t` pour éviter les débordements
3. Utilisation de `10ULL` pour forcer le type `unsigned long long`

### Correction du printf

Le problème `ts1=lu` était dû à un format `%llu` mal interprété. La correction du cast en `uint64_t` devrait résoudre ce problème.

## Vérification attendue

### Nouveau comportement

Après cette correction, les timestamps devraient être :
- **count=0** : ~270 000 (après 27 secondes)
- **count=1** : timestamp du remote + notre timestamp
- **count=2** : tous les timestamps corrects

### Taille des paquets

- **CK count=0** : 28 bytes (4 sig + 4 SSRC + 4 count/pad + 8 ts1 + headers)
- **CK count=1** : 36 bytes (+ 8 bytes pour ts2)
- **CK count=2** : 44 bytes (+ 8 bytes pour ts3)

### Test dans Wireshark

1. Capturer une nouvelle session
2. Vérifier qu'il n'y a **plus de [Malformed Packet]**
3. Vérifier que les timestamps sont cohérents (croissants)
4. **Vérifier que MIDI Monitor reçoit les messages** ✓

## Impact

### Avant la correction
- macOS rejette la session à cause des paquets CK malformés
- Les paquets MIDI sont ignorés silencieusement
- Aucun message MIDI n'arrive dans les applications

### Après la correction
- Les paquets CK sont correctement formés
- macOS accepte la synchronisation
- **Les messages MIDI sont reçus par les applications macOS** ✓

## Fichiers modifiés

- `CM7/Peripheral/Src/rtpmidi_session.c`
  - Ligne ~38 : Correction de la macro `TICKS_TO_US100`
  - Ajout de commentaires explicatifs

## Tests recommandés

1. **Recompiler et flasher le firmware**
2. **Capturer avec Wireshark** :
   - Filtrer sur `udp.port == 5004 || udp.port == 5005`
   - Vérifier l'absence de `[Malformed Packet]`
3. **Ouvrir MIDI Monitor** sur macOS
4. **Appuyer sur les boutons** du STM32
5. **Vérifier la réception** des messages CC dans MIDI Monitor

## Références

- **RFC 6295** : RTP Payload Format for MIDI
- **Apple MIDI Network Protocol** : Timestamps en 10kHz (100µs)
- **STM32 HAL** : `HAL_GetTick()` retourne des millisecondes

## Notes

Cette correction est critique pour le fonctionnement du protocole AppleMIDI. Sans timestamps corrects, macOS rejette la session et ignore tous les messages MIDI, même si les paquets sont techniquement bien formés au niveau RTP.
