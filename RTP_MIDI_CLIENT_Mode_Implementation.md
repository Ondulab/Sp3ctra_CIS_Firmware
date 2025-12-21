# RTP-MIDI Mode CLIENT - Implémentation avec IP du config.txt

## ✅ Modifications effectuées

### **Fichier modifié : `CM7/Core/Src/freertos.c`**

Le système RTP-MIDI utilise maintenant le **mode CLIENT** avec l'IP de destination lue depuis le fichier `CONFIG.TXT`.

---

## 🔧 Code implémenté

```c
// Convert config destination IP to LwIP ip_addr_t
ip_addr_t remote_ip;
IP4_ADDR(&remote_ip,
         shared_config.network_dest_ip[0],
         shared_config.network_dest_ip[1],
         shared_config.network_dest_ip[2],
         shared_config.network_dest_ip[3]);

printf("RTP-MIDI: Using destination IP from config: %d.%d.%d.%d\n",
       shared_config.network_dest_ip[0],
       shared_config.network_dest_ip[1],
       shared_config.network_dest_ip[2],
       shared_config.network_dest_ip[3]);

// Initialize RTP-MIDI in CLIENT mode
if (rtpmidi_init("Sp3ctra_CIS", &remote_ip, RTPMIDI_MODE_CLIENT) != RTPMIDI_OK)
{
    printf("RTP-MIDI initialization ERROR\n");
}
else
{
    midi_button_mapper_init();
    midi_led_mapper_init(LED_MODE_SIMPLE);
    rtpmidi_register_rx_callback(midi_led_mapper_handle_cc);

    // Initiate connection to remote server
    if (rtpmidi_connect() != RTPMIDI_OK)
    {
        printf("RTP-MIDI: Failed to initiate connection\n");
    }
    else
    {
        printf("RTP-MIDI: Initialized in CLIENT mode (connecting to remote)\n");
        printf("RTP-MIDI initialization SUCCESS\n");
    }
}
```

---

## 📝 Configuration dans CONFIG.TXT

**Exemple de fichier `CONFIG.TXT` :**
```
NETWORK_IP=192.168.100.1
NETWORK_NETMASK=255.255.255.0
NETWORK_GW=0.0.0.0
NETWORK_DEST_IP=192.168.100.10
NETWORK_UDP_PORT=55151
NETWORK_TCP_PORT=5000
```

**Champ important :**
- `NETWORK_DEST_IP` : Adresse IP du serveur RTP-MIDI distant (macOS)

---

## 🔄 Comportement au démarrage

1. **Lecture du config.txt** → `shared_config.network_dest_ip[]`
2. **Conversion en `ip_addr_t`** → Format LwIP
3. **Initialisation RTP-MIDI** en mode CLIENT
4. **Connexion automatique** vers l'IP configurée
5. **Envoi INVITE** → Serveur distant
6. **Réception OK** → Session établie

---

## 📊 Logs attendus

```
--- RTP-MIDI INITIALIZATIONS --
RTP-MIDI: Using destination IP from config: 192.168.100.10
RTP-MIDI: Initialized 'Sp3ctra_CIS' on ports 5004/5005, SSRC=0x...
RTP-MIDI: Control invitation sent to 192.168.100.10
RTP-MIDI: Initialized in CLIENT mode (connecting to remote)
RTP-MIDI initialization SUCCESS
```

Puis dans la boucle MIDI :
```
RTP-MIDI: Control accepted, inviting on data port
RTP-MIDI: Data invitation accepted. Session CONNECTED.
RTP-MIDI: Sync complete. Offset=...
```

---

## ⚠️ Points importants

### **1. Prérequis sur macOS**
Vous devez **lancer une session RTP-MIDI** sur macOS avant que le STM32 ne démarre :
- Ouvrir **Audio MIDI Setup**
- Créer une session réseau MIDI
- Activer "Accepter les connexions"

### **2. IP fixe requise**
L'IP du Mac doit être **fixe** et correspondre à celle dans `CONFIG.TXT`.

### **3. mDNS désactivé**
En mode CLIENT, le mDNS n'est **pas utilisé**. Le device ne sera pas découvert automatiquement.

### **4. Pas de découverte automatique**
Le STM32 n'apparaîtra **pas** dans la liste des périphériques réseau MIDI de macOS.

---

## 🔄 Pour revenir en mode SERVER

Si vous souhaitez revenir au mode SERVER (découverte automatique via mDNS) :

```c
// Mode SERVER (passif)
if (rtpmidi_init("Sp3ctra_CIS", NULL, RTPMIDI_MODE_SERVER) != RTPMIDI_OK)
{
    printf("RTP-MIDI initialization ERROR\n");
}
else
{
    midi_button_mapper_init();
    midi_led_mapper_init(LED_MODE_SIMPLE);
    rtpmidi_register_rx_callback(midi_led_mapper_handle_cc);

    // PAS d'appel à rtpmidi_connect() en mode SERVER
    printf("RTP-MIDI: Initialized in SERVER mode\n");
}
```

---

## 🧪 Test de la configuration

### **1. Modifier CONFIG.TXT**
```
NETWORK_DEST_IP=192.168.100.10
```
(Remplacer par l'IP réelle de votre Mac)

### **2. Recompiler et flasher**
```bash
# Clean + Build dans STM32CubeIDE
# Flash le binaire sur le STM32
```

### **3. Préparer macOS**
- Ouvrir **Audio MIDI Setup**
- Créer une session réseau MIDI
- Activer "Accepter les connexions"

### **4. Démarrer le STM32**
Le device devrait se connecter automatiquement.

### **5. Vérifier les logs**
```
RTP-MIDI: Using destination IP from config: 192.168.100.10
RTP-MIDI: Control invitation sent to 192.168.100.10
RTP-MIDI: Control accepted, inviting on data port
RTP-MIDI: Data invitation accepted. Session CONNECTED.
```

---

## 📋 Résumé

| Aspect | Mode CLIENT (actuel) |
|--------|---------------------|
| **Initialisation** | Actif - STM32 initie |
| **IP** | Lue depuis CONFIG.TXT |
| **mDNS** | Non utilisé |
| **Découverte** | Aucune (IP fixe) |
| **Connexion** | Automatique au démarrage |
| **Prérequis macOS** | Session RTP-MIDI active |

---

## ✅ Avantages du mode CLIENT

- ✅ Configuration flexible via CONFIG.TXT
- ✅ Pas besoin de mDNS
- ✅ Connexion automatique au démarrage
- ✅ Fonctionne avec n'importe quel serveur RTP-MIDI

## ⚠️ Inconvénients

- ❌ IP fixe requise
- ❌ Pas de découverte automatique
- ❌ Nécessite une session active sur le serveur
- ❌ Ne fonctionne pas avec Audio MIDI Setup standard

---

**Date de modification :** 21/12/2025
**Version firmware :** 3.12.1
**Mode RTP-MIDI :** CLIENT avec IP du config.txt
