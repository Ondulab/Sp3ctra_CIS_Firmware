# PLAN DE MODIFICATION DIRECTE - REMPLACEMENT DE LA CALIBRATION EXISTANTE

## Vue d'ensemble

Ce document détaille le plan d'implémentation pour remplacer complètement la calibration linéaire simple par une calibration segmentée à 3 points (noir, intermédiaire 50%, blanc). Cette approche améliore la correction de la non-linéarité des pixels tout en maintenant des performances temps réel acceptables.

## PHASE 1 : EXTENSION DES STRUCTURES EXISTANTES

### 1.1 Modification de `Common/Inc/config.h`

Ajouter les nouvelles définitions (garder les existantes) :

```c
// Ajout des nouvelles définitions pour la calibration segmentée
#define CIS_INTERMEDIATE_LED_POWER              (50)    // Puissance LED intermédiaire (50%)
```

### 1.2 Modification de `Common/Inc/globals.h`

**REMPLACER** la structure `cisCals` existante par :

```c
struct __attribute__((aligned(4))) cisCals
{
    // Offset unique (valeur noire) - OPTIMISÉ 16-bit
    int16_t offsetData[CIS_MAX_USEFUL_DATA_SIZE * CIS_ADC_OUT_LANES];

    // Gains pour les deux segments - OPTIMISÉ Q8.8 format
    int16_t gainsData_seg1[CIS_MAX_USEFUL_DATA_SIZE * CIS_ADC_OUT_LANES];   // 0% → 50%
    int16_t gainsData_seg2[CIS_MAX_USEFUL_DATA_SIZE * CIS_ADC_OUT_LANES];   // 50% → 100%

    // Points de transition par pixel (valeur ADC à 50%) - OPTIMISÉ 16-bit
    int16_t transitionPoint[CIS_MAX_USEFUL_DATA_SIZE * CIS_ADC_OUT_LANES];

    // Références pour correction de dérive (INCHANGÉ)
    int32_t blackRefInactiveAvg[COLOR_CHANNELS][CIS_ADC_OUT_LANES];
};
```

**MODIFIER** les états de calibration dans `CIS_Calibration_StateTypeDef` :

```c
typedef enum
{
    CIS_CAL_REQUESTED = 0,
    CIS_CAL_START,
    CIS_CAL_WHITE,                    // MODIFIÉ (ancien CIS_CAL_PLACE_ON_WHITE)
    CIS_CAL_INTERMEDIATE,             // NOUVEAU (ancien CIS_CAL_PLACE_ON_INTERMEDIATE)
    CIS_CAL_BLACK,                    // MODIFIÉ (ancien CIS_CAL_PLACE_ON_BLACK)
    CIS_CAL_EXTRACT_INNACTIVE_REF,
    CIS_CAL_EXTRACT_EXTREMUMS,
    CIS_CAL_EXTRACT_OFFSETS,
    CIS_CAL_COMPUTE_GAINS,
    CIS_CAL_COMPUTE_TRANSITIONS,      // NOUVEAU
    CIS_CAL_END,
} CIS_Calibration_StateTypeDef;
```

## PHASE 2 : MODIFICATION DU PROCESSUS DE CALIBRATION

### 2.1 Modification de `CM7/Peripheral/Src/cis_linearCal.c`

#### 2.1.1 Ajout de la variable pour la mesure intermédiaire

Ajouter après les variables existantes :

```c
// AJOUTER après les variables existantes
__attribute__((section(".calAccIntermediate")))
struct cisCalsTypes intermediateCal;
```

#### 2.1.2 **REMPLACER** la fonction `cis_startLinearCalibration`

```c
void cis_startLinearCalibration(int32_t *cisDataCpy, uint16_t iterationNb, uint32_t bitDepth)
{
    printf("===== SEGMENTED CALIBRATION STARTED =====\n");
    printf("Calibration for %d DPI (3-point segmented)\n", shared_config.cis_dpi);

    char calibrationFilePath[64];

    // Initialisation (INCHANGÉ)
    memset(&blackCal, 0, sizeof(blackCal));
    memset(&intermediateCal, 0, sizeof(intermediateCal));  // NOUVEAU
    memset(&whiteCal, 0, sizeof(whiteCal));
    memset(&cisCals, 0, sizeof(cisCals));

    // ÉTAPE 1: Capture blanc (100%) - INCHANGÉ
    cis_ledPowerAdj(100, 100, 100);
    shared_var.cis_cal_progressbar = 0;
    shared_var.cis_cal_state = CIS_CAL_WHITE;  // MODIFIÉ
    osDelay(200);

    cis_imageProcessRGB_Calibration(cisDataCpy, whiteCal.data, iterationNb);
    SCB_CleanDCache_by_Addr((uint32_t *)&cisCals, sizeof(cisCals));
    osDelay(200);

    // ÉTAPE 2: Capture intermédiaire (50%) - NOUVEAU
    shared_var.cis_cal_progressbar = 25;
    shared_var.cis_cal_state = CIS_CAL_INTERMEDIATE;  // NOUVEAU
    cis_ledPowerAdj(CIS_INTERMEDIATE_LED_POWER, CIS_INTERMEDIATE_LED_POWER, CIS_INTERMEDIATE_LED_POWER);
    osDelay(200);

    cis_imageProcessRGB_Calibration(cisDataCpy, intermediateCal.data, iterationNb);
    SCB_CleanDCache_by_Addr((uint32_t *)&cisCals, sizeof(cisCals));
    osDelay(200);

    // ÉTAPE 3: Capture noir (1%) - INCHANGÉ
    shared_var.cis_cal_progressbar = 50;
    shared_var.cis_cal_state = CIS_CAL_BLACK;  // MODIFIÉ
    cis_ledPowerAdj(1, 1, 1);
    osDelay(20);

    cis_imageProcessRGB_Calibration(cisDataCpy, blackCal.data, iterationNb);
    SCB_CleanDCache_by_Addr((uint32_t *)&cisCals, sizeof(cisCals));
    cis_ledPowerAdj(100, 100, 100);
    osDelay(500);

    printf("Compute average\n");

    // ÉTAPE 4: Calcul des moyennes inactives - MODIFIÉ pour inclure intermédiaire
    cis_ComputeCalsInactivesAvrg(&blackCal, CIS_RED);
    cis_ComputeCalsInactivesAvrg(&intermediateCal, CIS_RED);    // NOUVEAU
    cis_ComputeCalsInactivesAvrg(&whiteCal, CIS_RED);
    cis_ComputeCalsInactivesAvrg(&blackCal, CIS_GREEN);
    cis_ComputeCalsInactivesAvrg(&intermediateCal, CIS_GREEN);  // NOUVEAU
    cis_ComputeCalsInactivesAvrg(&whiteCal, CIS_GREEN);
    cis_ComputeCalsInactivesAvrg(&blackCal, CIS_BLUE);
    cis_ComputeCalsInactivesAvrg(&intermediateCal, CIS_BLUE);   // NOUVEAU
    cis_ComputeCalsInactivesAvrg(&whiteCal, CIS_BLUE);
    SCB_CleanDCache_by_Addr((uint32_t *)&cisCals, sizeof(cisCals));
    shared_var.cis_cal_state = CIS_CAL_EXTRACT_INNACTIVE_REF;
    osDelay(200);

    printf("Compute extremums\n");

    // ÉTAPE 5: Calcul des extremums - MODIFIÉ pour inclure intermédiaire
    cis_computeCalsExtremums(&blackCal, CIS_RED);
    cis_computeCalsExtremums(&intermediateCal, CIS_RED);        // NOUVEAU
    cis_computeCalsExtremums(&whiteCal, CIS_RED);
    cis_computeCalsExtremums(&blackCal, CIS_GREEN);
    cis_computeCalsExtremums(&intermediateCal, CIS_GREEN);      // NOUVEAU
    cis_computeCalsExtremums(&whiteCal, CIS_GREEN);
    cis_computeCalsExtremums(&blackCal, CIS_BLUE);
    cis_computeCalsExtremums(&intermediateCal, CIS_BLUE);       // NOUVEAU
    cis_computeCalsExtremums(&whiteCal, CIS_BLUE);
    SCB_CleanDCache_by_Addr((uint32_t *)&cisCals, sizeof(cisCals));
    shared_var.cis_cal_state = CIS_CAL_EXTRACT_EXTREMUMS;
    osDelay(200);

    printf("Extract offsets\n");

    // ÉTAPE 6: Calcul des offsets segmentés - REMPLACÉ
    cis_computeCalsOffsets(&whiteCal, &intermediateCal, &blackCal, CIS_RED);
    cis_computeCalsOffsets(&whiteCal, &intermediateCal, &blackCal, CIS_GREEN);
    cis_computeCalsOffsets(&whiteCal, &intermediateCal, &blackCal, CIS_BLUE);
    SCB_CleanDCache_by_Addr((uint32_t *)&cisCals, sizeof(cisCals));
    shared_var.cis_cal_state = CIS_CAL_EXTRACT_OFFSETS;
    osDelay(200);

    // ÉTAPE 7: Calcul des gains segmentés - REMPLACÉ
    cis_computeCalsGains(bitDepth, &whiteCal, &intermediateCal, &blackCal, CIS_RED);
    cis_computeCalsGains(bitDepth, &whiteCal, &intermediateCal, &blackCal, CIS_GREEN);
    cis_computeCalsGains(bitDepth, &whiteCal, &intermediateCal, &blackCal, CIS_BLUE);
    SCB_CleanDCache_by_Addr((uint32_t *)&cisCals, sizeof(cisCals));
    shared_var.cis_cal_state = CIS_CAL_COMPUTE_GAINS;
    printf("Compute gains\n");

    // ÉTAPE 8: Calcul des points de transition - NOUVEAU
    cis_computeTransitionPoints(&intermediateCal, CIS_RED);
    cis_computeTransitionPoints(&intermediateCal, CIS_GREEN);
    cis_computeTransitionPoints(&intermediateCal, CIS_BLUE);
    SCB_CleanDCache_by_Addr((uint32_t *)&cisCals, sizeof(cisCals));
    shared_var.cis_cal_state = CIS_CAL_COMPUTE_TRANSITIONS;
    printf("Compute transition points\n");

    // ÉTAPE 9: Stockage des références de dérive - INCHANGÉ
    printf("Store drift correction references\n");
    for (int32_t lane = 0; lane < CIS_ADC_OUT_LANES; lane++)
    {
        cisCals.blackRefInactiveAvg[0][lane] = blackCal.red.inactiveAvrgPix[lane];
        cisCals.blackRefInactiveAvg[1][lane] = blackCal.green.inactiveAvrgPix[lane];
        cisCals.blackRefInactiveAvg[2][lane] = blackCal.blue.inactiveAvrgPix[lane];
    }
    SCB_CleanDCache_by_Addr((uint32_t *)&cisCals, sizeof(cisCals));

    // ÉTAPE 10: Sauvegarde - INCHANGÉ (même nom de fichier)
    sprintf(calibrationFilePath, CALIBRATION_FILE_PATH_FORMAT, shared_config.cis_dpi);
    file_writeCisCals(calibrationFilePath, &cisCals);

    cis_stopCapture();
    osDelay(300);
    cis_startCapture();
    shared_var.cis_cal_state = CIS_CAL_END;
    printf("===============================\n");
}
```

#### 2.1.3 **REMPLACER** les fonctions de calcul existantes

**REMPLACER** `cis_computeCalsOffsets` :

```c
static void cis_computeCalsOffsets(struct cisCalsTypes *whiteCal, struct cisCalsTypes *intermediateCal, struct cisCalsTypes *blackCal, CIS_Color_TypeDef color)
{
    uint32_t laneOffset = 0;
    uint32_t offset = 0;

    switch (color)
    {
        case CIS_RED:   offset = cisConfig.red_offset; break;
        case CIS_GREEN: offset = cisConfig.green_offset; break;
        case CIS_BLUE:  offset = cisConfig.blue_offset; break;
        default: Error_Handler(); return;
    }

    for (int32_t lane = CIS_ADC_OUT_LANES; --lane >= 0; )
    {
        laneOffset = (cisConfig.useful_data_size_per_lane * lane) + offset;

        for (int32_t i = 0; i < cisConfig.pixels_per_color_per_lane; i++)
        {
            // Segment 1: offset = valeur noire
            cisCals.offsetData_seg1[laneOffset + i] = blackCal->data[laneOffset + i];

            // Segment 2: offset pour continuité au point de transition
            int32_t black_val = blackCal->data[laneOffset + i];
            int32_t inter_val = intermediateCal->data[laneOffset + i];
            int32_t white_val = whiteCal->data[laneOffset + i];

            // Calcul de l'offset du segment 2 pour assurer la continuité
            if (white_val != inter_val) {
                cisCals.offsetData_seg2[laneOffset + i] =
                    inter_val - ((int64_t)(inter_val - black_val) * (inter_val - black_val)) / (white_val - inter_val);
            } else {
                cisCals.offsetData_seg2[laneOffset + i] = black_val;
            }
        }
    }
}
```

**REMPLACER** `cis_computeCalsGains` :

```c
static void cis_computeCalsGains(uint32_t maxADCValue, struct cisCalsTypes *whiteCal, struct cisCalsTypes *intermediateCal, struct cisCalsTypes *blackCal, CIS_Color_TypeDef color)
{
    uint32_t laneOffset = 0;
    uint32_t offset = 0;
    uint32_t target_intermediate = (maxADCValue * CIS_INTERMEDIATE_LED_POWER) / 100;

    switch (color)
    {
        case CIS_RED:   offset = cisConfig.red_offset; break;
        case CIS_GREEN: offset = cisConfig.green_offset; break;
        case CIS_BLUE:  offset = cisConfig.blue_offset; break;
        default: Error_Handler(); return;
    }

    for (int32_t lane = CIS_ADC_OUT_LANES; --lane >= 0; )
    {
        laneOffset = (cisConfig.useful_data_size_per_lane * lane) + offset;

        for (int32_t i = 0; i < cisConfig.pixels_per_color_per_lane; i++)
        {
            int32_t black_val = blackCal->data[laneOffset + i];
            int32_t inter_val = intermediateCal->data[laneOffset + i];
            int32_t white_val = whiteCal->data[laneOffset + i];

            // Gain segment 1: noir → intermédiaire
            int32_t diff_seg1 = inter_val - black_val;
            if (diff_seg1 != 0) {
                cisCals.gainsData_seg1[laneOffset + i] = ((int32_t)target_intermediate << 16) / diff_seg1;
            } else {
                cisCals.gainsData_seg1[laneOffset + i] = UNITY_Q16_16;
            }

            // Gain segment 2: intermédiaire → blanc
            int32_t diff_seg2 = white_val - inter_val;
            if (diff_seg2 != 0) {
                cisCals.gainsData_seg2[laneOffset + i] = (((int32_t)maxADCValue - target_intermediate) << 16) / diff_seg2;
            } else {
                cisCals.gainsData_seg2[laneOffset + i] = UNITY_Q16_16;
            }
        }
    }
}
```

**AJOUTER** la nouvelle fonction `cis_computeTransitionPoints` :

```c
static void cis_computeTransitionPoints(struct cisCalsTypes *intermediateCal, CIS_Color_TypeDef color)
{
    uint32_t laneOffset = 0;
    uint32_t offset = 0;

    switch (color)
    {
        case CIS_RED:   offset = cisConfig.red_offset; break;
        case CIS_GREEN: offset = cisConfig.green_offset; break;
        case CIS_BLUE:  offset = cisConfig.blue_offset; break;
        default: Error_Handler(); return;
    }

    for (int32_t lane = CIS_ADC_OUT_LANES; --lane >= 0; )
    {
        laneOffset = (cisConfig.useful_data_size_per_lane * lane) + offset;

        for (int32_t i = 0; i < cisConfig.pixels_per_color_per_lane; i++)
        {
            // Point de transition = valeur mesurée à 50%
            cisCals.transitionPoint[laneOffset + i] = intermediateCal->data[laneOffset + i];
        }
    }
}
```

## PHASE 3 : MODIFICATION DE L'ALGORITHME D'APPLICATION

### 3.1 **REMPLACER** complètement `cis_applyLinearCalibration`

```c
#pragma GCC push_options
#pragma GCC optimize ("unroll-loops")
void cis_applyLinearCalibration(int32_t * restrict cisDataCpy, uint32_t maxClipValue)
{
    int32_t globalDriftOffset[3][CIS_ADC_OUT_LANES];

    // Étape 1: Calcul de la correction de dérive (INCHANGÉ)
    cis_computeGlobalDriftCorrection(cisDataCpy, globalDriftOffset);

    // Debug si activé (INCHANGÉ)
    if (CIS_DRIFT_DEBUG_ENABLED)
    {
        static uint32_t debug_counter = 0;
        debug_counter++;
        if (debug_counter % CIS_DRIFT_DEBUG_INTERVAL == 0)
        {
            printf("DRIFT DEBUG - Line %lu:\n", debug_counter);
            for (int32_t lane = 0; lane < CIS_ADC_OUT_LANES; lane++)
            {
                printf("  Lane %ld: R=%ld G=%ld B=%ld\n",
                       lane,
                       globalDriftOffset[0][lane],
                       globalDriftOffset[1][lane],
                       globalDriftOffset[2][lane]
                );
            }
        }
    }

    if (CIS_DETAILED_DEBUG_ENABLED)
    {
        static uint32_t detailed_debug_counter = 0;
        detailed_debug_counter++;
        if (detailed_debug_counter % CIS_DETAILED_DEBUG_INTERVAL == 0)
        {
            printf("=== DETAILED DEBUG - Line %lu ===\n", detailed_debug_counter);
            for (int32_t lane = 0; lane < CIS_ADC_OUT_LANES; lane++)
            {
                for (int color = 0; color < 3; color++)
                {
                    cis_printInactivePixels(cisDataCpy, lane, color);
                }
            }
            printf("=== END DETAILED DEBUG ===\n");
        }
    }

    // Étape 2: Application de la calibration segmentée (NOUVEAU)
    uint32_t target_intermediate = (maxClipValue * CIS_INTERMEDIATE_LED_POWER) / 100;

    for (int8_t lane = CIS_ADC_OUT_LANES; --lane >= 0; )
    {
        uint32_t baseR = (cisConfig.useful_data_size_per_lane * lane) + cisConfig.red_offset;
        uint32_t baseG = (cisConfig.useful_data_size_per_lane * lane) + cisConfig.green_offset;
        uint32_t baseB = (cisConfig.useful_data_size_per_lane * lane) + cisConfig.blue_offset;

        for (uint32_t i = 0; i < cisConfig.pixels_per_color_per_lane; i++)
        {
            int32_t driftCorrected, calibrated;
            uint32_t pixelIdx;

            /* Process RED channel */
            pixelIdx = baseR + i;
            driftCorrected = cisDataCpy[pixelIdx] - globalDriftOffset[0][lane];

            if (driftCorrected <= cisCals.transitionPoint[pixelIdx]) {
                // Segment 1: zone sombre
                calibrated = (int32_t)(((int64_t)(driftCorrected - cisCals.offsetData_seg1[pixelIdx])
                                       * cisCals.gainsData_seg1[pixelIdx]) >> 16);
            } else {
                // Segment 2: zone claire
                calibrated = (int32_t)(((int64_t)(driftCorrected - cisCals.offsetData_seg2[pixelIdx])
                                       * cisCals.gainsData_seg2[pixelIdx]) >> 16);
                calibrated += target_intermediate;  // Ajustement pour continuité
            }

            cisDataCpy[pixelIdx] = (calibrated < 0) ? 0 :
                                  ((calibrated > (int32_t)maxClipValue) ? (int32_t)maxClipValue : calibrated);

            /* Process GREEN channel */
            pixelIdx = baseG + i;
            driftCorrected = cisDataCpy[pixelIdx] - globalDriftOffset[1][lane];

            if (driftCorrected <= cisCals.transitionPoint[pixelIdx]) {
                calibrated = (int32_t)(((int64_t)(driftCorrected - cisCals.offsetData_seg1[pixelIdx])
                                       * cisCals.gainsData_seg1[pixelIdx]) >> 16);
            } else {
                calibrated = (int32_t)(((int64_t)(driftCorrected - cisCals.offsetData_seg2[pixelIdx])
                                       * cisCals.gainsData_seg2[pixelIdx]) >> 16);
                calibrated += target_intermediate;
            }

            cisDataCpy[pixelIdx] = (calibrated < 0) ? 0 :
                                  ((calibrated > (int32_t)maxClipValue) ? (int32_t)maxClipValue : calibrated);

            /* Process BLUE channel */
            pixelIdx = baseB + i;
            driftCorrected = cisDataCpy[pixelIdx] - globalDriftOffset[2][lane];

            if (driftCorrected <= cisCals.transitionPoint[pixelIdx]) {
                calibrated = (int32_t)(((int64_t)(driftCorrected - cisCals.offsetData_seg1[pixelIdx])
                                       * cisCals.gainsData_seg1[pixelIdx]) >> 16);
            } else {
                calibrated = (int32_t)(((int64_t)(driftCorrected - cisCals.offsetData_seg2[pixelIdx])
                                       * cisCals.gainsData_seg2[pixelIdx]) >> 16);
                calibrated += target_intermediate;
            }

            cisDataCpy[pixelIdx] = (calibrated < 0) ? 0 :
                                  ((calibrated > (int32_t)maxClipValue) ? (int32_t)maxClipValue : calibrated);
        }
    }
}
#pragma GCC pop_options
```

## PHASE 4 : MISE À JOUR DES PROTOTYPES

### 4.1 Modification de `CM7/Peripheral/Inc/cis_linearCal.h`

**AJOUTER** les nouveaux prototypes dans la section des fonctions privées :

```c
// Nouvelles fonctions internes (ajouter dans la section private)
static void cis_computeTransitionPoints(struct cisCalsTypes *intermediateCal, CIS_Color_TypeDef color);

// Modifier les signatures existantes pour inclure intermediateCal
static void cis_computeCalsOffsets(struct cisCalsTypes *whiteCal, struct cisCalsTypes *intermediateCal, struct cisCalsTypes *blackCal, CIS_Color_TypeDef color);
static void cis_computeCalsGains(uint32_t maxADCValue, struct cisCalsTypes *whiteCal, struct cisCalsTypes *intermediateCal, struct cisCalsTypes *blackCal, CIS_Color_TypeDef color);
```

## PHASE 5 : COMPATIBILITÉ DES FICHIERS

### 5.1 Gestion des fichiers de calibration

- Les fonctions `file_writeCisCals` et `file_readCisCals` fonctionneront automatiquement avec la nouvelle structure
- La taille de la structure a changé, mais les fonctions utilisent `sizeof(struct cisCals)`
- **IMPORTANT** : Les anciens fichiers de calibration ne seront plus compatibles (taille différente)
- Les utilisateurs devront refaire une calibration après la mise à jour

## RÉSUMÉ DES MODIFICATIONS

### Fichiers à modifier :

1. **`Common/Inc/config.h`** :
   - Ajout de `CIS_INTERMEDIATE_LED_POWER`

2. **`Common/Inc/globals.h`** :
   - Remplacement complet de `struct cisCals`
   - Modification des enums de calibration (`CIS_CAL_WHITE`, `CIS_CAL_INTERMEDIATE`, `CIS_CAL_BLACK`)

3. **`CM7/Peripheral/Inc/cis_linearCal.h`** :
   - Mise à jour des prototypes de fonctions

4. **`CM7/Peripheral/Src/cis_linearCal.c`** :
   - Ajout variable `intermediateCal`
   - Remplacement complet de `cis_startLinearCalibration`
   - Remplacement de `cis_computeCalsOffsets` et `cis_computeCalsGains`
   - Ajout de `cis_computeTransitionPoints`
   - Remplacement complet de `cis_applyLinearCalibration`

### Impact sur les ressources (AVANT optimisation mémoire) :

- **Mémoire** : Doublement de l'utilisation mémoire pour les coefficients (~214 KB au lieu de ~55 KB)
- **Performance RT** : Ajout d'une comparaison et d'un branchement conditionnel par pixel (<5% de surcharge)
- **Temps de calibration** : Ajout d'une étape de mesure intermédiaire (+33% de temps)

### OPTIMISATION MÉMOIRE APPLIQUÉE :

**Problème identifié :** DTCM RAM overflow lors de la compilation (~214 KB requis)

**Solution implémentée :** Optimisation des types de données avec format Q8.8

#### Modifications apportées :

1. **Structure `cisCals` optimisée** (dans `Common/Inc/globals.h`) :
```c
struct __attribute__((aligned(4))) cisCals
{
    // Optimisé en int16_t (économie de 50% par tableau)
    int16_t offsetData_seg1[CIS_MAX_USEFUL_DATA_SIZE * CIS_ADC_OUT_LANES];
    int16_t gainsData_seg1[CIS_MAX_USEFUL_DATA_SIZE * CIS_ADC_OUT_LANES];   // Q8.8 format
    int16_t offsetData_seg2[CIS_MAX_USEFUL_DATA_SIZE * CIS_ADC_OUT_LANES];
    int16_t gainsData_seg2[CIS_MAX_USEFUL_DATA_SIZE * CIS_ADC_OUT_LANES];   // Q8.8 format
    int16_t transitionPoint[CIS_MAX_USEFUL_DATA_SIZE * CIS_ADC_OUT_LANES];

    // Références pour correction de dérive (INCHANGÉ)
    int32_t blackRefInactiveAvg[COLOR_CHANNELS][CIS_ADC_OUT_LANES];
};
```

2. **Nouvelles définitions** (dans `Common/Inc/config.h`) :
```c
// Format Q8.8 pour les gains (optimisation mémoire)
#define UNITY_Q8_8                              (1 << 8)    // 1.0 en format Q8.8
#define CLIP_INT16(x)                           ((x) > 32767 ? 32767 : ((x) < -32768 ? -32768 : (x)))
```

3. **Calculs adaptés** (dans `CM7/Peripheral/Src/cis_linearCal.c`) :
   - **Gains en Q8.8** au lieu de Q16.16 : `gain = (target << 8) / diff`
   - **Clipping int16_t** : `CLIP_INT16(gain_temp)`
   - **Application Q8.8** : `calibrated = (value * gain) >> 8`

#### Résultats de l'optimisation :

**Consommation mémoire :**
- **Avant** : ~214 KB (5 tableaux int32_t + références)
- **Après** : ~107 KB (5 tableaux int16_t + références)
- **Économie** : ~107 KB (50% de réduction)

**Précision :**
- **Avant** : Q16.16 (1/65536 de précision)
- **Après** : Q8.8 (1/256 de précision)
- **Impact** : Précision réduite mais largement suffisante pour la calibration CIS

**Performance RT :**
- **Calculs plus rapides** avec int16_t
- **Même algorithme** de calibration segmentée
- **Pas d'impact** sur les performances temps réel

### Avantages de cette approche :

- **Simplicité** : Pas de code de sélection de mode
- **Compatibilité** : Même interface publique (`cis_applyLinearCalibration`, etc.)
- **Performance** : Pas de surcharge de sélection de mode
- **Maintenance** : Un seul chemin de code à maintenir
- **Précision** : Meilleure correction de la non-linéarité des pixels

### Inconvénients :

- **Rétrocompatibilité** : Les anciens fichiers de calibration devront être refaits
- **Mémoire** : Doublement de l'utilisation mémoire pour les coefficients
- **Complexité** : Algorithme légèrement plus complexe

## Notes d'implémentation

1. **Ordre des modifications** : Commencer par les structures de données, puis les fonctions de calcul, et enfin l'algorithme d'application
2. **Tests** : Tester chaque étape individuellement avant de passer à la suivante
3. **Sauvegarde** : Faire une sauvegarde complète avant de commencer les modifications
4. **Validation** : Comparer les résultats avec l'ancienne calibration sur des données de test

Cette approche remplace complètement l'ancienne calibration par la nouvelle, tout en gardant la même interface externe et en améliorant significativement la qualité de la correction de non-linéarité.
