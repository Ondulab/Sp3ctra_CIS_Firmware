# CIS Global Drift Correction - Implementation Documentation

## Vue d'ensemble

Cette implémentation ajoute une correction de dérive globale au système de calibration CIS existant, permettant de compenser les dérives thermiques et temporelles des capteurs en temps réel.

## Architecture de correction à deux niveaux

### 1. Correction globale de dérive (nouvelle)
- **Objectif** : Compenser les dérives globales affectant tous les pixels d'une couleur/lane
- **Méthode** : Mesure en temps réel des pixels inactifs (CIS_BLACK_LINE) et comparaison avec les références de calibration
- **Fréquence** : À chaque ligne scannée

### 2. Correction individuelle des pixels (existante)
- **Objectif** : Corriger les variations pixel par pixel
- **Méthode** : Application des offsets et gains individuels calculés lors de la calibration
- **Fréquence** : À chaque pixel

## Formule de calibration modifiée

**Ancienne formule :**
```c
calibrated = clip( ((raw - offset_pixel) * gain_pixel) >> 16, 0, maxClipValue )
```

**Nouvelle formule :**
```c
// Étape 1: Correction globale de dérive
drift_corrected = raw - global_drift_offset

// Étape 2: Correction individuelle
calibrated = clip( ((drift_corrected - offset_pixel) * gain_pixel) >> 16, 0, maxClipValue )
```

## Structures de données modifiées

### Structure `cisCals` étendue

```c
struct cisCals
{
    int32_t offsetData[CIS_MAX_USEFUL_DATA_SIZE * CIS_ADC_OUT_LANES];
    int32_t gainsData[CIS_MAX_USEFUL_DATA_SIZE * CIS_ADC_OUT_LANES];

    // Nouvelles données pour la correction de dérive
    int32_t blackRefInactiveAvg[3][CIS_ADC_OUT_LANES];  // [color][lane] - référence noire
    int32_t whiteRefInactiveAvg[3][CIS_ADC_OUT_LANES];  // [color][lane] - référence blanche
    int32_t driftCorrectionEnabled;                     // Activation/désactivation
    int32_t driftThreshold;                             // Seuil de dérive maximum
};
```

## Nouvelles fonctions implémentées

### 1. `cis_initDriftCorrectionDefaults()`
- **Rôle** : Initialise les paramètres par défaut de la correction de dérive
- **Paramètres par défaut** :
  - `driftCorrectionEnabled = 1` (activé)
  - `driftThreshold = 50` (50 comptes ADC maximum)

### 2. `cis_computeGlobalDriftCorrection()`
- **Rôle** : Calcule les offsets de dérive globale en temps réel
- **Algorithme** :
  1. Mesure la moyenne des pixels inactifs actuels pour chaque couleur/lane
  2. Compare avec les références de calibration noire
  3. Calcule l'offset de dérive : `drift_offset = current_avg - black_ref_avg`
  4. Applique un seuillage pour éviter les corrections excessives

### 3. `cis_applyLinearCalibrationWithDriftCorrection()`
- **Rôle** : Applique la calibration avec correction de dérive globale
- **Optimisations** :
  - Pragma GCC pour déroulement de boucles
  - Calcul des offsets de dérive une seule fois par ligne
  - Possibilité de désactiver la correction de dérive

## Modifications du processus de calibration

### Étapes ajoutées dans `cis_startLinearCalibration()`

**Étape 7 (nouvelle) : Sauvegarde des références de pixels inactifs**
```c
for (int32_t lane = 0; lane < CIS_ADC_OUT_LANES; lane++)
{
    cisCals.blackRefInactiveAvg[0][lane] = blackCal.red.inactiveAvrgPix[lane];    // Rouge
    cisCals.blackRefInactiveAvg[1][lane] = blackCal.green.inactiveAvrgPix[lane];  // Vert
    cisCals.blackRefInactiveAvg[2][lane] = blackCal.blue.inactiveAvrgPix[lane];   // Bleu

    cisCals.whiteRefInactiveAvg[0][lane] = whiteCal.red.inactiveAvrgPix[lane];    // Rouge
    cisCals.whiteRefInactiveAvg[1][lane] = whiteCal.green.inactiveAvrgPix[lane];  // Vert
    cisCals.whiteRefInactiveAvg[2][lane] = whiteCal.blue.inactiveAvrgPix[lane];   // Bleu
}
```

## Utilisation

### Activation/Désactivation
```c
// Activer la correction de dérive
cisCals.driftCorrectionEnabled = 1;

// Désactiver la correction de dérive
cisCals.driftCorrectionEnabled = 0;
```

### Ajustement du seuil de dérive
```c
// Seuil de dérive maximum (en comptes ADC)
cisCals.driftThreshold = 100;  // Permet une dérive jusqu'à ±100 comptes

// Désactiver le seuillage
cisCals.driftThreshold = 0;    // Pas de limitation
```

### Application de la calibration
```c
// Avec correction de dérive (recommandé)
cis_applyLinearCalibrationWithDriftCorrection(cisDataCpy, 255);

// Sans correction de dérive (ancienne méthode)
cis_applyLinearCalibration(cisDataCpy, 255);
```

## Avantages de l'implémentation

1. **Robustesse** : Compense automatiquement les dérives thermiques et temporelles
2. **Performance** : Impact minimal sur les performances (calcul une fois par ligne)
3. **Compatibilité** : Préserve la fonctionnalité existante
4. **Flexibilité** : Peut être activée/désactivée selon les besoins
5. **Sécurité** : Seuillage pour éviter les corrections excessives

## Considérations temps réel

- **Contraintes RT respectées** : Pas d'allocation dynamique, calculs O(1) par pixel
- **Optimisations GCC** : Déroulement de boucles pour les performances
- **Cache coherency** : Utilisation de `SCB_CleanDCache_by_Addr()` appropriée

## Tests et validation recommandés

1. **Test de dérive thermique** : Vérifier la correction lors de variations de température
2. **Test de performance** : Mesurer l'impact sur le temps de traitement par ligne
3. **Test de stabilité** : Vérifier la stabilité à long terme
4. **Test de seuillage** : Valider le comportement avec différents seuils

## Configuration recommandée

```c
// Configuration optimale pour la plupart des cas d'usage
cisCals.driftCorrectionEnabled = 1;    // Activé
cisCals.driftThreshold = 50;           // Seuil modéré
```

Cette implémentation fournit une solution robuste et performante pour la correction de dérive globale tout en maintenant la compatibilité avec le système existant.
