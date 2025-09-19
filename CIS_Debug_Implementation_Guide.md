# GUIDE DE DEBUG CIS - CALIBRATION SEGMENTÉE

## Vue d'ensemble

Ce document décrit les printf de debug ajoutés pour diagnostiquer le problème de "négatif saturé" dans la calibration CIS segmentée.

## DEBUG AJOUTÉS

### 1. **Debug dans `cis_computeCalsGains`**

**Localisation :** Fonction de calcul des gains de calibration

**Informations affichées :**
```c
printf("=== DEBUG GAINS COMPUTATION ===\n");
printf("Color: %s, maxADCValue: %lu, target_intermediate: %lu\n", colorNames[color], maxADCValue, target_intermediate);

// Pour chaque pixel représentatif :
printf("Lane %ld, Pixel %ld: Black=%ld, Inter=%ld, White=%ld\n", lane, i, black_val, inter_val, white_val);
printf("  Seg1: diff=%ld, gain_temp=%ld, gain_final=%d\n", diff_seg1, gain_temp, cisCals.gainsData_seg1[laneOffset + i]);
printf("  Seg2: diff=%ld, gain_temp=%ld, gain_final=%d\n", diff_seg2, gain_temp, cisCals.gainsData_seg2[laneOffset + i]);
```

**Pixels debuggés :**
- 5 premiers pixels de chaque lane
- Pixel du milieu (pixels_per_color_per_lane/2)
- 5 derniers pixels de chaque lane

### 2. **Debug dans `cis_applyLinearCalibration`**

**Localisation :** Fonction d'application de la calibration

**Fréquence :** Toutes les 100 lignes

**Informations affichées :**
```c
printf("=== DEBUG APPLICATION - Line %lu ===\n", app_debug_counter);
printf("maxClipValue: %lu, target_intermediate: %lu\n", maxClipValue, target_intermediate);

// Pour les 5 premiers pixels du canal RED de la lane 0 :
printf("RED Lane %d, Pixel %lu:\n", lane, i);
printf("  raw=%ld, drift_offset=%ld, driftCorrected=%ld\n", raw_value, drift_offset, driftCorrected);
printf("  offset=%d, corrected=%ld, transition=%d\n", offset, corrected, transition_point);
printf("  gain_seg1=%d, gain_seg2=%d\n", gain_seg1, gain_seg2);
printf("  Using Seg1: calibrated=%ld\n", calibrated);  // ou Seg2
printf("  final_value=%ld\n", final_value);
```

## ANALYSE DES SORTIES DE DEBUG

### **Problèmes potentiels à identifier :**

#### 1. **Valeurs de calibration aberrantes**
- **Black, Inter, White** : Vérifier la progression logique (Black < Inter < White)
- **Gains négatifs ou très élevés** : Indicateur de problème de mesure
- **Clipping des gains** : Vérifier si CLIP_INT16 intervient

#### 2. **Problèmes de calcul**
- **diff_seg1 ou diff_seg2 = 0** : Division par zéro évitée mais gain = UNITY_Q8_8
- **gain_temp très élevé** : Peut indiquer des différences trop faibles
- **Overflow dans les calculs** : Vérifier les valeurs intermédiaires

#### 3. **Problèmes d'application**
- **driftCorrected négatif** : Peut causer des problèmes
- **corrected très négatif** : Offset trop élevé
- **calibrated négatif** : Sera clippé à 0 (peut expliquer le "négatif")
- **calibrated > maxClipValue** : Sera clippé (peut expliquer la saturation)

### **Valeurs attendues normales :**

#### **Calibration (exemple 10-bit ADC, maxADCValue=1023) :**
- **Black** : ~50-200 (valeurs sombres)
- **Inter (50%)** : ~300-600 (valeurs intermédiaires)
- **White** : ~800-1000 (valeurs claires)
- **target_intermediate** : ~127 (pour maxClipValue=255)

#### **Gains Q8.8 :**
- **Valeurs normales** : 100-1000 (0.4 à 4.0 en décimal)
- **UNITY_Q8_8** : 256 (1.0 en décimal)
- **Gains très élevés** : >2000 (>8.0) - Problématique
- **Gains très faibles** : <50 (<0.2) - Problématique

#### **Application :**
- **raw** : Valeurs ADC brutes (0-1023)
- **driftCorrected** : Proche de raw (drift faible)
- **corrected** : raw - offset (peut être négatif)
- **calibrated** : Résultat final avant clipping
- **final_value** : 0-255 (après clipping)

## DIAGNOSTIC DU "NÉGATIF SATURÉ"

### **Hypothèses à vérifier :**

1. **Gains trop élevés** → Saturation à maxClipValue
2. **Offsets incorrects** → Valeurs négatives clippées à 0
3. **Points de transition incorrects** → Mauvais choix de segment
4. **Problème de format Q8.8** → Calculs incorrects
5. **Dérive excessive** → Correction de dérive problématique

### **Actions correctives possibles :**

1. **Si gains trop élevés :**
   - Vérifier les mesures de calibration
   - Ajuster le clipping CLIP_INT16
   - Revoir le calcul des gains

2. **Si offsets incorrects :**
   - Vérifier les mesures sur noir
   - Contrôler le clipping des offsets

3. **Si problème de format :**
   - Vérifier les shifts (>> 8 au lieu de >> 16)
   - Contrôler les conversions int64_t

## UTILISATION

### **Activation des debug :**
```c
// Dans config.h, activer temporairement :
#define CIS_DRIFT_DEBUG_ENABLED                 (1)
#define CIS_DETAILED_DEBUG_ENABLED              (1)
```

### **Procédure de diagnostic :**
1. **Faire une calibration** → Observer les debug de `cis_computeCalsGains`
2. **Capturer quelques lignes** → Observer les debug de `cis_applyLinearCalibration`
3. **Analyser les valeurs** selon les critères ci-dessus
4. **Identifier le problème** et appliquer les corrections

### **Désactivation après debug :**
```c
// Remettre à 0 pour les performances
#define CIS_DRIFT_DEBUG_ENABLED                 (0)
#define CIS_DETAILED_DEBUG_ENABLED              (0)
```

## EXEMPLE D'ANALYSE

### **Sortie normale attendue :**
```
=== DEBUG GAINS COMPUTATION ===
Color: RED, maxADCValue: 1023, target_intermediate: 127
Lane 0, Pixel 0: Black=120, Inter=450, White=900
  Seg1: diff=330, gain_temp=98, gain_final=98
  Seg2: diff=450, gain_temp=508, gain_final=508

=== DEBUG APPLICATION - Line 100 ===
maxClipValue: 255, target_intermediate: 127
RED Lane 0, Pixel 0:
  raw=500, drift_offset=5, driftCorrected=495
  offset=120, corrected=375, transition=450
  gain_seg1=98, gain_seg2=508
  Using Seg2: excess=45, calibrated=149
  final_value=149
```

### **Sortie problématique :**
```
Lane 0, Pixel 0: Black=120, Inter=125, White=130
  Seg1: diff=5, gain_temp=6553, gain_final=32767  ← CLIPPING!
  Seg2: diff=5, gain_temp=46284, gain_final=32767 ← CLIPPING!

RED Lane 0, Pixel 0:
  raw=500, drift_offset=5, driftCorrected=495
  offset=120, corrected=375, transition=125
  gain_seg1=32767, gain_seg2=32767
  Using Seg2: excess=370, calibrated=30847  ← SATURATION!
  final_value=255  ← CLIPPÉ À MAX!
```

Cette approche permettra d'identifier précisément la source du problème de "négatif saturé".
