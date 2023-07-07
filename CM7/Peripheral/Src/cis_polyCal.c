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
#define LEDS_MAX_POMER				LEDS_MAX_PWM

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
void leastSquares(float32_t* x, float32_t* y, uint32_t n, float32_t* a, float32_t* b, float32_t* c);
void cis_calibrateLeds(struct cisLeds_Power2PWM *power2PWM);

/* Private user code ---------------------------------------------------------*/

void cis_leastSquares(float32_t* x, float32_t* y, uint32_t n, float32_t* a, float32_t* b, float32_t* c) {
    float32_t sum_x = 0, sum_y = 0, sum_xx = 0, sum_xy = 0, sum_xxx = 0, sum_xxy = 0;

    for(uint32_t i = 0; i < n; i++) {
        sum_x += x[i];
        sum_y += y[i];
        sum_xx += x[i]*x[i];
        sum_xy += x[i]*y[i];
        sum_xxx += x[i]*x[i]*x[i];
        sum_xxy += x[i]*x[i]*y[i];
    }

    // Matrice des sommes (A dans AX=B)
    float32_t matrixA_data[9] = { n, sum_x, sum_xx,
                                  sum_x, sum_xx, sum_xxx,
                                  sum_xx, sum_xxx, sum_xxx*sum_x-sum_xx*sum_xx }; // Adjusted this line

    // Vecteur des sommes (B dans AX=B)
    float32_t vectorB_data[3] = { sum_y, sum_xy, sum_xxy };

    // Prepare CMSIS structures
    arm_matrix_instance_f32 matrixA, vectorB, vectorX;
    arm_mat_init_f32(&matrixA, 3, 3, matrixA_data);
    arm_mat_init_f32(&vectorB, 3, 1, vectorB_data);
    arm_mat_init_f32(&vectorX, 3, 1, vectorB_data); // Reuse vectorB_data for storing result

    // Solve AX = B for X
    arm_mat_solve_f32(&matrixA, &vectorB, &vectorX);

    // Coefficients are stored in vectorX
    *a = vectorX.pData[0];
    *b = vectorX.pData[1];
    *c = vectorX.pData[2];
}

void cis_calibrateLeds(struct cisLeds_Power2PWM *power2PWM)
{
	int32_t led_PWM, power;
	float32_t redLine[CIS_LINE_SIZE], greenLine[CIS_LINE_SIZE], blueLine[CIS_LINE_SIZE];
	float32_t meanRedValue[LEDS_MAX_PWM], meanGreenValue[LEDS_MAX_PWM], meanBlueValue[LEDS_MAX_PWM];
	float32_t redRange, greenRange, blueRange;

	for (led_PWM = LEDS_MAX_PWM; --led_PWM >= 0;)
	{
		cis_LedPowerAdj(led_PWM, led_PWM, led_PWM);
		cis_GetRGBImage(redLine, greenLine, blueLine, LEDS_CAL_MEASURE_CYCLES);
		arm_mean_f32(redLine, CIS_LINE_SIZE, &meanRedValue[led_PWM]);
		arm_mean_f32(greenLine, CIS_LINE_SIZE, &meanGreenValue[led_PWM]);
		arm_mean_f32(blueLine, CIS_LINE_SIZE, &meanBlueValue[led_PWM]);
	}
	redRange = meanRedValue[LEDS_MAX_PWM - 1] - meanRedValue[0];
	greenRange = meanGreenValue[LEDS_MAX_PWM - 1] - meanGreenValue[0];
	blueRange = meanBlueValue[LEDS_MAX_PWM - 1] - meanBlueValue[0];

	for (power = LEDS_MAX_POMER; --power >= 0;)
	{
		power2PWM->redLed[power] = ((meanRedValue[power] - meanRedValue[0]) * 100) / redRange;
		power2PWM->greenLed[power] = ((meanGreenValue[power] - meanGreenValue[0]) * 100) / greenRange;
		power2PWM->blueLed[power] = ((meanBlueValue[power] - meanBlueValue[0]) * 100) / blueRange;
	}

#ifdef PRINT_CIS_CALIBRATION
	printf("Print CIS LEDs Calibration\n");

	for (power = LEDS_MAX_POMER; --power >= 0;)
	{
		printf("Power = %d %%, PWM RGB = %d, %d, %d\n", (int)power, (int)power2PWM->redLed[power], (int)power2PWM->greenLed[power], (int)power2PWM->blueLed[power]);
	}

#endif
}

void cis_setLedsPower(struct cisLeds_Power2PWM *power2PWM, int32_t power)
{
	cis_LedPowerAdj(power2PWM->redLed[power], power2PWM->greenLed[power], power2PWM->blueLed[power]);
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

	//cis_LedPowerAdj(95);
	HAL_Delay(200);
	cis_GetRGBImage(redLine, greenLine, blueLine, iterationNb);
	HAL_Delay(200);
	//cis_LedPowerAdj(50);
	HAL_Delay(200);
	cis_GetRGBImage(redLine, greenLine, blueLine, iterationNb);
	HAL_Delay(200);
	//cis_LedPowerAdj(5);
	HAL_Delay(200);
	cis_GetRGBImage(redLine, greenLine, blueLine, iterationNb);
}
