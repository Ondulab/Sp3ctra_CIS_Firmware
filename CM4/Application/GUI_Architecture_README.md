# Architecture Modulaire GUI - Documentation

## Vue d'ensemble

Le fichier `gui.c` original a été refactorisé en une architecture modulaire pour améliorer la maintenabilité, la lisibilité et respecter les contraintes temps réel du STM32H7 dual-core.

## Structure des Modules

### 1. **gui_core** - Coordination principale
**Fichiers:** `gui_core.h`, `gui_core.c`

**Responsabilités:**
- Boucle principale `gui_mainLoop()`
- Gestion de l'état du screensaver
- Coordination entre tous les modules
- Initialisation des scanlines
- Calcul de la fréquence CIS

**Contraintes RT:** Non critique - coordination de haut niveau

### 2. **gui_cis_display** - Affichage des données CIS
**Fichiers:** `gui_cis_display.h`, `gui_cis_display.c`

**Responsabilités:**
- Rendu de l'image CIS (`gui_displayImage()`)
- Traitement des données RGB
- Conversion grayscale et calculs d'intensité
- Rendu pixel par pixel

**Contraintes RT:** Critique - traitement des données en temps réel

### 3. **gui_imu** - Gestion IMU et détection de mouvement
**Fichiers:** `gui_imu.h`, `gui_imu.c`

**Responsabilités:**
- Affichage des données IMU (conditionnel avec `GUI_SHOW_IMU`)
- Calcul des moyennes mobiles (`update_IMU_average()`)
- Détection de mouvement significatif (`gui_isSignificantMotion()`)
- Visualisation graphique des capteurs

**Contraintes RT:** Modérée - traitement des données capteurs

### 4. **gui_animations** - Effets visuels et animations
**Fichiers:** `gui_animations.h`, `gui_animations.c`

**Responsabilités:**
- Animation des vagues (`gui_renderWaveAnimation()`)
- Écran d'attente (`gui_displayWaiting()`)
- Économiseur d'écran (`gui_displayScreensaver()`)
- Effets visuels avancés (`ssd1362_drawBmpNoisyFx()`)
- Overlays et logos

**Contraintes RT:** Non critique - effets visuels

### 5. **gui_interaction** - Interface utilisateur
**Fichiers:** `gui_interaction.h`, `gui_interaction.c`

**Responsabilités:**
- Gestion des boutons (`gui_interractiveMenu()`)
- Affichage des popups (`gui_displayPopUp()`)
- Détection d'activité boutons (`gui_checkButtonActivity()`)
- Feedback LED et états des boutons

**Contraintes RT:** Modérée - réactivité utilisateur

### 6. **gui_calibration** - Processus de calibration
**Fichiers:** `gui_calibration.h`, `gui_calibration.c`

**Responsabilités:**
- Processus de calibration CIS (`gui_startCalibration()`)
- Changement d'orientation (`gui_changeHand()`)
- Interface de calibration étape par étape

**Contraintes RT:** Non critique - processus occasionnel

### 7. **gui_interrupts** - Gestionnaires d'interruption
**Fichiers:** `gui_interrupts.h`, `gui_interrupts.c`

**Responsabilités:**
- Gestionnaire HSEM (`HSEM2_IRQHandler()`)
- Variable de synchronisation `transferComplete`
- Synchronisation inter-cœurs CM4/CM7

**Contraintes RT:** Critique - gestionnaire d'interruption

## Avantages de l'Architecture

### 1. **Séparation des Responsabilités**
- Chaque module a une responsabilité claire et définie
- Facilite la maintenance et les tests
- Réduit les dépendances croisées

### 2. **Respect des Contraintes Temps Réel**
- Modules critiques identifiés et optimisés
- Séparation des chemins RT et non-RT
- Gestionnaires d'interruption isolés

### 3. **Maintenabilité Améliorée**
- Code plus lisible et organisé
- Facilite l'ajout de nouvelles fonctionnalités
- Debugging plus simple

### 4. **Compilation Conditionnelle**
- Support des flags comme `GUI_SHOW_IMU`
- Optimisation de la taille du code
- Flexibilité de configuration

### 5. **Architecture STM32H7 Dual-Core**
- Respect de la séparation CM4/CM7
- Synchronisation inter-cœurs appropriée
- Gestion mémoire optimisée

## Utilisation

### Include Principal
```c
#include "gui.h"  // Inclut tous les modules automatiquement
```

### Modules Individuels
```c
#include "gui_core.h"        // Pour la boucle principale
#include "gui_cis_display.h" // Pour l'affichage CIS
#include "gui_imu.h"         // Pour les données IMU
// etc.
```

### Compatibilité Rétroactive
L'interface publique reste identique :
```c
int gui_mainLoop(void);  // Fonction principale inchangée
```

## Dépendances

### Dépendances Externes
- HAL STM32H7
- FreeRTOS (pour HAL_GetTick())
- Drivers SSD1362, LEDs
- Globals et configuration partagées

### Dépendances Inter-Modules
- `gui_core` → tous les autres modules
- `gui_interaction` → `gui_calibration`
- `gui_cis_display` → `gui_interrupts`

## Considérations de Performance

### Modules Critiques (RT)
- `gui_interrupts` : Gestionnaire d'interruption
- `gui_cis_display` : Traitement données CIS
- `gui_imu` : Traitement capteurs

### Modules Non-Critiques
- `gui_animations` : Effets visuels
- `gui_calibration` : Processus occasionnel

### Optimisations
- Variables statiques pour éviter les allocations
- Calculs pré-compilés quand possible
- Accès direct GPIO pour réactivité

## Migration et Maintenance

### Ajout de Nouvelles Fonctionnalités
1. Identifier le module approprié
2. Ajouter les prototypes dans le `.h`
3. Implémenter dans le `.c`
4. Mettre à jour `gui.h` si nécessaire

### Debugging
- Chaque module peut être testé indépendamment
- Logs et traces par module
- Isolation des problèmes facilitée

Cette architecture modulaire respecte les bonnes pratiques du développement embarqué temps réel tout en maintenant la compatibilité avec l'existant.
