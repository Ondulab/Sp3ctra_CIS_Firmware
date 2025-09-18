# Mise à jour : Utilisation des pixels stables pour la correction de dérive

## Problème identifié
Les 6 derniers pixels inactifs (pixels 33-38) sont instables et affectent négativement la précision de la correction de dérive.

## Solution implémentée
Utilisation de la constante `CIS_USEFUL_BLACK_PIXELS = 32` au lieu de `CIS_BLACK_PIXELS = 38` pour les calculs de moyenne.

## Modifications apportées

### 1. Configuration (`Common/Inc/config.h`)
```c
#define CIS_BLACK_PIXELS                    (38)  // Nombre total de pixels inactifs
#define CIS_USEFUL_BLACK_PIXELS             (32)  // Nombre de pixels stables utilisés
```

### 2. Calibration (`cis_ComputeCalsInactivesAvrg`)
**Avant :**
```c
cis_mean(&currCals->data[laneOffset], CIS_BLACK_LINE, &currColor->inactiveAvrgPix[lane]);
```

**Après :**
```c
cis_mean(&currCals->data[laneOffset], CIS_USEFUL_BLACK_PIXELS, &currColor->inactiveAvrgPix[lane]);
```

### 3. Application de la correction (`cis_computeGlobalDriftCorrection`)
**Avant :**
```c
cis_mean((const uint32_t *)&cisDataCpy[laneOffset], CIS_BLACK_PIXELS, &currentInactiveAvg[color][lane]);
```

**Après :**
```c
cis_mean((const uint32_t *)&cisDataCpy[laneOffset], CIS_USEFUL_BLACK_PIXELS, &currentInactiveAvg[color][lane]);
```

### 4. Debug amélioré (`cis_printInactivePixels`)
Le debug affiche maintenant :
- **Tous les 38 pixels** pour visualisation complète
- **Moyenne des 38 pixels** pour comparaison
- **Moyenne des 32 premiers pixels** (utilisée pour la correction)

**Exemple de sortie :**
```
INACTIVE PIXELS - Lane 0 RED (38 pixels, using first 32 for average):
  1234 1245 1256 1267 1278 1289 1290 1301 1312 1323
  1334 1345 1356 1367 1378 1389 1390 1401 1412 1423
  1434 1445 1456 1467 1478 1489 1490 1501 1512 1523
  1534 1545 1556 1567 1578 1589 1590 1601
  Average (all 38): 1456
  Average (first 32): 1445 (used for drift correction)
```

## Avantages de cette approche

### 1. **Stabilité améliorée**
- Élimination des pixels instables (33-38)
- Correction de dérive plus précise et fiable
- Réduction du bruit dans les mesures

### 2. **Compatibilité préservée**
- Les 38 pixels sont toujours disponibles pour le debug
- Possibilité de comparer les deux moyennes
- Aucun impact sur les autres fonctionnalités

### 3. **Debug complet**
- Visualisation de tous les pixels pour diagnostic
- Comparaison entre moyenne complète et moyenne stable
- Identification facile des pixels problématiques

## Impact sur les performances

- **Calcul plus rapide** : 32 pixels au lieu de 38 (-16% de calculs)
- **Précision améliorée** : Élimination du bruit des pixels instables
- **Mémoire inchangée** : Même structure de données

## Utilisation

### Calibration
La calibration utilise automatiquement les 32 premiers pixels pour calculer les références de dérive.

### Application temps réel
```c
// La correction de dérive utilise automatiquement les 32 pixels stables
cis_applyLinearCalibrationWithDriftCorrection(cisDataCpy, 255);
```

### Debug
```c
// Activer le debug détaillé pour voir la différence
cis_enableDetailedDebug(500);

// Ou afficher manuellement une lane/couleur spécifique
cis_printInactivePixels(cisDataCpy, 0, 0); // Lane 0, Rouge
```

## Validation recommandée

1. **Comparer les moyennes** : Vérifier la différence entre moyenne 38 vs 32 pixels
2. **Stabilité temporelle** : Observer la variation des moyennes dans le temps
3. **Correction de dérive** : Valider l'amélioration de la précision
4. **Tests de température** : Vérifier le comportement sur différentes plages thermiques

Cette modification améliore significativement la robustesse de la correction de dérive en se concentrant sur les pixels les plus stables.
