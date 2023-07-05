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

/* Private define ------------------------------------------------------------*/
#define LEDS_CAL_MEASURE_CYCLES		500
#define LEDS_MAX_PWM				101

/* Private typedef -----------------------------------------------------------*/
struct cisLeds_Power2PWM {
	int32_t redLed[LEDS_MAX_PWM];
	int32_t greenLed[LEDS_MAX_PWM];
	int32_t blueLed[LEDS_MAX_PWM];
};

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Variable containing ADC conversions data */

/* Private function prototypes -----------------------------------------------*/
void cis_CalibrateLeds(struct cisLeds_Power2PWM *power2PWM);

/* Private user code ---------------------------------------------------------*/

void cis_CalibrateLeds(struct cisLeds_Power2PWM *power2PWM)
{
	int32_t led_PWM;
	float32_t redLine[CIS_LINE_SIZE], greenLine[CIS_LINE_SIZE], blueLine[CIS_LINE_SIZE];
	float32_t meanRedValue[LEDS_MAX_PWM], meanGreenValue[LEDS_MAX_PWM], meanBlueValue[LEDS_MAX_PWM];
	float32_t redRange, greenRange, blueRange;

	for (led_PWM = LEDS_MAX_PWM; --led_PWM >= 0;)
	{
		cis_LedPowerAdj(led_PWM);
		cis_GetRGBImage(redLine, greenLine, blueLine, LEDS_CAL_MEASURE_CYCLES);
		arm_mean_f32(redLine, CIS_LINE_SIZE, &meanRedValue[led_PWM]);
		arm_mean_f32(greenLine, CIS_LINE_SIZE, &meanGreenValue[led_PWM]);
		arm_mean_f32(blueLine, CIS_LINE_SIZE, &meanBlueValue[led_PWM]);
	}
	redRange = meanRedValue[LEDS_MAX_PWM - 1] - meanRedValue[0];
	greenRange = meanGreenValue[LEDS_MAX_PWM - 1] - meanGreenValue[0];
	blueRange = meanBlueValue[LEDS_MAX_PWM - 1] - meanBlueValue[0];

	for (led_PWM = LEDS_MAX_PWM; --led_PWM >= 0;)
	{
		power2PWM->redLed[led_PWM] = ((meanRedValue[led_PWM] - meanRedValue[0]) * 100) / redRange;
		power2PWM->greenLed[led_PWM] = ((meanGreenValue[led_PWM] - meanGreenValue[0]) * 100) / greenRange;
		power2PWM->blueLed[led_PWM] = ((meanBlueValue[led_PWM] - meanBlueValue[0]) * 100) / blueRange;
	}
}

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
