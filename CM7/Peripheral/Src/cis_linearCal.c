/**
 ******************************************************************************
 * @file           : cis_linearCal.c
 * @brief          :
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

#include "stm32_flash.h"
#include "cis.h"

#include "cis_linearCal.h"

/* Private includes ----------------------------------------------------------*/

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Variable containing ADC conversions data */

/* Private function prototypes -----------------------------------------------*/
static void cis_ComputeCalsExtremums(struct cisCalsTypes *currCisCals, CIS_Color_TypeDef color);
static void cis_ComputeCalsOffsets(CIS_Color_TypeDef color);
static void cis_ComputeCalsGains(CIS_Color_TypeDef color);

/* Private user code ---------------------------------------------------------*/

void cis_lanealCalibrationInit()
{
	stm32_flashCalibrationRW(CIS_READ_CAL);
}

#pragma GCC push_options
#pragma GCC optimize ("unroll-loops")
float calculate_mean(float *array, int length) {
    float sum = 0.0;
    for (int i = 0; i < length; i++) {
        sum += array[i];
    }
    return sum / length;
}

void elementwise_subtract(float *result, float *array1, float *array2, int length) {
    for (int i = 0; i < length; i++) {
        result[i] = array1[i] - array2[i];
    }
}

void elementwise_multiply(float *result, float *array1, float *array2, int length) {
    for (int i = 0; i < length; i++) {
        result[i] = array1[i] * array2[i];
    }
}

void clip_values(float *array, float min, float max, int length) {
    for (int i = 0; i < length; i++) {
        if (array[i] < min) array[i] = min;
        else if (array[i] > max) array[i] = max;
    }
}

void cis_ApplyLinearCalibration(void)
{
#ifndef CIS_DESACTIVATE_CALIBRATION
	static uint32_t dataOffset_Rx, dataOffset_Gx, dataOffset_Bx;

	for (int8_t lane = CIS_ADC_OUT_LANES; --lane >= 0;)
	{
		dataOffset_Rx = (CIS_ADC_BUFF_SIZE * lane) + CIS_RED_LANE_OFFSET;									//Rx
		dataOffset_Gx = (CIS_ADC_BUFF_SIZE * lane) + CIS_GREEN_LANE_OFFSET;									//Gx
		dataOffset_Bx = (CIS_ADC_BUFF_SIZE * lane) + CIS_BLUE_LANE_OFFSET;									//Bx

		//offset compensation
		/*
		static float32_t tmpImmactiveAvrg_R = 0.0;
		static float32_t tmpImmactiveAvrg_G = 0.0;
		static float32_t tmpImmactiveAvrg_B = 0.0;
		arm_mean_f32(&cisDataCpy_f32[dataOffset_Rx - CIS_INACTIVE_WIDTH], CIS_INACTIVE_WIDTH, &tmpImmactiveAvrg_R);
		arm_mean_f32(&cisDataCpy_f32[dataOffset_Gx - CIS_INACTIVE_WIDTH], CIS_INACTIVE_WIDTH, &tmpImmactiveAvrg_G);
		arm_mean_f32(&cisDataCpy_f32[dataOffset_Bx - CIS_INACTIVE_WIDTH], CIS_INACTIVE_WIDTH, &tmpImmactiveAvrg_B);

		tmpImmactiveAvrg_R -= cisCals.whiteCal.red.inactiveAvrgPix[lane];
		tmpImmactiveAvrg_G -= cisCals.whiteCal.green.inactiveAvrgPix[lane];
		tmpImmactiveAvrg_B -= cisCals.whiteCal.blue.inactiveAvrgPix[lane];

		arm_offset_f32(&cisDataCpy_f32[dataOffset_Rx], tmpImmactiveAvrg_R, &cisDataCpy_f32[dataOffset_Rx], CIS_PIXELS_PER_LINE);
		arm_offset_f32(&cisDataCpy_f32[dataOffset_Gx], tmpImmactiveAvrg_G, &cisDataCpy_f32[dataOffset_Gx], CIS_PIXELS_PER_LINE);
		arm_offset_f32(&cisDataCpy_f32[dataOffset_Bx], tmpImmactiveAvrg_B, &cisDataCpy_f32[dataOffset_Bx], CIS_PIXELS_PER_LINE);
		 */
		//end offset compensation

		arm_sub_f32(&cisDataCpy_f32[dataOffset_Rx], &cisCals.blackCal.data[dataOffset_Rx], &cisDataCpy_f32[dataOffset_Rx], CIS_PIXELS_PER_LANE);
		arm_sub_f32(&cisDataCpy_f32[dataOffset_Gx], &cisCals.blackCal.data[dataOffset_Gx], &cisDataCpy_f32[dataOffset_Gx], CIS_PIXELS_PER_LANE);
		arm_sub_f32(&cisDataCpy_f32[dataOffset_Bx], &cisCals.blackCal.data[dataOffset_Bx], &cisDataCpy_f32[dataOffset_Bx], CIS_PIXELS_PER_LANE);

		arm_mult_f32(&cisDataCpy_f32[dataOffset_Rx], &cisCals.gainsData[dataOffset_Rx], &cisDataCpy_f32[dataOffset_Rx], CIS_PIXELS_PER_LANE);
		arm_mult_f32(&cisDataCpy_f32[dataOffset_Gx], &cisCals.gainsData[dataOffset_Gx], &cisDataCpy_f32[dataOffset_Gx], CIS_PIXELS_PER_LANE);
		arm_mult_f32(&cisDataCpy_f32[dataOffset_Bx], &cisCals.gainsData[dataOffset_Bx], &cisDataCpy_f32[dataOffset_Bx], CIS_PIXELS_PER_LANE);

		arm_clip_f32(&cisDataCpy_f32[dataOffset_Rx], &cisDataCpy_f32[dataOffset_Rx], 0, 255, CIS_PIXELS_PER_LANE);
		arm_clip_f32(&cisDataCpy_f32[dataOffset_Gx], &cisDataCpy_f32[dataOffset_Gx], 0, 255, CIS_PIXELS_PER_LANE);
		arm_clip_f32(&cisDataCpy_f32[dataOffset_Bx], &cisDataCpy_f32[dataOffset_Bx], 0, 255, CIS_PIXELS_PER_LANE);
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
void cis_ComputeCalsExtremums(struct cisCalsTypes *currCisCals, CIS_Color_TypeDef color)
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
void cis_ComputeCalsOffsets(CIS_Color_TypeDef color)
{
	uint32_t laneOffset = 0, offset = 0;
	struct cisColorsParams *currColor;

	switch (color)
	{
	case CIS_RED :
		currColor = &cisCals.blackCal.red;
		offset = CIS_RED_LANE_OFFSET;
		break;
	case CIS_GREEN :
		currColor = &cisCals.blackCal.green;
		offset = CIS_GREEN_LANE_OFFSET;
		break;
	case CIS_BLUE :
		currColor = &cisCals.blackCal.blue;
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
			cisCals.offsetData[laneOffset + i] = currColor->maxPix - cisCals.blackCal.data[laneOffset + i];
		}
	}
}

/**
 * @brief  CIS compute gains
 * @param  current color calibration
 * @retval None
 */
void cis_ComputeCalsGains(CIS_Color_TypeDef color)
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
#ifdef RGBA_BUFFER
			cisCals.gainsData[laneOffset + i] = (float32_t)(CIS_ADC_MAX_VALUE) / (float32_t)(cisCals.whiteCal.data[laneOffset + i] - cisCals.blackCal.data[laneOffset + i]);
#else
			cisCals.gainsData[laneOffset + i] = (float32_t)(255) / (float32_t)(cisCals.whiteCal.data[laneOffset + i] - cisCals.blackCal.data[laneOffset + i]);
#endif
		}
	}
}

/**
 * @brief  CIS start calibration
 * @param  calibration iteration
 * @retval None
 */
void cis_StartLinearCalibration(uint16_t iterationNb)
{
	/* Set header description */
	printf("------ START CALIBRATION ------\n");
	/*-------- 1 --------*/
	// Read black and white level
	cis_LedPowerAdj(95, 95, 95);
	shared_var.cis_cal_progressbar = 0;
	shared_var.cis_cal_state = CIS_CAL_PLACE_ON_WHITE;
	HAL_Delay(200);

	cis_ImageProcessRGB_Calibration(cisCals.whiteCal.data, iterationNb);
	SCB_CleanDCache_by_Addr((uint32_t *)&cisCals, sizeof(cisCals) * (sizeof(uint32_t)));

	HAL_Delay(200);
	shared_var.cis_cal_progressbar = 0;
	shared_var.cis_cal_state = CIS_CAL_PLACE_ON_BLACK;
	cis_LedPowerAdj(5, 5, 5);
	HAL_Delay(20);

	cis_ImageProcessRGB_Calibration(cisCals.blackCal.data, iterationNb);
	SCB_CleanDCache_by_Addr((uint32_t *)&cisCals, sizeof(cisCals) * (sizeof(uint32_t)));
	cis_LedPowerAdj(100, 100, 100);
	HAL_Delay(2000);

	printf("------- LOAD CALIBRATION ------\n");
	/*-------- 1 --------*/
	// Extrat Min Max and delta
	cis_ComputeCalsInactivesAvrg(&cisCals.blackCal, CIS_RED);
	cis_ComputeCalsInactivesAvrg(&cisCals.whiteCal, CIS_RED);

	cis_ComputeCalsInactivesAvrg(&cisCals.blackCal, CIS_GREEN);
	cis_ComputeCalsInactivesAvrg(&cisCals.whiteCal, CIS_GREEN);

	cis_ComputeCalsInactivesAvrg(&cisCals.blackCal, CIS_BLUE);
	cis_ComputeCalsInactivesAvrg(&cisCals.whiteCal, CIS_BLUE);

	SCB_CleanDCache_by_Addr((uint32_t *)&cisCals, sizeof(cisCals) * (sizeof(uint32_t)));
	shared_var.cis_cal_state = CIS_CAL_EXTRACT_INNACTIVE_REF;
	HAL_Delay(500);

	/*-------- 2 --------*/
	// Extrat Min Max and delta
	cis_ComputeCalsExtremums(&cisCals.blackCal, CIS_RED);
	cis_ComputeCalsExtremums(&cisCals.whiteCal, CIS_RED);

	cis_ComputeCalsExtremums(&cisCals.blackCal, CIS_GREEN);
	cis_ComputeCalsExtremums(&cisCals.whiteCal, CIS_GREEN);

	cis_ComputeCalsExtremums(&cisCals.blackCal, CIS_BLUE);
	cis_ComputeCalsExtremums(&cisCals.whiteCal, CIS_BLUE);

	SCB_CleanDCache_by_Addr((uint32_t *)&cisCals, sizeof(cisCals) * (sizeof(uint32_t)));
	shared_var.cis_cal_state = CIS_CAL_EXTRACT_EXTREMUMS;
	HAL_Delay(500);

	/*-------- 3 --------*/
	// Extract differential offsets
	cis_ComputeCalsOffsets(CIS_RED);
	cis_ComputeCalsOffsets(CIS_GREEN);
	cis_ComputeCalsOffsets(CIS_BLUE);

	SCB_CleanDCache_by_Addr((uint32_t *)&cisCals, sizeof(cisCals) * (sizeof(uint32_t)));
	shared_var.cis_cal_state = CIS_CAL_EXTRACT_OFFSETS;
	HAL_Delay(500);

	/*-------- 4 --------*/
	// Compute gains
	cis_ComputeCalsGains(CIS_RED);
	cis_ComputeCalsGains(CIS_GREEN);
	cis_ComputeCalsGains(CIS_BLUE);

	SCB_CleanDCache_by_Addr((uint32_t *)&cisCals, sizeof(cisCals) * (sizeof(uint32_t)));
	shared_var.cis_cal_state = CIS_CAL_COMPUTE_GAINS;
	HAL_Delay(500);

	printf("-------- COMPUTE GAINS --------\n");
#ifdef PRINT_CIS_CALIBRATION
	for (uint32_t pix = 0; pix < CIS_PIXELS_NB; pix++)
	{
		printf("Pix = %d, Val = %0.3f\n", (int)pix, (float)cisCals.gainsData[pix]);
	}
#endif

	SCB_CleanDCache_by_Addr((uint32_t *)&cisCals, sizeof(cisCals) * (sizeof(uint32_t)));
	stm32_flashCalibrationRW(CIS_WRITE_CAL);
	shared_var.cis_cal_state = CIS_CAL_END;
	printf("-------------------------------\n");
}
