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
	uint32_t data[CIS_MAX_ADC_BUFF_SIZE * 3];
	struct cisColorsParams red;
	struct cisColorsParams green;
	struct cisColorsParams blue;
};

/* Private define ------------------------------------------------------------*/
#define UNITY_Q16_16   (1 << 16)

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
__attribute__((section(".calAccBlack")))
struct cisCalsTypes blackCal;

__attribute__((section(".calAccWhite")))
struct cisCalsTypes whiteCal;

/* Variable containing ADC conversions data */

/* Private function prototypes -----------------------------------------------*/
static void cis_mean(const uint32_t * pSrc, uint32_t blockSize, int32_t * pResult);
static void cis_max(const uint32_t * pSrc, uint32_t blockSize, int32_t * pResult, uint32_t * pIndex);
static void cis_min(const uint32_t * pSrc, uint32_t blockSize, int32_t * pResult, uint32_t * pIndex);
static void cis_ComputeCalsInactivesAvrg(struct cisCalsTypes *currCals, CIS_Color_TypeDef color);
static void cis_computeCalsExtremums(struct cisCalsTypes *currCals, CIS_Color_TypeDef color);
static void cis_computeCalsOffsets(struct cisCalsTypes *whiteCal, struct cisCalsTypes *blackCal, CIS_Color_TypeDef color);
static void cis_computeCalsGains(uint32_t maxADCValue, struct cisCalsTypes *whiteCal, struct cisCalsTypes *blackCal, CIS_Color_TypeDef color);

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

void cis_max(const uint32_t * pSrc, uint32_t blockSize, int32_t * pResult, uint32_t * pIndex)
{
    uint32_t i;
    int32_t maxVal;
    uint32_t maxIdx;

    /* On suppose blockSize >= 1 */
    maxVal = pSrc[0];
    maxIdx = 0;

    for (i = 1; i < blockSize; i++)
    {
        if (pSrc[i] > maxVal)
        {
            maxVal = pSrc[i];
            maxIdx = i;
        }
    }

    *pResult = maxVal;

    if (pIndex != NULL)
    {
        *pIndex = maxIdx;
    }
}

void cis_min(const uint32_t * pSrc, uint32_t blockSize, int32_t * pResult, uint32_t * pIndex)
{
    uint32_t i;
    int32_t minVal;
    uint32_t minIdx;

    /* On suppose blockSize >= 1 */
    minVal = pSrc[0];
    minIdx = 0;

    for (i = 1; i < blockSize; i++)
    {
        if (pSrc[i] < minVal)
        {
            minVal = pSrc[i];
            minIdx = i;
        }
    }

    *pResult = minVal;

    if (pIndex != NULL)
    {
        *pIndex = minIdx;
    }
}

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
    FIL file;
    FRESULT fres;
    char calibrationFilePath[64];

    /* Build the calibration file path according to DPI */
    sprintf(calibrationFilePath, CALIBRATION_FILE_PATH_FORMAT, shared_config.cis_dpi);

    /* Open the calibration file in read mode */
    fres = f_open(&file, calibrationFilePath, FA_READ);
    if (fres == FR_OK)
    {
    	//printf("CIS load calibration SUCCESS\n");
        /* Read the entire calibration – implement this function as needed */
        file_readCisCals(calibrationFilePath, /* pointer to your full calibration structure */ &cisCals);
        f_close(&file);
        shared_var.cis_cal_state = CIS_CAL_END;
    }
    else
    {
        printf("INT calibration file not found for %d DPI, calibration requested.\n", shared_config.cis_dpi);
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
    printf("===== CALIBRATION STARTED =====\n");
    printf("Calibration for %d DPI\n", shared_config.cis_dpi);

    char calibrationFilePath[64];

    memset(&blackCal, 0, sizeof(blackCal));
    memset(&whiteCal, 0, sizeof(whiteCal));
    memset(&cisCals, 0, sizeof(cisCals));

    // Step 1: Capture white calibration data
    cis_ledPowerAdj(100, 100, 100);
    shared_var.cis_cal_progressbar = 0;
    shared_var.cis_cal_state = CIS_CAL_PLACE_ON_WHITE;
    osDelay(200);

    cis_imageProcessRGB_Calibration(cisDataCpy, whiteCal.data, iterationNb);
    SCB_CleanDCache_by_Addr((uint32_t *)&cisCals, sizeof(cisCals));
    osDelay(200);

    // Step 2: Capture black calibration data
    shared_var.cis_cal_progressbar = 0;
    shared_var.cis_cal_state = CIS_CAL_PLACE_ON_BLACK;
    cis_ledPowerAdj(1, 1, 1);
    osDelay(20);

    cis_imageProcessRGB_Calibration(cisDataCpy, blackCal.data, iterationNb);
    SCB_CleanDCache_by_Addr((uint32_t *)&cisCals, sizeof(cisCals));
    cis_ledPowerAdj(100, 100, 100);
    osDelay(500);

    printf("Compute average\n");

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

    printf("Compute extremums\n");

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

    printf("Extract offsets\n");

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
    printf("Compute gains\n");

    // Step 7: Save calibration data
    sprintf(calibrationFilePath, CALIBRATION_FILE_PATH_FORMAT, shared_config.cis_dpi);
    file_writeCisCals(calibrationFilePath, &cisCals);

    cis_stopCapture();
    cis_startCapture();
    shared_var.cis_cal_state = CIS_CAL_END;
    printf("===============================\n");
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
        cis_mean(&currCals->data[laneOffset], CIS_INACTIVE_WIDTH, &currColor->inactiveAvrgPix[lane]);
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
        cis_max(&currCals->data[laneOffset], cisConfig.pixels_per_lane, &tmpMax, NULL);
        cis_min(&currCals->data[laneOffset], cisConfig.pixels_per_lane, &tmpMin, NULL);

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
