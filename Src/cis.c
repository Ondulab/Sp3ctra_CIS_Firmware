
/**
 ******************************************************************************
 * @file           : cis.c
 * @brief          :
 ******************************************************************************
 */
/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"
#include "stdlib.h"

#include "tim.h"
#include "cis.h"

/* Extern variables ---------------------------------------------------------*/
//extern TIM_HandleTypeDef htim15;
//extern TIM_HandleTypeDef htim1;
extern TIM_HandleTypeDef htim6;
extern HRTIM_HandleTypeDef hhrtim;
extern ADC_HandleTypeDef hadc1;
extern DAC_HandleTypeDef hdac1;

/* Private variables ---------------------------------------------------------*/
__IO static int32_t aADCxConvertedDataDMA = 0;
__IO static int16_t aADCxConvertedData[CIS_PIXELS_NB] = {0};

__IO static int16_t redData[CIS_PIXELS_NB + 1000] = {0};
__IO static int16_t greenData[CIS_PIXELS_NB + 1000] = {0};
__IO static int16_t blueData[CIS_PIXELS_NB + 1000] = {0};

__IO static int16_t aADCxConvertedDataOffset[CIS_PIXELS_NB] = {0};

__IO static int16_t redDataOffset[CIS_PIXELS_NB + 1000] = {0};
__IO static int16_t greenDataOffset[CIS_PIXELS_NB + 1000] = {0};
__IO static int16_t blueDataOffset[CIS_PIXELS_NB + 1000] = {0};

__IO static uint16_t deadZones[TOTAL_DEADZONE + 1000] = {0};

const uint32_t aEscalator16bit[32] = {0x111, 0x222, 0x333, 0x444, 0x555, 0x666, 0x777, 0x888,
		0x999, 0xAAA, 0xBBB, 0xCCC, 0xDDD, 0xEEE, 0xFFF, 0xEEE,
		0xDDD, 0xCCC, 0xBBB, 0xAAA, 0x999, 0x888, 0x777, 0x666,
		0x555, 0x444, 0x333, 0x222, 0x111};
static __IO uint32_t pixel_cnt = 0;
static __IO uint16_t color_selector = 0;
static __IO uint32_t calib_cnt = 1;

TIM_MasterConfigTypeDef sMasterConfig;

/* Private function prototypes -----------------------------------------------*/
void timesBaseInit(void);
void setDacCarrier(int freq);

void cisInit(void)
{
	int k = 0;
	for (int i = 0; i < SEGMENT_NB; i++)
	{
		for (int j = 0; j < DEADZONE_WIDTH; j++)
		{
			deadZones[k++] = (i * SEGMENT_WIDTH) + (SEGMENT_WIDTH / 2) - ((DEADZONE_WIDTH / 2) - j);
		}
	}

	/* ### - 4 - Start conversion in DMA mode ################################# */
	if (HAL_ADC_Start_DMA(&hadc1, (uint32_t *)&aADCxConvertedDataDMA , 1) != HAL_OK)
	{
		Error_Handler();
	}

	//	if (HAL_DAC_Start_DMA(&hdac1, DAC1_CHANNEL_1, (uint32_t *)aEscalator16bit, CIS_PIXELS_NB, DAC_ALIGN_12B_R) != HAL_OK)
	//	{
	//		/* Start DMA Error */
	//		Error_Handler();
	//	}

	//	float ratio = (4095.00 / ((float)CIS_PIXELS_NB / 2.0));
	//	for (int i = 0; i < (CIS_PIXELS_NB / 2); i++)
	//		aADCxConvertedData[i] = (int)((float)i * ratio);
	//	for (int i = (CIS_PIXELS_NB / 2); i > 0; i--)
	//		aADCxConvertedData[CIS_PIXELS_NB - i] = (int)((float)i * ratio);

	//	SCB_CleanDCache();

	/*##-2- Enable DAC selected channel and associated DMA #############################*/
	if (HAL_DAC_Start_DMA(&hdac1, DAC_CHANNEL_1, (uint32_t *)aADCxConvertedData + NOTE_ZONE, CIS_PIXELS_NB - TOTAL_DEADZONE - PIXEL_CNT_OFFSET - VOLUME_ZONE, DAC_ALIGN_12B_R) != HAL_OK)
	{
		/* Start DMA Error */
		Error_Handler();
	}

	/*##-2- Enable DAC selected channel and associated DMA #############################*/
	if (HAL_DAC_Start_DMA(&hdac1, DAC_CHANNEL_2, (uint32_t *)aADCxConvertedData + NOTE_ZONE, CIS_PIXELS_NB - TOTAL_DEADZONE - PIXEL_CNT_OFFSET - VOLUME_ZONE, DAC_ALIGN_12B_R) != HAL_OK)
	{
		/* Start DMA Error */
		Error_Handler();
	}

	HAL_DAC_Start(&hdac1, DAC_CHANNEL_2);
	HAL_DAC_Start(&hdac1, DAC_CHANNEL_1);

	timesBaseInit();

	int redVal = 0;
	int volume = 0;
	int firstMax = 0;
	int oldRedVal = 0;

	HAL_Delay(2000);
	while(1)
	{
		int i = 0;
		HAL_Delay(100);

		for (i = 0; i < VOLUME_ZONE; i++)
		{
			volume += (aADCxConvertedData[CIS_PIXELS_NB - i - 500]);
		}

		volume /= i;
		volume /= 1000;
		if (volume < 1)
			volume = 1;
		if (volume > 10)
			volume = 10;

		for (i = 0; i < CIS_PIXELS_NB; i++)
		{
			aADCxConvertedData[i] =  aADCxConvertedData[i] / volume;
		}

		for (i = 0; i < NOTE_ZONE; i++)
		{
			redVal += (aADCxConvertedData[i]);
		}
		redVal /= i;
		redVal /= 2;

		redVal = redVal - 1000;

		if (redVal < 20)
			redVal = 20;
		if (redVal > 2000)
			redVal = 2000;

		if (redVal != oldRedVal)
		{
			setDacCarrier(redVal);
			oldRedVal = redVal;
		}

		printf("%d  ", volume);
//		printf("%d  ", (int)redData[150]);
//		printf("%d  ", (int)redVal);
		printf("\n");
	}
}

/**
 * @brief  End of conversion callback in non blocking mode
 * @param  hadc : hadc handle
 * @retval None
 */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
	if (hadc != &hadc1)
		return;

	static uint8_t colorIsSet = 0;
	static uint16_t deadZone_cnt = 0;

	if (!colorIsSet)
	{
		if (color_selector == RED)
		{
			HAL_GPIO_WritePin(CIS_LED_B_GPIO_Port, CIS_LED_B_Pin, GPIO_PIN_RESET);
			HAL_GPIO_WritePin(CIS_LED_R_GPIO_Port, CIS_LED_R_Pin, GPIO_PIN_SET);
			colorIsSet = 1;
		}
		else if (color_selector == GREEN)
		{
			HAL_GPIO_WritePin(CIS_LED_R_GPIO_Port, CIS_LED_R_Pin, GPIO_PIN_RESET);
			HAL_GPIO_WritePin(CIS_LED_G_GPIO_Port, CIS_LED_G_Pin, GPIO_PIN_SET);
			colorIsSet = 1;
		}
		else if (color_selector == BLUE)
		{
			HAL_GPIO_WritePin(CIS_LED_G_GPIO_Port, CIS_LED_G_Pin, GPIO_PIN_RESET);
			HAL_GPIO_WritePin(CIS_LED_B_GPIO_Port, CIS_LED_B_Pin, GPIO_PIN_SET);
			colorIsSet = 1;
		}
		else if (color_selector == 3)
		{
			HAL_GPIO_WritePin(CIS_LED_R_GPIO_Port, CIS_LED_R_Pin, GPIO_PIN_RESET);
			HAL_GPIO_WritePin(CIS_LED_G_GPIO_Port, CIS_LED_G_Pin, GPIO_PIN_RESET);
			HAL_GPIO_WritePin(CIS_LED_B_GPIO_Port, CIS_LED_B_Pin, GPIO_PIN_RESET);
			colorIsSet = 1;
		}
		else if (color_selector == 4)
		{
			HAL_GPIO_WritePin(CIS_LED_R_GPIO_Port, CIS_LED_R_Pin, GPIO_PIN_SET);
			HAL_GPIO_WritePin(CIS_LED_G_GPIO_Port, CIS_LED_G_Pin, GPIO_PIN_SET);
			HAL_GPIO_WritePin(CIS_LED_B_GPIO_Port, CIS_LED_B_Pin, GPIO_PIN_SET);
			colorIsSet = 1;
		}
	}

	if (pixel_cnt == 0)
	{
		for (int i = 0; i < 25; i++)
		{
			HAL_GPIO_WritePin(CIS_SP_GPIO_Port, CIS_SP_Pin, GPIO_PIN_SET);
		}
		HAL_GPIO_WritePin(CIS_SP_GPIO_Port, CIS_SP_Pin, GPIO_PIN_RESET);
	}
	else if (pixel_cnt >= PIXEL_CNT_OFFSET)
	{
		if ((pixel_cnt - PIXEL_CNT_OFFSET) != deadZones[deadZone_cnt])
		{
			if (calib_cnt < 100)
			{
				switch (color_selector)
				{
				case RED :
					redDataOffset[pixel_cnt - PIXEL_CNT_OFFSET - deadZone_cnt] += (4096 - (aADCxConvertedDataDMA >> 1));
					redDataOffset[pixel_cnt - PIXEL_CNT_OFFSET - deadZone_cnt] /= 2;
					break;
				case GREEN :
					greenDataOffset[pixel_cnt - PIXEL_CNT_OFFSET - deadZone_cnt] += (4096 - (aADCxConvertedDataDMA >> 1));
					greenDataOffset[pixel_cnt - PIXEL_CNT_OFFSET - deadZone_cnt] /= 2;
					break;
				case BLUE :
					blueDataOffset[pixel_cnt - PIXEL_CNT_OFFSET - deadZone_cnt] += (4096 - (aADCxConvertedDataDMA >> 1));
					blueDataOffset[pixel_cnt - PIXEL_CNT_OFFSET - deadZone_cnt] /= 2;
					break;
				default :
					aADCxConvertedDataOffset[pixel_cnt - PIXEL_CNT_OFFSET - deadZone_cnt] += (4096 - (aADCxConvertedDataDMA >> 1));
					aADCxConvertedDataOffset[pixel_cnt - PIXEL_CNT_OFFSET - deadZone_cnt] /= 2;
					break;
				}
			}
			else
			{
				switch (color_selector)
				{
				case RED :
					redData[pixel_cnt - PIXEL_CNT_OFFSET - deadZone_cnt] = ((4096 - (aADCxConvertedDataDMA >> 1)) - redDataOffset[pixel_cnt - PIXEL_CNT_OFFSET - deadZone_cnt]);
					if (redData[pixel_cnt - PIXEL_CNT_OFFSET - deadZone_cnt] < 0)
						redData[pixel_cnt - PIXEL_CNT_OFFSET - deadZone_cnt] = 0;
					break;
				case GREEN :
					greenData[pixel_cnt - PIXEL_CNT_OFFSET - deadZone_cnt] = ((4096 - (aADCxConvertedDataDMA >> 1)) - greenDataOffset[pixel_cnt - PIXEL_CNT_OFFSET - deadZone_cnt]);
					if (greenData[pixel_cnt - PIXEL_CNT_OFFSET - deadZone_cnt] < 0)
						greenData[pixel_cnt - PIXEL_CNT_OFFSET - deadZone_cnt] = 0;
					break;
				case BLUE :
					blueData[pixel_cnt - PIXEL_CNT_OFFSET - deadZone_cnt] = ((4096 - (aADCxConvertedDataDMA >> 1)) - blueDataOffset[pixel_cnt - PIXEL_CNT_OFFSET - deadZone_cnt]);
					if (blueData[pixel_cnt - PIXEL_CNT_OFFSET - deadZone_cnt] < 0)
						blueData[pixel_cnt - PIXEL_CNT_OFFSET - deadZone_cnt] = 0;
					break;
				default :
					aADCxConvertedData[pixel_cnt - PIXEL_CNT_OFFSET - deadZone_cnt] = ((4096 - (aADCxConvertedDataDMA >> 1)) - aADCxConvertedDataOffset[pixel_cnt - PIXEL_CNT_OFFSET - deadZone_cnt]);
					if (aADCxConvertedData[pixel_cnt - PIXEL_CNT_OFFSET - deadZone_cnt] < 0)
						aADCxConvertedData[pixel_cnt - PIXEL_CNT_OFFSET -deadZone_cnt] = 0;
					break;
				}
#ifndef BLACK_AND_WITHE
				aADCxConvertedData[pixel_cnt - PIXEL_CNT_OFFSET - deadZone_cnt] = ((redData[pixel_cnt - PIXEL_CNT_OFFSET - deadZone_cnt]) +
						(greenData[pixel_cnt - PIXEL_CNT_OFFSET - deadZone_cnt]) +
						(blueData[pixel_cnt - PIXEL_CNT_OFFSET - deadZone_cnt] )) / 3;
#endif
			}
		}
		else
		{
			deadZone_cnt++;
		}
	}

	pixel_cnt++;

	if (pixel_cnt > CIS_PIXELS_NB)
	{
		if (calib_cnt < 10000)
			calib_cnt++;
		pixel_cnt = 0;
		deadZone_cnt = 0;

#ifdef LED_ON
#ifdef BLACK_AND_WITHE
		color_selector = 4;
#else
		color_selector++;
		if (color_selector > 2)
		{
			color_selector = 0;
		}
#endif
#else
		color_selector = 3;
#endif
		colorIsSet = 0;
	}
	/* Invalidate Data Cache to get the updated content of the SRAM on the second half of the ADC converted data buffer: 32 bytes */
	//	SCB_InvalidateDCache_by_Addr((uint32_t *)&aADCxConvertedDataDMA, 1);
	//	SCB_CleanDCache();
}

/**
 * @brief  Period elapsed callback in non blocking mode
 * @param  htim : TIM handle
 * @retval None
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	pixel_cnt++;
}

void setDacCarrier(int freq)
{
	uint32_t uwPrescalerValue = 0;

	/* Compute the prescaler value */
	uwPrescalerValue = (uint32_t) ((SystemCoreClock / 2) / (freq * 10 * CIS_PIXELS_NB));

	HAL_TIM_Base_DeInit(&htim6);

	/*##-1- Configure the TIM peripheral #######################################*/
	/* Time base configuration */
	//	htim6.Instance = TIM6;
	//	htim6.Init.Period            = 10 - 1;
	htim6.Init.Prescaler         = uwPrescalerValue;
	//	htim6.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
	//	htim6.Init.CounterMode       = TIM_COUNTERMODE_UP;
	//	htim6.Init.RepetitionCounter = 0;
	HAL_TIM_Base_Init(&htim6);

	//		/* TIM6 TRGO selection */
	//		sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE;
	//		sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
	//
	//		HAL_TIMEx_MasterConfigSynchronization(&htim6, &sMasterConfig);

	/*##-2- Enable TIM peripheral counter ######################################*/
	HAL_TIM_Base_Start(&htim6);
}

/**
 * @brief  Period elapsed callback in non blocking mode
 * @param  htim : TIM handle
 * @retval None
 */
void timesBaseInit(void)
{
	uint32_t uwPrescalerValue = 0;

	/* Compute the prescaler value */
	uwPrescalerValue = (uint32_t) ((SystemCoreClock / 2) / (DAC_TIME_FREQ * 10 * CIS_PIXELS_NB));

	/*##-1- Configure the TIM peripheral #######################################*/
	/* Time base configuration */
	htim6.Instance = TIM6;
	htim6.Init.Period            = 10 - 1;
	htim6.Init.Prescaler         = uwPrescalerValue;
	htim6.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
	htim6.Init.CounterMode       = TIM_COUNTERMODE_UP;
	htim6.Init.RepetitionCounter = 0;
	HAL_TIM_Base_Init(&htim6);

	/* TIM6 TRGO selection */
	sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE;
	sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;

	HAL_TIMEx_MasterConfigSynchronization(&htim6, &sMasterConfig);

	/*##-2- Enable TIM peripheral counter ######################################*/
	HAL_TIM_Base_Start(&htim6);

	//	/* Compute the prescaler value */
	//	uwPrescalerValue = (uint32_t) ((SystemCoreClock / 2) / (HI_TIME_FREQ * 2));
	//
	//	htim15.Instance = TIM15;
	//	htim15.Init.Prescaler = 0;
	//	htim15.Init.CounterMode = TIM_COUNTERMODE_UP;
	//	htim15.Init.Period = uwPrescalerValue;
	//	htim15.Init.ClockDivision = TIM_CLOCKDIVISION_DIV4;
	//	htim15.Init.RepetitionCounter = 0;
	//	htim15.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
	//	if (HAL_TIM_OC_Init(&htim15) != HAL_OK)
	//	{
	//		_Error_Handler(__FILE__, __LINE__);
	//	}
	//
	//	sConfigOC.OCMode = TIM_OCMODE_TOGGLE;
	//	sConfigOC.Pulse = uwPrescalerValue / 2;
	//	sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
	//	sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
	//	sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
	//	sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
	//	sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
	//	if (HAL_TIM_OC_ConfigChannel(&htim15, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
	//	{
	//		_Error_Handler(__FILE__, __LINE__);
	//	}
	//
	//	//	//	/* Start channel 1 in Output compare mode */
	//	//	if(HAL_TIM_OC_Start_IT(&htim15, TIM_CHANNEL_1) != HAL_OK)
	//	//	{
	//	//		/* Starting Error */
	//	//		Error_Handler();
	//	//	}
	//	//
	//	//	HAL_TIM_Base_Start_IT(&htim15);
	//
	//	/* Compute the prescaler value */
	//	uwPrescalerValue = (uint32_t) ((SystemCoreClock / 2) / (HI_TIME_FREQ * 8));
	//
	//	htim1.Instance = TIM1;
	//	htim1.Init.Prescaler = 0;
	//	htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
	//	htim1.Init.Period = uwPrescalerValue;;
	//	htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	//	htim1.Init.RepetitionCounter = 0;
	//	htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
	//	if (HAL_TIM_OC_Init(&htim1) != HAL_OK)
	//	{
	//		_Error_Handler(__FILE__, __LINE__);
	//	}
	//
	//	sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
	//	sMasterConfig.MasterOutputTrigger2 = TIM_TRGO2_RESET;
	//	sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
	//	if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
	//	{
	//		_Error_Handler(__FILE__, __LINE__);
	//	}
	//
	//	sConfigOC.OCMode = TIM_OCMODE_TOGGLE;
	//	sConfigOC.Pulse = uwPrescalerValue/2;
	//	sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
	//	sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
	//	sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
	//	sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
	//	sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
	//	if (HAL_TIM_OC_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
	//	{
	//		_Error_Handler(__FILE__, __LINE__);
	//	}

	//	/* Start channel 1 in Output compare mode */
	//	if(HAL_TIM_OC_Start(&htim1, TIM_CHANNEL_1) != HAL_OK)
	//	{
	//		/* Starting Error */
	//		Error_Handler();
	//	}
	//
	//	HAL_TIM_Base_Start_IT(&htim1);

	//	HRTIM_TimeBaseCfgTypeDef pTimeBaseCfg;
	//	HRTIM_SimplePWMChannelCfgTypeDef pSimplePWMChannelCfg;
	//
	//		uwPrescalerValue = (uint32_t) ((SystemCoreClock / 2) / (HI_TIME_FREQ));
	//
	//		pTimeBaseCfg.Period = uwPrescalerValue;
	//		pTimeBaseCfg.RepetitionCounter = 0x00;
	//		pTimeBaseCfg.PrescalerRatio = HRTIM_PRESCALERRATIO_DIV1;
	//		pTimeBaseCfg.Mode = HRTIM_MODE_CONTINUOUS;
	//		if (HAL_HRTIM_TimeBaseConfig(&hhrtim, HRTIM_TIMERINDEX_TIMER_A, &pTimeBaseCfg) != HAL_OK)
	//		{
	//			_Error_Handler(__FILE__, __LINE__);
	//		}
	//
	//		pSimplePWMChannelCfg.Pulse = uwPrescalerValue / 2;
	//		pSimplePWMChannelCfg.Polarity = HRTIM_OUTPUTPOLARITY_HIGH;
	//		pSimplePWMChannelCfg.IdleLevel = HRTIM_OUTPUTIDLELEVEL_INACTIVE;
	//		if (HAL_HRTIM_SimplePWMChannelConfig(&hhrtim, HRTIM_TIMERINDEX_TIMER_A, HRTIM_OUTPUT_TA1, &pSimplePWMChannelCfg) != HAL_OK)
	//		{
	//			_Error_Handler(__FILE__, __LINE__);
	//		}

	//		//	HAL_HRTIM_SimplePWMStart(&hhrtim, HRTIM_TIMERINDEX_TIMER_A, HRTIM_OUTPUT_TA1);
	//		HAL_HRTIM_SimpleOCStart(&hhrtim, HRTIM_TIMERID_MASTER + HRTIM_TIMERINDEX_TIMER_A, HRTIM_OUTPUT_TA1);
	//		HAL_HRTIM_SimpleBaseStart(&hhrtim, HRTIM_TIMERID_MASTER);
	//		//	HAL_HRTIM_SimplePWMStart(&hhrtim, HRTIM_TIMERINDEX_TIMER_B, HRTIM_OUTPUT_TB1);

	/*##-7- Start PWM signals generation ########################################################*/
	if (HAL_HRTIM_WaveformOutputStart(&hhrtim, HRTIM_OUTPUT_TA1) != HAL_OK)
	{
		/* PWM Generation Error */
		Error_Handler();
	}

	/*##-8- Start HRTIM counter ###################################################################*/
	if (HAL_HRTIM_WaveformCounterStart(&hhrtim, HRTIM_TIMERID_MASTER + HRTIM_TIMERID_TIMER_A) != HAL_OK)
	{
		/* PWM Generation Error */
		Error_Handler();
	}
}

/**
 * @brief  Conversion complete callback in non-blocking mode
 * @param  hadc: ADC handle
 * @retval None
 */
//void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef* hadc)
//{
//	/* Invalidate Data Cache to get the updated content of the SRAM on the first half of the ADC converted data buffer: 32 bytes */
//	SCB_InvalidateDCache_by_Addr((uint32_t *) &aADCxConvertedDataDMA, 1);
//}
