/**
 ******************************************************************************
 * @file           : cis_ledsCal.c
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
#include "cmsis_os.h"

#include "cis.h"

#include "cis_ledsCal.h"

/* Private includes ----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/
#define CIS_LEDS_CAL_MEASURE_CYCLES				(10)

#define CAL_POWER_INC				10
#define CAL_POWER_MIN				10
#define CAL_POWER_MAX				90

#define CAL_POWER_NB_INC			((CAL_POWER_MAX - CAL_POWER_MIN) / CAL_POWER_INC)

/* Private typedef -----------------------------------------------------------*/


/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Variable containing ADC conversions data */

/* Private function prototypes -----------------------------------------------*/
static void cis_setLedsPower(int32_t power);

void cis_getMeanAtLedPower(struct RAWImage* RAWImage, struct cisLeds_Calibration *cisLeds_Calibration, int32_t led_PWM)
{
	cis_ledPowerAdj(led_PWM, led_PWM, led_PWM);

	//float32_t cisDataCpy_f32[CIS_ADC_BUFF_SIZE * 3] = {0}; todo update definition

	// Capture a raw image
	//cis_getRAWImage(cisDataCpy_f32, CIS_LEDS_CAL_MEASURE_CYCLES);

	// Convert the raw image to an integer array
	//cis_convertRAWImageToFloatArray(cisDataCpy_f32, RAWImage);

	// Calculate the mean red, green, and blue values
	//arm_mean_f32(RAWImage->redLine, CIS_PIXELS_NB, &cisLeds_Calibration->redMeanAtLedPower); todo update definition
	//arm_mean_f32(RAWImage->greenLine, CIS_PIXELS_NB, &cisLeds_Calibration->greenMeanAtLedPower); todo update definition
	//arm_mean_f32(RAWImage->blueLine, CIS_PIXELS_NB, &cisLeds_Calibration->blueMeanAtLedPower); todo update definition
}

void cis_calibrateLeds(void)
{
	printf("----- CIS LED CALIBRATION -----\n");

	// Declare necessary variables and arrays
	int32_t led_PWM, power;
	float32_t meanRedValue[CIS_LEDS_MAX_PWM], meanGreenValue[CIS_LEDS_MAX_PWM], meanBlueValue[CIS_LEDS_MAX_PWM];
	float32_t redRange, greenRange, blueRange;

	struct RAWImage RAWImage = {0};

	// Set to max led power range for eatch leds colors
	cisLeds_Calibration.redLed_maxPulse = cisConfig.leds_off_index;
	cisLeds_Calibration.greenLed_maxPulse = cisConfig.leds_off_index;
	cisLeds_Calibration.blueLed_maxPulse = cisConfig.leds_off_index;

	// Set to max power all LEDs
	led_PWM = 100;
	cis_getMeanAtLedPower(&RAWImage, &cisLeds_Calibration, led_PWM);

	// get the min value
	float minVal = fmin(cisLeds_Calibration.redMeanAtLedPower, fmin(cisLeds_Calibration.greenMeanAtLedPower, cisLeds_Calibration.blueMeanAtLedPower));

	// calibrate power led for eatch color (white balance)
	if (minVal < cisLeds_Calibration.redMeanAtLedPower)
	{
		while (cisLeds_Calibration.redMeanAtLedPower > minVal)
		{
			--cisLeds_Calibration.redLed_maxPulse;
			cis_getMeanAtLedPower(&RAWImage, &cisLeds_Calibration, led_PWM);
		}
	}

	if (minVal < cisLeds_Calibration.greenMeanAtLedPower)
	{
		while (cisLeds_Calibration.greenMeanAtLedPower > minVal)
		{
			--cisLeds_Calibration.greenLed_maxPulse;
			cis_getMeanAtLedPower(&RAWImage, &cisLeds_Calibration, led_PWM);
		}
	}

	if (minVal < cisLeds_Calibration.blueMeanAtLedPower)
	{
		while (cisLeds_Calibration.blueMeanAtLedPower > minVal)
		{
			--cisLeds_Calibration.blueLed_maxPulse;
			cis_getMeanAtLedPower(&RAWImage, &cisLeds_Calibration, led_PWM);
		}
	}

	// Loop over the range of PWM values
	for (led_PWM = CIS_LEDS_MAX_PWM; --led_PWM >= 0;)
	{
		cis_getMeanAtLedPower(&RAWImage, &cisLeds_Calibration, led_PWM);

		meanRedValue[led_PWM] = cisLeds_Calibration.redMeanAtLedPower;
		meanGreenValue[led_PWM] = cisLeds_Calibration.greenMeanAtLedPower;
		meanBlueValue[led_PWM] = cisLeds_Calibration.blueMeanAtLedPower;
	}

	led_PWM = 100;
	cis_ledPowerAdj(led_PWM, led_PWM, led_PWM);

	// Calculate the ranges of the mean color values
	redRange = meanRedValue[CIS_LEDS_MAX_PWM - 1] - meanRedValue[0];
	greenRange = meanGreenValue[CIS_LEDS_MAX_PWM - 1] - meanGreenValue[0];
	blueRange = meanBlueValue[CIS_LEDS_MAX_PWM - 1] - meanBlueValue[0];

	// Loop over the power values
	for (power = CIS_LEDS_MAX_POMER; --power >= 0;)
	{
		// Calculate the power to PWM mapping for each color
		cisLeds_Calibration.redLed_power2PWM[power] = ((meanRedValue[power] - meanRedValue[0]) * 100) / redRange;
		cisLeds_Calibration.greenLed_power2PWM[power] = ((meanGreenValue[power] - meanGreenValue[0]) * 100) / greenRange;
		cisLeds_Calibration.blueLed_power2PWM[power] = ((meanBlueValue[power] - meanBlueValue[0]) * 100) / blueRange;
	}

#ifdef PRINT_CIS_CALIBRATION
	// Print the calibration data if the macro is defined
	for (power = CIS_LEDS_MAX_POMER; --power >= 0;)
	{
		printf("Power = %d %%, PWM RGB = %d, %d, %d\n", (int)power, (int)cisLeds_Calibration.redLed_power2PWM[power], (int)cisLeds_Calibration.greenLed_power2PWM[power], (int)cisLeds_Calibration.blueLed_power2PWM[power]);
	}

#endif
}

void cis_setLedsPower(int32_t power)
{
	cis_ledPowerAdj(cisLeds_Calibration.redLed_power2PWM[power], cisLeds_Calibration.greenLed_power2PWM[power], cisLeds_Calibration.blueLed_power2PWM[power]);
}
