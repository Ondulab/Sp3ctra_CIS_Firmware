# RTP-MIDI Session Establishment Timing Fix

## Problème Identifié

### Symptômes
- Les paquets RTP-MIDI (port 5005) arrivent correctement au Mac avec un format conforme à RFC 6295
- Le Mac envoie des paquets de synchronisation (CK) sur le port 5004
- **MAIS** : Aucun périphérique "Sp3ctra_CIS" n'apparaît dans Audio MIDI Setup
- MIDI Monitor ne voit aucun message MIDI

### Analyse Wireshark
```
Port 5004 (Contrôle AppleMIDI):
- ❌ Aucun paquet IN (Invitation) du STM32 → Mac
- ❌ Aucun paquet OK (Invitation Accepted) du Mac → STM32
- ⚠️  Seulement des paquets CK (Synchronization) du Mac → STM32 (toutes les 10s)

Port 5005 (Données MIDI):
- ✅ Paquets RTP-MIDI correctement formatés
- ✅ Payload conforme à RFC 6295 Section 3
```

### Cause Racine

**Problème de timing dans l'initialisation** :

```c
// AVANT (freertos.c) - INCORRECT
1. rtpmidi_init()
2. rtpmidi_connect()  // ← Envoie l'invitation AVANT que le réseau soit prêt!
3. while(isConnected == 0) { osDelay(500); }  // Attente réseau
4. ... plus tard dans StartMidiTask()
5. rtpmidi_process()  // ← Gère les retries, mais trop tard!
```

**Séquence des événements** :
1. `rtpmidi_connect()` envoie l'Invitation (IN) **avant** que le réseau soit établi
2. Le paquet IN est perdu ou envoyé trop tôt
3. `rtpmidi_process()` (qui gère les retries) ne démarre que bien plus tard
4. Le Mac détecte du trafic RTP-MIDI et envoie des CK, mais la session n'est jamais établie
5. macOS ne crée jamais le périphérique MIDI réseau "Sp3ctra_CIS"

## Solution Implémentée

### Changement dans CM7/Core/Src/freertos.c

**Déplacer l'initialisation RTP-MIDI APRÈS l'attente de la connexion réseau** :

```c
// APRÈS - CORRECT
1. while(isConnected == 0) { osDelay(500); }  // Attente réseau D'ABORD
2. rtpmidi_init()
3. rtpmidi_connect()  // ← Maintenant le réseau est prêt!
4. ... dans StartMidiTask()
5. rtpmidi_process()  // ← Peut gérer les retries si nécessaire
```

### Protocole AppleMIDI Complet (RFC 6295)

Pour qu'une session soit reconnue par macOS, il faut cet échange :

```
STM32 (192.168.100.10)          Mac (192.168.100.1)
        |                              |
        |--- IN (Invitation) --------->|
        |     "Sp3ctra_CIS"            |
        |     SSRC: 0x05276414         |
        |                              |
        |<-- OK (Accept) --------------|
        |     SSRC du Mac              |
        |                              |
        |<-- CK (Sync) --------------->|
        |    (bidirectionnel)          |
        |                              |
    [SESSION ÉTABLIE]
        |                              |
        |--- MIDI data (port 5005) --->|
```

## Vérification

### Après la correction, vérifier dans Wireshark :

**Port 5004** :
```
✅ Paquet IN du STM32 → Mac (avec "Sp3ctra_CIS")
✅ Paquet OK du Mac → STM32
✅ Paquets CK bidirectionnels
```

### Dans Audio MIDI Setup :
```
✅ Section "Network" visible
✅ Périphérique "Sp3ctra_CIS" présent
✅ État "Connected"
```

### Dans MIDI Monitor :
```
✅ Source "Sp3ctra_CIS" visible dans la liste
✅ Messages MIDI Control Change visibles
```

## Format RTP-MIDI Vérifié Correct

Le payload RTP-MIDI était déjà **100% conforme à RFC 6295** :

```
Exemple de paquet capturé:
80e1002c0000002c05276414800400b0157f

Décomposition:
- RTP Header (12 octets): 80 e1 00 2c 00 00 00 2c 05 27 64 14
  * Version=2, PT=97 (RTP-MIDI), Seq=44, Timestamp, SSRC

- MIDI Command Section (6 octets): 80 04 00 b0 15 7f
  * Header: 80 04 (B=1, LEN=4)
  * MIDI List: 00 b0 15 7f
    - Delta Time: 00 (immédiat)
    - CC Message: b0 15 7f (Canal 0, CC#21, valeur 127)
```

## Fichiers Modifiés

- `CM7/Core/Src/freertos.c` : Ordre d'initialisation corrigé

## Références

- RFC 6295 : RTP Payload Format for MIDI
- Section 2.1 : RTP Header
- Section 3 : MIDI Command Section
- Appendix B : AppleMIDI Session Protocol

## Date

18 décembre 2025
