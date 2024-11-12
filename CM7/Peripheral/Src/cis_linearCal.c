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
	float32_t maxPix;
	float32_t minPix;
	float32_t deltaPix;
	float32_t inactiveAvrgPix[3];
};

__attribute__ ((packed))
struct cisCalsTypes {
	float32_t data[CIS_MAX_ADC_BUFF_SIZE * 3];
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
	FIL file;
	FRESULT fres;
	char calibrationFilePath[64];

	// Construire le chemin du fichier de calibration en fonction du DPI actuel
	sprintf(calibrationFilePath, CALIBRATION_FILE_PATH_FORMAT, shared_config.cis_dpi);

	// Tenter d'ouvrir le fichier de calibration en mode lecture
	fres = f_open(&file, calibrationFilePath, FA_READ);

	if (fres == FR_OK)
	{
		printf("Read calibration file for %d DPI SUCCESS\n", shared_config.cis_dpi);
		// Le fichier existe et a été ouvert avec succès
		file_readCisCals(calibrationFilePath, &cisCals);
		f_close(&file);  // Fermer le fichier après la lecture
		shared_var.cis_cal_state = CIS_CAL_END;  // Pas de calibration requise
	}
	else
	{
		// Le fichier n'existe pas ou une autre erreur s'est produite lors de l'ouverture
		printf("Calibration file not found for %d DPI, requesting new calibration.\n", shared_config.cis_dpi);
		shared_var.cis_cal_state = CIS_CAL_REQUESTED;  // Demander une nouvelle calibration
	}
}

void cis_startLinearCalibration(uint16_t iterationNb, uint32_t bitDepth)
{
	/* Set header description */
	printf("--- CIS CALIBRATION STARTED for %d DPI ---\n", shared_config.cis_dpi);

	struct cisCalsTypes blackCal;
	struct cisCalsTypes whiteCal;
	char calibrationFilePath[64];

	memset(&blackCal, 0, sizeof(blackCal));
	memset(&whiteCal, 0, sizeof(whiteCal));
	memset(&cisCals, 0, sizeof(cisCals));

	/* Step 1: Capture white calibration data */
	cis_ledPowerAdj(100, 100, 100);
	shared_var.cis_cal_progressbar = 0;
	shared_var.cis_cal_state = CIS_CAL_PLACE_ON_WHITE;
	osDelay(200);

	cis_imageProcessRGB_Calibration(whiteCal.data, iterationNb);
	SCB_CleanDCache_by_Addr((uint32_t *)&cisCals, sizeof(cisCals));

	osDelay(200);
	shared_var.cis_cal_progressbar = 0;
	shared_var.cis_cal_state = CIS_CAL_PLACE_ON_BLACK;
	cis_ledPowerAdj(1, 1, 1);
	osDelay(20);

	/* Step 2: Capture black calibration data */
	cis_imageProcessRGB_Calibration(blackCal.data, iterationNb);

	SCB_CleanDCache_by_Addr((uint32_t *)&cisCals, sizeof(cisCals));
	cis_ledPowerAdj(100, 100, 100);
	osDelay(500);

	printf("------- LOAD CALIBRATION ------\n");

	/* Step 3: Compute inactive averages */
	cis_ComputeCalsInactivesAvrg(&blackCal, CIS_RED);
	cis_ComputeCalsInactivesAvrg(&whiteCal, CIS_RED);

	cis_ComputeCalsInactivesAvrg(&blackCal, CIS_GREEN);
	cis_ComputeCalsInactivesAvrg(&whiteCal, CIS_GREEN);

	cis_ComputeCalsInactivesAvrg(&blackCal, CIS_BLUE);
	cis_ComputeCalsInactivesAvrg(&whiteCal, CIS_BLUE);

	SCB_CleanDCache_by_Addr((uint32_t *)&cisCals, sizeof(cisCals));
	shared_var.cis_cal_state = CIS_CAL_EXTRACT_INNACTIVE_REF;
	osDelay(200);

	/* Step 4: Compute extremums */
	cis_computeCalsExtremums(&blackCal, CIS_RED);
	cis_computeCalsExtremums(&whiteCal, CIS_RED);

	cis_computeCalsExtremums(&blackCal, CIS_GREEN);
	cis_computeCalsExtremums(&whiteCal, CIS_GREEN);

	cis_computeCalsExtremums(&blackCal, CIS_BLUE);
	cis_computeCalsExtremums(&whiteCal, CIS_BLUE);

	SCB_CleanDCache_by_Addr((uint32_t *)&cisCals, sizeof(cisCals));
	shared_var.cis_cal_state = CIS_CAL_EXTRACT_EXTREMUMS;
	osDelay(200);

	/* Step 5: Compute offsets */
	cis_computeCalsOffsets(&whiteCal, &blackCal, CIS_RED);
	cis_computeCalsOffsets(&whiteCal, &blackCal, CIS_GREEN);
	cis_computeCalsOffsets(&whiteCal, &blackCal, CIS_BLUE);

	SCB_CleanDCache_by_Addr((uint32_t *)&cisCals, sizeof(cisCals));
	shared_var.cis_cal_state = CIS_CAL_EXTRACT_OFFSETS;
	osDelay(200);

	/* Step 6: Compute gains */
	cis_computeCalsGains(bitDepth, &whiteCal, &blackCal, CIS_RED);
	cis_computeCalsGains(bitDepth, &whiteCal, &blackCal, CIS_GREEN);
	cis_computeCalsGains(bitDepth, &whiteCal, &blackCal, CIS_BLUE);

	SCB_CleanDCache_by_Addr((uint32_t *)&cisCals, sizeof(cisCals));
	shared_var.cis_cal_state = CIS_CAL_COMPUTE_GAINS;

	printf("-------- COMPUTE GAINS --------\n");

	/* Step 7: Save calibration data */
	sprintf(calibrationFilePath, CALIBRATION_FILE_PATH_FORMAT, shared_config.cis_dpi);
	file_writeCisCals(calibrationFilePath, &cisCals);

	cis_stopCapture();
	cis_startCapture();
	shared_var.cis_cal_state = CIS_CAL_END;
	printf("-------------------------------\n");
}

#pragma GCC push_options
#pragma GCC optimize ("unroll-loops")
void cis_applyLinearCalibration(float32_t* cisDataCpy_f32, uint32_t maxClipValue)
{
#ifndef CIS_DESACTIVATE_CALIBRATION
	uint32_t dataOffset_Rx, dataOffset_Gx, dataOffset_Bx;

	for (int8_t lane = CIS_ADC_OUT_LANES; --lane >= 0;)
	{
		dataOffset_Rx = (cisConfig.adc_buff_size * lane) + cisConfig.red_lane_offset;     // Rx
		dataOffset_Gx = (cisConfig.adc_buff_size * lane) + cisConfig.green_lane_offset;   // Gx
		dataOffset_Bx = (cisConfig.adc_buff_size * lane) + cisConfig.blue_lane_offset;    // Bx

		arm_sub_f32(&cisDataCpy_f32[dataOffset_Rx], &cisCals.offsetData[dataOffset_Rx], &cisDataCpy_f32[dataOffset_Rx], cisConfig.pixels_per_lane);
		arm_sub_f32(&cisDataCpy_f32[dataOffset_Gx], &cisCals.offsetData[dataOffset_Gx], &cisDataCpy_f32[dataOffset_Gx], cisConfig.pixels_per_lane);
		arm_sub_f32(&cisDataCpy_f32[dataOffset_Bx], &cisCals.offsetData[dataOffset_Bx], &cisDataCpy_f32[dataOffset_Bx], cisConfig.pixels_per_lane);

		arm_mult_f32(&cisDataCpy_f32[dataOffset_Rx], &cisCals.gainsData[dataOffset_Rx], &cisDataCpy_f32[dataOffset_Rx], cisConfig.pixels_per_lane);
		arm_mult_f32(&cisDataCpy_f32[dataOffset_Gx], &cisCals.gainsData[dataOffset_Gx], &cisDataCpy_f32[dataOffset_Gx], cisConfig.pixels_per_lane);
		arm_mult_f32(&cisDataCpy_f32[dataOffset_Bx], &cisCals.gainsData[dataOffset_Bx], &cisDataCpy_f32[dataOffset_Bx], cisConfig.pixels_per_lane);

		arm_clip_f32(&cisDataCpy_f32[dataOffset_Rx], &cisDataCpy_f32[dataOffset_Rx], 0, maxClipValue, cisConfig.pixels_per_lane);
		arm_clip_f32(&cisDataCpy_f32[dataOffset_Gx], &cisDataCpy_f32[dataOffset_Gx], 0, maxClipValue, cisConfig.pixels_per_lane);
		arm_clip_f32(&cisDataCpy_f32[dataOffset_Bx], &cisDataCpy_f32[dataOffset_Bx], 0, maxClipValue, cisConfig.pixels_per_lane);
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
static void cis_ComputeCalsInactivesAvrg(struct cisCalsTypes *currCisCals, CIS_Color_TypeDef color)
{
	int32_t laneOffset = 0, offset = 0;
	struct cisColorsParams *currColor;

	switch (color)
	{
	case CIS_RED :
		currColor = &currCisCals->red;
		offset = cisConfig.red_lane_offset - CIS_INACTIVE_WIDTH;
		break;
	case CIS_GREEN :
		currColor = &currCisCals->green;
		offset = cisConfig.green_lane_offset - CIS_INACTIVE_WIDTH;
		break;
	case CIS_BLUE :
		currColor = &currCisCals->blue;
		offset = cisConfig.blue_lane_offset - CIS_INACTIVE_WIDTH;
		break;
	default :
		Error_Handler();
		return;
	}

	for (int32_t lane = CIS_ADC_OUT_LANES; --lane >= 0;)
	{
		laneOffset = (cisConfig.adc_buff_size * lane) + offset;
		// Extract average value for inactive pixels
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
static void cis_computeCalsExtremums(struct cisCalsTypes *currCisCals, CIS_Color_TypeDef color)
{
	float32_t tmpMaxpix = 0, tmpMinpix = 0;
	int32_t laneOffset = 0, offset = 0;
	struct cisColorsParams *currColor;

	switch (color)
	{
	case CIS_RED :
		currColor = &currCisCals->red;
		offset = cisConfig.red_lane_offset;
		break;
	case CIS_GREEN :
		currColor = &currCisCals->green;
		offset = cisConfig.green_lane_offset;
		break;
	case CIS_BLUE :
		currColor = &currCisCals->blue;
		offset = cisConfig.blue_lane_offset;
		break;
	default :
		Error_Handler();
		return;
	}

	currColor->maxPix = 0;
	currColor->minPix = 0xFFFF;

	for (int32_t lane = CIS_ADC_OUT_LANES; --lane >= 0;)
	{
		laneOffset = (cisConfig.adc_buff_size * lane) + offset;
		// Extract Max, Min, and Delta
		arm_max_f32(&currCisCals->data[laneOffset], cisConfig.pixels_per_lane, &tmpMaxpix, NULL);
		arm_min_f32(&currCisCals->data[laneOffset], cisConfig.pixels_per_lane, &tmpMinpix, NULL);

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
static void cis_computeCalsOffsets(struct cisCalsTypes *whiteCal, struct cisCalsTypes *blackCal, CIS_Color_TypeDef color)
{
	uint32_t laneOffset = 0, offset = 0;

	switch (color)
	{
	case CIS_RED :
		offset = cisConfig.red_lane_offset;
		break;
	case CIS_GREEN :
		offset = cisConfig.green_lane_offset;
		break;
	case CIS_BLUE :
		offset = cisConfig.blue_lane_offset;
		break;
	default :
		Error_Handler();
		return;
	}

	for (int32_t lane = CIS_ADC_OUT_LANES; --lane >= 0;)
	{
		laneOffset = (cisConfig.adc_buff_size * lane) + offset;

		// Extract differential offsets
		for (int32_t i = cisConfig.pixels_per_lane; --i >= 0;)
		{
			cisCals.offsetData[laneOffset + i] = blackCal->data[laneOffset + i];
		}
	}
}

/**
 * @brief  CIS compute differential offsets
 * @param  current color calibration
 * @retval None
 */
static void cis_computeCalsGains(uint32_t maxADCValue, struct cisCalsTypes *whiteCal, struct cisCalsTypes *blackCal, CIS_Color_TypeDef color)
{
	uint32_t laneOffset = 0, offset;

	switch (color)
	{
	case CIS_RED :
		offset = cisConfig.red_lane_offset;
		break;
	case CIS_GREEN :
		offset = cisConfig.green_lane_offset;
		break;
	case CIS_BLUE :
		offset = cisConfig.blue_lane_offset;
		break;
	default :
		Error_Handler();
		return;
	}

	for (int32_t lane = CIS_ADC_OUT_LANES; --lane >= 0;)
	{
		laneOffset = (cisConfig.adc_buff_size * lane) + offset;

		// Compute gains
		for (int32_t i = cisConfig.pixels_per_lane; --i >= 0;)
		{
			float32_t diff = whiteCal->data[laneOffset + i] - blackCal->data[laneOffset + i];
			if (diff != 0)
			{
				cisCals.gainsData[laneOffset + i] = (float32_t)(maxADCValue) / diff;
			}
			else
			{
				cisCals.gainsData[laneOffset + i] = 1.0f; // Avoid division by zero
			}
		}
	}
}

/**
 * @brief  CIS start characterisation
 * @retval None
 */
void cis_printForCharacterization(float32_t* cisDataCpy_f32)
{
	/* Set header description */
	printf("--- START CHARACTERISATION ----\n");

	// Start calibration with specified min and max values
	cis_startLinearCalibration(100, 4095);

	// Declare and initialize a RAWImage structure
	static struct RAWImage RAWImage = {0};

	// Loop over LED intensities
	for (int ledIntensity = 1; ledIntensity < 100; ledIntensity += 10 )
	{
		// Adjust LED power for red channel and print intensity
		cis_ledPowerAdj(ledIntensity, ledIntensity, ledIntensity);
		printf("\n Red LED intensity = %d\n", ledIntensity);

		// Loop for red channel
		for (int i = 0; i < 5; i++)
		{
			printf("\n Red line %d\n", i);
			cis_getRAWImage(cisDataCpy_f32, 1);
			cis_applyLinearCalibration(cisDataCpy_f32, 4095);
			cis_convertRAWImageToFloatArray(cisDataCpy_f32, &RAWImage);

			SCB_CleanDCache_by_Addr((uint32_t *)&RAWImage, sizeof(RAWImage));

			for (int print = 0; print < cisConfig.pixels_nb; print++)
			{
				printf(",%d", (int)RAWImage.redLine[print]);
			}
		}

		// Adjust LED power for green channel and print intensity
		cis_ledPowerAdj(ledIntensity, ledIntensity, ledIntensity);
		printf("\n Green LED intensity = %d\n", ledIntensity);

		// Loop for green channel
		for (int i = 0; i < 5; i++)
		{
			printf("\n Green line %d\n", i);
			cis_getRAWImage(cisDataCpy_f32, 2);
			cis_applyLinearCalibration(cisDataCpy_f32, 4095);
			cis_convertRAWImageToFloatArray(cisDataCpy_f32, &RAWImage);

			SCB_CleanDCache_by_Addr((uint32_t *)&RAWImage, sizeof(RAWImage));

			for (int print = 0; print < cisConfig.pixels_nb; print++)
			{
				printf(",%d", (int)RAWImage.greenLine[print]);
			}
		}

		// Adjust LED power for blue channel and print intensity
		cis_ledPowerAdj(ledIntensity, ledIntensity, ledIntensity);
		printf("\n Blue LED intensity = %d\n", ledIntensity);

		// Loop for blue channel
		for (int i = 0; i < 5; i++)
		{
			printf("\n Blue line %d\n", i);
			cis_getRAWImage(cisDataCpy_f32, 3);
			cis_applyLinearCalibration(cisDataCpy_f32, 4095);
			cis_convertRAWImageToFloatArray(cisDataCpy_f32, &RAWImage);

			SCB_CleanDCache_by_Addr((uint32_t *)&RAWImage, sizeof(RAWImage));

			for (int print = 0; print < cisConfig.pixels_nb; print++)
			{
				printf(",%d", (int)RAWImage.blueLine[print]);
			}
		}
	}

	printf("\n-------------------------------\n");
}
