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
static void leastSquares(float32_t* x, float32_t* y, uint32_t n, float32_t* a, float32_t* b, float32_t* c);
static void cis_setLedsPower(int32_t power);
void cis_StartPolynomialCalibration(struct RGB_Calibration* rgbCalibration);

/* Private user code ---------------------------------------------------------*/

int cis_polyCalInit(void)
{
	// Définir le nombre d'itérations de calibration
	uint16_t iterationNb = 5; // Changer cela selon votre besoin

	// Appel de la fonction cis_StartPolynomialCalibration
	//cis_StartPolynomialCalibration(iterationNb, &rgbCalibration);

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
	cis_LedPowerAdj(led_PWM, led_PWM, led_PWM);

	float32_t cisDataCpy_f32[CIS_ADC_BUFF_SIZE * 3] = {0};

	// Capture a raw image
	cis_getRAWImage(cisDataCpy_f32, CIS_LEDS_CAL_MEASURE_CYCLES);

	// Convert the raw image to an integer array
	cis_ConvertRAWImageToFloatArray(cisDataCpy_f32, &RAWImage);

	// Calculate the mean red, green, and blue values
	arm_mean_f32(RAWImage.redLine, CIS_LINE_SIZE, &cisLeds_Calibration->redMeanAtLedPower);
	arm_mean_f32(RAWImage.greenLine, CIS_LINE_SIZE, &cisLeds_Calibration->greenMeanAtLedPower);
	arm_mean_f32(RAWImage.blueLine, CIS_LINE_SIZE, &cisLeds_Calibration->blueMeanAtLedPower);
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

	memset(cisDataCpy_f32, 0, CIS_ADC_BUFF_SIZE * 3 * sizeof(uint32_t));

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

	cis_StartPolynomialCalibration(rgbCalibration);
}

float calibratePixel(int pixelIndex, float rawValue, int interval, float32_t *colorLine) {
    // Calculer les valeurs de x1, x2, y1 et y2
    float x1 = interval * 409.5;
    float x2 = (interval + 1) * 409.5;
    float y1 = colorLine[interval];
    float y2 = colorLine[interval + 1];

    // Utiliser la régression linéaire pour estimer la valeur calibrée
    return y1 + (rawValue - x1) * (y2 - y1) / (x2 - x1);
}

void calibrate(struct RAWImage* rawImage)
{
    // Parcourir chaque pixel
    for (int i = 0; i < CIS_PIXELS_NB; ++i)
    {
        // Trouver l'intervalle où se trouve la valeur brute
        int interval = rawImage->redLine[i] / 409.5; // Supposer que les valeurs sont réparties uniformément entre 0 et 4095
        if (interval >= 10) { // Si on dépasse la limite, retourner la dernière valeur connue
            rawImage->redLine[i] = RAWImageCalibration[10].redLine[i];
        }
        // Faire la calibration pour chaque canal de couleur
        rawImage->redLine[i] = calibratePixel(i, rawImage->redLine[i], interval, RAWImageCalibration[interval].redLine);

        // Trouver l'intervalle où se trouve la valeur brute
        interval = rawImage->greenLine[i] / 409.5; // Supposer que les valeurs sont réparties uniformément entre 0 et 4095
        if (interval >= 10) { // Si on dépasse la limite, retourner la dernière valeur connue
            rawImage->greenLine[i] = RAWImageCalibration[10].greenLine[i];
        }
        rawImage->greenLine[i] = calibratePixel(i, rawImage->greenLine[i], interval, RAWImageCalibration[interval].greenLine);

        // Trouver l'intervalle où se trouve la valeur brute
        interval = rawImage->blueLine[i] / 409.5; // Supposer que les valeurs sont réparties uniformément entre 0 et 4095
        if (interval >= 10) { // Si on dépasse la limite, retourner la dernière valeur connue
            rawImage->blueLine[i] = RAWImageCalibration[10].blueLine[i];
        }
        rawImage->blueLine[i] = calibratePixel(i, rawImage->blueLine[i], interval, RAWImageCalibration[interval].blueLine);
    }
}


/**
 * @brief  CIS start calibration
 * @param  iterationNb: calibration iteration
 * @param  rgbCalibration: pointer to structure containing calibration coefficients for all three RGB components
 * @retval None
 */
void cis_StartPolynomialCalibration(struct RGB_Calibration* rgbCalibration)
{
    // Assumer que les niveaux de luminosité sont stockés dans un tableau séparé:
	float32_t luminosityLevels[11] = {
	    0.0f, 409.5f, 819.0f, 1228.5f, 1638.0f,
	    2047.5f, 2457.0f, 2866.5f, 3276.0f, 3685.5f,
	    4095.0f
	};

    // Pour chaque pixel, calculer les coefficients de calibration
    for (uint16_t pixel = 0; pixel < CIS_PIXELS_NB; pixel++) {
        // Créer les tableaux x et y pour la méthode des moindres carrés pour chaque couleur
        float32_t yRed[11], yGreen[11], yBlue[11];

        // Remplir les tableaux avec les données de calibration pour chaque couleur et chaque niveau de luminosité
        for (uint16_t lvl = 0; lvl < 11; lvl++) {
            yRed[lvl] = RAWImageCalibration[lvl].redLine[pixel];
            yGreen[lvl] = RAWImageCalibration[lvl].greenLine[pixel];
            yBlue[lvl] = RAWImageCalibration[lvl].blueLine[pixel];
        }

        // Calculer les moindres carrés pour chaque couleur
        cis_leastSquares(luminosityLevels, yRed, 11, &rgbCalibration->red[pixel].a, &rgbCalibration->red[pixel].b, &rgbCalibration->red[pixel].c);
        cis_leastSquares(luminosityLevels, yGreen, 11, &rgbCalibration->green[pixel].a, &rgbCalibration->green[pixel].b, &rgbCalibration->green[pixel].c);
        cis_leastSquares(luminosityLevels, yBlue, 11, &rgbCalibration->blue[pixel].a, &rgbCalibration->blue[pixel].b, &rgbCalibration->blue[pixel].c);
    }
    // La fonction cis_leastSquares() doit être adaptée ou écrite pour traiter une régression polynomiale si ce n'est déjà le cas.
}


void applyCalibrationToColorLine(float32_t* colorLine, struct CalibrationCoefficients* coeff, uint32_t numPixels)
{
    float32_t temp1[numPixels], temp2[numPixels];

    // Calculate colorLine^2 and store in temp1
    arm_mult_f32(colorLine, colorLine, temp1, numPixels);

    // Calculate a * colorLine^2 and store in temp1
    arm_scale_f32(temp1, coeff->a, temp1, numPixels);

    // Calculate b * colorLine and store in temp2
    arm_scale_f32(colorLine, coeff->b, temp2, numPixels);

    // Add a * colorLine^2 and b * colorLine and store in temp1
    arm_add_f32(temp1, temp2, temp1, numPixels);

    // Add c to each element of temp1 and store back in colorLine
    arm_offset_f32(temp1, coeff->c, colorLine, numPixels);
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
