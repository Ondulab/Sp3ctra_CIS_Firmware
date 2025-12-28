# RTP-MIDI Invitation Race Condition Fix

## Date
21 décembre 2024, 02:17 AM

## Problème identifié

### Trace réseau observée
```
183157  12.252866  192.168.100.1  192.168.100.10  AppleMIDI  70  Invitation: peer = "Sp3ctra_CIS"
200311  13.254760  192.168.100.1  192.168.100.10  AppleMIDI  70  Invitation: peer = "Sp3ctra_CIS"
200314  13.255111  192.168.100.10  192.168.100.1  AppleMIDI  79  Invitation Accepted: peer = "MacBook Pro de Zhonx"
200329  13.255797  192.168.100.1  192.168.100.10  AppleMIDI  70  Invitation: peer = "Sp3ctra_CIS"  ❌
200330  13.255916  192.168.100.10  192.168.100.1  AppleMIDI  79  Invitation Accepted: peer = "MacBook Pro de Zhonx"
```

### Analyse
- **192.168.100.1** = Sp3ctra_CIS (STM32)
- **192.168.100.10** = MacBook Pro

Le STM32 envoie **3 invitations** alors qu'il devrait s'arrêter après avoir reçu l'acceptance du MacBook :
1. **12.252s** : Invitation #1 (normale)
2. **13.254s** : Invitation #2 (retry après 1 seconde - normal)
3. **13.255s** : MacBook répond OK ✓
4. **13.255s** : **Invitation #3 envoyée APRÈS réception de l'OK** ❌ (race condition)

### Causes identifiées

#### 1. Race condition dans le timer de retry
Le timer de retry continue de s'exécuter même après réception de l'OK du remote. Entre le moment où :
- L'état passe à `RTPMIDI_STATE_CONTROL_CONNECTED` (réception de l'OK)
- Le prochain appel à `rtpmidi_process()` vérifie le timer

Il peut s'écouler quelques millisecondes pendant lesquelles une invitation supplémentaire est envoyée.

#### 2. Absence de garde dans `rtpmidi_send_invitation()`
La fonction `rtpmidi_send_invitation()` n'avait aucune vérification d'état avant d'envoyer le paquet, permettant l'envoi d'invitations même dans des états inappropriés.

## Solution implémentée

### 1. Arrêt immédiat du timer de retry (Control Port)
**Fichier** : `CM7/Peripheral/Src/rtpmidi_session.c`
**Fonction** : `rtpmidi_process()` - Handler `RTPMIDI_CMD_IN`

```c
case RTPMIDI_CMD_IN:  // Invitation from remote
    if (g_session.state == RTPMIDI_STATE_IDLE ||
        g_session.state == RTPMIDI_STATE_INVITED) {
        // ... extraction des données ...

        g_session.state = RTPMIDI_STATE_CONTROL_CONNECTED;

        // CRITICAL: Stop retry timer by clearing connection_attempts
        // This prevents sending additional invitations after receiving remote's INVITE
        g_session.connection_attempts = 0;  // ✓ AJOUTÉ

        rtpmidi_send_ok(token, remote_ssrc, 1);
    }
    break;
```

**Effet** : Dès réception d'une invitation du remote, on met `connection_attempts = 0`, ce qui désactive la logique de retry dans `rtpmidi_process()`.

### 2. Arrêt immédiat du timer de retry (Data Port)
**Fichier** : `CM7/Peripheral/Src/rtpmidi_session.c`
**Fonction** : `rtpmidi_data_recv_thread()` - Handler `RTPMIDI_CMD_IN`

```c
case RTPMIDI_CMD_IN: // Invitation on data port
    if (g_session.state == RTPMIDI_STATE_CONTROL_CONNECTED) {
        // ... vérification SSRC ...

        if (remote_ssrc == g_session.remote_ssrc) {
            // CRITICAL: Stop retry timer by clearing connection_attempts
            g_session.connection_attempts = 0;  // ✓ AJOUTÉ

            rtpmidi_send_ok(token, remote_ssrc, 0);
            g_session.state = RTPMIDI_STATE_CONNECTED;
        }
    }
    break;
```

### 3. Garde dans `rtpmidi_send_invitation()`
**Fichier** : `CM7/Peripheral/Src/rtpmidi_session.c`
**Fonction** : `rtpmidi_send_invitation()`

```c
static void rtpmidi_send_invitation(uint8_t is_control_port)
{
    // Guard: Do not send invitations if we're already connected or synchronized
    // This prevents race conditions where we might send an invitation after receiving one
    if (g_session.state == RTPMIDI_STATE_CONNECTED ||
        g_session.state == RTPMIDI_STATE_SYNCHRONIZED) {
        printf("RTP-MIDI: Skipping invitation send - already in state %d\n", g_session.state);
        return;  // ✓ AJOUTÉ
    }

    // ... reste du code ...
}
```

**Effet** : Protection supplémentaire qui empêche l'envoi d'invitations si la session est déjà établie, même si la logique de retry est défaillante.

## Bénéfices

### 1. Élimination de la race condition
- Les invitations s'arrêtent **immédiatement** dès réception d'un OK du remote
- Plus de paquets superflus sur le réseau
- Comportement conforme au protocole AppleMIDI

### 2. Robustesse accrue
- Double protection (arrêt du timer + garde dans send)
- Défense en profondeur contre les états incohérents
- Logs de debug pour identifier les tentatives d'envoi inappropriées

### 3. Compatibilité améliorée
- Comportement plus prévisible pour macOS
- Réduction du risque de confusion de session
- Meilleure gestion des reconnexions rapides

## Comportement attendu après correction

### Trace réseau corrigée (attendue)
```
T+0.000s  STM32 → Mac  Invitation #1
T+1.000s  STM32 → Mac  Invitation #2 (retry)
T+1.001s  Mac → STM32  Invitation Accepted ✓
          [Plus d'invitations envoyées] ✓
```

### Machine à états
```
IDLE → INVITED (envoi IN #1)
     ↓ (timeout 1s)
     → INVITED (envoi IN #2)
     ↓ (réception OK)
     → CONTROL_CONNECTED (connection_attempts = 0) ✓
     ↓ (pas de retry car attempts = 0)
     → Attente invitation data port
```

## Tests recommandés

1. **Test de connexion normale**
   - Vérifier qu'une seule invitation est envoyée si le remote répond rapidement
   - Vérifier que les retries fonctionnent si le remote ne répond pas

2. **Test de reconnexion rapide**
   - Déconnecter et reconnecter rapidement
   - Vérifier qu'il n'y a pas d'invitations croisées

3. **Capture Wireshark**
   - Filtrer sur `udp.port == 5004 || udp.port == 5005`
   - Vérifier l'absence d'invitations après réception d'OK

## Fichiers modifiés

- `CM7/Peripheral/Src/rtpmidi_session.c`
  - Ligne ~220 : Ajout `connection_attempts = 0` dans handler `RTPMIDI_CMD_IN` (control)
  - Ligne ~650 : Ajout `connection_attempts = 0` dans handler `RTPMIDI_CMD_IN` (data)
  - Ligne ~420 : Ajout garde dans `rtpmidi_send_invitation()`

## Références

- **Protocole AppleMIDI** : RFC 6295 (RTP-MIDI)
- **Spécification Apple** : Apple MIDI Network Protocol
- **Timing** : RTPMIDI_INVITE_INTERVAL_MS = 1000ms (conforme spec Apple)

## Notes

Cette correction s'applique aux deux modes :
- **SERVER mode** : STM32 reçoit l'invitation de macOS (cas actuel)
- **CLIENT mode** : STM32 initie la connexion vers PC/Linux

La logique de retry reste active uniquement en CLIENT mode lorsque nous sommes l'initiateur.
