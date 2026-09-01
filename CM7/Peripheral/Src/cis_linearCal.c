/**
 ******************************************************************************
 * @file           : cis_linearCal.c
 ******************************************************************************
 * @attention
 *
 * Copyright (C) 2018-present Reso-nance Numerique.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 *
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "config.h"
#include "basetypes.h"
#include "globals.h"

#include "stdlib.h"
#include "stdio.h"
#include "stdbool.h"

#include "arm_math.h"

#include "file_manager.h"
#include "cis.h"

#include "cis_linearCal.h"

/* Private includes ----------------------------------------------------------*/

/* Private typedef -----------------------------------------------------------*/
__attribute__ ((packed))
struct cisColorsParams {
    int32_t inactiveAvrgPix[3];
};

/* Toutes les grandeurs d'une struct cisCalsTypes (data, extremums, moyennes inactives)
   sont en virgule fixe Q<CIS_CAL_FRAC_BITS> de comptages ADC, telles que produites par
   cis_imageProcessRGB_Calibration. La reconversion en entiers a lieu au seul moment où
   les valeurs entrent dans cisCals, que le runtime lit en comptages ADC. */
__attribute__ ((packed))
struct cisCalsTypes {
	uint32_t data[CIS_MAX_USEFUL_DATA_SIZE * CIS_ADC_OUT_LANES];
	struct cisColorsParams red;
	struct cisColorsParams green;
	struct cisColorsParams blue;
};

/* Private define ------------------------------------------------------------*/
#define UNITY_Q16_16   (1 << 16)  // Gardé pour compatibilité legacy

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
__attribute__((section(".calAccBlack")))
struct cisCalsTypes blackCal;

__attribute__((section(".calAccWhite")))
struct cisCalsTypes whiteCal;

__attribute__((section(".calAccIntermediate")))
struct cisCalsTypes intermediateCal;

/* Debug variables - now using config.h defines */
// Debug configuration moved to Common/Inc/config.h

/* Variable containing ADC conversions data */

/* Private function prototypes -----------------------------------------------*/
static void cis_mean(const uint32_t * pSrc, uint32_t blockSize, int32_t * pResult);
static void cis_ComputeCalsInactivesAvrg(struct cisCalsTypes *currCals, CIS_Color_TypeDef color);
static void cis_computeAffine(struct cisCalsTypes *whiteCal, struct cisCalsTypes *blackCal, CIS_Color_TypeDef color);
static void cis_measureLevel(struct cisCalsTypes *levelCal, uint32_t level);
static void cis_buildCurves(uint32_t maxOut);

/* Rapports cycliques LED des points de calibration. Le premier est 0 (obscurite) et le
   dernier 100 (blanc) : ce sont les deux ancres de la normalisation affine. */
static const uint8_t cisCalLevels[CIS_CAL_LEVEL_COUNT] = CIS_CAL_LEVELS;

/* Position normalisee moyenne de chaque niveau, par couleur et par voie. C'est TOUT ce
   qu'on retient d'un niveau intermediaire : 9 scalaires au lieu des 43 Ko d'une capture
   pixel a pixel. C'est ce qui rend le nombre de niveaux gratuit en memoire. */
static int32_t cisLevelNorm[CIS_CAL_LEVEL_COUNT][COLOR_CHANNELS][CIS_ADC_OUT_LANES];

/* Etat de l'IIR de derive (Q3), au niveau fichier : cis_refreshDarkReferences le
   re-amorce apres avoir rebase offsets et references noires. */
static int32_t driftState[3][CIS_ADC_OUT_LANES];
static bool driftPrimed = false;

#if CIS_LINE_LOG_ENABLED
/* Non statiques : leurs adresses se lisent dans la map pour le dump SWD. */
struct cisLineLogEntry
{
    int16_t noir[3][CIS_ADC_OUT_LANES];   /* moyenne fenetre noire BRUTE   [couleur][voie] */
    int16_t act[3][CIS_ADC_OUT_LANES];    /* moyenne sortie calibree en Q4 [couleur][voie] */
    int16_t rawact[3][CIS_ADC_OUT_LANES]; /* moyenne actifs BRUTE, avant toute correction */
};
static int16_t cisLineLogRawAct[3][CIS_ADC_OUT_LANES];
volatile uint32_t cisLineLogHead;
struct cisLineLogEntry cisLineLog[CIS_LINE_LOG_N];
#endif

/* Private user code ---------------------------------------------------------*/

void cis_mean(const uint32_t * pSrc, uint32_t blockSize, int32_t * pResult)
{
    int64_t sum = 0;

    for (uint32_t i = 0; i < blockSize; i++)
    {
        sum += pSrc[i];
    }

    /* Attention : blockSize ne doit pas être zéro */
    *pResult = (int32_t)(sum / blockSize);
}

/**
 * @brief       Print the values of inactive pixels for a specific lane and color.
 * @param       cisDataCpy    Pointer to the current image data.
 * @param       lane          Lane number (0, 1, 2...).
 * @param       color         Color channel (0=Red, 1=Green, 2=Blue).
 * @retval      None
 *
 * Prints all 38 inactive pixel values for debugging purposes.
 */
void cis_printInactivePixels(const int32_t * restrict cisDataCpy, uint32_t lane, int color)
{
    const char* colorNames[] = {"RED", "GREEN", "BLUE"};
    int32_t laneOffset = 0;
    int32_t colorOffsets[3] = {
        cisConfig.red_offset - CIS_BLACK_PIXELS,
        cisConfig.green_offset - CIS_BLACK_PIXELS,
        cisConfig.blue_offset - CIS_BLACK_PIXELS
    };

    if (color < 0 || color > 2 || lane >= CIS_ADC_OUT_LANES)
    {
        printf("ERROR: Invalid color (%d) or lane (%lu)\n", color, lane);
        return;
    }

    laneOffset = (cisConfig.useful_data_size_per_lane * lane) + colorOffsets[color];

    printf("INACTIVE PIXELS - Lane %lu %s (38 pixels, using pixels 9-32 for average):\n", lane, colorNames[color]);
    printf("  ");
    for (uint32_t i = 0; i < CIS_BLACK_PIXELS; i++)
    {
        // Highlight the pixels used for drift correction (9-32)
        if (i == CIS_IGNORE_FIRST_BLACK_PIXELS)
        {
            printf("[");
        }
        printf("%ld ", cisDataCpy[laneOffset + i]);
        if (i == CIS_IGNORE_FIRST_BLACK_PIXELS + CIS_USEFUL_BLACK_PIXELS - 1)
        {
            printf("] ");
        }
        if ((i + 1) % 10 == 0) // New line every 10 values
        {
            printf("\n  ");
        }
    }
    printf("\n");

    // Calculate and print averages
    int32_t average_all, average_useful;
    cis_mean((const uint32_t *)&cisDataCpy[laneOffset], CIS_BLACK_PIXELS, &average_all);
    cis_mean((const uint32_t *)&cisDataCpy[laneOffset + CIS_IGNORE_FIRST_BLACK_PIXELS], CIS_USEFUL_BLACK_PIXELS, &average_useful);
    printf("  Average (all 38): %ld\n", average_all);
    printf("  Average (pixels 9-32): %ld (used for drift correction)\n", average_useful);
}

/**
 * @brief       Compute global drift correction offsets based on current inactive pixels.
 * @param       cisDataCpy              Pointer to the current image data.
 * @param       globalDriftOffset       Output array for drift offsets [color][lane].
 * @retval      None
 *
 * This function measures the current inactive pixel averages and compares them
 * with the calibration references to compute drift correction offsets.
 */
void cis_computeGlobalDriftCorrection(const int32_t * restrict cisDataCpy, int32_t globalDriftOffset[3][CIS_ADC_OUT_LANES])
{
    int32_t currentInactiveAvg[3][CIS_ADC_OUT_LANES];  // [color][lane]
    int32_t laneOffset = 0;

    // Color offsets for inactive pixel regions
    int32_t colorOffsets[3] = {
        cisConfig.red_offset - CIS_BLACK_PIXELS,
        cisConfig.green_offset - CIS_BLACK_PIXELS,
        cisConfig.blue_offset - CIS_BLACK_PIXELS
    };

    // Compute current inactive pixel averages for each color and lane
    for (int32_t color = 0; color < 3; color++)
    {
        for (int32_t lane = 0; lane < CIS_ADC_OUT_LANES; lane++)
        {
            laneOffset = (cisConfig.useful_data_size_per_lane * lane) + colorOffsets[color] + CIS_IGNORE_FIRST_BLACK_PIXELS;
            cis_mean((const uint32_t *)&cisDataCpy[laneOffset], CIS_USEFUL_BLACK_PIXELS, &currentInactiveAvg[color][lane]);
        }
    }

    // Compute drift correction offsets using saved reference averages
    for (int32_t lane = 0; lane < CIS_ADC_OUT_LANES; lane++)
    {
        for (int32_t color = 0; color < 3; color++)
        {
            // Drift offset = current_inactive_avg - saved_black_reference
            globalDriftOffset[color][lane] = currentInactiveAvg[color][lane] - cisCals.blackRefInactiveAvg[color][lane];

            // Apply threshold limiting to prevent excessive corrections
            if (CIS_DRIFT_THRESHOLD > 0)
            {
                if (globalDriftOffset[color][lane] > CIS_DRIFT_THRESHOLD)
                {
                    globalDriftOffset[color][lane] = CIS_DRIFT_THRESHOLD;
                }
                else if (globalDriftOffset[color][lane] < -CIS_DRIFT_THRESHOLD)
                {
                    globalDriftOffset[color][lane] = -CIS_DRIFT_THRESHOLD;
                }
            }
        }
    }
}

/**
 * @brief       Apply linear calibration with global drift correction on the image buffer.
 *
 * The applied formula is:
 *      1. Global drift correction: drift_corrected = raw - global_drift_offset
 *      2. Individual calibration: calibrated = clip( ((drift_corrected - offset) * gain) >> 16, 0, maxClipValue )
 *
 * @param       cisDataCpy    Pointer to the image buffer (int32_t).
 * @param       maxClipValue  Clipping value (e.g., 255).
 * @retval      None
 */
#pragma GCC push_options
#pragma GCC optimize ("unroll-loops")
void cis_applyLinearCalibration(int32_t * restrict cisDataCpy, uint32_t maxClipValue)
{
    int32_t globalDriftOffset[3][CIS_ADC_OUT_LANES];  // [color][lane]

    // Step 1: Compute global drift correction offsets (always enabled)
    cis_computeGlobalDriftCorrection(cisDataCpy, globalDriftOffset);

    /* Lissage temporel (IIR 1/8) : la derive corrigee est THERMIQUE, elle evolue en
       secondes. Appliquee brute, la moyenne de 24 echantillons injecte son bruit ligne
       a ligne dans TOUTE la ligne -- c'est du banding fabrique par la correction
       elle-meme. Etat en Q3 pour ne pas perdre la resolution sous l'IIR. */
    {
        for (int32_t c = 0; c < 3; c++)
        {
            for (int32_t l = 0; l < CIS_ADC_OUT_LANES; l++)
            {
                const int32_t target = globalDriftOffset[c][l] << 3;
                if (!driftPrimed)
                {
                    driftState[c][l] = target;
                }
                else
                {
                    driftState[c][l] += (target - driftState[c][l]) >> CIS_DRIFT_IIR_SHIFT;
                }
                globalDriftOffset[c][l] = driftState[c][l] >> 3;
            }
        }
        driftPrimed = true;
    }

    // DEBUG: Print drift correction values if enabled
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
                       globalDriftOffset[0][lane], // Red
                       globalDriftOffset[1][lane], // Green
                       globalDriftOffset[2][lane]  // Blue
                );
            }
        }
    }

    // DETAILED DEBUG: Print all 38 inactive pixel values if enabled
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

    /* Step 2 : normalisation affine par pixel, puis courbe partagée.
     *
     * La boucle est SANS BRANCHE. L'ancien schéma testait, pour chaque canal de chaque
     * pixel, de quel côté du coude on se trouvait : 10 368 branches par ligne dont
     * l'issue suit les données de l'image, donc largement imprévisibles. Sur Cortex-M7
     * chaque erreur de prédiction coûte une dizaine de cycles. __USAT sature en une
     * instruction et remplace du même coup les deux comparaisons de l'écrêtage, et
     * l'écrêtage à 255 est désormais porté par la courbe elle-même.
     *
     * Bornes : l'entrée est un ADC 10 bits corrigé de la dérive (bornée à
     * CIS_DRIFT_THRESHOLD) puis de l'offset, donc |x| <= 1123 ; les gains sont des
     * int16 <= 32767 ; le produit plafonne à 3,7e7, très loin de 2^31.
     */
    (void)maxClipValue;  /* la sortie est bornée par construction de la courbe */

#if CIS_LINE_LOG_ENABLED
    /* Actifs BRUTS de cette ligne (la boucle pixel ci-dessous les reecrit en 0..255) :
       necessaires pour mesurer le couplage reel piedestal->photodiodes sans que la
       correction de derive ne s'entremele a la mesure. */
    {
        const uint32_t offs[3] = { (uint32_t)cisConfig.red_offset,
                                   (uint32_t)cisConfig.green_offset,
                                   (uint32_t)cisConfig.blue_offset };
        for (int32_t c = 0; c < 3; c++)
        {
            for (int32_t l = 0; l < CIS_ADC_OUT_LANES; l++)
            {
                const int32_t *pa = &cisDataCpy[(cisConfig.useful_data_size_per_lane * l) + offs[c]];
                int32_t sa = 0;
                for (int32_t k = 0; k < cisConfig.pixels_per_color_per_lane; k++) sa += pa[k];
                cisLineLogRawAct[c][l] = (int16_t)(sa / cisConfig.pixels_per_color_per_lane);
            }
        }
    }
#endif

    /* Les DONNEES sont a des positions tournantes (l'assignation ligne->couleur du
       capteur change a la mise sous tension, mesuree par l'identification LED) ; les
       TABLEAUX de calibration sont canoniques : R, G, B dans cet ordre, toujours.
       Sans ce decouplage, le gain par pixel -- qui embarque la luminosite de SA LED --
       serait applique aux donnees d'une autre LED apres un cycle d'alimentation :
       ecart global ~10 % par ligne, changeant a chaque boot (vecu). */
    const uint32_t stride = cisConfig.useful_data_size_per_color_per_lane;

    for (int8_t lane = CIS_ADC_OUT_LANES; --lane >= 0; )
    {
        const uint32_t baseR = (cisConfig.useful_data_size_per_lane * lane) + cisConfig.red_offset;
        const uint32_t baseG = (cisConfig.useful_data_size_per_lane * lane) + cisConfig.green_offset;
        const uint32_t baseB = (cisConfig.useful_data_size_per_lane * lane) + cisConfig.blue_offset;
        const uint32_t calR  = (cisConfig.useful_data_size_per_lane * lane) + 0 * stride + CIS_BLACK_PIXELS;
        const uint32_t calG  = (cisConfig.useful_data_size_per_lane * lane) + 1 * stride + CIS_BLACK_PIXELS;
        const uint32_t calB  = (cisConfig.useful_data_size_per_lane * lane) + 2 * stride + CIS_BLACK_PIXELS;

        /* Sorties de la boucle interne : une indirection de moins par pixel. */
        const uint8_t * restrict curveR = cisCals.curve[0][lane];
        const uint8_t * restrict curveG = cisCals.curve[1][lane];
        const uint8_t * restrict curveB = cisCals.curve[2][lane];

        const int32_t driftR = globalDriftOffset[0][lane];
        const int32_t driftG = globalDriftOffset[1][lane];
        const int32_t driftB = globalDriftOffset[2][lane];

        for (uint32_t i = 0; i < cisConfig.pixels_per_color_per_lane; i++)
        {
            uint32_t pixelIdx, calIdx;
            int32_t  v;

            /* Process RED channel */
            pixelIdx = baseR + i;
            calIdx = calR + i;
            v = cisDataCpy[pixelIdx] - driftR - cisCals.offsetData[calIdx];
            cisDataCpy[pixelIdx] = curveR[__USAT((v * cisCals.gainData[calIdx]) >> CIS_CAL_GAIN_SHIFT,
                                                CIS_CAL_CURVE_BITS)];

            /* Process GREEN channel */
            pixelIdx = baseG + i;
            calIdx = calG + i;
            v = cisDataCpy[pixelIdx] - driftG - cisCals.offsetData[calIdx];
            cisDataCpy[pixelIdx] = curveG[__USAT((v * cisCals.gainData[calIdx]) >> CIS_CAL_GAIN_SHIFT,
                                                CIS_CAL_CURVE_BITS)];

            /* Process BLUE channel */
            pixelIdx = baseB + i;
            calIdx = calB + i;
            v = cisDataCpy[pixelIdx] - driftB - cisCals.offsetData[calIdx];
            cisDataCpy[pixelIdx] = curveB[__USAT((v * cisCals.gainData[calIdx]) >> CIS_CAL_GAIN_SHIFT,
                                                CIS_CAL_CURVE_BITS)];
        }
    }

#if CIS_LINE_LOG_ENABLED
    /* Journal par ligne : la fenetre noire de cisDataCpy n'est PAS reecrite par la
       boucle ci-dessus (elle demarre aux offsets actifs), on y lit donc encore le brut
       de CETTE ligne ; les actifs, eux, sont desormais en sortie calibree 0..255. */
    {
        struct cisLineLogEntry *e = &cisLineLog[cisLineLogHead % CIS_LINE_LOG_N];
        const uint32_t offs[3] = { (uint32_t)cisConfig.red_offset,
                                   (uint32_t)cisConfig.green_offset,
                                   (uint32_t)cisConfig.blue_offset };
        for (int32_t c = 0; c < 3; c++)
        {
            for (int32_t lane = 0; lane < CIS_ADC_OUT_LANES; lane++)
            {
                const uint32_t base = (cisConfig.useful_data_size_per_lane * lane) + offs[c];
                const int32_t *pn = &cisDataCpy[base - CIS_BLACK_PIXELS + CIS_IGNORE_FIRST_BLACK_PIXELS];
                int32_t sn = 0;
                for (int32_t k = 0; k < CIS_USEFUL_BLACK_PIXELS; k++) sn += pn[k];
                e->noir[c][lane] = (int16_t)(sn / CIS_USEFUL_BLACK_PIXELS);

                const int32_t *pa = &cisDataCpy[base];
                int32_t sa = 0;
                for (int32_t k = 0; k < cisConfig.pixels_per_color_per_lane; k++) sa += pa[k];
                e->act[c][lane] = (int16_t)((sa * 16) / cisConfig.pixels_per_color_per_lane);
                e->rawact[c][lane] = cisLineLogRawAct[c][lane];
            }
        }
        cisLineLogHead++;
    }
#endif
}
#pragma GCC pop_options

/**
 * @brief       Initialize the linear calibration (integer version).
 *
 * Opens the calibration file based on the current DPI and loads the calibration
 * parameters. If the file is not found, a calibration is requested.
 *
 * @param       None
 * @retval      None
 */
CISCALIBRATION_StatusTypeDef cis_linearCalibrationInit(void)
{
    char calibrationFilePath[64];

    /* Build the calibration file path according to DPI */
    sprintf(calibrationFilePath, CALIBRATION_FILE_PATH_FORMAT, shared_config.cis_dpi);

    /* Le resultat de la lecture est desormais TESTE. Il ne l'etait pas, et le fichier
       etait donc charge tel quel meme quand il ne correspondait plus au format attendu :
       l'appareil aurait tourne sur une calibration aberrante au lieu d'en redemander une.
       Le fichier etait aussi ouvert deux fois, ici puis dans file_readCisCals. */
    if (file_readCisCals(calibrationFilePath, &cisCals) == FILEMANAGER_OK)
    {
        shared_var.cis_cal_state = CIS_CAL_END;
    }
    else
    {
        printf("No usable calibration for %d DPI, calibration requested.\n", shared_config.cis_dpi);
        shared_var.cis_cal_state = CIS_CAL_REQUESTED;
    }

    return CISCALIBRATION_OK;
}

/**
 * @brief       Start the linear calibration (integer version).
 *
 * Captures white and black calibration data, computes inactive averages,
 * extremums, offsets and gains, then saves the calibration data.
 *
 * @param       iterationNb   Number of iterations to average.
 * @param       bitDepth      Bit depth (used for gain calculation).
 * @retval      None
 */
void cis_startLinearCalibration(int32_t *cisDataCpy, uint16_t iterationNb, uint32_t bitDepth)
{
    /* iterationNb n'est plus un reglage unique : les ancres (blanc, noir) fixent gain
       et offset PAR PIXEL et meritent la moyenne longue, alors qu'un niveau
       intermediaire ne produit que 9 scalaires deja moyennes sur 1152 pixels par ligne.
       Voir CIS_CAL_ITER_ANCHOR et CIS_CAL_ITER_LEVEL. */
    (void)iterationNb;

    printf("===== CALIBRATION STARTED =====\n");
    printf("Calibration for %d DPI (%d levels, per-pixel affine + shared curve)\n",
           shared_config.cis_dpi, CIS_CAL_LEVEL_COUNT);

    char calibrationFilePath[64];

    memset(&blackCal, 0, sizeof(blackCal));
    memset(&intermediateCal, 0, sizeof(intermediateCal));
    memset(&whiteCal, 0, sizeof(whiteCal));
    memset(&cisCals, 0, sizeof(cisCals));
    memset(cisLevelNorm, 0, sizeof(cisLevelNorm));

    /* ---- Ancre haute : blanc a 100 %. Donne le GAIN par pixel. ----------------
       On attend un mouvement SOUTENU (500 ms continues) avant de capturer : la pose du
       capteur sur le papier produit des a-coups brefs, la glisse un mouvement continu.
       Une ancre blanche prise pendant le positionnement est ~6x trop sombre et fait
       exploser toute l'echelle normalisee (vecu). Timeout 30 s -> abandon propre. */
    cis_ledPowerAdj(100, 100, 100);
    shared_var.cis_cal_progressbar = 0;
    shared_var.cis_cal_state = CIS_CAL_WHITE;
    {
        uint32_t sustained = 0;
        const uint32_t t0 = HAL_GetTick();
        while (sustained < 500U)
        {
            osDelay(20);
            sustained = cis_isMoving() ? (sustained + 20U) : 0U;
            if ((HAL_GetTick() - t0) > 30000U)
            {
                printf("Calibration ABORTED: no sustained motion\n");
                goto abort;
            }
        }
    }

    if (cis_imageProcessRGB_Calibration(cisDataCpy, whiteCal.data, CIS_CAL_ITER_ANCHOR, 0, 100, true) != CIS_OK)
    {
        printf("Calibration ABORTED during the white capture, previous calibration kept\n");
        goto abort;
    }
    osDelay(200);

    /* ---- Ancre basse : LEDs eteintes. Donne l'OFFSET par pixel. ---------------
       Vraie trame d'obscurite : a 1 % les LEDs eclairaient encore et l'offset dependait
       de la reflectance du papier et du positionnement. Voir CIS_BLACK_LED_POWER. */
    shared_var.cis_cal_progressbar = 0;
    shared_var.cis_cal_state = CIS_CAL_BLACK;
    cis_ledPowerAdj(CIS_BLACK_LED_POWER, CIS_BLACK_LED_POWER, CIS_BLACK_LED_POWER);
    osDelay(200);

    if (cis_imageProcessRGB_Calibration(cisDataCpy, blackCal.data, CIS_CAL_ITER_ANCHOR, 0, 100, true) != CIS_OK)
    {
        printf("Calibration ABORTED during the black capture, previous calibration kept\n");
        goto abort;
    }
    osDelay(300);

    /* ---- Moyennes des pixels masques : reference de derive --------------------- */
    cis_ComputeCalsInactivesAvrg(&blackCal, CIS_RED);
    cis_ComputeCalsInactivesAvrg(&blackCal, CIS_GREEN);
    cis_ComputeCalsInactivesAvrg(&blackCal, CIS_BLUE);
    shared_var.cis_cal_state = CIS_CAL_EXTRACT_INNACTIVE_REF;
    osDelay(100);

    /* ---- Normalisation affine par pixel ---------------------------------------- */
    printf("Compute per-pixel offset and gain\n");
    cis_computeAffine(&whiteCal, &blackCal, CIS_RED);
    cis_computeAffine(&whiteCal, &blackCal, CIS_GREEN);
    cis_computeAffine(&whiteCal, &blackCal, CIS_BLUE);
    shared_var.cis_cal_state = CIS_CAL_EXTRACT_OFFSETS;
    osDelay(100);

    /* ---- Balayage des niveaux intermediaires ----------------------------------
       Les deux ancres sont, par construction, les extremites de la plage normalisee :
       le noir tombe sur 0 et le blanc sur CIS_CAL_CURVE_MAX. Seuls les niveaux
       intermediaires sont a mesurer, et on n'en retient que la position normalisee
       moyenne par couleur et par voie. */
    cisLevelNorm[0][0][0] = 0; /* renseigne ci-dessous pour toutes les voies */
    for (int32_t c = 0; c < COLOR_CHANNELS; c++)
    {
        for (int32_t lane = 0; lane < CIS_ADC_OUT_LANES; lane++)
        {
            cisLevelNorm[0][c][lane]                        = 0;
            cisLevelNorm[CIS_CAL_LEVEL_COUNT - 1][c][lane]  = CIS_CAL_CURVE_MAX;
        }
    }

    {
        const uint32_t nInter = CIS_CAL_LEVEL_COUNT - 2U;

        shared_var.cis_cal_state = CIS_CAL_INTERMEDIATE;
        shared_var.cis_cal_progressbar = 0;

        for (uint32_t k = 1; k <= nInter; k++)
        {
            const int32_t duty = (int32_t)cisCalLevels[k];
            const uint8_t base = (uint8_t)(((k - 1U) * 100U) / nInter);
            const uint8_t span = (uint8_t)(100U / nInter);

            cis_ledPowerAdj(duty, duty, duty);
            osDelay(200);

            if (cis_imageProcessRGB_Calibration(cisDataCpy, intermediateCal.data,
                                                CIS_CAL_ITER_LEVEL, base, span, true) != CIS_OK)
            {
                printf("Calibration ABORTED during the %d%% level capture, previous calibration kept\n",
                       (int)duty);
                goto abort;
            }

            cis_measureLevel(&intermediateCal, k);
        }

        shared_var.cis_cal_progressbar = 100;
    }

    /* ---- Construction des courbes de reponse ----------------------------------- */
    printf("Build response curves\n");
    cis_buildCurves(bitDepth);
    shared_var.cis_cal_state = CIS_CAL_COMPUTE_GAINS;
    osDelay(100);

    /* ---- References de derive --------------------------------------------------
       Ramenees en comptages ADC entiers : cis_computeGlobalDriftCorrection les compare
       a des moyennes calculees sur l'image brute, qui est en comptages ADC. */
    printf("Store drift correction references\n");
    for (int32_t lane = 0; lane < CIS_ADC_OUT_LANES; lane++)
    {
        cisCals.blackRefInactiveAvg[0][lane] = (blackCal.red.inactiveAvrgPix[lane]   + CIS_CAL_FRAC_ROUND) >> CIS_CAL_FRAC_BITS;
        cisCals.blackRefInactiveAvg[1][lane] = (blackCal.green.inactiveAvrgPix[lane] + CIS_CAL_FRAC_ROUND) >> CIS_CAL_FRAC_BITS;
        cisCals.blackRefInactiveAvg[2][lane] = (blackCal.blue.inactiveAvrgPix[lane]  + CIS_CAL_FRAC_ROUND) >> CIS_CAL_FRAC_BITS;
    }

    /* Temperature de reference de l'asservissement LED. */
    cisCals.cal_temp_c = shared_imu.temp_c;

    /* Marqueur de format, ecrit en dernier : un fichier sans lui serait relu comme
       des donnees valides par une version differente du firmware. */
    cisCals.magic   = CIS_CAL_FILE_MAGIC;
    cisCals.version = CIS_CAL_FILE_VERSION;

    // ÉTAPE 10: Sauvegarde (même nom de fichier)
    // Traces de progression : cette fin de séquence gelait sans le moindre message.
    sprintf(calibrationFilePath, CALIBRATION_FILE_PATH_FORMAT, shared_config.cis_dpi);
    printf("Write calibration file %s (%u bytes)\n", calibrationFilePath, (unsigned)sizeof(cisCals));
    if (file_writeCisCals(calibrationFilePath, &cisCals) != FILEMANAGER_OK)
    {
        printf("Write calibration file FAILED\n");
    }
    else
    {
        printf("Write calibration file OK\n");
    }

    printf("Stop capture\n");
    cis_stopCapture();
    osDelay(300);
    printf("Restart capture\n");
    cis_startCapture();
    shared_var.cis_cal_state = CIS_CAL_END;
    printf("Calibration END\n");
    printf("===============================\n");
    return;

abort:
    /* Aucune ecriture de fichier : le fichier existant reste la meilleure calibration
       connue. On se contente de remettre la capture en marche.
       La barre est forcee a 100 AVANT de passer a CIS_CAL_END : l'afficheur du CM4
       attend dans un while (cis_cal_progressbar < 99) et resterait bloque a l'infini
       sur une acquisition interrompue a mi-course, l'etat final ne le liberant pas. */
    shared_var.cis_cal_progressbar = 100;
    cis_ledPowerAdj(100, 100, 100);
    cis_stopCapture();
    osDelay(300);
    cis_startCapture();
    shared_var.cis_cal_state = CIS_CAL_END;
    printf("===============================\n");
}

/**
 * @brief       Compute the average value of inactive pixels for a given color.
 * @param       currCals    Pointer to the current calibration data structure.
 * @param       color       Color channel (CIS_RED, CIS_GREEN, or CIS_BLUE).
 * @retval      None
 *
 * This function computes the mean value over the inactive region (of width CIS_BLACK_LINE)
 * for each ADC lane and stores the result in the respective inactiveAvrgPix element.
 */
static void cis_ComputeCalsInactivesAvrg(struct cisCalsTypes *currCals, CIS_Color_TypeDef color)
{
    int32_t laneOffset = 0;
    int32_t offset = 0;
    struct cisColorsParams *currColor = NULL;

    switch (color)
    {
        case CIS_RED:
        {
            currColor = &currCals->red;
            offset = cisConfig.red_offset - CIS_BLACK_PIXELS;
            break;
        }
        case CIS_GREEN:
        {
            currColor = &currCals->green;
            offset = cisConfig.green_offset - CIS_BLACK_PIXELS;
            break;
        }
        case CIS_BLUE:
        {
            currColor = &currCals->blue;
            offset = cisConfig.blue_offset - CIS_BLACK_PIXELS;
            break;
        }
        default:
        {
            Error_Handler();
            return;
        }
    }

    for (int32_t lane = CIS_ADC_OUT_LANES; --lane >= 0; )
    {
        laneOffset = (cisConfig.useful_data_size_per_lane * lane) + offset + CIS_IGNORE_FIRST_BLACK_PIXELS;
        cis_mean(&currCals->data[laneOffset], CIS_USEFUL_BLACK_PIXELS, &currColor->inactiveAvrgPix[lane]);
    }
}

/**
 * @brief       Normalisation affine par pixel : offset (niveau noir) et gain (sensibilite).
 *
 * C'est la seule partie de la calibration qui reste PAR PIXEL, parce que c'est la seule
 * qui decrit une dispersion de fabrication. La forme de la reponse, elle, est commune a
 * la chaine et vit dans la courbe partagee.
 *
 * Le gain est mesure contre l'offset ENTIER que le runtime soustraira reellement, et non
 * contre la reference noire brute : l'erreur d'arrondi de l'offset est ainsi absorbee par
 * le gain au lieu de s'y ajouter.
 */
static void cis_computeAffine(struct cisCalsTypes *whiteCal, struct cisCalsTypes *blackCal, CIS_Color_TypeDef color)
{
    /* Les captures sont ordonnees par POSITION tampon (offsets donnees, tournants) ;
       les tableaux se remplissent aux positions CANONIQUES de la couleur. */
    uint32_t offset = 0;
    const uint32_t calOff = (uint32_t)color * cisConfig.useful_data_size_per_color_per_lane + CIS_BLACK_PIXELS;

    switch (color)
    {
        case CIS_RED:   offset = cisConfig.red_offset; break;
        case CIS_GREEN: offset = cisConfig.green_offset; break;
        case CIS_BLUE:  offset = cisConfig.blue_offset; break;
        default: Error_Handler(); return;
    }

    for (int32_t lane = CIS_ADC_OUT_LANES; --lane >= 0; )
    {
        const uint32_t laneOffset = (cisConfig.useful_data_size_per_lane * lane) + offset;
        const uint32_t laneCal    = (cisConfig.useful_data_size_per_lane * lane) + calOff;

        for (int32_t i = 0; i < cisConfig.pixels_per_color_per_lane; i++)
        {
            const uint32_t idx = laneOffset + i;
            const uint32_t cal = laneCal + i;

            /* Niveau noir arrondi (et non tronque) au comptage ADC : le runtime
               travaille en entiers, seule la mesure du gain exploite la precision
               fractionnaire des references. */
            const int32_t offs = ((int32_t)blackCal->data[idx] + CIS_CAL_FRAC_ROUND) >> CIS_CAL_FRAC_BITS;
            cisCals.offsetData[cal] = (int16_t)offs;

            /* Dynamique utile du pixel, dans le domaine fractionnaire des references. */
            const int32_t span_fx = (int32_t)whiteCal->data[idx] - (offs << CIS_CAL_FRAC_BITS);

            if (span_fx > 0)
            {
                /* gain = CURVE_MAX / span, en Q<CIS_CAL_GAIN_SHIFT>.
                   Numerateur : 2047 << (12 + 4) = 1,3e8, tient dans un int32.
                   Arrondi au plus proche et non tronque : tronquer coute regulierement
                   un cran a l'ancre haute, un pixel exactement blanc normalisant alors
                   un cran a l'ancre haute, un pixel exactement blanc normalisant alors sous
                   CIS_CAL_CURVE_MAX -- soit un blanc pur qui ne sort jamais a 255. */
                const int32_t g = (((int32_t)CIS_CAL_CURVE_MAX << (CIS_CAL_GAIN_SHIFT + CIS_CAL_FRAC_BITS))
                                   + (span_fx / 2)) / span_fx;
                cisCals.gainData[cal] = CLIP_INT16(g);
            }
            else
            {
                /* Pixel mort ou reponse inversee : un gain negatif n'a jamais de sens. */
                cisCals.gainData[cal] = CIS_CAL_GAIN_UNITY;
            }
        }
    }
}

/**
 * @brief  Ancre noire fraiche a chaque demarrage de capture.
 *
 * Mesure du 2026-08-31 (SWD, scene statique) : entre les offsets stockes par la
 * calibration et le noir courant subsiste un residu PAR PIXEL de 16-21 LSB14 --
 * la derive du courant d'obscurite depuis la calibration, que la correction de
 * derive par voie ne peut pas voir. A travers gain x pente de courbe, ce residu
 * explique 50-70 %% du banding des teintes sombres. Remede : recapturer l'ancre
 * noire (LEDs eteintes, aucun mouvement requis) et rebaser offsets et references
 * de derive. Les GAINS restent : ils decrivent la sensibilite, qui ne derive pas
 * a cette echelle (span modifie de ~0,3 %%).
 *
 * S'execute en ~200 ms en fin de cis_startCapture, identification LED comprise :
 * la rotation couleur est donc deja connue et les positions canoniques justes.
 */
void cis_refreshDarkReferences(int32_t *cisDataCpy)
{
    if (cisCals.magic != CIS_CAL_FILE_MAGIC || cisCals.version != CIS_CAL_FILE_VERSION)
    {
        printf("CIS: dark refresh skipped (no valid calibration)\n");
        return;
    }

    cis_ledPowerAdj(CIS_BLACK_LED_POWER, CIS_BLACK_LED_POWER, CIS_BLACK_LED_POWER);
    osDelay(40);   /* purge : charge integree pendant l'etat LEDs allumees + pipeline */

    if (cis_imageProcessRGB_Calibration(cisDataCpy, blackCal.data,
                                        CIS_CAL_DARK_REFRESH_ITER, 0, 100, false) != CIS_OK)
    {
        printf("CIS: dark refresh capture FAILED, offsets kept\n");
        cis_ledPowerAdj(100, 100, 100);
        return;
    }

    /* References de derive par couleur et par voie (memes conversions que la
       calibration, cis_startLinearCalibration). */
    cis_ComputeCalsInactivesAvrg(&blackCal, CIS_RED);
    cis_ComputeCalsInactivesAvrg(&blackCal, CIS_GREEN);
    cis_ComputeCalsInactivesAvrg(&blackCal, CIS_BLUE);
    for (int32_t lane = 0; lane < CIS_ADC_OUT_LANES; lane++)
    {
        cisCals.blackRefInactiveAvg[0][lane] = (blackCal.red.inactiveAvrgPix[lane]   + CIS_CAL_FRAC_ROUND) >> CIS_CAL_FRAC_BITS;
        cisCals.blackRefInactiveAvg[1][lane] = (blackCal.green.inactiveAvrgPix[lane] + CIS_CAL_FRAC_ROUND) >> CIS_CAL_FRAC_BITS;
        cisCals.blackRefInactiveAvg[2][lane] = (blackCal.blue.inactiveAvrgPix[lane]  + CIS_CAL_FRAC_ROUND) >> CIS_CAL_FRAC_BITS;
    }

    /* Offsets par pixel : positions donnees (tournantes) vers positions canoniques,
       exactement comme cis_computeAffine -- mais SANS toucher aux gains. */
    int32_t shiftMin = INT32_MAX, shiftMax = INT32_MIN;
    for (int32_t c = 0; c < COLOR_CHANNELS; c++)
    {
        const uint32_t dataOff = (c == 0) ? (uint32_t)cisConfig.red_offset
                               : (c == 1) ? (uint32_t)cisConfig.green_offset
                                          : (uint32_t)cisConfig.blue_offset;
        const uint32_t calOff = (uint32_t)c * cisConfig.useful_data_size_per_color_per_lane + CIS_BLACK_PIXELS;

        for (int32_t lane = CIS_ADC_OUT_LANES; --lane >= 0; )
        {
            const uint32_t laneData = (cisConfig.useful_data_size_per_lane * lane) + dataOff;
            const uint32_t laneCal  = (cisConfig.useful_data_size_per_lane * lane) + calOff;

            for (int32_t i = 0; i < cisConfig.pixels_per_color_per_lane; i++)
            {
                const int32_t offs = ((int32_t)blackCal.data[laneData + i] + CIS_CAL_FRAC_ROUND) >> CIS_CAL_FRAC_BITS;
                const int32_t d = offs - cisCals.offsetData[laneCal + i];
                if (d < shiftMin) shiftMin = d;
                if (d > shiftMax) shiftMax = d;
                cisCals.offsetData[laneCal + i] = (int16_t)offs;
            }
        }
    }

    driftPrimed = false;   /* l'IIR de derive repart de la nouvelle reference */
    printf("CIS: dark refresh, offsets rebased (shift %ld..%ld)\n",
           (long)shiftMin, (long)shiftMax);

    cis_ledPowerAdj(100, 100, 100);
}

/**
 * @brief       Position normalisee moyenne d'un niveau de stimulus, par couleur et par voie.
 *
 * On n'en retient que la moyenne : c'est ce qui rend le cout memoire d'un niveau
 * independant du nombre de pixels, et donc le nombre de niveaux gratuit.
 *
 * La dispersion est mesuree et tracee au passage. Elle repond a la seule question que
 * pose ce decoupage : la forme de la reponse est-elle vraiment COMMUNE aux pixels ?
 * Un ecart absolu moyen de quelques unites sur CIS_CAL_CURVE_MAX valide l'hypothese ;
 * un ecart large
 * signifierait que la non-linearite varie pixel a pixel et qu'une courbe partagee perd
 * de l'information que l'ancien schema par pixel captait.
 */
static void cis_measureLevel(struct cisCalsTypes *levelCal, uint32_t level)
{
    static const char *colorName[COLOR_CHANNELS] = { "R", "G", "B" };

    for (int32_t c = 0; c < COLOR_CHANNELS; c++)
    {
        uint32_t offset = 0;
        switch (c)
        {
            case 0: offset = cisConfig.red_offset; break;
            case 1: offset = cisConfig.green_offset; break;
            default: offset = cisConfig.blue_offset; break;
        }

        int32_t  means[CIS_ADC_OUT_LANES];
        int32_t  worstMad = 0;
        int32_t  worstMax = 0;

        const uint32_t calOff = (uint32_t)c * cisConfig.useful_data_size_per_color_per_lane + CIS_BLACK_PIXELS;

        for (int32_t lane = 0; lane < CIS_ADC_OUT_LANES; lane++)
        {
            const uint32_t laneOffset = (cisConfig.useful_data_size_per_lane * lane) + offset;
            const uint32_t laneCal    = (cisConfig.useful_data_size_per_lane * lane) + calOff;
            const int32_t  nPix       = cisConfig.pixels_per_color_per_lane;
            int64_t sum = 0;

            for (int32_t i = 0; i < nPix; i++)
            {
                const uint32_t idx = laneOffset + i;
                const uint32_t cal = laneCal + i;
                const int32_t  v_fx = (int32_t)levelCal->data[idx]
                                    - ((int32_t)cisCals.offsetData[cal] << CIS_CAL_FRAC_BITS);
                /* v_fx est en Q<FRAC> : le decalage du gain absorbe les deux echelles. */
                sum += (int32_t)(((int64_t)v_fx * cisCals.gainData[cal])
                                 >> (CIS_CAL_GAIN_SHIFT + CIS_CAL_FRAC_BITS));
            }

            const int32_t mean = (int32_t)(sum / nPix);
            means[lane] = mean;
            cisLevelNorm[level][c][lane] = mean;

            /* Seconde passe : dispersion autour de cette moyenne. */
            int64_t devSum = 0;
            int32_t devMax = 0;
            for (int32_t i = 0; i < nPix; i++)
            {
                const uint32_t idx = laneOffset + i;
                const uint32_t cal = laneCal + i;
                const int32_t  v_fx = (int32_t)levelCal->data[idx]
                                    - ((int32_t)cisCals.offsetData[cal] << CIS_CAL_FRAC_BITS);
                const int32_t n = (int32_t)(((int64_t)v_fx * cisCals.gainData[cal])
                                            >> (CIS_CAL_GAIN_SHIFT + CIS_CAL_FRAC_BITS));
                const int32_t d = (n > mean) ? (n - mean) : (mean - n);
                devSum += d;
                if (d > devMax) devMax = d;
            }

            const int32_t mad = (int32_t)(devSum / nPix);
            if (mad > worstMad) worstMad = mad;
            if (devMax > worstMax) worstMax = devMax;
        }

        printf("CAL level %3d%% %s  n=[%4ld %4ld %4ld]  spread MAD %ld max %ld\n",
               (int)cisCalLevels[level], colorName[c],
               (long)means[0], (long)means[1], (long)means[2],
               (long)worstMad, (long)worstMax);
    }
}

/**
 * @brief       Tabule la courbe de reponse de chaque couple (couleur, voie).
 *
 * Interpolation lineaire par morceaux entre les points (position normalisee mesuree,
 * sortie visee). La sortie visee d'un niveau est proportionnelle a son rapport cyclique :
 * c'est la definition meme de ce que l'ISP doit rendre, une sortie proportionnelle a la
 * lumiere. Toute la non-linearite de la chaine se retrouve donc dans l'ecart entre les
 * abscisses mesurees et cette droite, et c'est exactement ce que la table corrige.
 *
 * Avec 3 niveaux {0, 30, 100} on retombe trait pour trait sur l'ancien schema a deux
 * segments -- a ceci pres que le coude est desormais partage par la voie au lieu d'etre
 * repete pixel a pixel, et qu'il est evalue par lecture de table au lieu d'une branche.
 */
static void cis_buildCurves(uint32_t maxOut)
{
    for (int32_t c = 0; c < COLOR_CHANNELS; c++)
    {
        for (int32_t lane = 0; lane < CIS_ADC_OUT_LANES; lane++)
        {
            int32_t xs[CIS_CAL_LEVEL_COUNT];
            int32_t ys[CIS_CAL_LEVEL_COUNT];

            for (int32_t k = 0; k < CIS_CAL_LEVEL_COUNT; k++)
            {
                int32_t x = cisLevelNorm[k][c][lane];
                if (x < 0)                    x = 0;
                if (x > CIS_CAL_CURVE_MAX)    x = CIS_CAL_CURVE_MAX;
                xs[k] = x;
                ys[k] = (int32_t)(((uint32_t)maxOut * cisCalLevels[k] + 50U) / 100U);
            }

            /* Les ancres sont exactes par construction de la normalisation affine. */
            xs[0] = 0;
            ys[0] = 0;
            xs[CIS_CAL_LEVEL_COUNT - 1] = CIS_CAL_CURVE_MAX;
            ys[CIS_CAL_LEVEL_COUNT - 1] = (int32_t)maxOut;

            /* Monotonie : le bruit peut faire reculer un point intermediaire, ce qui
               produirait une courbe non croissante -- visible comme une inversion de
               contraste dans une plage de gris. On force la progression, sans jamais
               deplacer l'ancre haute. */
            for (int32_t k = 1; k < CIS_CAL_LEVEL_COUNT - 1; k++)
            {
                if (xs[k] <= xs[k - 1]) xs[k] = xs[k - 1] + 1;
                if (xs[k] >= CIS_CAL_CURVE_MAX) xs[k] = CIS_CAL_CURVE_MAX - 1;
                if (ys[k] <  ys[k - 1]) ys[k] = ys[k - 1];
            }

            uint8_t *lut = cisCals.curve[c][lane];
            int32_t  k   = 0;

            for (int32_t n = 0; n < CIS_CAL_CURVE_SIZE; n++)
            {
                while ((k + 2) < CIS_CAL_LEVEL_COUNT && n >= xs[k + 1])
                {
                    k++;
                }

                const int32_t dx = xs[k + 1] - xs[k];
                int32_t y = (dx > 0)
                          ? (ys[k] + ((n - xs[k]) * (ys[k + 1] - ys[k])) / dx)
                          : ys[k + 1];

                if (y < 0)                  y = 0;
                if (y > (int32_t)maxOut)    y = (int32_t)maxOut;
                lut[n] = (uint8_t)y;
            }
        }
    }
}
