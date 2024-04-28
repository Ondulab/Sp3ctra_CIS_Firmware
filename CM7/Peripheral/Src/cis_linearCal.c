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
#include "shared.h"

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
	float32_t maxPix;
	float32_t minPix;
	float32_t deltaPix;
	float32_t inactiveAvrgPix[3];
};

__attribute__ ((packed))
struct cisCalsTypes {
	float32_t data[CIS_ADC_BUFF_SIZE * 3];
	struct cisColorsParams red;
	struct cisColorsParams green;
	struct cisColorsParams blue;
};

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Variable containing ADC conversions data */

/* Private function prototypes -----------------------------------------------*/
static void cis_ComputeCalsInactivesAvrg(struct cisCalsTypes *currCisCals, CIS_Color_TypeDef color);
static void cis_computeCalsExtremums(struct cisCalsTypes *currCisCals, CIS_Color_TypeDef color);
static void cis_computeCalsOffsets(struct cisCalsTypes *whiteCal, struct cisCalsTypes *blackCal, CIS_Color_TypeDef color);
static void cis_computeCalsGains(uint32_t maxADCValue, struct cisCalsTypes *whiteCal, struct cisCalsTypes *blackCal, CIS_Color_TypeDef color);

/* Private user code ---------------------------------------------------------*/

void cis_linearCalibrationInit()
{
    FIL file;              // File object for FatFs
    FRESULT fres;          // Variable to store the result of FatFs operations

    // Attempt to open the calibration file in read mode
    fres = f_open(&file, CALIBRATION_FILE_PATH, FA_READ);

    if (fres == FR_OK)
    {
        // The file exists and has been opened successfully
        file_readCisCals(CALIBRATION_FILE_PATH, &cisCals);
        f_close(&file);  // Close the file after reading
        shared_var.cis_cal_state = CIS_CAL_END;  // No calibration requested
    }
    else
    {
        // The file does not exist or another file opening error occurred
        printf("Calibration file not found, requesting new calibration.\n");
        shared_var.cis_cal_state = CIS_CAL_REQUESTED;  // Request new calibration
    }
}

#pragma GCC push_options
#pragma GCC optimize ("unroll-loops")
void cis_applyLinearCalibration(float32_t* cisDataCpy_f32, uint32_t maxClipValue)
{
#ifndef CIS_DESACTIVATE_CALIBRATION
	static uint32_t dataOffset_Rx, dataOffset_Gx, dataOffset_Bx;

	for (int8_t lane = CIS_ADC_OUT_LANES; --lane >= 0;)
	{
		dataOffset_Rx = (CIS_ADC_BUFF_SIZE * lane) + CIS_RED_LANE_OFFSET;									//Rx
		dataOffset_Gx = (CIS_ADC_BUFF_SIZE * lane) + CIS_GREEN_LANE_OFFSET;									//Gx
		dataOffset_Bx = (CIS_ADC_BUFF_SIZE * lane) + CIS_BLUE_LANE_OFFSET;									//Bx

		arm_sub_f32(&cisDataCpy_f32[dataOffset_Rx], &cisCals.offsetData[dataOffset_Rx], &cisDataCpy_f32[dataOffset_Rx], CIS_PIXELS_PER_LANE);
		arm_sub_f32(&cisDataCpy_f32[dataOffset_Gx], &cisCals.offsetData[dataOffset_Gx], &cisDataCpy_f32[dataOffset_Gx], CIS_PIXELS_PER_LANE);
		arm_sub_f32(&cisDataCpy_f32[dataOffset_Bx], &cisCals.offsetData[dataOffset_Bx], &cisDataCpy_f32[dataOffset_Bx], CIS_PIXELS_PER_LANE);

		arm_mult_f32(&cisDataCpy_f32[dataOffset_Rx], &cisCals.gainsData[dataOffset_Rx], &cisDataCpy_f32[dataOffset_Rx], CIS_PIXELS_PER_LANE);
		arm_mult_f32(&cisDataCpy_f32[dataOffset_Gx], &cisCals.gainsData[dataOffset_Gx], &cisDataCpy_f32[dataOffset_Gx], CIS_PIXELS_PER_LANE);
		arm_mult_f32(&cisDataCpy_f32[dataOffset_Bx], &cisCals.gainsData[dataOffset_Bx], &cisDataCpy_f32[dataOffset_Bx], CIS_PIXELS_PER_LANE);

		arm_clip_f32(&cisDataCpy_f32[dataOffset_Rx], &cisDataCpy_f32[dataOffset_Rx], 0, maxClipValue, CIS_PIXELS_PER_LANE);
		arm_clip_f32(&cisDataCpy_f32[dataOffset_Gx], &cisDataCpy_f32[dataOffset_Gx], 0, maxClipValue, CIS_PIXELS_PER_LANE);
		arm_clip_f32(&cisDataCpy_f32[dataOffset_Bx], &cisDataCpy_f32[dataOffset_Bx], 0, maxClipValue, CIS_PIXELS_PER_LANE);
	}
#endif
}
#pragma GCC pop_options

/**
 * @brief  CIS get inactive pixels average
 * @param  current calibration type : white or black
 * @param  current color calibration
 * @retval None
 */
void cis_ComputeCalsInactivesAvrg(struct cisCalsTypes *currCisCals, CIS_Color_TypeDef color)
{
	int32_t laneOffset = 0, offset = 0;
	struct cisColorsParams *currColor;

	switch (color)
	{
	case CIS_RED :
		currColor = &currCisCals->red;
		offset = CIS_RED_LANE_OFFSET - CIS_INACTIVE_WIDTH;
		break;
	case CIS_GREEN :
		currColor = &currCisCals->green;
		offset = CIS_GREEN_LANE_OFFSET - CIS_INACTIVE_WIDTH;
		break;
	case CIS_BLUE :
		currColor = &currCisCals->blue;
		offset = CIS_BLUE_LANE_OFFSET - CIS_INACTIVE_WIDTH;
		break;
	default :
		Error_Handler();
		return;
	}

	for (int32_t lane = CIS_ADC_OUT_LANES; --lane >= 0;)
	{
		laneOffset = (CIS_ADC_BUFF_SIZE * lane) + offset;
		// Extrat average value for innactives pixels
		arm_mean_f32(&currCisCals->data[laneOffset], CIS_INACTIVE_WIDTH, &currColor->inactiveAvrgPix[lane]);
	}

#ifdef PRINT_CIS_CALIBRATION
	printf("Avrg    Pix = %d\n", (int)currColor->inactiveAvrgPix);
	printf("-------------------------------\n");
#endif
}

/**
 * @brief  CIS get Min Max and Delta
 * @param  current calibration type : white or black
 * @param  current color calibration
 * @retval None
 */
void cis_computeCalsExtremums(struct cisCalsTypes *currCisCals, CIS_Color_TypeDef color)
{
	float32_t tmpMaxpix = 0, tmpMinpix = 0;
	int32_t laneOffset = 0, offset = 0;
	struct cisColorsParams *currColor;

	switch (color)
	{
	case CIS_RED :
		currColor = &currCisCals->red;
		offset = CIS_RED_LANE_OFFSET;
		break;
	case CIS_GREEN :
		currColor = &currCisCals->green;
		offset = CIS_GREEN_LANE_OFFSET;
		break;
	case CIS_BLUE :
		currColor = &currCisCals->blue;
		offset = CIS_BLUE_LANE_OFFSET;
		break;
	default :
		Error_Handler();
		return;
	}

	currColor->maxPix = 0;
	currColor->minPix = 0xFFFF;

	for (int32_t lane = CIS_ADC_OUT_LANES; --lane >= 0;)
	{
		laneOffset = (CIS_ADC_BUFF_SIZE * lane) + offset;
		// Extrat Min Max and delta
		arm_max_f32(&currCisCals->data[laneOffset], CIS_PIXELS_PER_LANE, &tmpMaxpix, NULL);
		arm_min_f32(&currCisCals->data[laneOffset], CIS_PIXELS_PER_LANE, &tmpMinpix, NULL);

		if (tmpMaxpix >= currColor->maxPix)
			currColor->maxPix = tmpMaxpix;

		if (tmpMinpix <= currColor->minPix)
			currColor->minPix = tmpMinpix;

		currColor->deltaPix = currColor->maxPix - currColor->minPix;
	}

#ifdef PRINT_CIS_CALIBRATION
	printf("Max     Pix = %d\n", (int)currColor->maxPix);
	printf("Min     Pix = %d\n", (int)currColor->minPix);
	printf("Delta   Pix = %d\n", (int)currColor->deltaPix);
	printf("-------------------------------\n");
#endif
}

/**
 * @brief  CIS compute differential offsets
 * @param  current color calibration
 * @retval None
 */
void cis_computeCalsOffsets(struct cisCalsTypes *whiteCal, struct cisCalsTypes *blackCal, CIS_Color_TypeDef color)
{
	uint32_t laneOffset = 0, offset = 0;
	//struct cisColorsParams *currColor;

	switch (color)
	{
	case CIS_RED :
		//currColor = &blackCal->red;
		offset = CIS_RED_LANE_OFFSET;
		break;
	case CIS_GREEN :
		//currColor = &blackCal->green;
		offset = CIS_GREEN_LANE_OFFSET;
		break;
	case CIS_BLUE :
		//currColor = &blackCal->blue;
		offset = CIS_BLUE_LANE_OFFSET;
		break;
	default :
		Error_Handler();
		return;
	}

	for (int32_t lane = CIS_ADC_OUT_LANES; --lane >= 0;)
	{
		laneOffset = (CIS_ADC_BUFF_SIZE * lane) + offset;

		// Extract differential offsets
		for (int32_t i = CIS_PIXELS_NB / CIS_ADC_OUT_LANES; --i >= 0;)
		{
			//cisCals.offsetData[laneOffset + i] = currColor->maxPix - cisCals.blackCal.data[laneOffset + i];
			cisCals.offsetData[laneOffset + i] = blackCal->data[laneOffset + i];
		}
	}
}

/**
 * @brief  CIS compute gains
 * @param  current color calibration
 * @retval None
 */
void cis_computeCalsGains(uint32_t maxADCValue, struct cisCalsTypes *whiteCal, struct cisCalsTypes *blackCal, CIS_Color_TypeDef color)
{
	uint32_t laneOffset = 0, offset;

	switch (color)
	{
	case CIS_RED :
		offset = CIS_RED_LANE_OFFSET;
		break;
	case CIS_GREEN :
		offset = CIS_GREEN_LANE_OFFSET;
		break;
	case CIS_BLUE :
		offset = CIS_BLUE_LANE_OFFSET;
		break;
	default :
		Error_Handler();
		return;
	}

	for (int32_t lane = CIS_ADC_OUT_LANES; --lane >= 0;)
	{
		laneOffset = (CIS_ADC_BUFF_SIZE * lane) + offset;

		// Extract differential offsets
		for (int32_t i = CIS_PIXELS_NB / CIS_ADC_OUT_LANES; --i >= 0;)
		{
			cisCals.gainsData[laneOffset + i] = (float32_t)(maxADCValue) / (float32_t)(whiteCal->data[laneOffset + i] - blackCal->data[laneOffset + i]);
		}
	}
}

/**
 * @brief  CIS start calibration
 * @param  calibration iteration
 * @retval None
 */
void cis_startLinearCalibration(uint16_t iterationNb, uint32_t bitDepth)
{
	/* Set header description */
	printf("--- CIS CALIBRATION STARTED ---\n");
	                                          //

	struct cisCalsTypes blackCal;
	struct cisCalsTypes whiteCal;

	memset(&blackCal, 0, sizeof(blackCal));
	memset(&whiteCal, 0, sizeof(whiteCal));
	memset(&cisCals, 0, sizeof(cisCals));

	/*-------- 1 --------*/
	// Read black and white level
	cis_ledPowerAdj(100, 100, 100);
	shared_var.cis_cal_progressbar = 0;
	shared_var.cis_cal_state = CIS_CAL_PLACE_ON_WHITE;
	HAL_Delay(200);

	cis_imageProcessRGB_Calibration(whiteCal.data, iterationNb);
	SCB_CleanDCache_by_Addr((uint32_t *)&cisCals, sizeof(cisCals) * (sizeof(uint32_t)));

	HAL_Delay(200);
	shared_var.cis_cal_progressbar = 0;
	shared_var.cis_cal_state = CIS_CAL_PLACE_ON_BLACK;
	cis_ledPowerAdj(1, 1, 1);
	HAL_Delay(20);

	cis_imageProcessRGB_Calibration(blackCal.data, iterationNb);

	SCB_CleanDCache_by_Addr((uint32_t *)&cisCals, sizeof(cisCals) * (sizeof(uint32_t)));
	cis_ledPowerAdj(100, 100, 100);
	HAL_Delay(500);

	printf("------- LOAD CALIBRATION ------\n");
	/*-------- 1 --------*/
	// Extrat Min Max and delta
	cis_ComputeCalsInactivesAvrg(&blackCal, CIS_RED);
	cis_ComputeCalsInactivesAvrg(&whiteCal, CIS_RED);

	cis_ComputeCalsInactivesAvrg(&blackCal, CIS_GREEN);
	cis_ComputeCalsInactivesAvrg(&whiteCal, CIS_GREEN);

	cis_ComputeCalsInactivesAvrg(&blackCal, CIS_BLUE);
	cis_ComputeCalsInactivesAvrg(&whiteCal, CIS_BLUE);

	SCB_CleanDCache_by_Addr((uint32_t *)&cisCals, sizeof(cisCals) * (sizeof(uint32_t)));
	shared_var.cis_cal_state = CIS_CAL_EXTRACT_INNACTIVE_REF;
	HAL_Delay(200);

	/*-------- 2 --------*/
	// Extrat Min Max and delta
	cis_computeCalsExtremums(&blackCal, CIS_RED);
	cis_computeCalsExtremums(&whiteCal, CIS_RED);

	cis_computeCalsExtremums(&blackCal, CIS_GREEN);
	cis_computeCalsExtremums(&whiteCal, CIS_GREEN);

	cis_computeCalsExtremums(&blackCal, CIS_BLUE);
	cis_computeCalsExtremums(&whiteCal, CIS_BLUE);

	SCB_CleanDCache_by_Addr((uint32_t *)&cisCals, sizeof(cisCals) * (sizeof(uint32_t)));
	shared_var.cis_cal_state = CIS_CAL_EXTRACT_EXTREMUMS;
	HAL_Delay(200);

	/*-------- 3 --------*/
	// Extract differential offsets
	cis_computeCalsOffsets(&whiteCal, &blackCal, CIS_RED);
	cis_computeCalsOffsets(&whiteCal, &blackCal, CIS_GREEN);
	cis_computeCalsOffsets(&whiteCal, &blackCal, CIS_BLUE);

	SCB_CleanDCache_by_Addr((uint32_t *)&cisCals, sizeof(cisCals) * (sizeof(uint32_t)));
	shared_var.cis_cal_state = CIS_CAL_EXTRACT_OFFSETS;
	HAL_Delay(200);

	/*-------- 4 --------*/
	// Compute gains
	cis_computeCalsGains(bitDepth, &whiteCal, &blackCal, CIS_RED);
	cis_computeCalsGains( bitDepth, &whiteCal, &blackCal, CIS_GREEN);
	cis_computeCalsGains( bitDepth, &whiteCal, &blackCal, CIS_BLUE);

	SCB_CleanDCache_by_Addr((uint32_t *)&cisCals, sizeof(cisCals) * (sizeof(uint32_t)));
	shared_var.cis_cal_state = CIS_CAL_COMPUTE_GAINS;

	printf("-------- COMPUTE GAINS --------\n");
#ifdef PRINT_CIS_CALIBRATION
	for (uint32_t pix = 0; pix < CIS_PIXELS_NB; pix++)
	{
		printf("Pix = %d, Val = %0.3f\n", (int)pix, (float)cisCals.gainsData[pix]);
	}
#endif

	SCB_CleanDCache_by_Addr((uint32_t *)&cisCals, sizeof(cisCals) * (sizeof(uint32_t)));
	cis_stopCapture();
	//stm32_flashCalibrationRW(CIS_WRITE_CAL);
	file_writeCisCals(CALIBRATION_FILE_PATH, &cisCals);
	cis_startCapture();
	shared_var.cis_cal_state = CIS_CAL_END;
	printf("-------------------------------\n");
}

/**
 * @brief  CIS start characterisation
 * @retval None
 */
void cis_printForCharacterization(float32_t* cisDataCpy_f32)
{
	/* Set header description */
	printf("--- START CHARACTERISATION ----\n");

	cis_startLinearCalibration(100, 4095);

	static struct RAWImage RAWImage = {0};

	for (int ledIntensity = 1; ledIntensity < 100; ledIntensity+=10 )
	{
		cis_ledPowerAdj(ledIntensity, ledIntensity, ledIntensity);

		printf("\n Red led intensity = %d\n", ledIntensity);
		for (int i = 0; i < 5; i++)
		{
			printf("\n Red line %d\n", i);
			cis_getRAWImage(cisDataCpy_f32, 1);
			cis_applyLinearCalibration(cisDataCpy_f32, 4095);
			cis_convertRAWImageToFloatArray(cisDataCpy_f32, &RAWImage);

			SCB_CleanDCache_by_Addr((uint32_t *)&RAWImage, sizeof(RAWImage) * (sizeof(uint32_t)));

			for (int print = 0; print < CIS_PIXELS_NB; print++)
			{
				printf(",%d", (int)RAWImage.redLine[print]);
			}
		}
	}

	for (int ledIntensity = 1; ledIntensity < 100; ledIntensity+=10 )
	{
		cis_ledPowerAdj(ledIntensity, ledIntensity, ledIntensity);

		printf("\n Green led intensity = %d\n", ledIntensity);
		for (int i = 0; i < 5; i++)
		{
			printf("\n Green line %d\n", i);
			cis_getRAWImage(cisDataCpy_f32, 1);
			cis_applyLinearCalibration(cisDataCpy_f32, 4095);
			cis_convertRAWImageToFloatArray(cisDataCpy_f32, &RAWImage);

			SCB_CleanDCache_by_Addr((uint32_t *)&RAWImage, sizeof(RAWImage) * (sizeof(uint32_t)));

			for (int print = 0; print < CIS_PIXELS_NB; print++)
			{
				printf(",%d", (int)RAWImage.greenLine[print]);
			}
		}
	}

	for (int ledIntensity = 1; ledIntensity < 100; ledIntensity+=10 )
	{
		cis_ledPowerAdj(ledIntensity, ledIntensity, ledIntensity);

		printf("\n Blue led intensity = %d\n", ledIntensity);
		for (int i = 0; i < 5; i++)
		{
			printf("\n Blue line %d\n", i);
			cis_getRAWImage(cisDataCpy_f32, 1);
			cis_applyLinearCalibration(cisDataCpy_f32, 4095);
			cis_convertRAWImageToFloatArray(cisDataCpy_f32, &RAWImage);

			SCB_CleanDCache_by_Addr((uint32_t *)&RAWImage, sizeof(RAWImage) * (sizeof(uint32_t)));

			for (int print = 0; print < CIS_PIXELS_NB; print++)
			{
				printf(",%d", (int)RAWImage.blueLine[print]);
			}
		}
	}

	printf("\n-------------------------------\n");
}
