# Solution pour le problème de synchronisation UDP au démarrage

## Problème identifié

Lorsque l'STM32 et le Raspberry Pi démarrent simultanément (branchés sur la même prise), le flux UDP ne fonctionne pas. Il faut débrancher et rebrancher l'STM32 pour que la communication fonctionne.

### Cause du problème

**Race condition** au démarrage :
1. L'STM32 et le Pi démarrent en même temps
2. L'STM32 initialise LWIP et tente de se connecter en UDP
3. Le Raspberry Pi n'a pas encore fini de démarrer son stack réseau
4. La connexion UDP échoue car le destinataire n'est pas disponible
5. Aucun mécanisme de retry n'était implémenté

## Solution implémentée

### 1. Mécanisme de retry automatique

**Nouvelles fonctions ajoutées dans `udp_client.c` :**

- `udpClient_reinit()` : Réinitialise la connexion UDP
- `udpClient_testConnection()` : Teste la connectivité en envoyant un petit paquet
- `udpClient_connectionMonitorTask()` : Tâche de monitoring qui surveille et rétablit la connexion

**Nouvelles variables :**
- `udpConnectionEstablished` : Indique si la connexion UDP est établie
- `retryAttempts` : Compteur de tentatives de reconnexion

### 2. Paramètres configurables

**Constantes définies dans `udp_client.h` :**
```c
#define UDP_CONNECTION_RETRY_DELAY_MS    2000  // Délai entre les tentatives
#define UDP_CONNECTION_TEST_INTERVAL_MS  5000  // Intervalle de test de connexion
#define UDP_MAX_RETRY_ATTEMPTS          10     // Nombre max de tentatives
```

### 3. Tâche de monitoring

Une nouvelle tâche FreeRTOS `udpMonitorTask` a été créée qui :
- Surveille l'état de la connexion réseau
- Teste périodiquement la connectivité UDP
- Tente automatiquement la reconnexion en cas d'échec
- Envoie le paquet de startup après reconnexion

## Fonctionnement de la solution

### Au démarrage normal
1. L'STM32 démarre et initialise LWIP
2. La tâche de monitoring démarre en parallèle
3. Si le Pi n'est pas encore prêt, la connexion UDP échoue
4. La tâche de monitoring détecte l'échec et retente automatiquement
5. Dès que le Pi est prêt, la connexion s'établit

### En cas de déconnexion réseau
1. La tâche de monitoring détecte la perte de connexion
2. Elle tente automatiquement de se reconnecter
3. Après reconnexion, elle renvoie le paquet de startup si nécessaire

## Avantages de cette solution

1. **Robustesse** : Gère tous les cas de démarrage simultané
2. **Automatique** : Aucune intervention manuelle requise
3. **Configurable** : Paramètres ajustables selon les besoins
4. **Logging** : Messages de debug pour suivre l'état de la connexion
5. **Récupération** : Gère aussi les déconnexions réseau en cours d'utilisation

## Messages de debug

La solution ajoute plusieurs messages de debug pour faciliter le diagnostic :

```
UDP connection monitor task started
Reinitializing UDP connection...
UDP connection reinitialized successfully
UDP connection lost, attempting to reconnect...
UDP connection restored
Startup packet sent after reconnection
UDP reconnection failed (attempt X/10)
Max retry attempts reached, waiting longer...
Network is down, waiting for connection...
```

## Test de la solution

Pour tester la solution :

1. **Test de démarrage simultané :**
   - Brancher l'STM32 et le Pi sur la même prise
   - Allumer simultanément
   - Vérifier que la communication UDP s'établit automatiquement

2. **Test de déconnexion réseau :**
   - Débrancher le câble réseau pendant le fonctionnement
   - Rebrancher le câble
   - Vérifier que la connexion se rétablit automatiquement

3. **Monitoring des logs :**
   - Surveiller les messages de debug via UART/USB
   - Vérifier que les tentatives de reconnexion fonctionnent

## Configuration avancée

Si nécessaire, vous pouvez ajuster les paramètres dans `udp_client.h` :

- **Réduire `UDP_CONNECTION_TEST_INTERVAL_MS`** pour des tests plus fréquents
- **Augmenter `UDP_MAX_RETRY_ATTEMPTS`** pour plus de tentatives
- **Modifier `UDP_CONNECTION_RETRY_DELAY_MS`** pour changer le délai entre tentatives

## Impact sur les performances

- **Minimal** : La tâche de monitoring utilise peu de ressources
- **Configurable** : Les intervalles peuvent être ajustés selon les besoins
- **Optimisé** : Les tests de connexion utilisent de petits paquets

Cette solution résout définitivement le problème de synchronisation au démarrage tout en améliorant la robustesse générale de la communication UDP.
