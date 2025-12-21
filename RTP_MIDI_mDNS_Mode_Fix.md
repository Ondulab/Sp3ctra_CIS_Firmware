# RTP-MIDI mDNS Mode Fix - Désactivation automatique en mode CLIENT

## 📋 Résumé

Le mDNS est maintenant automatiquement désactivé lorsque le RTP-MIDI est configuré en mode CLIENT. Cette modification garantit que le service mDNS n'est actif que lorsqu'il est nécessaire (mode SERVER pour la découverte par macOS).

**Date:** 21/12/2025
**Version firmware:** 3.12.1+

---

## 🔍 Problème identifié

Le service mDNS était initialisé indépendamment du mode RTP-MIDI, ce qui causait:
- ✗ mDNS actif en mode CLIENT alors qu'il n'est pas nécessaire
- ✗ Ressources réseau gaspillées inutilement
- ✗ Confusion sur le comportement du système

---

## ✅ Solution implémentée

### **1. Ajout du champ `rtpmidi_mode` dans la configuration**

**Fichiers modifiés:**
- `Common/Inc/globals.h`
- `Common/Inc/config.h`
- `CM7/Peripheral/Src/file_manager.c`

**Nouveau champ dans `struct shared_config`:**
```c
uint8_t rtpmidi_mode;  // RTP-MIDI mode (0=SERVER, 1=CLIENT)
```

**Valeur par défaut dans `config.h`:**
```c
#define RTPMIDI_MODE_DEFAULT  0  // 0=SERVER (macOS), 1=CLIENT (PC/Linux)
```

---

### **2. Modification de l'initialisation mDNS**

**Fichier modifié:** `CM7/LWIP/App/lwip.c`

**Logique conditionnelle:**
```c
// mDNS is only needed in SERVER mode (for discovery by macOS)
// In CLIENT mode, the device connects directly to a fixed IP
if (shared_config.mdns_enabled && shared_config.rtpmidi_mode == 0) {
    // Initialize mDNS only in SERVER mode
    printf("--- mDNS INITIALIZATIONS ---\n");
    mdns_resp_init();
    // ... rest of mDNS initialization
} else {
    if (!shared_config.mdns_enabled) {
        printf("mDNS: Service disabled in configuration\n");
    } else if (shared_config.rtpmidi_mode == 1) {
        printf("mDNS: Service disabled (RTP-MIDI CLIENT mode uses fixed IP)\n");
    }
}
```

---

### **3. Utilisation du mode depuis la configuration**

**Fichier modifié:** `CM7/Core/Src/freertos.c`

**Initialisation RTP-MIDI:**
```c
// Determine RTP-MIDI mode from configuration
rtpmidi_mode_t mode = (rtpmidi_mode_t)shared_config.rtpmidi_mode;
const char* mode_str = (mode == RTPMIDI_MODE_SERVER) ? "SERVER" : "CLIENT";

printf("RTP-MIDI: Mode=%s, Destination IP=%d.%d.%d.%d\n",
       mode_str,
       shared_config.network_dest_ip[0],
       shared_config.network_dest_ip[1],
       shared_config.network_dest_ip[2],
       shared_config.network_dest_ip[3]);

// Initialize RTP-MIDI with mode from configuration
if (rtpmidi_init("Sp3ctra_CIS", &remote_ip, mode) != RTPMIDI_OK) {
    // Error handling
}
```

---

## 📝 Configuration dans CONFIG.TXT

### **Mode SERVER (par défaut)**
```
MDNS_ENABLED=1
RTPMIDI_MODE=0
NETWORK_DEST_IP_ADDR0=192
NETWORK_DEST_IP_ADDR1=168
NETWORK_DEST_IP_ADDR2=100
NETWORK_DEST_IP_ADDR3=255
```

**Comportement:**
- ✅ mDNS actif (découverte automatique)
- ✅ Apparaît dans Audio MIDI Setup (macOS)
- ✅ Attend les connexions entrantes
- ✅ IP de destination ignorée

---

### **Mode CLIENT**
```
MDNS_ENABLED=1
RTPMIDI_MODE=1
NETWORK_DEST_IP_ADDR0=192
NETWORK_DEST_IP_ADDR1=168
NETWORK_DEST_IP_ADDR2=100
NETWORK_DEST_IP_ADDR3=10
```

**Comportement:**
- ❌ mDNS désactivé automatiquement (même si `MDNS_ENABLED=1`)
- ❌ N'apparaît pas dans Audio MIDI Setup
- ✅ Initie la connexion vers l'IP configurée
- ✅ Fonctionne avec PC/Linux/serveurs RTP-MIDI

---

## 🔄 Matrice de décision mDNS

| `MDNS_ENABLED` | `RTPMIDI_MODE` | Résultat mDNS | Raison |
|----------------|----------------|---------------|--------|
| 0 | 0 (SERVER) | ❌ Désactivé | Désactivé par config |
| 0 | 1 (CLIENT) | ❌ Désactivé | Désactivé par config |
| 1 | 0 (SERVER) | ✅ Activé | Mode SERVER nécessite mDNS |
| 1 | 1 (CLIENT) | ❌ Désactivé | Mode CLIENT n'utilise pas mDNS |

---

## 📊 Logs attendus

### **Mode SERVER avec mDNS**
```
---- LWIP INITIALIZATIONS -----
--- mDNS INITIALIZATIONS ---
mDNS: Network interface added successfully
mDNS: Apple MIDI TXT records added (txtvers=1, protovers=2)
mDNS: RTP-MIDI service registered successfully (slot=0)

--- RTP-MIDI INITIALIZATIONS --
RTP-MIDI: Mode=SERVER, Destination IP=192.168.100.255
RTP-MIDI: Initialized 'Sp3ctra_CIS' on ports 5004/5005, SSRC=0x...
RTP-MIDI initialization SUCCESS
```

---

### **Mode CLIENT sans mDNS**
```
---- LWIP INITIALIZATIONS -----
mDNS: Service disabled (RTP-MIDI CLIENT mode uses fixed IP)

--- RTP-MIDI INITIALIZATIONS --
RTP-MIDI: Mode=CLIENT, Destination IP=192.168.100.10
RTP-MIDI: Initialized 'Sp3ctra_CIS' on ports 5004/5005, SSRC=0x...
RTP-MIDI: Control invitation sent to 192.168.100.10
RTP-MIDI: Initialized in CLIENT mode (connecting to remote)
RTP-MIDI initialization SUCCESS
```

---

## 🎯 Avantages de cette solution

### **Automatique**
- ✅ Pas besoin de modifier manuellement `MDNS_ENABLED`
- ✅ Le mode RTP-MIDI contrôle automatiquement le mDNS
- ✅ Configuration cohérente et prévisible

### **Économie de ressources**
- ✅ mDNS désactivé quand inutile (mode CLIENT)
- ✅ Moins de trafic réseau
- ✅ Moins de charge CPU

### **Clarté**
- ✅ Logs explicites sur l'état du mDNS
- ✅ Comportement documenté
- ✅ Facile à déboguer

---

## 🔧 Migration depuis l'ancienne version

### **Si vous utilisiez le mode SERVER (par défaut)**
**Aucune action requise** - Le comportement reste identique.

### **Si vous utilisiez le mode CLIENT**
**Aucune action requise** - Le mDNS sera automatiquement désactivé.

### **Nouveau fichier CONFIG.TXT**
Si vous créez un nouveau `CONFIG.TXT`, ajoutez simplement:
```
RTPMIDI_MODE=0  # Pour SERVER (macOS)
# ou
RTPMIDI_MODE=1  # Pour CLIENT (PC/Linux)
```

---

## 📚 Références

- **Mode SERVER:** Voir `RTP_MIDI_Implementation_Summary.md`
- **Mode CLIENT:** Voir `RTP_MIDI_CLIENT_Mode_Implementation.md`
- **Changement de mode:** Voir `RTP_MIDI_Mode_Switch_Guide.md`

---

## ✅ Tests recommandés

### **Test 1: Mode SERVER avec mDNS**
1. Configurer `RTPMIDI_MODE=0` dans CONFIG.TXT
2. Redémarrer le STM32
3. Vérifier les logs: mDNS doit être initialisé
4. Vérifier dans Audio MIDI Setup (macOS): "Sp3ctra_CIS" doit apparaître

### **Test 2: Mode CLIENT sans mDNS**
1. Configurer `RTPMIDI_MODE=1` dans CONFIG.TXT
2. Configurer `NETWORK_DEST_IP` avec l'IP du serveur
3. Redémarrer le STM32
4. Vérifier les logs: mDNS doit être désactivé
5. Vérifier la connexion: le STM32 doit initier la connexion

### **Test 3: Basculement de mode**
1. Démarrer en mode SERVER
2. Modifier CONFIG.TXT pour passer en mode CLIENT
3. Redémarrer
4. Vérifier que le comportement change correctement

---

## 🐛 Dépannage

### **mDNS toujours actif en mode CLIENT**
- Vérifier que `RTPMIDI_MODE=1` dans CONFIG.TXT
- Vérifier les logs au démarrage
- Redémarrer le STM32 après modification

### **mDNS inactif en mode SERVER**
- Vérifier que `MDNS_ENABLED=1` dans CONFIG.TXT
- Vérifier que `RTPMIDI_MODE=0` dans CONFIG.TXT
- Vérifier les logs pour les erreurs d'initialisation

### **Device non visible dans Audio MIDI Setup**
- Normal en mode CLIENT (utilise IP fixe)
- En mode SERVER, vérifier que mDNS est bien initialisé
- Vérifier que macOS et STM32 sont sur le même réseau

---

**Auteur:** Cline AI Assistant
**Date:** 21/12/2025
**Version:** 1.0
