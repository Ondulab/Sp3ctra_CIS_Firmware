#if 0
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

    // Build the calibration file path based on the current DPI setting
    sprintf(calibrationFilePath, CALIBRATION_FILE_PATH_FORMAT, shared_config.cis_dpi);

    // Attempt to open the calibration file in read mode
    fres = f_open(&file, calibrationFilePath, FA_READ);

    if (fres == FR_OK)
    {
        printf("Read calibration file for %d DPI SUCCESS\n", shared_config.cis_dpi);
        // File exists and was successfully opened
        file_readCisCals(calibrationFilePath, &cisCals);
        f_close(&file);  // Close the file after reading
        shared_var.cis_cal_state = CIS_CAL_END;  // No calibration needed
    }
    else
    {
        // File not found or an error occurred while opening
        printf("Calibration file not found for %d DPI, requesting new calibration.\n", shared_config.cis_dpi);
        shared_var.cis_cal_state = CIS_CAL_REQUESTED;  // Request new calibration
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
	SCB_CleanDCache_by_Addr((uint32_t *)&cisCals, sizeof(cisCals) * (sizeof(uint32_t)));

	osDelay(200);
	shared_var.cis_cal_progressbar = 0;
	shared_var.cis_cal_state = CIS_CAL_PLACE_ON_BLACK;
	cis_ledPowerAdj(1, 1, 1);
	osDelay(20);

	/* Step 2: Capture black calibration data */
	cis_imageProcessRGB_Calibration(blackCal.data, iterationNb);

	SCB_CleanDCache_by_Addr((uint32_t *)&cisCals, sizeof(cisCals) * (sizeof(uint32_t)));
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

	SCB_CleanDCache_by_Addr((uint32_t *)&cisCals, sizeof(cisCals) * (sizeof(uint32_t)));
	shared_var.cis_cal_state = CIS_CAL_EXTRACT_INNACTIVE_REF;
	osDelay(200);

	/* Step 4: Compute extremums */
	cis_computeCalsExtremums(&blackCal, CIS_RED);
	cis_computeCalsExtremums(&whiteCal, CIS_RED);

	cis_computeCalsExtremums(&blackCal, CIS_GREEN);
	cis_computeCalsExtremums(&whiteCal, CIS_GREEN);

	cis_computeCalsExtremums(&blackCal, CIS_BLUE);
	cis_computeCalsExtremums(&whiteCal, CIS_BLUE);

	SCB_CleanDCache_by_Addr((uint32_t *)&cisCals, sizeof(cisCals) * (sizeof(uint32_t)));
	shared_var.cis_cal_state = CIS_CAL_EXTRACT_EXTREMUMS;
	osDelay(200);

	/* Step 5: Compute offsets */
	cis_computeCalsOffsets(&whiteCal, &blackCal, CIS_RED);
	cis_computeCalsOffsets(&whiteCal, &blackCal, CIS_GREEN);
	cis_computeCalsOffsets(&whiteCal, &blackCal, CIS_BLUE);

	SCB_CleanDCache_by_Addr((uint32_t *)&cisCals, sizeof(cisCals) * (sizeof(uint32_t)));
	shared_var.cis_cal_state = CIS_CAL_EXTRACT_OFFSETS;
	osDelay(200);

	/* Step 6: Compute gains */
	cis_computeCalsGains(bitDepth, &whiteCal, &blackCal, CIS_RED);
	cis_computeCalsGains(bitDepth, &whiteCal, &blackCal, CIS_GREEN);
	cis_computeCalsGains(bitDepth, &whiteCal, &blackCal, CIS_BLUE);

	SCB_CleanDCache_by_Addr((uint32_t *)&cisCals, sizeof(cisCals) * (sizeof(uint32_t)));
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
#endif






/**
 ******************************************************************************
 * @file           : cis_linearCal_int.c
 * @brief          : Calibration linéaire en arithmétique entière (Q16.16)
 *                   pour le CIS, version « int » collée à vos déclarations.
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "arm_math.h"
#include "file_manager.h"
#include "cis.h"
#include "cis_linearCal.h"  // Déclarations de ces fonctions (à créer)

#define UNITY_Q16_16   (1 << 16)

/* ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––
   Définition d’une structure de calibration en entier (Q16.16)
––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––– */
__attribute__ ((packed))
struct cisCalsTypes_int {
    int32_t data[CIS_MAX_ADC_BUFF_SIZE * 3];      // Données d'acquisition (en Q16.16)
    int32_t offsetData[CIS_MAX_ADC_BUFF_SIZE * 3];  // Offsets calculés (en Q16.16)
    int32_t gainsData[CIS_MAX_ADC_BUFF_SIZE * 3];   // Gains calculés (en Q16.16)
};

/* Pour simplifier, on utilisera la même nomenclature pour les moyennes et
   les extrêmes. La structure suivante est utilisée dans les fonctions internes. */
__attribute__ ((packed))
struct cisColorsParams_int {
    int32_t maxPix;                  // Valeur max (Q16.16)
    int32_t minPix;                  // Valeur min (Q16.16)
    int32_t deltaPix;                // Différence (Q16.16)
    int32_t inactiveAvrgPix[CIS_ADC_OUT_LANES];  // Moyenne des pixels inactifs par lane (Q16.16)
};

/* ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––
   Déclarations internes de nos fonctions de calibration en entier
––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––– */
static void cis_ComputeCalsInactivesAvrg_int(struct cisCalsTypes_int *currCals, CIS_Color_TypeDef color);
static void cis_computeCalsExtremums_int(struct cisCalsTypes_int *currCals, CIS_Color_TypeDef color);
static void cis_computeCalsOffsets_int(struct cisCalsTypes_int *whiteCal, struct cisCalsTypes_int *blackCal, CIS_Color_TypeDef color);
static void cis_computeCalsGains_int(uint32_t maxADCValue, struct cisCalsTypes_int *whiteCal, struct cisCalsTypes_int *blackCal, CIS_Color_TypeDef color);

/* ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––
   Fonctions Exportées
–––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––*/

/**
 * @brief Initialise la calibration linéaire en entier.
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
 * @brief Démarre la calibration linéaire en entier.
 * @param iterationNb : Nombre d'itérations pour la moyenne
 * @param bitDepth : Plage maximale (par exemple 4095)
 */
void cis_startLinearCalibration_int(uint16_t iterationNb, uint32_t bitDepth)
{
    //printf("--- CALIBRATION INT DÉMARRÉE pour %d DPI ---\n", shared_config.cis_dpi);

    struct cisCalsTypes_int blackCal;
    struct cisCalsTypes_int whiteCal;
    char calibrationFilePath[64];

    memset(&blackCal, 0, sizeof(blackCal));
    memset(&whiteCal, 0, sizeof(whiteCal));
    /* On réinitialise la structure de calibration globale.
       (Selon votre projet, vous pouvez stocker la calibration entière dans une autre variable.) */
    memset(&cisCals, 0, sizeof(cisCals));

    /* Étape 1 : Acquisition des données sur surface blanche */
    cis_ledPowerAdj(100, 100, 100);
    shared_var.cis_cal_progressbar = 0;
    shared_var.cis_cal_state = CIS_CAL_PLACE_ON_WHITE;
    osDelay(200);

    /* La fonction cis_imageProcessRGB_Calibration_int() doit remplir whiteCal.data
       en effectuant une moyenne sur « iterationNb » acquisitions en entier */
    cis_imageProcessRGB_Calibration_int(whiteCal.data, iterationNb);
    SCB_CleanDCache_by_Addr((uint32_t *)&cisCals, sizeof(cisCals));

    osDelay(200);
    shared_var.cis_cal_progressbar = 0;
    shared_var.cis_cal_state = CIS_CAL_PLACE_ON_BLACK;
    cis_ledPowerAdj(1, 1, 1);
    osDelay(20);

    /* Étape 2 : Acquisition sur surface noire */
    cis_imageProcessRGB_Calibration_int(blackCal.data, iterationNb);
    SCB_CleanDCache_by_Addr((uint32_t *)&cisCals, sizeof(cisCals));
    cis_ledPowerAdj(100, 100, 100);
    osDelay(500);

    printf("------- CALCUL DE LA CALIBRATION INT -------\n");

    /* Étape 3 : Calcul des moyennes sur pixels inactifs */
    cis_ComputeCalsInactivesAvrg_int(&blackCal, CIS_RED);
    cis_ComputeCalsInactivesAvrg_int(&whiteCal, CIS_RED);
    cis_ComputeCalsInactivesAvrg_int(&blackCal, CIS_GREEN);
    cis_ComputeCalsInactivesAvrg_int(&whiteCal, CIS_GREEN);
    cis_ComputeCalsInactivesAvrg_int(&blackCal, CIS_BLUE);
    cis_ComputeCalsInactivesAvrg_int(&whiteCal, CIS_BLUE);
    SCB_CleanDCache_by_Addr((uint32_t *)&cisCals, sizeof(cisCals));
    shared_var.cis_cal_state = CIS_CAL_EXTRACT_INNACTIVE_REF;
    osDelay(200);

    /* Étape 4 : Calcul des extremums */
    cis_computeCalsExtremums_int(&blackCal, CIS_RED);
    cis_computeCalsExtremums_int(&whiteCal, CIS_RED);
    cis_computeCalsExtremums_int(&blackCal, CIS_GREEN);
    cis_computeCalsExtremums_int(&whiteCal, CIS_GREEN);
    cis_computeCalsExtremums_int(&blackCal, CIS_BLUE);
    cis_computeCalsExtremums_int(&whiteCal, CIS_BLUE);
    SCB_CleanDCache_by_Addr((uint32_t *)&cisCals, sizeof(cisCals));
    shared_var.cis_cal_state = CIS_CAL_EXTRACT_EXTREMUMS;
    osDelay(200);

    /* Étape 5 : Calcul des offsets différentiels */
    cis_computeCalsOffsets_int(&whiteCal, &blackCal, CIS_RED);
    cis_computeCalsOffsets_int(&whiteCal, &blackCal, CIS_GREEN);
    cis_computeCalsOffsets_int(&whiteCal, &blackCal, CIS_BLUE);
    SCB_CleanDCache_by_Addr((uint32_t *)&cisCals, sizeof(cisCals));
    shared_var.cis_cal_state = CIS_CAL_EXTRACT_OFFSETS;
    osDelay(200);

    /* Étape 6 : Calcul des gains
       Le gain est calculé en Q16.16 : gain = (maxADCValue << 16) / (white – black) */
    cis_computeCalsGains_int(bitDepth, &whiteCal, &blackCal, CIS_RED);
    cis_computeCalsGains_int(bitDepth, &whiteCal, &blackCal, CIS_GREEN);
    cis_computeCalsGains_int(bitDepth, &whiteCal, &blackCal, CIS_BLUE);
    SCB_CleanDCache_by_Addr((uint32_t *)&cisCals, sizeof(cisCals));
    shared_var.cis_cal_state = CIS_CAL_COMPUTE_GAINS;
    printf("-------- GAINS CALCULÉS EN INT --------\n");

    /* Étape 7 : Sauvegarde de la calibration */
    sprintf(calibrationFilePath, CALIBRATION_FILE_PATH_FORMAT, shared_config.cis_dpi);
    file_writeCisCals(calibrationFilePath, &cisCals);

    cis_stopCapture();
    cis_startCapture();
    shared_var.cis_cal_state = CIS_CAL_END;
    printf("-------------------------------\n");
}

/**
 * @brief Applique la calibration linéaire sur le buffer d'image (version INT).
 * La formule appliquée est :
 *      calibrated = clip( ((raw - offset) * gain) >> 16, 0, maxClipValue )
 *
 * @param cisDataCpy_int : pointeur sur le buffer d'image en int32_t (format Q16.16)
 * @param maxClipValue   : valeur de clip (par exemple 255)
 */
#pragma GCC push_options
#pragma GCC optimize ("unroll-loops")
void cis_applyLinearCalibration_int(int32_t* cisDataCpy_int, uint32_t maxClipValue)
{
    uint32_t dataOffset_Rx, dataOffset_Gx, dataOffset_Bx;
    for (int8_t lane = CIS_ADC_OUT_LANES; --lane >= 0;)
    {
        dataOffset_Rx = (cisConfig.adc_buff_size * lane) + cisConfig.red_lane_offset;     // R
        dataOffset_Gx = (cisConfig.adc_buff_size * lane) + cisConfig.green_lane_offset;   // G
        dataOffset_Bx = (cisConfig.adc_buff_size * lane) + cisConfig.blue_lane_offset;    // B

        /* Soustraction de l'offset : data = data - offset */
        arm_sub_q31(&cisDataCpy_int[dataOffset_Rx],
                    (q31_t *)&cisCals.offsetData[dataOffset_Rx],
                    &cisDataCpy_int[dataOffset_Rx],
                    cisConfig.pixels_per_lane);
        arm_sub_q31(&cisDataCpy_int[dataOffset_Gx],
                    (q31_t *)&cisCals.offsetData[dataOffset_Gx],
                    &cisDataCpy_int[dataOffset_Gx],
                    cisConfig.pixels_per_lane);
        arm_sub_q31(&cisDataCpy_int[dataOffset_Bx],
                    (q31_t *)&cisCals.offsetData[dataOffset_Bx],
                    &cisDataCpy_int[dataOffset_Bx],
                    cisConfig.pixels_per_lane);

        /* Multiplication par le gain en Q16.16 puis décalage de 16 bits */
        for (int i = 0; i < cisConfig.pixels_per_lane; i++) {
            int64_t temp = (int64_t)cisDataCpy_int[dataOffset_Rx + i] * (int64_t)(((q31_t *)cisCals.gainsData)[dataOffset_Rx + i]);
            cisDataCpy_int[dataOffset_Rx + i] = (int32_t)(temp >> 16);
        }
        for (int i = 0; i < cisConfig.pixels_per_lane; i++) {
            int64_t temp = (int64_t)cisDataCpy_int[dataOffset_Gx + i] * (int64_t)(((q31_t *)cisCals.gainsData)[dataOffset_Gx + i]);
            cisDataCpy_int[dataOffset_Gx + i] = (int32_t)(temp >> 16);
        }
        for (int i = 0; i < cisConfig.pixels_per_lane; i++) {
            int64_t temp = (int64_t)cisDataCpy_int[dataOffset_Bx + i] * (int64_t)(((q31_t *)cisCals.gainsData)[dataOffset_Bx + i]);
            cisDataCpy_int[dataOffset_Bx + i] = (int32_t)(temp >> 16);
        }

        /* Clipping sur [0, maxClipValue] */
        arm_clip_q31(&cisDataCpy_int[dataOffset_Rx], 0, (q31_t)maxClipValue, &cisDataCpy_int[dataOffset_Rx], cisConfig.pixels_per_lane);
        arm_clip_q31(&cisDataCpy_int[dataOffset_Gx], 0, (q31_t)maxClipValue, &cisDataCpy_int[dataOffset_Gx], cisConfig.pixels_per_lane);
        arm_clip_q31(&cisDataCpy_int[dataOffset_Bx], 0, (q31_t)maxClipValue, &cisDataCpy_int[dataOffset_Bx], cisConfig.pixels_per_lane);
    }
}
#pragma GCC pop_options

/* ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––
   Fonctions internes de calcul de la calibration INT
–––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––*/

/**
 * @brief Calcule la moyenne des pixels inactifs pour une couleur donnée.
 */
static void cis_ComputeCalsInactivesAvrg_int(struct cisCalsTypes_int *currCals, CIS_Color_TypeDef color)
{
    int32_t laneOffset = 0, offset = 0;
    struct cisColorsParams_int *currColor = NULL;

    switch (color)
    {
        case CIS_RED:
            currColor = (struct cisColorsParams_int *)&cisCals.offsetData[0]; // Utilisation locale ou stockage dans une variable globale
            offset = cisConfig.red_lane_offset - CIS_INACTIVE_WIDTH;
            break;
        case CIS_GREEN:
            currColor = (struct cisColorsParams_int *)&cisCals.offsetData[0];
            offset = cisConfig.green_lane_offset - CIS_INACTIVE_WIDTH;
            break;
        case CIS_BLUE:
            currColor = (struct cisColorsParams_int *)&cisCals.offsetData[0];
            offset = cisConfig.blue_lane_offset - CIS_INACTIVE_WIDTH;
            break;
        default:
            Error_Handler();
            return;
    }
    for (int32_t lane = CIS_ADC_OUT_LANES; --lane >= 0;)
    {
        laneOffset = (cisConfig.adc_buff_size * lane) + offset;
        arm_mean_q31(&currCals->data[laneOffset], CIS_INACTIVE_WIDTH, &currColor->inactiveAvrgPix[lane]);
    }
}

/**
 * @brief Calcule les valeurs extrêmes (min, max et delta) pour une couleur donnée.
 */
static void cis_computeCalsExtremums_int(struct cisCalsTypes_int *currCals, CIS_Color_TypeDef color)
{
    int32_t laneOffset = 0, offset = 0;
    struct cisColorsParams_int *currColor = NULL;
    q31_t tmpMax = 0, tmpMin = INT32_MAX;

    switch (color)
    {
        case CIS_RED:
            currColor = (struct cisColorsParams_int *)&cisCals.offsetData[0];
            offset = cisConfig.red_lane_offset;
            break;
        case CIS_GREEN:
            currColor = (struct cisColorsParams_int *)&cisCals.offsetData[0];
            offset = cisConfig.green_lane_offset;
            break;
        case CIS_BLUE:
            currColor = (struct cisColorsParams_int *)&cisCals.offsetData[0];
            offset = cisConfig.blue_lane_offset;
            break;
        default:
            Error_Handler();
            return;
    }
    currColor->maxPix = 0;
    currColor->minPix = INT32_MAX;
    for (int32_t lane = CIS_ADC_OUT_LANES; --lane >= 0;)
    {
        laneOffset = (cisConfig.adc_buff_size * lane) + offset;
        arm_max_q31(&currCals->data[laneOffset], cisConfig.pixels_per_lane, &tmpMax, NULL);
        arm_min_q31(&currCals->data[laneOffset], cisConfig.pixels_per_lane, &tmpMin, NULL);
        if (tmpMax > currColor->maxPix)
            currColor->maxPix = tmpMax;
        if (tmpMin < currColor->minPix)
            currColor->minPix = tmpMin;
        currColor->deltaPix = currColor->maxPix - currColor->minPix;
    }
}

/**
 * @brief Copie les valeurs de calibration noire dans le tableau des offsets pour une couleur.
 */
static void cis_computeCalsOffsets_int(struct cisCalsTypes_int *whiteCal, struct cisCalsTypes_int *blackCal, CIS_Color_TypeDef color)
{
    uint32_t laneOffset = 0, offset = 0;
    switch (color)
    {
        case CIS_RED:
            offset = cisConfig.red_lane_offset;
            break;
        case CIS_GREEN:
            offset = cisConfig.green_lane_offset;
            break;
        case CIS_BLUE:
            offset = cisConfig.blue_lane_offset;
            break;
        default:
            Error_Handler();
            return;
    }
    for (int32_t lane = CIS_ADC_OUT_LANES; --lane >= 0;)
    {
        laneOffset = (cisConfig.adc_buff_size * lane) + offset;
        for (int32_t i = 0; i < cisConfig.pixels_per_lane; i++)
        {
            cisCals.offsetData[laneOffset + i] = blackCal->data[laneOffset + i];
        }
    }
}

/**
 * @brief Calcule les gains en fonction des données blanche et noire pour une couleur.
 * Le gain est calculé en Q16.16 : gain = (maxADCValue << 16) / (white - black)
 */
static void cis_computeCalsGains_int(uint32_t maxADCValue, struct cisCalsTypes_int *whiteCal, struct cisCalsTypes_int *blackCal, CIS_Color_TypeDef color)
{
    uint32_t laneOffset = 0, offset;
    switch (color)
    {
        case CIS_RED:
            offset = cisConfig.red_lane_offset;
            break;
        case CIS_GREEN:
            offset = cisConfig.green_lane_offset;
            break;
        case CIS_BLUE:
            offset = cisConfig.blue_lane_offset;
            break;
        default:
            Error_Handler();
            return;
    }
    for (int32_t lane = CIS_ADC_OUT_LANES; --lane >= 0;)
    {
        laneOffset = (cisConfig.adc_buff_size * lane) + offset;
        for (int32_t i = 0; i < cisConfig.pixels_per_lane; i++)
        {
            int32_t diff = whiteCal->data[laneOffset + i] - blackCal->data[laneOffset + i];
            if (diff != 0)
            {
                /* Calcul du gain en Q16.16 */
                cisCals.gainsData[laneOffset + i] = ((int32_t)maxADCValue << 16) / diff;
            }
            else
            {
                cisCals.gainsData[laneOffset + i] = UNITY_Q16_16; // 1.0 en Q16.16
            }
        }
    }
}
