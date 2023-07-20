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
#define CIS_LEDS_CAL_MEASURE_CYCLES				(5)

#define CAL_POWER_INC				10
#define CAL_POWER_MIN				10
#define CAL_POWER_MAX				90

#define CAL_POWER_NB_INC			((CAL_POWER_MAX - CAL_POWER_MIN) / CAL_POWER_INC)

/* Private typedef -----------------------------------------------------------*/


/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Variable containing ADC conversions data */

/* Private function prototypes -----------------------------------------------*/
void leastSquares(float32_t* x, float32_t* y, uint32_t n, float32_t* a, float32_t* b, float32_t* c);
void cis_setLedsPower(struct cisLeds_Calibration *cisLeds_Calibration, int32_t power);
void cis_StartPolynomialCalibration(uint16_t iterationNb, struct RGB_Calibration* rgbCalibration);

/* Private user code ---------------------------------------------------------*/

int cis_polyCalInit(void)
{
	// Définir le nombre d'itérations de calibration
	uint16_t iterationNb = 5; // Changer cela selon votre besoin

	// Appel de la fonction cis_StartPolynomialCalibration
	cis_StartPolynomialCalibration(iterationNb, &rgbCalibration);

	// Votre code continue ici

	return 0;
}

void cis_leastSquares(float32_t* x, float32_t* y, uint32_t n, float32_t* a, float32_t* b, float32_t* c)
{
	float32_t sum_x = 0, sum_y = 0, sum_xx = 0, sum_xy = 0, sum_xxx = 0, sum_xxy = 0, sum_xxxx = 0;

	for(uint32_t i = 0; i < n; i++) {
		sum_x += x[i];
		sum_y += y[i];
		sum_xx += x[i]*x[i];
		sum_xy += x[i]*y[i];
		sum_xxx += x[i]*x[i]*x[i];
		sum_xxy += x[i]*x[i]*y[i];
		sum_xxxx += x[i]*x[i]*x[i]*x[i];
	}

	// Matrice des sommes (A dans AX=B)
	float32_t matrixA_data[9] = { n, sum_x, sum_xx,
			sum_x, sum_xx, sum_xxx,
			sum_xx, sum_xxx, sum_xxxx };

	// Vecteur des sommes (B dans AX=B)
	float32_t vectorB_data[3] = { sum_y, sum_xy, sum_xxy };

	// Prepare CMSIS structures
	arm_matrix_instance_f32 matrixA, vectorB, vectorX, matrixA_inv;
	arm_mat_init_f32(&matrixA, 3, 3, matrixA_data);
	arm_mat_init_f32(&vectorB, 3, 1, vectorB_data);
	arm_mat_init_f32(&vectorX, 3, 1, vectorB_data); // Reuse vectorB_data for storing result
	float32_t matrixA_inv_data[9];
	arm_mat_init_f32(&matrixA_inv, 3, 3, matrixA_inv_data);

	// Compute inverse of A
	arm_mat_inverse_f32(&matrixA, &matrixA_inv);

	// Multiply inverse of A with B to get X
	arm_mat_mult_f32(&matrixA_inv, &vectorB, &vectorX);

	// Coefficients are stored in vectorX
	*a = vectorX.pData[0];
	*b = vectorX.pData[1];
	*c = vectorX.pData[2];
}

void cis_getMeanAtLedPower(struct RAWImage RAWImage, struct cisLeds_Calibration *cisLeds_Calibration, int32_t led_PWM)
{
	static float32_t cisDataCpy_f32[CIS_ADC_BUFF_SIZE * 3] = {0};

	cis_LedPowerAdj(led_PWM, led_PWM, led_PWM);

	// Capture a raw image
	cis_getRAWImage(cisDataCpy_f32, CIS_LEDS_CAL_MEASURE_CYCLES);

	// Convert the raw image to an integer array
	cis_ConvertRAWImageToIntArray(cisDataCpy_f32, &RAWImage);

	// Calculate the mean red, green, and blue values
	arm_mean_f32(RAWImage.redLine, CIS_LINE_SIZE, &cisLeds_Calibration->redMeanAtLedPower);
	arm_mean_f32(RAWImage.greenLine, CIS_LINE_SIZE, &cisLeds_Calibration->greenMeanAtLedPower);
	arm_mean_f32(RAWImage.blueLine, CIS_LINE_SIZE, &cisLeds_Calibration->blueMeanAtLedPower);
}

void cis_calibrateLeds(void)
{
	// Declare necessary variables and arrays
	int32_t led_PWM, power;
	float32_t meanRedValue[CIS_LEDS_MAX_PWM], meanGreenValue[CIS_LEDS_MAX_PWM], meanBlueValue[CIS_LEDS_MAX_PWM];
	float32_t redRange, greenRange, blueRange;

	struct RAWImage RAWImage = {0};

	// Set to max led power range for eatch leds colors
	cisLeds_Calibration.redLed_maxPulse = CIS_LED_RED_OFF;
	cisLeds_Calibration.greenLed_maxPulse = CIS_LED_GREEN_OFF;
	cisLeds_Calibration.blueLed_maxPulse = CIS_LED_BLUE_OFF;

	// Set to max power all LEDs
	led_PWM = 100;
	cis_getMeanAtLedPower(RAWImage, &cisLeds_Calibration, led_PWM);

	// get the min value
	float minVal = fmin(cisLeds_Calibration.redMeanAtLedPower, fmin(cisLeds_Calibration.greenMeanAtLedPower, cisLeds_Calibration.blueMeanAtLedPower));

	// calibrate power led for eatch color (white balance)
	if (minVal < cisLeds_Calibration.redMeanAtLedPower)
	{
		while (cisLeds_Calibration.redMeanAtLedPower > minVal)
		{
			--cisLeds_Calibration.redLed_maxPulse;
			cis_getMeanAtLedPower(RAWImage, &cisLeds_Calibration, led_PWM);
		}
	}

	if (minVal < cisLeds_Calibration.greenMeanAtLedPower)
	{
		while (cisLeds_Calibration.greenMeanAtLedPower > minVal)
		{
			--cisLeds_Calibration.greenLed_maxPulse;
			cis_getMeanAtLedPower(RAWImage, &cisLeds_Calibration, led_PWM);
		}
	}

	if (minVal < cisLeds_Calibration.blueMeanAtLedPower)
	{
		while (cisLeds_Calibration.blueMeanAtLedPower > minVal)
		{
			--cisLeds_Calibration.blueLed_maxPulse;
			cis_getMeanAtLedPower(RAWImage, &cisLeds_Calibration, led_PWM);
		}
	}

	// Loop over the range of PWM values
	for (led_PWM = CIS_LEDS_MAX_PWM; --led_PWM >= 0;)
	{
		cis_getMeanAtLedPower(RAWImage, &cisLeds_Calibration, led_PWM);

		meanRedValue[led_PWM] = cisLeds_Calibration.redMeanAtLedPower;
		meanGreenValue[led_PWM] = cisLeds_Calibration.greenMeanAtLedPower;
		meanBlueValue[led_PWM] = cisLeds_Calibration.blueMeanAtLedPower;
	}

	led_PWM = 100;
	cis_LedPowerAdj(led_PWM, led_PWM, led_PWM);

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
	printf("Print CIS LEDs Calibration\n");

	for (power = CIS_LEDS_MAX_POMER; --power >= 0;)
	{
		printf("Power = %d %%, PWM RGB = %d, %d, %d\n", (int)power, (int)cisLeds_Calibration.redLed[power], (int)cisLeds_Calibration.greenLed_power2PWM[power], (int)cisLeds_Calibration.blueLed_power2PWM[power]);
	}

#endif
}

void cis_setLedsPower(struct cisLeds_Calibration *cisLeds_Calibration, int32_t power)
{
	cis_LedPowerAdj(cisLeds_Calibration->redLed_power2PWM[power], cisLeds_Calibration->greenLed_power2PWM[power], cisLeds_Calibration->blueLed_power2PWM[power]);
}

/**
 * @brief  CIS start calibration
 * @param  iterationNb: calibration iteration
 * @param  rgbCalibration: pointer to structure containing calibration coefficients for all three RGB components
 * @retval None
 */
void cis_StartPolynomialCalibration(uint16_t iterationNb, struct RGB_Calibration* rgbCalibration)
{
	struct RAWImage RAWImage = {0};
	static float32_t cisDataCpy_f32[CIS_ADC_BUFF_SIZE * 3] = {0};

	// For each calibration iteration
	cis_calibrateLeds();  // Calibrate LEDs

	// Retrieve RGB images for this iteration
	float32_t redLine[CAL_POWER_NB_INC][CIS_PIXELS_NB], greenLine[CAL_POWER_NB_INC][CIS_PIXELS_NB], blueLine[CAL_POWER_NB_INC][CIS_PIXELS_NB];

	// Cycle through power increments
	for (uint16_t power_cycle = 0; power_cycle < CAL_POWER_NB_INC; power_cycle++)
	{
		// Set LED power and get RGB image
		cis_setLedsPower(&cisLeds_Calibration, power_cycle * CAL_POWER_INC + CAL_POWER_MIN);

		cis_getRAWImage(cisDataCpy_f32, iterationNb);
		cis_ConvertRAWImageToIntArray(cisDataCpy_f32, &RAWImage);
		arm_copy_f32(RAWImage.redLine, &redLine[power_cycle][0], CIS_PIXELS_NB);
		arm_copy_f32(RAWImage.greenLine, &greenLine[power_cycle][0], CIS_PIXELS_NB);
		arm_copy_f32(RAWImage.blueLine, &blueLine[power_cycle][0], CIS_PIXELS_NB);
	}

	// For each pixel, calculate the calibration coefficients
	for (uint16_t pixel = 0; pixel < CIS_PIXELS_NB; pixel++)
	{
		// Create x and y arrays for the least squares method
		float32_t x[CAL_POWER_NB_INC], y[CAL_POWER_NB_INC];

		// Fill in arrays and calculate least squares for red line
		for (uint16_t power_cycle = 0; power_cycle < CAL_POWER_NB_INC; power_cycle++)
		{
			x[power_cycle] = (CIS_ADC_MAX_VALUE * (power_cycle * CAL_POWER_INC + CAL_POWER_MIN)) / 100.00;
			y[power_cycle] = redLine[power_cycle][pixel];
		}
		cis_leastSquares(x, y, CAL_POWER_NB_INC, &rgbCalibration->red[pixel].a, &rgbCalibration->red[pixel].b, &rgbCalibration->red[pixel].c);

		// Fill in arrays and calculate least squares for green line
		for (uint16_t power_cycle = 0; power_cycle < CAL_POWER_NB_INC; power_cycle++)
		{
			x[power_cycle] = (CIS_ADC_MAX_VALUE * (power_cycle * CAL_POWER_INC + CAL_POWER_MIN)) / 100.00;
			y[power_cycle] = greenLine[power_cycle][pixel];
		}
		cis_leastSquares(x, y, CAL_POWER_NB_INC, &rgbCalibration->green[pixel].a, &rgbCalibration->green[pixel].b, &rgbCalibration->green[pixel].c);

		// Fill in arrays and calculate least squares for blue line
		for (uint16_t power_cycle = 0; power_cycle < CAL_POWER_NB_INC; power_cycle++)
		{
			x[power_cycle] = (CIS_ADC_MAX_VALUE * (power_cycle * CAL_POWER_INC + CAL_POWER_MIN)) / 100.00;
			y[power_cycle] = blueLine[power_cycle][pixel];
		}
		cis_leastSquares(x, y, CAL_POWER_NB_INC, &rgbCalibration->blue[pixel].a, &rgbCalibration->blue[pixel].b, &rgbCalibration->blue[pixel].c);
	}
}

void cis_StartPolynomialCalibration_WIP(uint16_t iterationNb)
{
	/* Set header description */
	printf("------ START CALIBRATION ------\n");
	/*-------- 1 --------*/
	// Read black and white level

	shared_var.cis_cal_progressbar = 0;
	shared_var.cis_cal_state = CIS_CAL_PLACE_ON_WHITE;
}


/**
 * @brief  Applique la calibration à une ligne de couleur
 * @param   struct ColorLine*  ColorLine - ligne de couleur et coefficients de calibration
 * @retval None
 */
void applyCalibrationToColorLine( struct ColorLine* colorLine)
{
	float32_t temp1[CIS_PIXELS_NB], temp2[CIS_PIXELS_NB];

	// Calculate X^2 and apply calibration
	arm_mult_f32(colorLine->line, colorLine->line, temp1, CIS_PIXELS_NB);
	arm_mult_f32(temp1, &(colorLine->coeff->a), temp1, CIS_PIXELS_NB);
	arm_mult_f32(colorLine->line, &(colorLine->coeff->b), temp2, CIS_PIXELS_NB);
	arm_add_f32(temp1, temp2, temp1, CIS_PIXELS_NB);
	arm_offset_f32(temp1, colorLine->coeff->c, colorLine->line, CIS_PIXELS_NB);
}

/**
 * @brief  Applique la calibration à une image RGB
 * @param  RGBImage* image - image brute
 * @retval None
 */
void cis_ApplyCalibration(struct RAWImage* RAWImage)
{
	/*
    applyCalibrationToColorLine(&(RAWImage->redLine));
    applyCalibrationToColorLine(&(RAWImage->greenLine));
    applyCalibrationToColorLine(&(RAWImage->blueLine));
	 */
}

/**
 * @brief  Convert an RGB image to a single int32 array
 * @param  struct RGBImage* image - calibrated image
 * @param  uint32_t* output - output array, must be pre-allocated with size CIS_PIXELS_NB
 * @retval None
 */
void cis_ConvertRAWImageToRGBImage(struct RAWImage* RAWImage, int32_t* RGBimage)
{
	uint32_t i;

	for(i = 0; i < CIS_PIXELS_NB; i++)
	{
		int32_t r = (int32_t)(RAWImage->redLine[i]) >> 4;   // Convert float to uint32 and keep only lower 8 bits
		int32_t g = (int32_t)(RAWImage->greenLine[i]) >> 4; // Convert float to uint32 and keep only lower 8 bits
		int32_t b = (int32_t)(RAWImage->blueLine[i]) >> 4;  // Convert float to uint32 and keep only lower 8 bits

		// Combine R, G, and B into a single uint32
		RGBimage[i] = (b << 16) | (g << 8) | r;
	}
}
