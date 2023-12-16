/**
 ******************************************************************************
 * @file           : cis_polyCal.c
 * @brief          :
 *
 *
Calcul du Polynôme de Réponse pour Chaque Pixel :
Pour chaque pixel, calculez les coefficients du polynôme (a, b, c) à l'aide de la méthode des moindres carrés.

Calcul de la Courbe de Régression Moyenne :
Calculez la courbe moyenne pour tous les pixels. Cela signifie trouver les valeurs moyennes des coefficients a, b, c pour l'ensemble de l'image.

Correction des Courbes Individuelles :
Ajustez chaque courbe de pixel individuelle pour qu'elle corresponde à la courbe moyenne.

Calcul de la Fonction de Scaling :
Déterminez le minimum et le maximum de la courbe de référence moyenne.
Calculez la fonction de scaling pour ajuster ces valeurs à la plage complète de 0 à 4095.

Application du Scaling et Écrêtage :
Appliquez la fonction de scaling à chaque courbe individuelle corrigée.
Si les valeurs calibrées dépassent 4095 ou sont inférieures à 0, écrêtez-les à ces limites.

Validation :
Assurez-vous que toutes les courbes corrigées et scalées respectent la plage de 0 à 4095 et sont bien alignées avec la courbe de référence moyenne.
 *
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

#include "cis.h"

#include "cis_polyCal.h"

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
void cis_leastSquares(double* x, double* y, uint32_t n, double* a, double* b, double* c);
static void cis_setLedsPower(int32_t power);
void cis_StartPolynomialCalibration(struct RGB_Calibration* rgbCalibration);

/* Private user code ---------------------------------------------------------*/

int cis_polyCalInit(void)
{
	return 0;
}

void cis_getMeanAtLedPower(struct RAWImage* RAWImage, struct cisLeds_Calibration *cisLeds_Calibration, int32_t led_PWM)
{
	cis_LedPowerAdj(led_PWM, led_PWM, led_PWM);

	float32_t cisDataCpy_f32[CIS_ADC_BUFF_SIZE * 3] = {0};

	// Capture a raw image
	cis_getRAWImage(cisDataCpy_f32, CIS_LEDS_CAL_MEASURE_CYCLES);

	// Convert the raw image to an integer array
	cis_ConvertRAWImageToFloatArray(cisDataCpy_f32, RAWImage);

	// Calculate the mean red, green, and blue values
	arm_mean_f32(RAWImage->redLine, CIS_PIXELS_NB, &cisLeds_Calibration->redMeanAtLedPower);
	arm_mean_f32(RAWImage->greenLine, CIS_PIXELS_NB, &cisLeds_Calibration->greenMeanAtLedPower);
	arm_mean_f32(RAWImage->blueLine, CIS_PIXELS_NB, &cisLeds_Calibration->blueMeanAtLedPower);
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
	cisLeds_Calibration.redLed_maxPulse = CIS_LED_RED_OFF;
	cisLeds_Calibration.greenLed_maxPulse = CIS_LED_GREEN_OFF;
	cisLeds_Calibration.blueLed_maxPulse = CIS_LED_BLUE_OFF;

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
	for (power = CIS_LEDS_MAX_POMER; --power >= 0;)
	{
		printf("Power = %d %%, PWM RGB = %d, %d, %d\n", (int)power, (int)cisLeds_Calibration.redLed_power2PWM[power], (int)cisLeds_Calibration.greenLed_power2PWM[power], (int)cisLeds_Calibration.blueLed_power2PWM[power]);
	}

#endif
}

void cis_setLedsPower(int32_t power)
{
	cis_LedPowerAdj(cisLeds_Calibration.redLed_power2PWM[power], cisLeds_Calibration.greenLed_power2PWM[power], cisLeds_Calibration.blueLed_power2PWM[power]);
}

/**
 * @brief  CIS start calibration
 * @param  iterationNb: calibration iteration
 * @param  rgbCalibration: pointer to structure containing calibration coefficients for all three RGB components
 * @retval None
 */
void cis_StartCalibration(uint16_t iterationNb)
{
	cis_calibrateLeds();

	printf("------- CIS CALIBRATION -------\n");

	float32_t cisDataCpy_f32[CIS_ADC_BUFF_SIZE * 3] = {0};

	int32_t Leds_Power_Array[11] = {0, 10, 20, 30, 40, 50, 60, 70, 80, 90, 100};

	// Cycle through power increments
	for (int32_t power_idx = 10; power_idx >= 0; power_idx--)
	{
		//uint32_t current_ADC_value = (Leds_Power_Array[power_idx] * 4095) / 100;

		cis_setLedsPower(Leds_Power_Array[power_idx]);
		HAL_Delay(5);

		cis_getRAWImage(cisDataCpy_f32, iterationNb);
		cis_ConvertRAWImageToFloatArray(cisDataCpy_f32, &RAWImageCalibration[power_idx]);
	}

	cis_setLedsPower(100);

	cis_StartPolynomialCalibration(&rgbCalibration);
}

void calculateAverageCoefficients(struct RGB_Calibration* rgbCalibration, float32_t* avg_a, float32_t* avg_b, float32_t* avg_c, uint32_t numPixels) {
    float32_t sum_a = 0.0f, sum_b = 0.0f, sum_c = 0.0f;

    for (uint32_t i = 0; i < numPixels; i++) {
        sum_a += rgbCalibration->red[i].a;
        sum_b += rgbCalibration->red[i].b;
        sum_c += rgbCalibration->red[i].c;
    }

    *avg_c = sum_a / numPixels;
    *avg_b = sum_b / numPixels;
    *avg_a = sum_c / numPixels;

    printf("Average Coefficients: Red(a: %f, b: %f, c: %f)\n", *avg_a, *avg_b, *avg_c);
}

void adjustIndividualCurves(struct RGB_Calibration* rgbCalibration, float32_t avg_a, float32_t avg_b, float32_t avg_c, uint32_t numPixels) {
    for (uint32_t i = 0; i < numPixels; i++) {
        rgbCalibration->red[i].a = avg_a;
        rgbCalibration->red[i].b = avg_b;
        rgbCalibration->red[i].c = avg_c;
    }

    // Log pour les 10 premiers pixels après ajustement
    for (uint32_t i = 0; i < 10; i++) {
        printf("Pixel %d - Adjusted Coefficients: Red(a: %f, b: %f, c: %f)\n", (int)i, rgbCalibration->red[i].a, rgbCalibration->red[i].b, rgbCalibration->red[i].c);
    }
}

void calculateScalingFunction(float32_t avg_a, float32_t avg_b, float32_t avg_c, float32_t* scale, float32_t* offset) {
    float32_t minVal = avg_a; // Supposons que ceci est le minimum de la courbe de référence moyenne.
    float32_t maxVal = avg_a + avg_b * 4095 + avg_c * 4095 * 4095; // Supposons que ceci est le maximum.

    *scale = 4095.0f / (maxVal - minVal);
    *offset = -minVal * (*scale);
}

void applyScalingAndClipping(struct RGB_Calibration* rgbCalibration, float32_t scale, float32_t offset, uint32_t numPixels) {
    for (uint32_t i = 0; i < numPixels; i++) {
        rgbCalibration->red[i].a = rgbCalibration->red[i].a * scale + offset;
        // Clip if out of bounds
        if (rgbCalibration->red[i].a > 4095.0f) rgbCalibration->red[i].a = 4095.0f;
        if (rgbCalibration->red[i].a < 0.0f) rgbCalibration->red[i].a = 0.0f;

        // Répétez pour les coefficients b et c, si nécessaire.
    }

    // Log pour les 10 premiers pixels après scaling et écrêtage
    for (uint32_t i = 0; i < 10; i++) {
        printf("Pixel %d - Scaled and Clipped Coefficients: Red(a: %f)\n", (int)i, rgbCalibration->red[i].a);
        // Répétez pour les coefficients b et c si nécessaire
    }
}

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

// Function to multiply two 3x3 matrices
void multiplyMatrix3x3(double mat1[3][3], double mat2[3][3], double result[3][3]) {
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            result[i][j] = 0.0;
            for (int k = 0; k < 3; k++) {
                result[i][j] += mat1[i][k] * mat2[k][j];
            }
        }
    }
}

// Function to inverse a 3x3 matrix
int inverseMatrix3x3(double mat[3][3], double inv[3][3]) {
    double det = mat[0][0] * (mat[1][1] * mat[2][2] - mat[2][1] * mat[1][2]) -
                 mat[0][1] * (mat[1][0] * mat[2][2] - mat[1][2] * mat[2][0]) +
                 mat[0][2] * (mat[1][0] * mat[2][1] - mat[1][1] * mat[2][0]);

    printf("Determinant: %f\n", det);
    if (fabs(det) < 1e-10) {
        printf("Determinant is too close to zero, inversion may be unstable.\n");
        return 0;
    }

    double inv_det = 1.0 / det;

    inv[0][0] = (mat[1][1] * mat[2][2] - mat[2][1] * mat[1][2]) * inv_det;
    inv[0][1] = (mat[0][2] * mat[2][1] - mat[0][1] * mat[2][2]) * inv_det;
    inv[0][2] = (mat[0][1] * mat[1][2] - mat[0][2] * mat[1][1]) * inv_det;
    inv[1][0] = (mat[1][2] * mat[2][0] - mat[1][0] * mat[2][2]) * inv_det;
    inv[1][1] = (mat[0][0] * mat[2][2] - mat[0][2] * mat[2][0]) * inv_det;
    inv[1][2] = (mat[1][0] * mat[0][2] - mat[0][0] * mat[1][2]) * inv_det;
    inv[2][0] = (mat[1][0] * mat[2][1] - mat[2][0] * mat[1][1]) * inv_det;
    inv[2][1] = (mat[2][0] * mat[0][1] - mat[0][0] * mat[2][1]) * inv_det;
    inv[2][2] = (mat[0][0] * mat[1][1] - mat[1][0] * mat[0][1]) * inv_det;

    // Debug: Verify matrix inversion by checking if inv(A) * A = I
    double identity[3][3];
    multiplyMatrix3x3(inv, mat, identity);
    printf("Verifying matrix inversion, the following should be close to an identity matrix:\n");
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            printf("%f ", identity[i][j]);
        }
        printf("\n");
    }

    return 1;
}

// Function to multiply a 3x3 matrix by a 3x1 vector
void multiplyMatrix3x1(double mat[3][3], double vec[3], double result[3]) {
    for (int i = 0; i < 3; i++) {
        result[i] = 0.0;
        for (int j = 0; j < 3; j++) {
            result[i] += mat[i][j] * vec[j];
        }
    }
}

void cis_leastSquares(double* x, double* y, uint32_t n, double* a, double* b, double* c) {
    // Initialization of necessary sums
    double sum_x = 0.0, sum_y = 0.0, sum_x2 = 0.0, sum_x3 = 0.0, sum_x4 = 0.0, sum_xy = 0.0, sum_x2y = 0.0;

    // Calculation of sums for the necessary terms
    for (uint32_t i = 0; i < n; i++) {
        double xi = x[i];
        double yi = y[i];
        double xi2 = xi * xi;
        sum_x += xi;
        sum_y += yi;
        sum_x2 += xi2;
        sum_x3 += xi2 * xi;
        sum_x4 += xi2 * xi2;
        sum_xy += xi * yi;
        sum_x2y += xi2 * yi;
    }

    // Construction of matrix A and vector B
    double A[3][3] = {
        {n, sum_x, sum_x2},
        {sum_x, sum_x2, sum_x3},
        {sum_x2, sum_x3, sum_x4}
    };
    double B[3] = {sum_y, sum_xy, sum_x2y};

    // Inversion of matrix A
    double A_inv[3][3];
    if (!inverseMatrix3x3(A, A_inv)) {
        printf("Matrix inversion failed.\n");
        return;
    }

    // Multiplication of the inverted matrix by vector B
    double X[3]; // The vector for the coefficients
    multiplyMatrix3x1(A_inv, B, X);

    // Assignment of the coefficients
    *a = X[0];
    *b = X[1];
    *c = X[2];

    // Debug: Print the coefficients
    printf("Coefficients: a=%f, b=%f, c=%f\n", *a, *b, *c);
}



void cis_StartPolynomialCalibration(struct RGB_Calibration* rgbCalibration)
{
	double luminosityLevels[11] = {
        0.00, 409.50, 819.00, 1228.50, 1638.00,
        2047.50, 2457.00, 2866.50, 3276.00, 3685.50,
        4095.00
    };

    memset(rgbCalibration, 0, sizeof(struct RGB_Calibration));

    // Calcul des coefficients pour chaque pixel
    for (uint16_t pixel = 0; pixel < 1; pixel++)
    {
       double yRed[11], yGreen[11], yBlue[11];
        for (uint16_t lvl = 0; lvl < 11; lvl++)
        {
            yRed[lvl] = (double)RAWImageCalibration[lvl].redLine[pixel];
            yGreen[lvl] = (double)RAWImageCalibration[lvl].greenLine[pixel];
            yBlue[lvl] = (double)RAWImageCalibration[lvl].blueLine[pixel];
        }

    	double yTest[11] = {
    		    1003.049988, 1158.200073, 1345.300049, 1517.800049, 1740.050049,
    		    1943.400024, 2134.550049, 2300.949951, 2484.350098, 2613.800049,
    		    2622.900146
        };

        cis_leastSquares(yTest, luminosityLevels, 11, (double*)&rgbCalibration->red[pixel].a, (double*)&rgbCalibration->red[pixel].b, (double*)&rgbCalibration->red[pixel].c);
        //cis_leastSquares(yGreen, luminosityLevels, 11, (double*)&rgbCalibration->green[pixel].a, (double*)&rgbCalibration->green[pixel].b, (double*)&rgbCalibration->green[pixel].c);
        //cis_leastSquares(yBlue, luminosityLevels, 11, (double*)&rgbCalibration->blue[pixel].a, (double*)&rgbCalibration->blue[pixel].b, (double*)&rgbCalibration->blue[pixel].c);

        // Log pour les 10 premiers pixels
        if (pixel < 1)
        {
            printf("Pixel %d - Coefficients: Red(a: %f, b: %f, c: %f), Green(a: %f, b: %f, c: %f), Blue(a: %f, b: %f, c: %f)\n",
                pixel,
                rgbCalibration->red[pixel].a, rgbCalibration->red[pixel].b, rgbCalibration->red[pixel].c,
                rgbCalibration->green[pixel].a, rgbCalibration->green[pixel].b, rgbCalibration->green[pixel].c,
                rgbCalibration->blue[pixel].a, rgbCalibration->blue[pixel].b, rgbCalibration->blue[pixel].c);

            // Log des valeurs yRed, yGreen, yBlue pour le pixel actuel
            printf("Pixel %d - Values: Red(", pixel);
            for (int i = 0; i < 11; ++i) {
                printf("%f", yRed[i]);
                if (i < 10) printf(", ");
            }
            printf("), Green(");
            for (int i = 0; i < 11; ++i) {
                printf("%f", yGreen[i]);
                if (i < 10) printf(", ");
            }
            printf("), Blue(");
            for (int i = 0; i < 11; ++i) {
                printf("%f", yBlue[i]);
                if (i < 10) printf(", ");
            }
            printf(")\n");
        }
    }
}

void applyCalibrationToColorLine(float32_t* colorLine, struct CalibrationCoefficients* coeff, uint32_t numPixels) {
    float32_t temp1[numPixels], temp2[numPixels];

    // Effectuez la calibration en utilisant les coefficients normalisés
    arm_mult_f32(colorLine, colorLine, temp1, numPixels); // Calculate colorLine^2 and store in temp1
    arm_scale_f32(temp1, coeff->a, temp1, numPixels); // Scale a * colorLine^2 and store in temp1
    arm_scale_f32(colorLine, coeff->b, temp2, numPixels); // Scale b * colorLine and store in temp2
    arm_add_f32(temp1, temp2, temp1, numPixels); // Add a * colorLine^2 and b * colorLine and store in temp1
    arm_offset_f32(temp1, coeff->c, colorLine, numPixels); // Add c to each element of temp1 and store back in colorLine
}


void cis_ApplyCalibration(struct RAWImage* RAWImage, struct RGB_Calibration* rgbCalibration)
{
    // Apply calibration to the red color line
    applyCalibrationToColorLine(RAWImage->redLine, rgbCalibration->red, CIS_PIXELS_NB);

    // Apply calibration to the green color line
    applyCalibrationToColorLine(RAWImage->greenLine, rgbCalibration->green, CIS_PIXELS_NB);

    // Apply calibration to the blue color line
    applyCalibrationToColorLine(RAWImage->blueLine, rgbCalibration->blue, CIS_PIXELS_NB);
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
		//RGBimage[i] = 0;

		int32_t r = (int32_t)(RAWImage->redLine[i]) >> 4;   // Convert float to uint32 and keep only lower 8 bits
		int32_t g = (int32_t)(RAWImage->greenLine[i]) >> 4; // Convert float to uint32 and keep only lower 8 bits
		int32_t b = (int32_t)(RAWImage->blueLine[i]) >> 4;  // Convert float to uint32 and keep only lower 8 bits

		// Combine R, G, and B into a single uint32
		RGBimage[i] = (b << 16) | (g << 8) | r;
	}
}
