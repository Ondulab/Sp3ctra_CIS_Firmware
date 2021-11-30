/**
 ******************************************************************************
 * @file           : cis_BW_.c
 * @brief          :
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"
#include "main.h"
#include "config.h"
#include "basetypes.h"
#include "tim.h"
#include "adc.h"
#include "buttons.h"

#include "arm_math.h"

#include "stdlib.h"
#include "stdio.h"
#include "stdbool.h"

#include "ssd1362.h"
#include "eeprom.h"

#include "cis.h"

/* Private includes ----------------------------------------------------------*/

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
#ifdef CIS_BW
/* Definition of ADCx conversions data table size this buffer contains BW conversion */
static int32_t *cisData = NULL;
static int32_t *cisWhiteCalData     = NULL;
static int32_t *cisBlackCalData     = NULL;
static int32_t *cisOffsetCalData    = NULL;
static float32_t *cisGainsCalData 	= NULL;

static int32_t minBlackPix = 0;
static int32_t maxBlackPix = 0;
static int32_t maxWhitePix = 0;

static const uint16_t blackCalibVirtAddVar = 0x2222;
static const uint16_t whiteCalibVirtAddVar = 0x5555;

//static uint16_t cisData[((CIS_END_CAPTURE * CIS_ADC_OUT_LINES) / CIS_IFFT_OVERSAMPLING_RATIO)]; // for debug

#else
/* Definition of ADCx conversions data table size this buffer contains RGB conversion */
#define ADC_CONVERTED_DATA_BUFFER_SIZE (CIS_END_CAPTURE * 3) /* Size of array cisData[] */
ALIGN_32BYTES (static uint8_t cisData[ADC_CONVERTED_DATA_BUFFER_SIZE]);
#endif



CIS_BUFF_StateTypeDef  cisBufferState[3] = {0};

/* Variable containing ADC conversions data */

/* Private function prototypes -----------------------------------------------*/
void cis_StartCalibration(uint16_t VirtAddVar, uint16_t iterationNb);
void cis_LoadCalibration(uint16_t VirtAddVar, int32_t * cisCalData, int32_t * minPix, int32_t * maxPix);
void cis_TIM_CLK_Init(void);
void cis_TIM_SP_Init(void);
void cis_TIM_LED_R_Init(void);
void cis_TIM_LED_G_Init(void);
void cis_TIM_LED_B_Init(void);

void cis_ADC_Init(void);
void cis_ImageFilterBW(int32_t *cis_buff);

static void cis_CalibrationMenu(void);

/* Private user code ---------------------------------------------------------*/

/**
 * @brief  CIS init
 * @param  Void
 * @retval None
 */
void cis_Init(void)
{
	printf("----------- CIS INIT ----------\n");
	printf("-------------------------------\n");

	// Enable 5V power DC/DC for display
	HAL_GPIO_WritePin(EN_5V_GPIO_Port, EN_5V_Pin, GPIO_PIN_SET);

	// Allocate the contiguous memory area for storage cis datas
	cisData = malloc(CIS_ADC_BUFF_SIZE * sizeof(uint32_t));
	if (cisData == NULL)
	{
		Error_Handler();
	}
	cisWhiteCalData = malloc(CIS_EFFECTIVE_PIXELS * sizeof(uint32_t));
	if (cisData == NULL)
	{
		Error_Handler();
	}
	cisBlackCalData = malloc(CIS_EFFECTIVE_PIXELS * sizeof(uint32_t));
	if (cisData == NULL)
	{
		Error_Handler();
	}
	cisOffsetCalData = malloc(CIS_EFFECTIVE_PIXELS * sizeof(uint32_t));
	if (cisData == NULL)
	{
		Error_Handler();
	}
	cisGainsCalData = malloc(CIS_EFFECTIVE_PIXELS * sizeof(float32_t));
	if (cisData == NULL)
	{
		Error_Handler();
	}

	// Clear all buffers
	memset(cisData, 0, CIS_ADC_BUFF_SIZE * sizeof(uint32_t));
	memset(cisWhiteCalData, 0, CIS_EFFECTIVE_PIXELS * sizeof(uint32_t));
	memset(cisBlackCalData, 0, CIS_EFFECTIVE_PIXELS * sizeof(uint32_t));
	memset(cisOffsetCalData, 0, CIS_EFFECTIVE_PIXELS * sizeof(uint32_t));
	memset(cisGainsCalData, 0, CIS_EFFECTIVE_PIXELS * sizeof(float32_t));

#ifdef CIS_400DPI
	HAL_GPIO_WritePin(CIS_RS_GPIO_Port, CIS_RS_Pin, GPIO_PIN_RESET); //SET : 200DPI   RESET : 400DPI
#else
	HAL_GPIO_WritePin(CIS_RS_GPIO_Port, CIS_RS_Pin, GPIO_PIN_SET); //SET : 200DPI   RESET : 400DPI
#endif

	cis_ADC_Init();
	cis_TIM_SP_Init();
	cis_TIM_LED_R_Init();
	cis_TIM_LED_G_Init();
	cis_TIM_LED_B_Init();
	cis_LedsOn();

	if (HAL_ADC_Start_DMA(&hadc1, (uint32_t *)cisData, CIS_ADC_BUFF_END_CAPTURE) != HAL_OK)
	{
		Error_Handler();
	}

	if (HAL_ADC_Start_DMA(&hadc2, (uint32_t *)&cisData[CIS_ADC_BUFF_END_CAPTURE], CIS_ADC_BUFF_END_CAPTURE) != HAL_OK)
	{
		Error_Handler();
	}

	if (HAL_ADC_Start_DMA(&hadc3, (uint32_t *)&cisData[CIS_ADC_BUFF_END_CAPTURE * 2], CIS_ADC_BUFF_END_CAPTURE) != HAL_OK)
	{
		Error_Handler();
	}
	cis_TIM_CLK_Init();

	//Reset CLK counter
	__HAL_TIM_SET_COUNTER(&htim1, 0);

	//Reset SP counter
	__HAL_TIM_SET_COUNTER(&htim8, (CIS_END_CAPTURE) - CIS_SP_ON);

#ifdef CIS_BW
	//Set BW phase shift
	__HAL_TIM_SET_COUNTER(&htim5, (CIS_END_CAPTURE) - CIS_LED_ON);			//B
	__HAL_TIM_SET_COUNTER(&htim4, (CIS_END_CAPTURE) - CIS_LED_ON);			//G
	__HAL_TIM_SET_COUNTER(&htim3, (CIS_END_CAPTURE) - CIS_LED_ON);			//R
#else
	//Set RGB phase shift
	__HAL_TIM_SET_COUNTER(&htim5, (CIS_END_CAPTURE * 2) - CIS_LED_ON);		//B
	__HAL_TIM_SET_COUNTER(&htim4, (CIS_END_CAPTURE * 3) - CIS_LED_ON);		//G
	__HAL_TIM_SET_COUNTER(&htim3, (CIS_END_CAPTURE) - CIS_LED_ON);			//R
#endif

	//	if (buttonState[SW1] == SWITCH_PRESSED)
	//	{
	//		cis_CalibrationMenu();
	//	}

//	cis_CalibrationMenu();

	// Load levels and compute calibration gains and offset

	/*-------- 1 --------*/
	ssd1362_drawRect(0, DISPLAY_HEAD_Y1POS, DISPLAY_MAX_X_LENGTH, DISPLAY_HEAD_Y2POS, 4, true);
	ssd1362_drawString(10, DISPLAY_HEAD_Y1POS + 1, (int8_t *)"LOAD BLACK LEVELS", 0xF, 8);
	ssd1362_writeFullBuffer();

	// Get the black levels
	cis_LoadCalibration(blackCalibVirtAddVar, cisBlackCalData, &minBlackPix, &maxBlackPix);

	// Print curve
	ssd1362_drawRect(0, DISPLAY_AERA1_Y1POS, DISPLAY_MAX_X_LENGTH, DISPLAY_AERA1_Y2POS + 1, 7, false);
	for (uint32_t i = 0; i < (DISPLAY_MAX_X_LENGTH); i++)
	{
		int32_t cis_color = cisBlackCalData[(uint32_t)(i * ((float)(CIS_EFFECTIVE_PIXELS) / (float)DISPLAY_MAX_X_LENGTH))] >> 11;
		ssd1362_drawPixel(DISPLAY_MAX_X_LENGTH - 1 - i, DISPLAY_AERA1_Y2POS - cis_color, 15, false);
	}
	ssd1362_writeFullBuffer();

	HAL_Delay(2000);

	/*-------- 2 --------*/
	ssd1362_drawRect(0, DISPLAY_HEAD_Y1POS, DISPLAY_MAX_X_LENGTH, DISPLAY_HEAD_Y2POS, 4, true);
	ssd1362_drawString(10, DISPLAY_HEAD_Y1POS + 1, (int8_t *)"EXTRACT DIFFERENTIAL OFFSETS", 0xF, 8);
	ssd1362_writeFullBuffer();

	// Extract differential offsets
	for (uint32_t i = 0; i < CIS_EFFECTIVE_PIXELS; i++)
	{
		cisOffsetCalData[i] = maxBlackPix - cisBlackCalData[i];
	}

	// Print curve
	for (uint32_t i = 0; i < (DISPLAY_MAX_X_LENGTH); i++)
	{
		uint32_t index = i * ((float)(CIS_EFFECTIVE_PIXELS) / (float)DISPLAY_MAX_X_LENGTH);
		int32_t cis_color = (cisOffsetCalData[index] + cisBlackCalData[index]) >> 11;
		ssd1362_drawPixel(DISPLAY_MAX_X_LENGTH - 1 - i, DISPLAY_AERA1_Y2POS - cis_color, 3, false);
	}
	ssd1362_writeFullBuffer();

	HAL_Delay(2000);

	/*-------- 3 --------*/
	ssd1362_drawRect(0, DISPLAY_HEAD_Y1POS, DISPLAY_MAX_X_LENGTH, DISPLAY_HEAD_Y2POS, 4, true);
	ssd1362_drawString(10, DISPLAY_HEAD_Y1POS + 1, (int8_t *)"LOAD WHITE LEVELS", 0xF, 8);
	ssd1362_writeFullBuffer();

	// Get the white levels
	cis_LoadCalibration(whiteCalibVirtAddVar, cisWhiteCalData, NULL, NULL);

	// Print curve
	for (uint32_t i = 0; i < (DISPLAY_MAX_X_LENGTH); i++)
	{
		int32_t cis_color = cisWhiteCalData[(uint32_t)(i * ((float)(CIS_EFFECTIVE_PIXELS) / (float)DISPLAY_MAX_X_LENGTH))] >> 11;
		ssd1362_drawPixel(DISPLAY_MAX_X_LENGTH - 1 - i, DISPLAY_AERA1_Y2POS - cis_color, 0, false);
	}
	ssd1362_writeFullBuffer();

	HAL_Delay(2000);

	/*-------- 4 --------*/
	ssd1362_drawRect(0, DISPLAY_HEAD_Y1POS, DISPLAY_MAX_X_LENGTH, DISPLAY_HEAD_Y2POS, 4, true);
	ssd1362_drawString(10, DISPLAY_HEAD_Y1POS + 1, (int8_t *)"COMPUTE COMPENSATIION GAINS", 0xF, 8);
	ssd1362_writeFullBuffer();

	// Compute gains
	for (uint32_t i = 0; i < CIS_EFFECTIVE_PIXELS; i++)
	{
		cisGainsCalData[i] = (float32_t)(40000) / (float32_t)(cisWhiteCalData[i] - cisBlackCalData[i]);
	}

	printf("-------- COMPUTE GAINS --------\n");
#ifdef PRINT_CIS_CALIBRATION
	for (uint32_t pix = 0; pix < CIS_EFFECTIVE_PIXELS; pix++)
	{
		printf("Pix = %d, Val = %0.3f\n", (int)pix, (float)cisGainsCalData[pix]);
	}
	printf("-------------------------------\n");
#endif
}

/**
 * @brief  CIS calibration
 * @param  calibration iteration
 * @retval None
 */
void cis_StartCalibration(uint16_t VirtAddVar, uint16_t iterationNb)
{
	uint32_t currAvrgPix = 0;
	uint16_t letfBits = 0;
	uint16_t rightBits = 0;

	printf("------ START CALIBRATION ------\n");


	for (uint32_t pix = 0; pix < CIS_EFFECTIVE_PIXELS; pix++)
	{
		SCB_InvalidateDCache_by_Addr((uint32_t *)&cisData[0] , CIS_ADC_BUFF_SIZE * 4);
		for (uint32_t sampleNb = 0; sampleNb < iterationNb; sampleNb++)
		{
			currAvrgPix += cis_GetBuffData(pix);
		}
		currAvrgPix /= iterationNb;

		letfBits = currAvrgPix >> 16;
		rightBits = currAvrgPix;

		if((EE_WriteVariable(VirtAddVar + (pix * 2),  letfBits)) != HAL_OK)
		{
			printf("Flash write fail\n");
			Error_Handler();
		}
		if((EE_WriteVariable(VirtAddVar + (pix * 2) + 1,  rightBits)) != HAL_OK)
		{
			printf("Flash write fail\n");
			Error_Handler();
		}
#ifdef PRINT_CIS_CALIBRATION
		printf("Pix = %d, Val = %d\n", (int)pix, (int)currAvrgPix);
#endif
		currAvrgPix = 0;
	}
	printf("-------------------------------\n");
}

/**
 * @brief  Load CIS calibration
 * @param  None
 * @retval None
 */
void cis_LoadCalibration(uint16_t VirtAddVar, int32_t * cisCalData, int32_t * minPix, int32_t * maxPix)
{
	uint16_t letfBits = 0;
	uint16_t rightBits = 0;
	int32_t currAvrgPix = 0;
	int32_t deltaPix = 0;
	int32_t maxpix = 0;
	int32_t minpix = 0;

	printf("------- LOAD CALIBRATION ------\n");

	for (uint32_t pix = 0; pix < CIS_EFFECTIVE_PIXELS; pix++)
	{
		if((EE_ReadVariable(VirtAddVar + (pix * 2),  &letfBits)) != HAL_OK)
		{
			printf("Flash read fail\n");
			Error_Handler();
		}
		if((EE_ReadVariable(VirtAddVar + (pix * 2) + 1,  &rightBits)) != HAL_OK)
		{
			printf("Flash read fail\n");
			Error_Handler();
		}

		currAvrgPix = letfBits << 16;
		currAvrgPix |= rightBits;

		cisCalData[pix] = currAvrgPix;

		currAvrgPix = 0;
	}
	arm_max_q31(cisCalData, CIS_EFFECTIVE_PIXELS, &maxpix, NULL);
	arm_min_q31(cisCalData, CIS_EFFECTIVE_PIXELS, &minpix, NULL);

	deltaPix = maxpix - minpix;

	*maxPix = maxpix;
	*minPix = minpix;

	printf("Max   Pix = %d\n", (int)maxpix);
	printf("Min   Pix = %d\n", (int)minpix);
	printf("Delta Pix = %d\n", (int)deltaPix);
	printf("-------------------------------\n");

#ifdef PRINT_CIS_CALIBRATION
	for (uint32_t pix = 0; pix < CIS_EFFECTIVE_PIXELS; pix++)
	{
		printf("Pix = %d, Val = %d\n", (int)pix, (int)cisCalData[pix]);
	}
	printf("-------------------------------\n");
#endif
}

/**
 * @brief  Return buffer data
 * @param  index
 * @retval value
 */
uint32_t cis_GetBuffData(uint32_t index)
{
	if (index < CIS_EFFECTIVE_PIXELS_PER_LINE)
		return cisData[index + CIS_ADC_BUFF_START_OFFSET];
	if (index < CIS_EFFECTIVE_PIXELS_PER_LINE * 2)
		return cisData[index - CIS_EFFECTIVE_PIXELS_PER_LINE + CIS_ADC_BUFF_START_OFFSET + CIS_ADC_BUFF_END_CAPTURE];
	if (index < CIS_EFFECTIVE_PIXELS_PER_LINE * 3)
		return cisData[index - (CIS_EFFECTIVE_PIXELS_PER_LINE * 2) + CIS_ADC_BUFF_START_OFFSET + (CIS_ADC_BUFF_END_CAPTURE * 2)];
	return 0;
}

/**
 * @brief  Manages Image process.
 * @param  None
 * @retval Image error
 */
void cis_ImageProcessBW(int32_t *cis_buff)
{
	for (int32_t line = (CIS_ADC_OUT_LINES); --line >= 0;)
	{
		/* 1st half buffer played; so fill it and continue playing from bottom*/
		if(cisBufferState[line] == CIS_BUFFER_OFFSET_HALF)
		{
			int32_t dataOffset = (CIS_ADC_BUFF_END_CAPTURE * line) + CIS_ADC_BUFF_START_OFFSET;
			int32_t imageOffset = (CIS_EFFECTIVE_PIXELS_PER_LINE * line);

			cisBufferState[line] = CIS_BUFFER_OFFSET_NONE;
			/* Invalidate Data Cache to get the updated content of the SRAM on the first half of the ADC converted data buffer */
			SCB_InvalidateDCache_by_Addr((uint32_t *)&cisData[dataOffset] , CIS_EFFECTIVE_PIXELS_PER_LINE * 2);
#ifdef CIS_GAINS_COMPENSATION_OFF
			arm_copy_q31(&cisData[dataOffset], &cis_buff[imageOffset], CIS_EFFECTIVE_PIXELS_PER_LINE / 2);
#else
			arm_sub_q31(&cisData[dataOffset], &cisBlackCalData[imageOffset], &cis_buff[imageOffset], CIS_EFFECTIVE_PIXELS_PER_LINE / 2);
			arm_mult_f32((float32_t *)(&cis_buff[imageOffset]), &cisGainsCalData[imageOffset],(float32_t *)(&cis_buff[imageOffset]), CIS_EFFECTIVE_PIXELS_PER_LINE / 2);
//			arm_offset_q31(&cis_buff[imageOffset], maxBlackPix / 2, &cis_buff[imageOffset], CIS_EFFECTIVE_PIXELS_PER_LINE / 2);

			cis_ImageFilterBW(&cis_buff[imageOffset]);
#endif
		}

		/* 2nd half buffer played; so fill it and continue playing from top */
		if(cisBufferState[line] == CIS_BUFFER_OFFSET_FULL)
		{
			int32_t dataOffset = (CIS_ADC_BUFF_END_CAPTURE * line) + CIS_ADC_BUFF_START_OFFSET + (CIS_EFFECTIVE_PIXELS_PER_LINE / 2);
			int32_t imageOffset = (CIS_EFFECTIVE_PIXELS_PER_LINE * line) + (CIS_EFFECTIVE_PIXELS_PER_LINE / 2);

			cisBufferState[line] = CIS_BUFFER_OFFSET_NONE;
			/* Invalidate Data Cache to get the updated content of the SRAM on the second half of the ADC converted data buffer */
			SCB_InvalidateDCache_by_Addr((uint32_t *) &cisData[dataOffset], CIS_EFFECTIVE_PIXELS_PER_LINE * 2);
#ifdef CIS_GAINS_COMPENSATION_OFF
			arm_copy_q31(&cisData[dataOffset], &cis_buff[imageOffset], CIS_EFFECTIVE_PIXELS_PER_LINE / 2);
#else
			arm_sub_q31(&cisData[dataOffset], &cisBlackCalData[imageOffset], &cis_buff[imageOffset], CIS_EFFECTIVE_PIXELS_PER_LINE / 2);
			arm_mult_f32((float32_t *)(&cis_buff[imageOffset]), &cisGainsCalData[imageOffset],(float32_t *)(&cis_buff[imageOffset]), CIS_EFFECTIVE_PIXELS_PER_LINE / 2);
//			arm_offset_q31(&cis_buff[imageOffset], maxBlackPix / 2, &cis_buff[imageOffset], CIS_EFFECTIVE_PIXELS_PER_LINE / 2);

			cis_ImageFilterBW(&cis_buff[imageOffset]);
#endif
		}
	}
}

/**
 * @brief  Image filtering
 * @param  Audio buffer
 * @retval None
 */
void cis_ImageFilterBW(int32_t *cis_buff)
{
	for (int32_t i = 0; i < CIS_EFFECTIVE_PIXELS_PER_LINE / 2; i++)
	{
#ifdef CIS_INVERT_COLOR
		cis_buff[i] = (double)(65535 - cis_buff[i]);
#endif
#ifdef CIS_INVERT_COLOR_SMOOTH
		cis_buff[i] = (double)(65535 - cis_buff[i]) * (pow(10.00, ((double)(65535 - cis_buff[i]) / 65535.00)) / 10.00); //sensibility filer generate some glitchs
#endif
#ifdef CIS_NORMAL_COLOR_SMOOTH
		cis_buff[i] = (double)(cis_buff[i]) * (pow(10.00, ((double)(cis_buff[i]) / 65535.00)) / 10.00);
#endif
	}
}

/**
 * @brief  Init CIS clock Frequency
 * @param  sampling_frequency
 * @retval None
 */
void cis_TIM_CLK_Init()
{
	MX_TIM1_Init();

	/* Start ADC Timer #######################################*/
	if(HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1) != HAL_OK)
	{
		Error_Handler();
	}

	/* Start CLK generation ##################################*/
	if(HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2) != HAL_OK)
	{
		Error_Handler();
	}
}

/**
 * @brief  CIS start pulse timer init
 * @param  Void
 * @retval None
 */
void cis_TIM_SP_Init()
{
	MX_TIM8_Init();

	/* Start SP generation ##################################*/
	if(HAL_TIM_PWM_Start(&htim8, TIM_CHANNEL_3) != HAL_OK)
	{
		Error_Handler();
	}
}

/**
 * @brief  CIS red led timer init
 * @param  Void
 * @retval None
 */
void cis_TIM_LED_B_Init()
{
	MX_TIM3_Init();
}

/**
 * @brief  CIS green led timer init
 * @param  Void
 * @retval None
 */
void cis_TIM_LED_R_Init()
{
	MX_TIM4_Init();
}

/**
 * @brief  CIS blue led timer init
 * @param  Void
 * @retval None
 */
void cis_TIM_LED_G_Init()
{
	MX_TIM5_Init();
}

/**
 * @brief  CIS leds off
 * @param  Void
 * @retval None
 */
void cis_LedsOff()
{
	/* Start LED R generation ###############################*/
	if(HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_1) != HAL_OK)
	{
		Error_Handler();
	}
	/* Start LED G generation ###############################*/
	if(HAL_TIM_PWM_Stop(&htim4, TIM_CHANNEL_2) != HAL_OK)
	{
		Error_Handler();
	}
	/* Start LED B generation ###############################*/
	if(HAL_TIM_PWM_Stop(&htim5, TIM_CHANNEL_3) != HAL_OK)
	{
		Error_Handler();
	}
}

/**
 * @brief  CIS leds on
 * @param  Void
 * @retval None
 */
void cis_LedsOn()
{
	/* Start LED R generation ###############################*/
	if(HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1) != HAL_OK)
	{
		Error_Handler();
	}
	/* Start LED G generation ###############################*/
	if(HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_2) != HAL_OK)
	{
		Error_Handler();
	}
	/* Start LED B generation ###############################*/
	if(HAL_TIM_PWM_Start(&htim5, TIM_CHANNEL_3) != HAL_OK)
	{
		Error_Handler();
	}
}

/**
 * @brief  CIS adc init
 * @param  Void
 * @retval None
 */
void cis_ADC_Init(void)
{
	MX_ADC1_Init();
	MX_ADC2_Init();
	MX_ADC3_Init();

	/* ### Start calibration ############################################ */
	if (HAL_ADCEx_Calibration_Start(&hadc1, ADC_CALIB_OFFSET, ADC_SINGLE_ENDED) != HAL_OK)
	{
		Error_Handler();
	}

	if (HAL_ADCEx_Calibration_Start(&hadc2, ADC_CALIB_OFFSET, ADC_SINGLE_ENDED) != HAL_OK)
	{
		Error_Handler();
	}

	if (HAL_ADCEx_Calibration_Start(&hadc3, ADC_CALIB_OFFSET, ADC_SINGLE_ENDED) != HAL_OK)
	{
		Error_Handler();
	}
}

/**
 * @brief  Conversion DMA half-transfer callback in non-blocking mode
 * @param  hadc: ADC handle
 * @retval None
 */
void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef* hadc)
{
	if (hadc->Instance == ADC1)
	{
		cisBufferState[0] = CIS_BUFFER_OFFSET_HALF;
		return;
	}
	if (hadc->Instance == ADC2)
	{
		cisBufferState[1] = CIS_BUFFER_OFFSET_HALF;
		return;
	}
	if (hadc->Instance == ADC3)
	{
		cisBufferState[2] = CIS_BUFFER_OFFSET_HALF;
	}
}

/**
 * @brief  Conversion complete callback in non-blocking mode
 * @param  hadc: ADC handle
 * @retval None
 */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
	if (hadc->Instance == ADC1)
	{
		cisBufferState[0] = CIS_BUFFER_OFFSET_FULL;
		return;
	}
	if (hadc->Instance == ADC2)
	{
		cisBufferState[1] = CIS_BUFFER_OFFSET_FULL;
		return;
	}
	if (hadc->Instance == ADC3)
	{
		cisBufferState[2] = CIS_BUFFER_OFFSET_FULL;
	}
}

/**
 * @brief  CIS test
 * @param  Void
 * @retval None
 */
void cis_Test(void)
{
	uint32_t cis_color = 0;
	uint32_t i = 0;

	while (1)
	{
		ssd1362_drawRect(0, DISPLAY_AERA1_Y1POS, DISPLAY_MAX_X_LENGTH, DISPLAY_AERA1_Y2POS, 3, false);
		ssd1362_drawRect(0, DISPLAY_AERA2_Y1POS, DISPLAY_MAX_X_LENGTH, DISPLAY_AERA2_Y2POS, 8, false);

		for (i = 0; i < (DISPLAY_MAX_X_LENGTH); i++)
		{
			cis_color = cis_GetBuffData((i * ((float)CIS_EFFECTIVE_PIXELS / (float)DISPLAY_MAX_X_LENGTH))) >> 12;
			ssd1362_drawPixel(DISPLAY_MAX_X_LENGTH - 1 - i, DISPLAY_AERA1_Y1POS + DISPLAY_AERAS1_HEIGHT - DISPLAY_INTER_AERAS_HEIGHT - (cis_color) - 1, 15, false);

			ssd1362_drawVLine(DISPLAY_MAX_X_LENGTH - 1 - i, DISPLAY_AERA2_Y1POS + 1, DISPLAY_AERAS2_HEIGHT - 2, cis_color, false);
		}
		ssd1362_drawRect(200, DISPLAY_HEAD_Y1POS, DISPLAY_MAX_X_LENGTH, DISPLAY_HEAD_Y2POS, 4, false);
		ssd1362_writeFullBuffer();

		HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
	}
}

/**
 * @brief  Display calibration menu
 * @param  None
 * @retval None
 */
static void cis_CalibrationMenu(void)
{
	/* Set header description */
	buttonState[SW1] = SWITCH_RELEASED;

	ssd1362_drawRect(0, DISPLAY_HEAD_Y1POS, DISPLAY_MAX_X_LENGTH, DISPLAY_HEAD_Y2POS, 4, true);
	ssd1362_drawString(10, DISPLAY_HEAD_Y1POS + 1, (int8_t *)"CIS BW QUALIBRATION", 0xF, 8);
	ssd1362_writeFullBuffer();
	HAL_Delay(500);
	ssd1362_drawRect(0, DISPLAY_HEAD_Y1POS, DISPLAY_MAX_X_LENGTH, DISPLAY_HEAD_Y2POS, 4, true);
	ssd1362_drawString(10, DISPLAY_HEAD_Y1POS + 1, (int8_t *)"PLACE CIS ON WHITE SURFACE", 0xF, 8);
	ssd1362_writeFullBuffer();
	HAL_Delay(2000);
	cis_StartCalibration(whiteCalibVirtAddVar ,20);
	ssd1362_drawRect(0, DISPLAY_HEAD_Y1POS, DISPLAY_MAX_X_LENGTH, DISPLAY_HEAD_Y2POS, 4, true);
	ssd1362_drawString(10, DISPLAY_HEAD_Y1POS + 1, (int8_t *)"PLACE CIS ON BLACK SURFACE", 0xF, 8);
	ssd1362_writeFullBuffer();
	HAL_Delay(2000);
	cis_StartCalibration(blackCalibVirtAddVar ,20);
}
