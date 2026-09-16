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
#include "ff.h"
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
static void cis_storeBreakpoints(void);
static int32_t cis_evalLinearCurveQ16(int32_t c, int32_t lane, int32_t n);
#if CIS_CAL_LOW_ANCHOR_ENABLED
static void cis_captureLowDeltas(struct cisCalsTypes *levelCal, uint32_t level, int8_t *dst);
#endif

/* Rapports cycliques LED des points de calibration. Le premier est 0 (obscurite) et le
   dernier 100 (blanc) : ce sont les deux ancres de la normalisation affine. */
static const uint8_t cisCalLevels[CIS_CAL_LEVEL_COUNT] = CIS_CAL_LEVELS;

/* Position normalisee moyenne de chaque niveau, par couleur et par voie. C'est TOUT ce
   qu'on retient d'un niveau intermediaire : 9 scalaires au lieu des 43 Ko d'une capture
   pixel a pixel. C'est ce qui rend le nombre de niveaux gratuit en memoire. */
static int32_t cisLevelNorm[CIS_CAL_LEVEL_COUNT][COLOR_CHANNELS][CIS_ADC_OUT_LANES];

/* Etat de l'IIR de derive (Q<CIS_DRIFT_IIR_STATE_Q>), au niveau fichier :
   cis_refreshDarkReferences le re-amorce apres avoir rebase offsets et
   references noires. */
static int32_t driftState[3][CIS_ADC_OUT_LANES];
static bool driftPrimed = false;

/* Moyennes brutes des demi-voies (gauche/droite) de la ligne courante : servent a
   l'erreur de l'annuleur LMS et au journal de lignes. */
/* LUT de RENDU lue par la boucle pixel : linearisation du fichier composee avec
   l'equilibre couleur (compile), le point noir (config appareil) et le gamma. */
static uint8_t cisRenderCurve[COLOR_CHANNELS][CIS_ADC_OUT_LANES][CIS_CAL_CURVE_SIZE];

#if CIS_CAL_LOW_ANCHOR_ENABLED
/* Ponderations des ancres basses par pixel, en Q<CIS_CAL_TENT_SHIFT>. Bases
   triangulaires confinees aux sombres : A culmine (=UNITY) a x(4%) et s'annule a
   x(8%), B culmine a x(8%) et s'annule a x(15%) ; toutes deux nulles au noir --
   les ancres affines restent exactes par pixel, et les tons moyens ne recoivent
   AUCUNE extrapolation. Reconstruites par cis_composeOutputLut ; toutes a zero
   tant qu'aucune calibration valide n'est chargee (correction inerte). */
static uint8_t cisTentLutA[COLOR_CHANNELS][CIS_ADC_OUT_LANES][CIS_CAL_CURVE_SIZE];
static uint8_t cisTentLutB[COLOR_CHANNELS][CIS_ADC_OUT_LANES][CIS_CAL_CURVE_SIZE];
#endif

static int16_t cisActHalfL[3][CIS_ADC_OUT_LANES];
static int16_t cisActHalfR[3][CIS_ADC_OUT_LANES];
#if CIS_LMS_CANCELLER_ENABLED
static int16_t cisLmsRef[3][CIS_ADC_OUT_LANES];   /* residu rapide fenetre noire */
static int16_t cisLmsYdbg;                        /* y du canal G voie1, pour le journal */
#endif

#if CIS_LINE_LOG_ENABLED
/* Non statiques : leurs adresses se lisent dans la map pour le dump SWD. */
struct cisLineLogEntry
{
    int16_t noir[3][CIS_ADC_OUT_LANES];   /* moyenne fenetre noire BRUTE   [couleur][voie] */
    int16_t act[3][CIS_ADC_OUT_LANES];    /* moyenne actifs BRUTE, demi-voie DROITE */
    int16_t rawact[3][CIS_ADC_OUT_LANES]; /* moyenne actifs BRUTE, demi-voie GAUCHE */
    int16_t y_dbg;                        /* sortie de l'annuleur LMS, canal G voie1 */
    uint32_t cyc;                         /* DWT->CYCCNT : l'acquisition tourne a 1107,4
                                             lps mais ~4 %% des lignes sont sautees ; les
                                             intervalles reels rendent le spectre exact
                                             (Lomb-Scargle) au lieu d'un axe suppose. */
};
volatile uint32_t cisLineLogHead;
struct cisLineLogEntry cisLineLog[CIS_LINE_LOG_N];
#endif

/* Private user code ---------------------------------------------------------*/

/* Index normalise d'un canal : normalisation affine par pixel. Inline et sans branche
   -- boucle la plus chaude du CM7. La correction de VOILE est REPLIEE dans gainData/
   offsetData a la calibration (cis_foldVeil), donc invisible ici : cout runtime nul. */
static inline uint32_t cis_normIdx(int32_t v, int32_t gain)
{
    return __USAT((v * gain) >> CIS_CAL_GAIN_SHIFT, CIS_CAL_CURVE_BITS);
}

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

    /* Lissage temporel : la derive corrigee est THERMIQUE, elle evolue en secondes.
       Appliquee brute, la moyenne de 24 echantillons injecte son bruit ligne a ligne
       dans TOUTE la ligne -- c'est du banding fabrique par la correction elle-meme.
       Etat en Q<CIS_DRIFT_IIR_STATE_Q> : la zone morte de troncature de l'increment
       (2^SHIFT en Q) doit rester une fraction de comptage, sinon l'IIR lent ne suit
       plus les petites derives. */
    {
        for (int32_t c = 0; c < 3; c++)
        {
            for (int32_t l = 0; l < CIS_ADC_OUT_LANES; l++)
            {
                const int32_t target = globalDriftOffset[c][l] << CIS_DRIFT_IIR_STATE_Q;
                if (!driftPrimed)
                {
                    driftState[c][l] = target;
                }
                else
                {
                    driftState[c][l] += (target - driftState[c][l]) >> CIS_DRIFT_IIR_SHIFT;
                }
                globalDriftOffset[c][l] = driftState[c][l] >> CIS_DRIFT_IIR_STATE_Q;
#if CIS_LMS_CANCELLER_ENABLED
                cisLmsRef[c][l] = (int16_t)((target >> CIS_DRIFT_IIR_STATE_Q) - globalDriftOffset[c][l]);
#endif
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

    /* Step 2 : normalisation affine par pixel, correction d'ancre basse, puis courbe.
     *
     * La boucle est SANS BRANCHE. L'ancien schéma testait, pour chaque canal de chaque
     * pixel, de quel côté du coude on se trouvait : 10 368 branches par ligne dont
     * l'issue suit les données de l'image, donc largement imprévisibles. Sur Cortex-M7
     * chaque erreur de prédiction coûte une dizaine de cycles. __USAT sature en une
     * instruction et remplace du même coup les deux comparaisons de l'écrêtage, et
     * l'écrêtage à 255 est désormais porté par la courbe elle-même. La correction
     * des ancres basses (dA·baseA + dB·baseB) respecte la même règle : deux
     * multiplications, deux lectures de table et un __USAT, aucune branche.
     *
     * Bornes : l'entrée est un ADC 14 bits corrigé de la dérive (bornée à
     * CIS_DRIFT_THRESHOLD ~1,6k) puis de l'offset, donc |x| <= ~18k ; les gains sont
     * des int16 <= 32767 ; le produit plafonne à 5,9e8, sous 2^31.
     */
    (void)maxClipValue;  /* la sortie est bornée par construction de la courbe */

    /* Moyennes brutes des demi-voies AVANT toute correction : erreur du LMS + journal. */
    {
        const uint32_t offs[3] = { (uint32_t)cisConfig.red_offset,
                                   (uint32_t)cisConfig.green_offset,
                                   (uint32_t)cisConfig.blue_offset };
        for (int32_t c = 0; c < 3; c++)
        {
            for (int32_t l = 0; l < CIS_ADC_OUT_LANES; l++)
            {
                const int32_t *pa = &cisDataCpy[(cisConfig.useful_data_size_per_lane * l) + offs[c]];
                const int32_t half = cisConfig.pixels_per_color_per_lane / 2;
                int32_t sa = 0, sb = 0;
                for (int32_t k = 0; k < half; k++) sa += pa[k];
                for (int32_t k = half; k < cisConfig.pixels_per_color_per_lane; k++) sb += pa[k];
                cisActHalfL[c][l] = (int16_t)(sa / half);
                cisActHalfR[c][l] = (int16_t)(sb / (cisConfig.pixels_per_color_per_lane - half));
            }
        }
    }

#if CIS_LMS_CANCELLER_ENABLED
    /* Annuleur adaptatif : voir config.h. y s'ajoute au terme de derive de CETTE
       ligne (avant la boucle pixel), l'adaptation minimise la correlation entre le
       residu des actifs et l'historique de la fenetre noire. */
    {
        static int16_t refHist[3][CIS_ADC_OUT_LANES][CIS_LMS_TAPS];
        static int32_t wq15[3][CIS_ADC_OUT_LANES][CIS_LMS_TAPS];
        static int32_t actSlowQ3[3][CIS_ADC_OUT_LANES];
        static uint32_t lmsIdx;
        const int32_t h0 = (int32_t)(lmsIdx % CIS_LMS_TAPS);
        for (int32_t c = 0; c < 3; c++)
        {
            for (int32_t l = 0; l < CIS_ADC_OUT_LANES; l++)
            {
                refHist[c][l][h0] = cisLmsRef[c][l];
                int32_t y = 0;
                for (int32_t k = 0; k < CIS_LMS_TAPS; k++)
                {
                    y += wq15[c][l][k] * refHist[c][l][(h0 - k + CIS_LMS_TAPS) % CIS_LMS_TAPS];
                }
                y >>= 15;
                if (y > CIS_DRIFT_THRESHOLD) { y = CIS_DRIFT_THRESHOLD; }
                else if (y < -CIS_DRIFT_THRESHOLD) { y = -CIS_DRIFT_THRESHOLD; }

                const int32_t act = ((int32_t)cisActHalfL[c][l] + (int32_t)cisActHalfR[c][l]) / 2;
                if (lmsIdx == 0U)
                {
                    actSlowQ3[c][l] = act << 3;
                }
                actSlowQ3[c][l] += ((act << 3) - actSlowQ3[c][l]) >> 3;
                int32_t e = act - (actSlowQ3[c][l] >> 3) - y;
                if (e > 127) { e = 127; } else if (e < -127) { e = -127; }  /* scene */

                for (int32_t k = 0; k < CIS_LMS_TAPS; k++)
                {
                    const int32_t r = refHist[c][l][(h0 - k + CIS_LMS_TAPS) % CIS_LMS_TAPS];
                    wq15[c][l][k] += (e * r) >> CIS_LMS_MU_SHIFT;
                    wq15[c][l][k] -= wq15[c][l][k] >> CIS_LMS_LEAK_SHIFT;
                }
                globalDriftOffset[c][l] += y;
#if CIS_LINE_LOG_ENABLED
                if (c == 1 && l == 1) { cisLmsYdbg = (int16_t)y; }
#endif
            }
        }
        lmsIdx++;
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
        const uint8_t * restrict curveR = cisRenderCurve[0][lane];
        const uint8_t * restrict curveG = cisRenderCurve[1][lane];
        const uint8_t * restrict curveB = cisRenderCurve[2][lane];

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
            cisDataCpy[pixelIdx] = curveR[cis_normIdx(v, cisCals.gainData[calIdx])];

            /* Process GREEN channel */
            pixelIdx = baseG + i;
            calIdx = calG + i;
            v = cisDataCpy[pixelIdx] - driftG - cisCals.offsetData[calIdx];
            cisDataCpy[pixelIdx] = curveG[cis_normIdx(v, cisCals.gainData[calIdx])];

            /* Process BLUE channel */
            pixelIdx = baseB + i;
            calIdx = calB + i;
            v = cisDataCpy[pixelIdx] - driftB - cisCals.offsetData[calIdx];
            cisDataCpy[pixelIdx] = curveB[cis_normIdx(v, cisCals.gainData[calIdx])];
        }
    }

#if CIS_LINE_LOG_ENABLED
    /* Journal par ligne : la fenetre noire de cisDataCpy n'est PAS reecrite par la
       boucle ci-dessus (elle demarre aux offsets actifs), on y lit donc encore le brut
       de CETTE ligne ; les actifs, eux, sont desormais en sortie calibree 0..255. */
    {
        if ((DWT->CTRL & DWT_CTRL_CYCCNTENA_Msk) == 0U)
        {
            CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
            DWT->CYCCNT = 0U;
            DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
        }
        struct cisLineLogEntry *e = &cisLineLog[cisLineLogHead % CIS_LINE_LOG_N];
        e->cyc = DWT->CYCCNT;
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

                e->act[c][lane] = cisActHalfR[c][lane];
                e->rawact[c][lane] = cisActHalfL[c][lane];
            }
        }
#if CIS_LMS_CANCELLER_ENABLED
        e->y_dbg = cisLmsYdbg;
#endif
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
            /* Les ancres basses sont conservees PAR PIXEL : moyenne d'ancre. */
            const uint16_t iter = (k == CIS_CAL_LOW_ANCHOR_IDX_A || k == CIS_CAL_LOW_ANCHOR_IDX_B)
                                ? CIS_CAL_ITER_LOW_ANCHOR : CIS_CAL_ITER_LEVEL;

            cis_ledPowerAdj(duty, duty, duty);
            osDelay(200);

            if (cis_imageProcessRGB_Calibration(cisDataCpy, intermediateCal.data,
                                                iter, base, span, true) != CIS_OK)
            {
                printf("Calibration ABORTED during the %d%% level capture, previous calibration kept\n",
                       (int)duty);
                goto abort;
            }

            cis_measureLevel(&intermediateCal, k);
#if CIS_CAL_LOW_ANCHOR_ENABLED
            if (k == CIS_CAL_LOW_ANCHOR_IDX_A)
            {
                cis_captureLowDeltas(&intermediateCal, k, cisCals.lowDeltaA);
            }
            else if (k == CIS_CAL_LOW_ANCHOR_IDX_B)
            {
                cis_captureLowDeltas(&intermediateCal, k, cisCals.lowDeltaB);
            }
#endif
        }

        shared_var.cis_cal_progressbar = 100;
    }

    /* ---- Points de cassure de la reponse ---------------------------------------
       La sortie n'est plus tabulee ici : seule la LUT de rendu l'est, composee en
       pleine precision par cis_composeOutputLut. bitDepth n'a donc plus de role,
       la quantification finale vit dans la composition. */
    (void)bitDepth;
    printf("Store response breakpoints\n");
    cis_storeBreakpoints();
    cis_composeOutputLut();
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
 * @brief  Linearisation en un index normalise, evaluee sur les points de cassure.
 *
 * Rend la sortie visee en Q16 (0..CIS_CAL_CURVE_Y_MAX). C'est la reference de
 * precision de toute la chaine : la LUT de rendu en est derivee, et la calibration
 * du point noir mesure a travers elle. Hors boucle pixel (la marche lineaire sur
 * les 7 points est sans enjeu de temps).
 */
static int32_t cis_evalLinearCurveQ16(int32_t c, int32_t lane, int32_t n)
{
    const int16_t *xs = cisCals.curveX[c][lane];
    const uint16_t *ys = cisCals.curveY;
    int32_t k = 0;

    while ((k + 2) < CIS_CAL_LEVEL_COUNT && n >= xs[k + 1])
    {
        k++;
    }

    const int32_t dx = xs[k + 1] - xs[k];
    /* (n - xs[k]) <= 2047 et |dy| <= 65535 : produit < 2^28, marge int32 confortable. */
    return (dx > 0) ? (int32_t)ys[k] + ((n - xs[k]) * ((int32_t)ys[k + 1] - (int32_t)ys[k])) / dx
                    : (int32_t)ys[k + 1];
}

/**
 * @brief  Compose la LUT de rendu : lineaire (points de cassure) -> equilibre ->
 *         point noir -> sRGB, et reconstruit les tentes d'ancre basse.
 *
 * Appelee au chargement de la calibration, en fin de calibration, et par le lien
 * SLP quand SLP_CFG_BLACK_POINT change : effet immediat, sans recalibration.
 * La quantification 8 bits n'a lieu QU'ICI, en bout de chaine : le lineaire
 * interpole reste en pleine precision jusqu'a l'encodage final -- l'ancienne
 * courbe uint8 posterisait les ombres avant meme le point noir et le sRGB.
 * ~18k evaluations flottantes, quelques millisecondes, hors boucle pixel.
 */
void cis_composeOutputLut(void)
{
    static const float trim[3] = { CIS_OUTPUT_TRIM_R_X1000 / 1000.0f,
                                   CIS_OUTPUT_TRIM_G_X1000 / 1000.0f,
                                   CIS_OUTPUT_TRIM_B_X1000 / 1000.0f };
    const float bp = (float)shared_config.cis_black_point / 1000.0f;

    for (int32_t c = 0; c < COLOR_CHANNELS; c++)
    {
        for (int32_t lane = 0; lane < CIS_ADC_OUT_LANES; lane++)
        {
            for (int32_t n = 0; n < CIS_CAL_CURVE_SIZE; n++)
            {
                float yf = (float)cis_evalLinearCurveQ16(c, lane, n) / (float)CIS_CAL_CURVE_Y_MAX;
                yf = (yf * trim[c] - bp) / (1.0f - bp);
                if (yf < 0.0f) { yf = 0.0f; } else if (yf > 1.0f) { yf = 1.0f; }
#if CIS_OUTPUT_GAMMA_SRGB
                yf = (yf <= 0.0031308f) ? (12.92f * yf)
                                        : (1.055f * powf(yf, 1.0f / 2.4f) - 0.055f);
#endif
                cisRenderCurve[c][lane][n] = (uint8_t)(yf * 255.0f + 0.5f);
            }

#if CIS_CAL_LOW_ANCHOR_ENABLED
            /* Bases Q7 des deux ancres basses, confinees aux sombres : A culmine a
               x(4%) et s'annule a x(8%), B culmine a x(8%) et s'annule a x(15%).
               Interpolation par pixel entre les deux points mesures ; au-dela de
               x(15%), aucune correction. Des positions incoherentes (calibration
               absente ou aberrante) eteignent la correction pour ce couple. */
            {
                const int32_t xA = cisCals.curveX[c][lane][CIS_CAL_LOW_ANCHOR_IDX_A];
                const int32_t xB = cisCals.curveX[c][lane][CIS_CAL_LOW_ANCHOR_IDX_B];
                const int32_t xT = cisCals.curveX[c][lane][CIS_CAL_LOW_ANCHOR_TOP_IDX];
                uint8_t *tA = cisTentLutA[c][lane];
                uint8_t *tB = cisTentLutB[c][lane];

                if (xA <= 0 || xB <= xA || xT <= xB || xT > CIS_CAL_CURVE_MAX)
                {
                    memset(tA, 0, CIS_CAL_CURVE_SIZE);
                    memset(tB, 0, CIS_CAL_CURVE_SIZE);
                }
                else
                {
                    for (int32_t n = 0; n < CIS_CAL_CURVE_SIZE; n++)
                    {
                        int32_t a = 0, b = 0;
                        if (n <= xA)
                        {
                            a = (n * CIS_CAL_TENT_UNITY) / xA;
                        }
                        else if (n <= xB)
                        {
                            a = ((xB - n) * CIS_CAL_TENT_UNITY) / (xB - xA);
                            b = ((n - xA) * CIS_CAL_TENT_UNITY) / (xB - xA);
                        }
                        else if (n < xT)
                        {
                            b = ((xT - n) * CIS_CAL_TENT_UNITY) / (xT - xB);
                        }
                        tA[n] = (uint8_t)a;
                        tB[n] = (uint8_t)b;
                    }
                }
            }
#endif
        }
    }
    printf("CIS: render LUT composed (black point %u/1000)\n",
           (unsigned)shared_config.cis_black_point);
}

volatile uint8_t cisBlackPointCalState = 0;   /* 0 repos, 1 demandee/en cours, 2 ok, 3 echec */

/* Index de courbe d'un pixel de la capture de point noir, avec le gain/offset COURANTS.
   Miroir exact de la boucle pixel. Avant le repli du voile il rend le plancher brut
   (etapes 1-2) ; apres, le plancher aplati (etape 3). Facteur commun aux passes. */
static inline uint32_t cis_bpRawIndex(int32_t c, int32_t lane, uint32_t laneData,
                                      uint32_t laneCal, int32_t i)
{
    (void)c; (void)lane;
    const int32_t raw = ((int32_t)whiteCal.data[laneData + i] + CIS_CAL_FRAC_ROUND) >> CIS_CAL_FRAC_BITS;
    const int32_t v = raw - cisCals.offsetData[laneCal + i];
    return cis_normIdx(v, cisCals.gainData[laneCal + i]);
}

/**
 * @brief  Calibration du point noir + carte de VOILE : glisser sur le papier noir, LEDs on.
 *
 * Une seule capture, trois etapes :
 *  1. Base par COULEUR : moyenne de l'index BRUT sur les 3 voies (un scalaire/couleur).
 *  2. Carte de voile : veilDelta[p] = plancher[p] - base_couleur (ecart PLEIN, toutes
 *     echelles spatiales, sature int8). Reference commune aux 3 voies -> pas de marche
 *     de voie. Appliquee par rampe (nulle au blanc, pleine au noir) : aplatit tout le
 *     plancher d'ombre (voile LED additif, commun aux 3 couleurs) sans toucher les
 *     clairs. C'est le banding vertical que le point noir scalaire ne peut aplatir.
 *  3. Point noir : percentile haut du residu APLATI (voile retire), le vrai residu
 *     scalaire une fois les stries supprimees. Le papier sort alors noir UNIFORME.
 * Applique a chaud (LUT recomposee) et persiste dans CONFIG.TXT ; la carte de voile
 * part dans le fichier de calibration au prochain ecrit... non : elle vit dans cisCals
 * en RAM et sera perdue au reboot -> on la sauve ici via file_writeCisCals.
 */
void cis_calibrateBlackPoint(int32_t *cisDataCpy)
{
    if (cisCals.magic != CIS_CAL_FILE_MAGIC || cisCals.version != CIS_CAL_FILE_VERSION)
    {
        printf("BP CAL: no valid calibration, aborted\n");
        cisBlackPointCalState = 3;
        return;
    }
    printf("===== BLACK POINT + VEIL CALIBRATION (GLIDE on black paper, LEDs on) =====\n");

    cis_refreshDarkReferences(cisDataCpy);           /* offsets = noir frais, LEDs restaurees */
    osDelay(40);

    /* Le MOUVEMENT moyenne la texture du papier, comme pour les ancres. */
    if (cis_imageProcessRGB_Calibration(cisDataCpy, whiteCal.data,
                                        CIS_BP_CAL_ITER, 0, 100, true) != CIS_OK)
    {
        printf("BP CAL: capture FAILED, black point + veil unchanged\n");
        cisBlackPointCalState = 3;
        return;
    }

    static const int32_t trim[3] = { CIS_OUTPUT_TRIM_R_X1000,
                                     CIS_OUTPUT_TRIM_G_X1000,
                                     CIS_OUTPUT_TRIM_B_X1000 };

    int32_t veilAbsMax = 0, veilClipped = 0;

    for (int32_t c = 0; c < COLOR_CHANNELS; c++)
    {
        const uint32_t dataOff = (c == 0) ? (uint32_t)cisConfig.red_offset
                               : (c == 1) ? (uint32_t)cisConfig.green_offset
                                          : (uint32_t)cisConfig.blue_offset;
        const uint32_t calOff = (uint32_t)c * cisConfig.useful_data_size_per_color_per_lane + CIS_BLACK_PIXELS;
        const int32_t  nPix   = cisConfig.pixels_per_color_per_lane;

        /* --- Etape 1 : base = moyenne de l'index brut sur les 3 voies de la couleur --- */
        int64_t sum = 0;
        for (int32_t lane = 0; lane < CIS_ADC_OUT_LANES; lane++)
        {
            const uint32_t laneData = (cisConfig.useful_data_size_per_lane * lane) + dataOff;
            const uint32_t laneCal  = (cisConfig.useful_data_size_per_lane * lane) + calOff;
            for (int32_t i = 0; i < nPix; i++)
            {
                sum += cis_bpRawIndex(c, lane, laneData, laneCal, i);
            }
        }
        const int32_t base = (int32_t)(sum / (nPix * CIS_ADC_OUT_LANES));

        /* --- Etape 2 : ecart plein par pixel, REPLIE dans gain/offset -----------------
           veilDelta stocke pour inspection. Le repli rend la rampe n'=n-d*(MAX-n)/MAX
           = n*(MAX+d)/MAX - d, affine en n donc en raw : gain' = gain*(MAX+d)/MAX,
           offset' = offset + (d<<GAIN_SHIFT)/gain'. Cout runtime nul. */
        for (int32_t lane = 0; lane < CIS_ADC_OUT_LANES; lane++)
        {
            const uint32_t laneData = (cisConfig.useful_data_size_per_lane * lane) + dataOff;
            const uint32_t laneCal  = (cisConfig.useful_data_size_per_lane * lane) + calOff;
            for (int32_t i = 0; i < nPix; i++)
            {
                int32_t d = (int32_t)cis_bpRawIndex(c, lane, laneData, laneCal, i) - base;
                const int32_t a = (d < 0) ? -d : d;
                if (a > veilAbsMax) { veilAbsMax = a; }
                if (d >  CIS_VEIL_DELTA_MAX) { d =  CIS_VEIL_DELTA_MAX; veilClipped++; }
                if (d < -CIS_VEIL_DELTA_MAX) { d = -CIS_VEIL_DELTA_MAX; veilClipped++; }
                cisCals.veilDelta[laneCal + i] = (int8_t)d;

                /* Repli dans gain/offset (guard : gain nul = pixel mort, on ne touche pas). */
                const int32_t g = cisCals.gainData[laneCal + i];
                if (g > 0)
                {
                    int32_t gp = (g * (CIS_CAL_CURVE_MAX + d) + CIS_CAL_CURVE_MAX / 2)
                               / CIS_CAL_CURVE_MAX;
                    gp = CLIP_INT16(gp);
                    if (gp > 0)
                    {
                        const int32_t off = cisCals.offsetData[laneCal + i];
                        const int32_t dOff = ((d << CIS_CAL_GAIN_SHIFT)
                                              + ((d >= 0 ? gp : -gp) / 2)) / gp;
                        cisCals.gainData[laneCal + i]   = (int16_t)gp;
                        cisCals.offsetData[laneCal + i] = CLIP_INT16(off + dOff);
                    }
                }
            }
        }
    }

    /* --- Etape 3 : point noir sur le residu APLATI (voile desormais retire) -------- */
    static uint16_t hist[1001];
    memset(hist, 0, sizeof(hist));
    int32_t total = 0;
    for (int32_t c = 0; c < COLOR_CHANNELS; c++)
    {
        const uint32_t dataOff = (c == 0) ? (uint32_t)cisConfig.red_offset
                               : (c == 1) ? (uint32_t)cisConfig.green_offset
                                          : (uint32_t)cisConfig.blue_offset;
        const uint32_t calOff = (uint32_t)c * cisConfig.useful_data_size_per_color_per_lane + CIS_BLACK_PIXELS;

        for (int32_t lane = 0; lane < CIS_ADC_OUT_LANES; lane++)
        {
            const uint32_t laneData = (cisConfig.useful_data_size_per_lane * lane) + dataOff;
            const uint32_t laneCal  = (cisConfig.useful_data_size_per_lane * lane) + calOff;
            for (int32_t i = 0; i < cisConfig.pixels_per_color_per_lane; i++)
            {
                /* Gain/offset desormais REPLIES : cis_bpRawIndex rend le plancher APLATI,
                   miroir exact du runtime. */
                const uint32_t n = cis_bpRawIndex(c, lane, laneData, laneCal, i);
                const int32_t yq = cis_evalLinearCurveQ16(c, lane, (int32_t)n);
                int32_t y = (yq * trim[c] + CIS_CAL_CURVE_Y_MAX / 2) / CIS_CAL_CURVE_Y_MAX;
                if (y < 0) { y = 0; } else if (y > 1000) { y = 1000; }
                hist[y]++;
                total++;
            }
        }
    }

    int32_t cum = 0, p = 0;
    const int32_t keep = (total * CIS_BP_CAL_KEEP_PERMILLE + 500) / 1000;
    for (p = 0; p <= 1000; p++)
    {
        cum += hist[p];
        if (cum >= keep) { break; }
    }

    if (p > CIS_BP_CAL_MAX_X1000)
    {
        printf("BP CAL: flattened residue %ld/1000 too bright for a black target, unchanged\n",
               (long)p);
        cisBlackPointCalState = 3;
        return;   /* veilDelta reste en RAM mais n'est pas persistee : perdue au reboot */
    }

    int32_t bp = p + CIS_BP_CAL_MARGIN_X1000;
    if (bp > 200) { bp = 200; }
    shared_config.cis_black_point = (uint8_t)bp;
    cis_composeOutputLut();
    file_writeConfig(CONFIG_FILE_PATH, &shared_config);

    /* La carte de voile vit dans cisCals : la persister avec le fichier de calibration,
       sinon elle disparait au reboot. */
    char calibrationFilePath[64];
    sprintf(calibrationFilePath, CALIBRATION_FILE_PATH_FORMAT, shared_config.cis_dpi);
    if (file_writeCisCals(calibrationFilePath, &cisCals) != FILEMANAGER_OK)
    {
        printf("BP CAL: veil map write FAILED (black point applied but veil lost on reboot)\n");
    }

    printf("BP CAL: veil map stored (max |ripple| %ld, clipped %ld), flattened P%d.%d %ld/1000"
           " -> black point %ld/1000, persisted\n",
           (long)veilAbsMax, (long)veilClipped,
           CIS_BP_CAL_KEEP_PERMILLE / 10, CIS_BP_CAL_KEEP_PERMILLE % 10,
           (long)p, (long)bp);
    printf("=====================================================\n");
    cisBlackPointCalState = 2;
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
 * @brief       Fige les points de cassure de la reponse de chaque couple (couleur, voie).
 *
 * Le fichier stocke la LINEARISATION physique pure sous forme de points (position
 * normalisee mesuree, sortie visee Q16) ; le rendu (equilibre couleur, point noir,
 * sRGB) est interpole et compose a chaud par cis_composeOutputLut(). La sortie visee
 * d'un niveau est proportionnelle a son rapport cyclique : c'est la definition meme
 * de ce que l'ISP doit rendre, une sortie proportionnelle a la lumiere. Toute la
 * non-linearite de la chaine se retrouve donc dans l'ecart entre les abscisses
 * mesurees et cette droite, et c'est exactement ce que l'interpolation corrige.
 */
static void cis_storeBreakpoints(void)
{
    /* Sorties visees, partagees par tous les couples : les rapports cycliques sont
       strictement croissants, donc curveY est monotone par construction. */
    for (int32_t k = 0; k < CIS_CAL_LEVEL_COUNT; k++)
    {
        cisCals.curveY[k] = (uint16_t)(((uint32_t)CIS_CAL_CURVE_Y_MAX * cisCalLevels[k] + 50U) / 100U);
    }
    cisCals.curveY[0] = 0;
    cisCals.curveY[CIS_CAL_LEVEL_COUNT - 1] = CIS_CAL_CURVE_Y_MAX;

    for (int32_t c = 0; c < COLOR_CHANNELS; c++)
    {
        for (int32_t lane = 0; lane < CIS_ADC_OUT_LANES; lane++)
        {
            int32_t xs[CIS_CAL_LEVEL_COUNT];

            for (int32_t k = 0; k < CIS_CAL_LEVEL_COUNT; k++)
            {
                int32_t x = cisLevelNorm[k][c][lane];
                if (x < 0)                    x = 0;
                if (x > CIS_CAL_CURVE_MAX)    x = CIS_CAL_CURVE_MAX;
                xs[k] = x;
            }

            /* Les ancres sont exactes par construction de la normalisation affine. */
            xs[0] = 0;
            xs[CIS_CAL_LEVEL_COUNT - 1] = CIS_CAL_CURVE_MAX;

            /* Monotonie : le bruit peut faire reculer un point intermediaire, ce qui
               produirait une courbe non croissante -- visible comme une inversion de
               contraste dans une plage de gris. On force la progression, sans jamais
               deplacer l'ancre haute. */
            for (int32_t k = 1; k < CIS_CAL_LEVEL_COUNT - 1; k++)
            {
                if (xs[k] <= xs[k - 1]) xs[k] = xs[k - 1] + 1;
                if (xs[k] >= CIS_CAL_CURVE_MAX) xs[k] = CIS_CAL_CURVE_MAX - 1;
            }

            for (int32_t k = 0; k < CIS_CAL_LEVEL_COUNT; k++)
            {
                cisCals.curveX[c][lane][k] = (int16_t)xs[k];
            }
        }
    }
}

#if CIS_CAL_LOW_ANCHOR_ENABLED
/**
 * @brief       Ecart par pixel a une ancre basse : ce que la courbe partagee ne voit pas.
 *
 * A appeler juste apres cis_measureLevel(level) : la moyenne de voie (cisLevelNorm)
 * vient d'etre posee sur la MEME capture. On conserve, par pixel, l'ecart de sa
 * position normalisee a cette moyenne, sature int8, dans dst (lowDeltaA ou lowDeltaB).
 * C'est la dispersion de non-linearite basse lumiere -- celle qui reste en banding
 * vertical dans les ombres quand seule la forme moyenne est corrigee.
 */
static void cis_captureLowDeltas(struct cisCalsTypes *levelCal, uint32_t level, int8_t *dst)
{
    int32_t clipped = 0;
    int32_t maxAbs  = 0;

    for (int32_t c = 0; c < COLOR_CHANNELS; c++)
    {
        uint32_t offset = 0;
        switch (c)
        {
            case 0: offset = cisConfig.red_offset; break;
            case 1: offset = cisConfig.green_offset; break;
            default: offset = cisConfig.blue_offset; break;
        }

        const uint32_t calOff = (uint32_t)c * cisConfig.useful_data_size_per_color_per_lane + CIS_BLACK_PIXELS;

        for (int32_t lane = 0; lane < CIS_ADC_OUT_LANES; lane++)
        {
            const uint32_t laneOffset = (cisConfig.useful_data_size_per_lane * lane) + offset;
            const uint32_t laneCal    = (cisConfig.useful_data_size_per_lane * lane) + calOff;
            const int32_t  mean       = cisLevelNorm[level][c][lane];

            for (int32_t i = 0; i < cisConfig.pixels_per_color_per_lane; i++)
            {
                const uint32_t idx = laneOffset + i;
                const uint32_t cal = laneCal + i;
                const int32_t  v_fx = (int32_t)levelCal->data[idx]
                                    - ((int32_t)cisCals.offsetData[cal] << CIS_CAL_FRAC_BITS);
                const int32_t  n = (int32_t)(((int64_t)v_fx * cisCals.gainData[cal])
                                             >> (CIS_CAL_GAIN_SHIFT + CIS_CAL_FRAC_BITS));

                int32_t d = n - mean;
                const int32_t a = (d < 0) ? -d : d;
                if (a > maxAbs) { maxAbs = a; }
                if (d >  CIS_CAL_LOW_DELTA_MAX) { d =  CIS_CAL_LOW_DELTA_MAX; clipped++; }
                if (d < -CIS_CAL_LOW_DELTA_MAX) { d = -CIS_CAL_LOW_DELTA_MAX; clipped++; }

                dst[cal] = (int8_t)d;
            }
        }
    }

    printf("CAL low anchor %d%%: per-pixel deltas stored (max |d| %ld, clipped %ld)\n",
           (int)cisCalLevels[level], (long)maxAbs, (long)clipped);
}
#endif /* CIS_CAL_LOW_ANCHOR_ENABLED */
