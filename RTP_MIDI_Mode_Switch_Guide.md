# Guide : Passer de SERVER à CLIENT en RTP-MIDI

## 📋 Résumé des modes

| Mode | Comportement | Utilisation |
|------|-------------|-------------|
| **SERVER** | Passif - Attend les connexions | ✅ **Recommandé** pour macOS (découverte mDNS) |
| **CLIENT** | Actif - Initie les connexions | Pour se connecter à un serveur RTP-MIDI distant |

---

## 🔧 Modifications nécessaires pour passer en CLIENT

### **1. Modifier `CM7/Core/Src/freertos.c` (ligne ~260)**

#### **Mode SERVER (actuel) :**
```c
// Initialize RTP-MIDI in SERVER mode (passive, waits for macOS to connect)
if (rtpmidi_init("Sp3ctra_CIS", NULL, RTPMIDI_MODE_SERVER) != RTPMIDI_OK)
{
    printf("RTP-MIDI initialization ERROR\n");
}
else
{
    // Initialize mappers
    midi_button_mapper_init();
    midi_led_mapper_init(LED_MODE_SIMPLE);
    rtpmidi_register_rx_callback(midi_led_mapper_handle_cc);

    // In SERVER mode, we do NOT call rtpmidi_connect()
    printf("RTP-MIDI: Initialized in SERVER mode (waiting for macOS connection)\n");
    printf("RTP-MIDI initialization SUCCESS\n");
}
```

#### **Mode CLIENT (à modifier) :**
```c
// Initialize RTP-MIDI in CLIENT mode (active, initiates connection)
// Define remote IP address (macOS IP)
ip_addr_t remote_ip;
IP4_ADDR(&remote_ip, 192, 168, 100, 10); // ← Remplacer par l'IP de votre Mac

if (rtpmidi_init("Sp3ctra_CIS", &remote_ip, RTPMIDI_MODE_CLIENT) != RTPMIDI_OK)
{
    printf("RTP-MIDI initialization ERROR\n");
}
else
{
    // Initialize mappers
    midi_button_mapper_init();
    midi_led_mapper_init(LED_MODE_SIMPLE);
    rtpmidi_register_rx_callback(midi_led_mapper_handle_cc);

    // In CLIENT mode, initiate connection to remote server
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

## 📊 Différences de comportement

### **Mode SERVER (actuel)**
```
1. STM32 démarre
2. mDNS annonce "sp3ctra._apple-midi._udp.local"
3. STM32 attend passivement
4. macOS découvre le device via mDNS
5. macOS envoie INVITE → STM32
6. STM32 répond OK
7. Session établie ✅
```

### **Mode CLIENT**
```
1. STM32 démarre
2. STM32 envoie INVITE → macOS (IP fixe)
3. macOS répond OK
4. STM32 envoie INVITE data port
5. macOS répond OK
6. Session établie ✅
```

---

## ⚠️ Limitations du mode CLIENT

1. **IP fixe requise** : Vous devez connaître l'IP du Mac à l'avance
2. **Pas de découverte automatique** : Le Mac n'apparaît pas dans Audio MIDI Setup
3. **Connexion manuelle** : Le STM32 doit initier la connexion à chaque démarrage

---

## ✅ Recommandation

**Restez en mode SERVER** pour une utilisation avec macOS :
- ✅ Découverte automatique via mDNS
- ✅ Apparaît dans Audio MIDI Setup
- ✅ Connexion simple depuis macOS
- ✅ Pas besoin de connaître l'IP à l'avance

**Utilisez le mode CLIENT** uniquement si :
- Vous voulez vous connecter à un serveur RTP-MIDI distant (pas macOS)
- Vous avez une IP fixe connue
- Le serveur distant ne supporte pas mDNS

---

## 🔄 Pour revenir en mode SERVER

Remettez simplement :
```c
rtpmidi_init("Sp3ctra_CIS", NULL, RTPMIDI_MODE_SERVER)
```

Et supprimez l'appel à `rtpmidi_connect()`.

---

## 📝 Notes importantes

- Le mode SERVER est **100% fonctionnel** actuellement
- Le MIDI fonctionne dans les deux sens (STM32 ↔ macOS)
- Le format RTP-MIDI est conforme RFC 6295
- La session est stable et synchronisée

**Conclusion : Pas besoin de changer de mode !** 🎉
