# Analyse de faisabilité - Optimisation mémoire CIS MDMA

## Situation actuelle

### Configuration des buffers
```c
#define CIS_SP_WIDTH              (2)
#define CIS_BLACK_LINE            (38)
#define CIS_OVER_SCAN            (12)
#define CIS_400DPI_PIXELS_PER_LANE (1152)
#define CIS_ADC_OUT_LANES         (3)

// Tailles actuelles
#define CIS_MAX_PIXEL_AERA_STOP   ((CIS_INACTIVE_WIDTH) + (CIS_MAX_PIXELS_PER_LANE))
#define CIS_MAX_LANE_SIZE         (CIS_MAX_PIXEL_AERA_STOP + CIS_OVER_SCAN)
#define CIS_MAX_ADC_BUFF_SIZE     ((CIS_MAX_LANE_SIZE) * (CIS_ADC_OUT_LANES))
```

**Calculs actuels :**
- `CIS_INACTIVE_WIDTH = 40` (CIS_BLACK_LINE + CIS_SP_WIDTH)
- `CIS_MAX_PIXEL_AERA_STOP = 1192` (40 + 1152)  
- `CIS_MAX_LANE_SIZE = 1204` (1192 + 12)
- `CIS_MAX_ADC_BUFF_SIZE = 3612` (1204 × 3)

### Problème identifié
- Overflow DTCM de 256 bytes lors du passage de overscan 12 à 24
- Les buffers `cisData_ADC1/2/3` et `cisDataCpy` consomment trop de RAM
- Les copies MDMA intégrales incluent des données inutiles (SP_WIDTH + OVER_SCAN)

## Proposition d'optimisation

### 1. Réduction de la taille des buffers

**Nouvelle taille proposée :**
```c
#define CIS_OPTIMIZED_BUFF_SIZE (((CIS_400DPI_PIXELS_PER_LANE + CIS_BLACK_LINE) * (CIS_ADC_OUT_LANES)))
// = (1152 + 38) × 3 = 3570 éléments
```

**Économie de mémoire :**
- Actuel : 3612 × 2 bytes = 7224 bytes par buffer
- Optimisé : 3570 × 2 bytes = 7140 bytes par buffer  
- **Économie par buffer : 84 bytes**
- **Économie totale : 84 × 4 buffers = 336 bytes** (cisData_ADC1/2/3 + cisDataCpy)

### 2. Copies MDMA sélectives avec Block Address Offset

**Faisabilité technique :** ✅ **POSSIBLE**

Le MDMA STM32H7 supporte les transferts avec offset de bloc via les paramètres :
- `SourceBlockAddressOffset` : offset source entre blocs
- `DestBlockAddressOffset` : offset destination entre blocs

**Configuration MDMA optimisée :**
```c
typedef struct {
    uint32_t SrcAddress;          // Adresse source initiale
    uint32_t DstAddress;          // Adresse destination initiale  
    uint32_t BlockDataLength;     // Taille d'un bloc (zone utile)
    uint32_t BlockCount;          // Nombre de blocs (9 dans votre cas)
    int32_t SourceBlockAddressOffset;  // Saut source = CIS_MAX_LANE_SIZE
    int32_t DestBlockAddressOffset;    // Saut destination = zone utile
} MDMA_OptimizedConfig;
```

### 3. Implémentation proposée

**Paramètres pour copies sélectives :**
```c
// Configuration pour chaque canal ADC
MDMA_LinkNodeConfTypeDef nodeConfig = {
    .SrcAddress = (uint32_t)&cisData_ADCx[CIS_SP_WIDTH],  // Skip SP_WIDTH
    .DstAddress = (uint32_t)&cisDataCpy[offset],
    .BlockDataLength = (CIS_BLACK_LINE + CIS_400DPI_PIXELS_PER_LANE) * sizeof(uint16_t),
    .BlockCount = 3,  // 3 couleurs par ADC
    .Init.SourceBlockAddressOffset = CIS_MAX_LANE_SIZE * sizeof(uint16_t),
    .Init.DestBlockAddressOffset = (CIS_BLACK_LINE + CIS_400DPI_PIXELS_PER_LANE) * sizeof(uint16_t)
};
```

**Structure des données optimisée :**
```
Source (ADC) :  [SP][BLACK_LINE][PIXELS][OVERSCAN] -> [SP][BLACK_LINE][PIXELS][OVERSCAN] -> [SP][BLACK_LINE][PIXELS][OVERSCAN]
                     ↓ Copy only  ↓ Copy only                 ↓ Copy only  ↓ Copy only                 ↓ Copy only  ↓ Copy only
Destination :        [BLACK_LINE][PIXELS]         ->         [BLACK_LINE][PIXELS]         ->         [BLACK_LINE][PIXELS]
```

## Avantages de cette approche

### ✅ Réduction mémoire
- **336 bytes économisés** sur les buffers principaux
- Suppression de la copie des zones inutiles (SP_WIDTH + OVERSCAN)
- **Réduction proportionnelle** de l'impact de l'augmentation d'overscan

### ✅ Performance maintenue
- Les copies MDMA restent efficaces avec les offsets matériels
- Pas de copie CPU supplémentaire nécessaire  
- Pas de modification de la logique de traitement d'image

### ✅ Compatibilité
- Les offsets dans `cis_imageProcess()` restent valides
- Les structures de données existantes sont préservées
- Modification minimale du code existant

## Risques et limitations

### ⚠️ Complexité MDMA
- Configuration plus complexe avec les linked lists  
- Débogage plus difficile en cas de problème
- Besoin de validation approfondie des offsets

### ⚠️ Taille fixe
- L'optimisation est spécifique au mode 400DPI
- Adaptation nécessaire pour le mode 200DPI

## Recommandation

**FAISABILITÉ : ✅ HAUTE**

L'optimisation est techniquement faisable et apportera l'économie mémoire nécessaire. Je recommande :

1. **Phase 1** : Implémentation avec conservation des buffers actuels pour validation
2. **Phase 2** : Réduction effective des tailles de buffers après validation  
3. **Phase 3** : Extension au mode 200DPI si nécessaire

L'économie de 336 bytes, bien que modeste, devrait suffire à résoudre l'overflow DTCM de 256 bytes mentionné.
