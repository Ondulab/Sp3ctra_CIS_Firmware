# Solution simplifiée pour le problème de synchronisation UDP au démarrage

## Problème identifié

Lorsque l'STM32 et le Raspberry Pi démarrent simultanément, le flux UDP ne fonctionne pas et le système peut se figer ("freeze"). Il fallait débrancher et rebrancher l'STM32 pour que la communication fonctionne.

### Causes du problème

1. **Race condition** au démarrage : L'STM32 tente de se connecter en UDP avant que le Pi soit prêt
2. **Initialisation du capteur CIS trop précoce** : Le CIS démarre avant que la connexion réseau soit stable
3. **Tâche de monitoring complexe** : La tâche de monitoring créée initialement causait des conflits et des freezes

## Solution simplifiée implémentée

### 1. Suppression de la tâche de monitoring problématique

**Supprimé :**
- `udpClient_connectionMonitorTask()` 
- `udpClient_reinit()`
- `udpClient_testConnection()`
- Variables complexes (`udpConnectionEstablished`, `retryAttempts`)

### 2. Réorganisation de l'ordre d'initialisation

**Nouvelle séquence dans `freertos.c` :**
```c
// 1. Initialiser LWIP et UDP
MX_LWIP_Init();
udpClient_init();

// 2. ATTENDRE que la connexion réseau soit établie
while(isConnected == 0) {
    osDelay(500);
    // Avec timeout de sécurité après 60 secondes
}

// 3. SEULEMENT APRÈS, initialiser le capteur CIS
cis_scanInit();
```

### 3. Mécanisme de retry simple dans l'envoi UDP

**Amélioration de `udpClient_sendData()` :**
- Retry automatique jusqu'à 3 tentatives
- Délai de 10ms entre les tentatives
- Logs d'erreur seulement sur l'échec final
- Vérification de la validité de la connexion

## Avantages de cette solution simplifiée

✅ **Plus stable** : Suppression des conflits de tâches
✅ **Plus simple** : Moins de code, moins de complexité
✅ **Plus robuste** : Ordre d'initialisation logique
✅ **Déterministe** : Comportement prévisible
✅ **Efficace** : Retry simple mais suffisant

## Fonctionnement

### Au démarrage simultané
1. L'STM32 démarre et initialise LWIP
2. Le système attend que `isConnected == 1`
3. Messages de progression toutes les 5 secondes
4. Timeout de sécurité après 60 secondes
5. Le CIS ne démarre qu'après la connexion réseau

### En cas d'erreur UDP
- Retry automatique jusqu'à 3 fois
- Délai de 10ms entre tentatives
- Log d'erreur seulement si tous les essais échouent

## Messages de debug

```
--- WAITING FOR NETWORK CONNECTION ---
Still waiting for network connection... (5 seconds)
Still waiting for network connection... (10 seconds)
Network connection established - proceeding with CIS initialization
----- CIS INITIALIZATIONS -----
```

En cas d'erreur :
```
Failed to send UDP data after 3 retries: -4
```

## Test de la solution

1. **Test de démarrage simultané :**
   - Brancher l'STM32 et le Pi sur la même prise
   - Allumer simultanément
   - Observer les logs : le système attend la connexion avant d'initialiser le CIS

2. **Vérification de stabilité :**
   - Plus de freezes du CM7
   - Pas de redémarrages en boucle
   - Communication UDP stable une fois établie

## Différences avec la solution complexe précédente

| Aspect | Solution complexe | Solution simplifiée |
|--------|------------------|-------------------|
| Tâches FreeRTOS | +1 tâche monitoring | Aucune tâche ajoutée |
| Fonctions | +3 fonctions complexes | Retry simple intégré |
| Variables | +3 variables d'état | Variables minimales |
| Stabilité | Freezes observés | Stable |
| Complexité | Élevée | Faible |
| Maintenance | Difficile | Simple |

## Impact sur les performances

- **Minimal** : Pas de tâche supplémentaire
- **Démarrage** : Légèrement plus lent (attente réseau) mais plus fiable
- **Fonctionnement** : Identique une fois la connexion établie
- **Mémoire** : Moins d'utilisation (suppression de code complexe)

## Configuration

Aucune configuration supplémentaire requise. Le système utilise :
- Timeout réseau : 60 secondes
- Retry UDP : 3 tentatives
- Délai retry : 10ms

Cette solution simplifiée résout le problème de synchronisation au démarrage tout en éliminant les problèmes de stabilité causés par la solution complexe précédente.
