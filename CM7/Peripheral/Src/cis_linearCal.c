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
    int32_t maxPix;
    int32_t minPix;
    int32_t deltaPix;
    int32_t inactiveAvrgPix[3];
};

__attribute__ ((packed))
struct cisCalsTypes {
	int32_t data[CIS_MAX_ADC_BUFF_SIZE * 3];
	struct cisColorsParams red;
	struct cisColorsParams green;
	struct cisColorsParams blue;
};

/* Private define ------------------------------------------------------------*/
#define UNITY_Q16_16   (1 << 16)

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Variable containing ADC conversions data */

/* Private function prototypes -----------------------------------------------*/
static void cis_ComputeCalsInactivesAvrg(struct cisCalsTypes *currCals, CIS_Color_TypeDef color);
static void cis_computeCalsExtremums(struct cisCalsTypes *currCals, CIS_Color_TypeDef color);
static void cis_computeCalsOffsets(struct cisCalsTypes *whiteCal, struct cisCalsTypes *blackCal, CIS_Color_TypeDef color);
static void cis_computeCalsGains(uint32_t maxADCValue, struct cisCalsTypes *whiteCal, struct cisCalsTypes *blackCal, CIS_Color_TypeDef color);

/* Private user code ---------------------------------------------------------*/

/**
 * @brief       Initialize the linear calibration (integer version).
 *
 * Opens the calibration file based on the current DPI and loads the calibration
 * parameters. If the file is not found, a calibration is requested.
 *
 * @param       None
 * @retval      None
 */
void cis_linearCalibrationInit(void)
{
    FIL file;
    FRESULT fres;
    char calibrationFilePath[64];

    /* Construction du chemin selon le DPI */
    sprintf(calibrationFilePath, CALIBRATION_FILE_PATH_FORMAT, shared_config.cis_dpi);

    /* Ouverture du fichier de calibration en lecture */
    fres = f_open(&file, calibrationFilePath, FA_READ);
    if (fres == FR_OK)
    {
        printf("Lecture de la calibration INT pour %d DPI SUCCÈS\n", shared_config.cis_dpi);
        /* On lit la calibration entière – fonction à implémenter selon votre projet */
        file_readCisCals(calibrationFilePath, /* pointeur sur votre structure de calibration entière */ &cisCals);
        f_close(&file);
        shared_var.cis_cal_state = CIS_CAL_END;
    }
    else
    {
        printf("Fichier de calibration INT non trouvé pour %d DPI, calibration demandée.\n", shared_config.cis_dpi);
        shared_var.cis_cal_state = CIS_CAL_REQUESTED;
    }
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
void cis_startLinearCalibration(uint16_t iterationNb, uint32_t bitDepth)
{
    printf("-------------------------------\n");
    printf("----- CALIBRATION STARTED -----\n");
    printf("----------- %d DPI -----------\n", shared_config.cis_dpi);
    printf("-------------------------------\n");

    struct cisCalsTypes blackCal;
    struct cisCalsTypes whiteCal;
    char calibrationFilePath[64];

    memset(&blackCal, 0, sizeof(blackCal));
    memset(&whiteCal, 0, sizeof(whiteCal));
    memset(&cisCals, 0, sizeof(cisCals));

    // Step 1: Capture white calibration data
    cis_ledPowerAdj(100, 100, 100);
    shared_var.cis_cal_progressbar = 0;
    shared_var.cis_cal_state = CIS_CAL_PLACE_ON_WHITE;
    osDelay(200);

    cis_imageProcessRGB_Calibration(whiteCal.data, iterationNb);
    SCB_CleanDCache_by_Addr((uint32_t *)&cisCals, sizeof(cisCals));
    osDelay(200);

    // Step 2: Capture black calibration data
    shared_var.cis_cal_progressbar = 0;
    shared_var.cis_cal_state = CIS_CAL_PLACE_ON_BLACK;
    cis_ledPowerAdj(1, 1, 1);
    osDelay(20);

    cis_imageProcessRGB_Calibration(blackCal.data, iterationNb);
    SCB_CleanDCache_by_Addr((uint32_t *)&cisCals, sizeof(cisCals));
    cis_ledPowerAdj(100, 100, 100);
    osDelay(500);

    printf("------- COMPUTE AVERAGES ------\n");

    // Step 3: Compute inactive averages
    cis_ComputeCalsInactivesAvrg(&blackCal, CIS_RED);
    cis_ComputeCalsInactivesAvrg(&whiteCal, CIS_RED);
    cis_ComputeCalsInactivesAvrg(&blackCal, CIS_GREEN);
    cis_ComputeCalsInactivesAvrg(&whiteCal, CIS_GREEN);
    cis_ComputeCalsInactivesAvrg(&blackCal, CIS_BLUE);
    cis_ComputeCalsInactivesAvrg(&whiteCal, CIS_BLUE);
    SCB_CleanDCache_by_Addr((uint32_t *)&cisCals, sizeof(cisCals));
    shared_var.cis_cal_state = CIS_CAL_EXTRACT_INNACTIVE_REF;
    osDelay(200);

    printf("------ COMPUTE EXTREMUMS ------\n");

    // Step 4: Compute extremums
    cis_computeCalsExtremums(&blackCal, CIS_RED);
    cis_computeCalsExtremums(&whiteCal, CIS_RED);
    cis_computeCalsExtremums(&blackCal, CIS_GREEN);
    cis_computeCalsExtremums(&whiteCal, CIS_GREEN);
    cis_computeCalsExtremums(&blackCal, CIS_BLUE);
    cis_computeCalsExtremums(&whiteCal, CIS_BLUE);
    SCB_CleanDCache_by_Addr((uint32_t *)&cisCals, sizeof(cisCals));
    shared_var.cis_cal_state = CIS_CAL_EXTRACT_EXTREMUMS;
    osDelay(200);

    printf("------- EXTRACT OFFSETS -------\n");

    // Step 5: Compute offsets (copy black calibration data)
    cis_computeCalsOffsets(&whiteCal, &blackCal, CIS_RED);
    cis_computeCalsOffsets(&whiteCal, &blackCal, CIS_GREEN);
    cis_computeCalsOffsets(&whiteCal, &blackCal, CIS_BLUE);
    SCB_CleanDCache_by_Addr((uint32_t *)&cisCals, sizeof(cisCals));
    shared_var.cis_cal_state = CIS_CAL_EXTRACT_OFFSETS;
    osDelay(200);

    // Step 6: Compute gains in Q16.16
    cis_computeCalsGains(bitDepth, &whiteCal, &blackCal, CIS_RED);
    cis_computeCalsGains(bitDepth, &whiteCal, &blackCal, CIS_GREEN);
    cis_computeCalsGains(bitDepth, &whiteCal, &blackCal, CIS_BLUE);
    SCB_CleanDCache_by_Addr((uint32_t *)&cisCals, sizeof(cisCals));
    shared_var.cis_cal_state = CIS_CAL_COMPUTE_GAINS;
    printf("-------- COMPUTE GAINS --------\n");

    // Step 7: Save calibration data
    sprintf(calibrationFilePath, CALIBRATION_FILE_PATH_FORMAT, shared_config.cis_dpi);
    file_writeCisCals(calibrationFilePath, &cisCals);

    cis_stopCapture();
    cis_startCapture();
    shared_var.cis_cal_state = CIS_CAL_END;
    printf("-------------------------------\n");
}


/**
 * @brief       Apply linear calibration on the image buffer (integer version).
 *
 * The applied formula is:
 *      calibrated = clip( ((raw - offset) * gain) >> 16, 0, maxClipValue )
 *
 * @param       cisDataCpy    Pointer to the image buffer (int32_t in Q16.16 format).
 * @param       maxClipValue  Clipping value (e.g., 255).
 * @retval      None
 */
#pragma GCC push_options
#pragma GCC optimize ("unroll-loops")
void cis_applyLinearCalibration(int32_t * restrict cisDataCpy, uint32_t maxClipValue)
{
    for (int8_t lane = CIS_ADC_OUT_LANES; --lane >= 0; )
    {
        uint32_t baseR = (cisConfig.adc_buff_size * lane) + cisConfig.red_lane_offset;
        uint32_t baseG = (cisConfig.adc_buff_size * lane) + cisConfig.green_lane_offset;
        uint32_t baseB = (cisConfig.adc_buff_size * lane) + cisConfig.blue_lane_offset;

        for (uint32_t i = 0; i < cisConfig.pixels_per_lane; i++)
        {
            int32_t value, calibrated;

            /* Process RED channel */
            value = cisDataCpy[baseR + i] - cisCals.offsetData[baseR + i];
            calibrated = (int32_t)(((int64_t)value * cisCals.gainsData[baseR + i]) >> 16);
            cisDataCpy[baseR + i] = (calibrated < 0) ? 0 : ((calibrated > (int32_t)maxClipValue) ? (int32_t)maxClipValue : calibrated);

            /* Process GREEN channel */
            value = cisDataCpy[baseG + i] - cisCals.offsetData[baseG + i];
            calibrated = (int32_t)(((int64_t)value * cisCals.gainsData[baseG + i]) >> 16);
            cisDataCpy[baseG + i] = (calibrated < 0) ? 0 : ((calibrated > (int32_t)maxClipValue) ? (int32_t)maxClipValue : calibrated);

            /* Process BLUE channel */
            value = cisDataCpy[baseB + i] - cisCals.offsetData[baseB + i];
            calibrated = (int32_t)(((int64_t)value * cisCals.gainsData[baseB + i]) >> 16);
            cisDataCpy[baseB + i] = (calibrated < 0) ? 0 : ((calibrated > (int32_t)maxClipValue) ? (int32_t)maxClipValue : calibrated);
        }
    }
}
#pragma GCC pop_options

/**
 * @brief       Compute the average value of inactive pixels for a given color.
 * @param       currCals    Pointer to the current calibration data structure.
 * @param       color       Color channel (CIS_RED, CIS_GREEN, or CIS_BLUE).
 * @retval      None
 *
 * This function computes the mean value over the inactive region (of width CIS_INACTIVE_WIDTH)
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
            offset = cisConfig.red_lane_offset - CIS_INACTIVE_WIDTH;
            break;
        }
        case CIS_GREEN:
        {
            currColor = &currCals->green;
            offset = cisConfig.green_lane_offset - CIS_INACTIVE_WIDTH;
            break;
        }
        case CIS_BLUE:
        {
            currColor = &currCals->blue;
            offset = cisConfig.blue_lane_offset - CIS_INACTIVE_WIDTH;
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
        laneOffset = (cisConfig.adc_buff_size * lane) + offset;
        arm_mean_q31(&currCals->data[laneOffset], CIS_INACTIVE_WIDTH, &currColor->inactiveAvrgPix[lane]);
    }
}

/**
 * @brief       Compute the minimum and maximum pixel values (extremums) for a given color.
 * @param       currCals    Pointer to the current calibration data structure.
 * @param       color       Color channel (CIS_RED, CIS_GREEN, or CIS_BLUE).
 * @retval      None
 *
 * The computed extremums are stored in the corresponding members (maxPix, minPix) of the
 * cisColorsParams structure, and the delta (difference) is updated accordingly.
 */
static void cis_computeCalsExtremums(struct cisCalsTypes *currCals, CIS_Color_TypeDef color)
{
    int32_t laneOffset = 0;
    int32_t offset = 0;
    struct cisColorsParams *currColor = NULL;
    q31_t tmpMax = 0;
    q31_t tmpMin = INT32_MAX;

    switch (color)
    {
        case CIS_RED:
        {
            currColor = &currCals->red;
            offset = cisConfig.red_lane_offset;
            break;
        }
        case CIS_GREEN:
        {
            currColor = &currCals->green;
            offset = cisConfig.green_lane_offset;
            break;
        }
        case CIS_BLUE:
        {
            currColor = &currCals->blue;
            offset = cisConfig.blue_lane_offset;
            break;
        }
        default:
        {
            Error_Handler();
            return;
        }
    }

    currColor->maxPix = 0;
    currColor->minPix = INT32_MAX;

    for (int32_t lane = CIS_ADC_OUT_LANES; --lane >= 0; )
    {
        laneOffset = (cisConfig.adc_buff_size * lane) + offset;
        arm_max_q31(&currCals->data[laneOffset], cisConfig.pixels_per_lane, &tmpMax, NULL);
        arm_min_q31(&currCals->data[laneOffset], cisConfig.pixels_per_lane, &tmpMin, NULL);

        if (tmpMax > currColor->maxPix)
        {
            currColor->maxPix = tmpMax;
        }

        if (tmpMin < currColor->minPix)
        {
            currColor->minPix = tmpMin;
        }

        currColor->deltaPix = currColor->maxPix - currColor->minPix;
    }
}

/**
 * @brief       Compute calibration offsets for a given color.
 * @param       whiteCal    Pointer to the calibration data obtained on a white surface.
 * @param       blackCal    Pointer to the calibration data obtained on a black surface.
 * @param       color       Color channel (CIS_RED, CIS_GREEN, or CIS_BLUE).
 * @retval      None
 *
 * For each pixel, this function copies the measured value on the black surface into the
 * global calibration structure (cisCals.offsetData).
 */
static void cis_computeCalsOffsets(struct cisCalsTypes *whiteCal, struct cisCalsTypes *blackCal, CIS_Color_TypeDef color)
{
    uint32_t laneOffset = 0;
    uint32_t offset = 0;

    switch (color)
    {
        case CIS_RED:
        {
            offset = cisConfig.red_lane_offset;
            break;
        }
        case CIS_GREEN:
        {
            offset = cisConfig.green_lane_offset;
            break;
        }
        case CIS_BLUE:
        {
            offset = cisConfig.blue_lane_offset;
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
        laneOffset = (cisConfig.adc_buff_size * lane) + offset;
        for (int32_t i = 0; i < cisConfig.pixels_per_lane; i++)
        {
            /* Copy the measured value on the black surface */
            cisCals.offsetData[laneOffset + i] = blackCal->data[laneOffset + i];
        }
    }
}


/**
 * @brief       Compute calibration gains for a given color.
 * @param       maxADCValue     Maximum ADC value (bit depth value).
 * @param       whiteCal        Pointer to the calibration data obtained on a white surface.
 * @param       blackCal        Pointer to the calibration data obtained on a black surface.
 * @param       color           Color channel (CIS_RED, CIS_GREEN, or CIS_BLUE).
 * @retval      None
 *
 * The gain is calculated in Q16.16 format using the formula:
 *      gain = (maxADCValue << 16) / (whiteCal - blackCal)
 * If the difference is zero, the gain is set to unity (1.0 in Q16.16).
 */
static void cis_computeCalsGains(uint32_t maxADCValue, struct cisCalsTypes *whiteCal, struct cisCalsTypes *blackCal, CIS_Color_TypeDef color)
{
    uint32_t laneOffset = 0;
    uint32_t offset = 0;

    switch (color)
    {
        case CIS_RED:
        {
            offset = cisConfig.red_lane_offset;
            break;
        }
        case CIS_GREEN:
        {
            offset = cisConfig.green_lane_offset;
            break;
        }
        case CIS_BLUE:
        {
            offset = cisConfig.blue_lane_offset;
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
        laneOffset = (cisConfig.adc_buff_size * lane) + offset;
        for (int32_t i = 0; i < cisConfig.pixels_per_lane; i++)
        {
            int32_t diff = whiteCal->data[laneOffset + i] - blackCal->data[laneOffset + i];
            if (diff != 0)
            {
                /* Calculate the gain in Q16.16: gain = (maxADCValue << 16) / (white - black) */
                cisCals.gainsData[laneOffset + i] = ((int32_t)maxADCValue << 16) / diff;
            }
            else
            {
                cisCals.gainsData[laneOffset + i] = UNITY_Q16_16;
            }
        }
    }
}
