/**
 ******************************************************************************
 * @file           : cis_polyCal.c
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

#include "cis.h"

#include "cis_polyCal.h"

/* Private includes ----------------------------------------------------------*/

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Variable containing ADC conversions data */

/* Private function prototypes -----------------------------------------------*/

/* Private user code ---------------------------------------------------------*/


/**
 * @brief  CIS start calibration
 * @param  calibration iteration
 * @retval None
 */
void cis_StartPolynomialCalibration(uint16_t iterationNb)
{
	static float32_t redLine[CIS_PIXELS_NB];
	static float32_t greenLine[CIS_PIXELS_NB];
	static float32_t blueLine[CIS_PIXELS_NB];

	/* Set header description */
	printf("------ START CALIBRATION ------\n");
	/*-------- 1 --------*/
	// Read black and white level

	shared_var.cis_cal_progressbar = 0;
	shared_var.cis_cal_state = CIS_CAL_PLACE_ON_WHITE;

	cis_LedPowerAdj(95);
	HAL_Delay(200);
	cis_GetRGBImage(redLine, greenLine, blueLine, iterationNb);
	HAL_Delay(200);
	cis_LedPowerAdj(50);
	HAL_Delay(200);
	cis_GetRGBImage(redLine, greenLine, blueLine, iterationNb);
	HAL_Delay(200);
	cis_LedPowerAdj(5);
	HAL_Delay(200);
	cis_GetRGBImage(redLine, greenLine, blueLine, iterationNb);
}
