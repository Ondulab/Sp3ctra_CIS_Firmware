
/**
 ******************************************************************************
 * @file           : cis_RGB888.c
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
#include "dac.h"
#include "opamp.h"

#include "stdlib.h"
#include "stdio.h"
#include "stdbool.h"

#include "cis.h"

/* Private includes ----------------------------------------------------------*/

/* Private typedef -----------------------------------------------------------*/

typedef enum
{
	BUFFER_OFFSET_NONE = 0,
	BUFFER_OFFSET_HALF,
	BUFFER_OFFSET_FULL,
}CIS_BUFF_StateTypeDef;

/* Private define ------------------------------------------------------------*/
/* Definition of ADCx conversions data table size this buffer contains RGB conversion */
#define ADC_CONVERTED_DATA_BUFFER_SIZE (CIS_END_CAPTURE * 3) /* Size of array cisData[] */

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
/* Variable containing ADC conversions data */
ALIGN_32BYTES (static __IO uint8_t cisData[ADC_CONVERTED_DATA_BUFFER_SIZE]);

static CIS_BUFF_StateTypeDef  cisBufferState = {0};

/* Private function prototypes -----------------------------------------------*/
void cis_RGB888_TIM_CLK_Init(uint32_t cis_clk_freq);
void cis_RGB888_TIM_SP_Init(void);
void cis_RGB888_TIM_LED_R_Init(void);
void cis_RGB888_TIM_LED_G_Init(void);
void cis_RGB888_TIM_LED_B_Init(void);

void cis_RGB888_ADC_Init(void);

void cis_RGB888_DisplayLine(void);

/* Private user code ---------------------------------------------------------*/

/**
 * @brief  CIS init
 * @param  Void
 * @retval None
 */
void cis_RGB888_Init(void)
{
	cis_RGB888_ADC_Init();
	cis_RGB888_TIM_CLK_Init(CIS_CLK_FREQ);
	cis_RGB888_TIM_SP_Init();
	cis_RGB888_TIM_LED_R_Init();
	cis_RGB888_TIM_LED_G_Init();
	cis_RGB888_TIM_LED_B_Init();

	// Reset SP counter
	__HAL_TIM_SET_COUNTER(&htim15, 0);

	//Set RGB phase shift
	__HAL_TIM_SET_COUNTER(&htim8, (CIS_END_CAPTURE * 2) - CIS_LED_ON);		//B
	__HAL_TIM_SET_COUNTER(&htim4, (CIS_END_CAPTURE * 3) - CIS_LED_ON);		//G
	__HAL_TIM_SET_COUNTER(&htim3, (CIS_END_CAPTURE) - CIS_LED_ON);			//R

	if (HAL_ADC_Start_DMA(&hadc1, (uint32_t *)cisData, (ADC_CONVERTED_DATA_BUFFER_SIZE)) != HAL_OK)
	{
		Error_Handler();
	}

	HAL_Delay(100);
}

/**
 * @brief  CIS test
 * @param  Void
 * @retval None
 */
void cis_RGB888_Test(void)
{
	uint32_t j = 0;
	uint32_t x_size, y_size;
	uint32_t color = 0;

	BSP_LCD_GetXSize(0, &x_size);
	BSP_LCD_GetYSize(0, &y_size);

	while(1)
	{
		/* 1st half buffer played; so fill it and continue playing from bottom*/
		if(cisBufferState == BUFFER_OFFSET_HALF)
		{
			cisBufferState = BUFFER_OFFSET_NONE;
			/* Invalidate Data Cache to get the updated content of the SRAM on the first half of the ADC converted data buffer */
			SCB_InvalidateDCache_by_Addr((uint32_t *) &cisData[0], ADC_CONVERTED_DATA_BUFFER_SIZE/2);
		}

		/* 2nd half buffer played; so fill it and continue playing from top */
		if(cisBufferState == BUFFER_OFFSET_FULL)
		{
			cisBufferState = BUFFER_OFFSET_NONE;
			/* Invalidate Data Cache to get the updated content of the SRAM on the second half of the ADC converted data buffer */
			SCB_InvalidateDCache_by_Addr((uint32_t *) &cisData[ADC_CONVERTED_DATA_BUFFER_SIZE/2], ADC_CONVERTED_DATA_BUFFER_SIZE/2);
			for (uint32_t i = 0; i < x_size; i++)
			{
				color = 0xFF000000;
				color |= cis_RGB888_GetBuffData((i * (CIS_PIXELS_NB/x_size)) + CIS_PIXEX_AERA_START) << 16;
				color |= cis_RGB888_GetBuffData((i * (CIS_PIXELS_NB/x_size)) + CIS_END_CAPTURE + CIS_PIXEX_AERA_START) << 8;
				color |= cis_RGB888_GetBuffData((i * (CIS_PIXELS_NB/x_size)) + (CIS_END_CAPTURE * 2) + CIS_PIXEX_AERA_START);
				GUI_SetPixel(i, j + 24, color);
			}
			j++;
			if (j >= (y_size - 24))
			{
				j = 0;
			}
		}
	}
}

/**
 * @brief  Return buffer data
 * @param  index
 * @retval value
 */
uint8_t cis_RGB888_GetBuffData(uint32_t index)
{
//	if (index >= CIS_PIXELS_NB)
//		Error_Handler();
	return cisData[index + CIS_PIXEX_AERA_START];
}

/**
 * @brief  Manages Image process.
 * @param  None
 * @retval Image error
 */
void cis_RGB888_ImageProcess(void)
{
	/* 1st half buffer played; so fill it and continue playing from bottom*/
	if(cisBufferState == BUFFER_OFFSET_HALF)
	{
		cisBufferState = BUFFER_OFFSET_NONE;
		/* Invalidate Data Cache to get the updated content of the SRAM on the first half of the ADC converted data buffer */
		SCB_InvalidateDCache_by_Addr((uint32_t *) &cisData[0], ADC_CONVERTED_DATA_BUFFER_SIZE/2);
	}

	/* 2nd half buffer played; so fill it and continue playing from top */
	if(cisBufferState == BUFFER_OFFSET_FULL)
	{
		cisBufferState = BUFFER_OFFSET_NONE;
		/* Invalidate Data Cache to get the updated content of the SRAM on the second half of the ADC converted data buffer */
		SCB_InvalidateDCache_by_Addr((uint32_t *) &cisData[ADC_CONVERTED_DATA_BUFFER_SIZE/2], ADC_CONVERTED_DATA_BUFFER_SIZE/2);
	}
}

/**
 * @brief  Init CIS clock Frequency
 * @param  sampling_frequency
 * @retval None
 */
void cis_RGB888_TIM_CLK_Init(uint32_t cis_clk_freq)
{
	/* Counter Prescaler value */
	uint32_t uwPrescalerValue = 0;

	TIM_SlaveConfigTypeDef sSlaveConfig = {0};
	TIM_MasterConfigTypeDef sMasterConfig = {0};
	TIM_OC_InitTypeDef sConfigOC = {0};
	TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};

	/* Compute the prescaler value */
	uwPrescalerValue = (uint32_t)(((SystemCoreClock / 20) / cis_clk_freq) - 1); //cis_clk_freq


	htim1.Instance = TIM1;

	htim1.Init.Period        = 10-1;
	htim1.Init.Prescaler     = uwPrescalerValue;
	htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	htim1.Init.CounterMode   = TIM_COUNTERMODE_UP;
	htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
	htim1.Init.RepetitionCounter = 0;
	if(HAL_TIM_OC_Init(&htim1) != HAL_OK)
	{
		/* Initialization Error */
		Error_Handler();
	}

	/*##-2- Configure the Output Compare channels ##############################*/
	/* Common configuration for all channels */
	sConfigOC.OCMode     = TIM_OCMODE_PWM1;
	sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
	sConfigOC.OCFastMode = TIM_OCFAST_ENABLE;

	/* Output Compare Toggle Mode configuration: Channel1 */
	sConfigOC.Pulse 	   = 5;
	if(HAL_TIM_OC_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
	{
		/* Configuration Error */
		Error_Handler();
	}

	/* Output Compare Toggle Mode configuration: Channel1 */
	sConfigOC.OCMode  	 = TIM_OCMODE_PWM2;
	sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
	sConfigOC.Pulse   	 = 2;
	if(HAL_TIM_OC_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
	{
		/* Configuration Error */
		Error_Handler();
	}

	sMasterConfig.MasterOutputTrigger = TIM_TRGO_OC1;
	sMasterConfig.MasterOutputTrigger2 = TIM_TRGO2_RESET;
	sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_ENABLE;
	if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
	{
		Error_Handler();
	}

	HAL_TIM_MspPostInit(&htim1);

	/* Start CLK generation ##################################*/
	if(HAL_TIM_OC_Start(&htim1, TIM_CHANNEL_1) != HAL_OK)
	{
		Error_Handler();
	}

	/* Start ADC Timer #######################################*/
	if(HAL_TIM_OC_Start(&htim1, TIM_CHANNEL_2) != HAL_OK)
	{
		Error_Handler();
	}
}

/**
 * @brief  CIS start pulse timer init
 * @param  Void
 * @retval None
 */
void cis_RGB888_TIM_SP_Init()
{
	TIM_SlaveConfigTypeDef sSlaveConfig = {0};
	TIM_MasterConfigTypeDef sMasterConfig = {0};
	TIM_OC_InitTypeDef sConfigOC = {0};
	TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};

	htim15.Instance = TIM15;
	htim15.Init.CounterMode = TIM_COUNTERMODE_UP;
	htim15.Init.Prescaler = 0;
	htim15.Init.Period = CIS_END_CAPTURE - 1;
	htim15.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	htim15.Init.RepetitionCounter = 0;
	htim15.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
	if (HAL_TIM_Base_Init(&htim15) != HAL_OK)
	{
		Error_Handler();
	}

	if (HAL_TIM_OC_Init(&htim15) != HAL_OK)
	{
		Error_Handler();
	}

	sSlaveConfig.SlaveMode = TIM_SLAVEMODE_GATED;
	sSlaveConfig.InputTrigger = TIM_TS_ITR0;
	sSlaveConfig.TriggerPolarity  = TIM_TRIGGERPOLARITY_NONINVERTED;
	sSlaveConfig.TriggerPrescaler = TIM_TRIGGERPRESCALER_DIV1;
	sSlaveConfig.TriggerFilter    = 0;
	if (HAL_TIM_SlaveConfigSynchro(&htim15, &sSlaveConfig) != HAL_OK)
	{
		Error_Handler();
	}

	sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
	sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
	if (HAL_TIMEx_MasterConfigSynchronization(&htim15, &sMasterConfig) != HAL_OK)
	{
		Error_Handler();
	}

	sConfigOC.OCMode = TIM_OCMODE_PWM2;
	sConfigOC.Pulse = CIS_SP_OFF;
	sConfigOC.OCPolarity = TIM_OCPOLARITY_LOW;
	sConfigOC.OCNPolarity = TIM_OCNPOLARITY_LOW;
	sConfigOC.OCFastMode = TIM_OCFAST_ENABLE;
	sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
	sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
	if (HAL_TIM_OC_ConfigChannel(&htim15, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
	{
		Error_Handler();
	}

	HAL_TIM_MspPostInit(&htim15);

	/* Start SP generation ##################################*/
	if(HAL_TIM_OC_Start(&htim15, TIM_CHANNEL_2) != HAL_OK)
	{
		Error_Handler();
	}
}

/**
 * @brief  CIS red led timer init
 * @param  Void
 * @retval None
 */
void cis_RGB888_TIM_LED_R_Init()
{
	TIM_SlaveConfigTypeDef sSlaveConfig = {0};
	TIM_MasterConfigTypeDef sMasterConfig = {0};
	TIM_OC_InitTypeDef sConfigOC = {0};

	htim3.Instance = TIM3;
	htim3.Init.Prescaler = 0;
	htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
	htim3.Init.Period = (CIS_END_CAPTURE * 3) - 1;
	htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
	htim3.Init.RepetitionCounter = 0;
	if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
	{
		Error_Handler();
	}
	if (HAL_TIM_OC_Init(&htim3) != HAL_OK)
	{
		Error_Handler();
	}
	sSlaveConfig.SlaveMode = TIM_SLAVEMODE_GATED;
	sSlaveConfig.InputTrigger = TIM_TS_ITR0;
	sSlaveConfig.TriggerPolarity  = TIM_TRIGGERPOLARITY_INVERTED;
	sSlaveConfig.TriggerPrescaler = TIM_TRIGGERPRESCALER_DIV1;
	sSlaveConfig.TriggerFilter    = 0;
	if (HAL_TIM_SlaveConfigSynchro(&htim3, &sSlaveConfig) != HAL_OK)
	{
		Error_Handler();
	}
	sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
	sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
	if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
	{
		Error_Handler();
	}
	sConfigOC.OCMode = TIM_OCMODE_PWM1;
	sConfigOC.Pulse = CIS_LED_RED_OFF;
	sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
	sConfigOC.OCFastMode = TIM_OCFAST_ENABLE;
	if (HAL_TIM_OC_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
	{
		Error_Handler();
	}
	HAL_TIM_MspPostInit(&htim3);

	/* Start LED R generation ###############################*/
	if(HAL_TIM_OC_Start(&htim3, TIM_CHANNEL_1) != HAL_OK)
	{
		Error_Handler();
	}
}

/**
 * @brief  CIS green led timer init
 * @param  Void
 * @retval None
 */
void cis_RGB888_TIM_LED_G_Init()
{
	TIM_SlaveConfigTypeDef sSlaveConfig = {0};
	TIM_MasterConfigTypeDef sMasterConfig = {0};
	TIM_OC_InitTypeDef sConfigOC = {0};

	htim4.Instance = TIM4;
	htim4.Init.Prescaler = 0;
	htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
	htim4.Init.Period = (CIS_END_CAPTURE * 3) - 1;
	htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	htim4.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
	htim4.Init.RepetitionCounter = 0;
	if (HAL_TIM_Base_Init(&htim4) != HAL_OK)
	{
		Error_Handler();
	}
	if (HAL_TIM_OC_Init(&htim4) != HAL_OK)
	{
		Error_Handler();
	}
	sSlaveConfig.SlaveMode = TIM_SLAVEMODE_GATED;
	sSlaveConfig.InputTrigger = TIM_TS_ITR0;
	if (HAL_TIM_SlaveConfigSynchro(&htim4, &sSlaveConfig) != HAL_OK)
	{
		Error_Handler();
	}
	sConfigOC.OCMode = TIM_OCMODE_PWM1;
	sConfigOC.Pulse = CIS_LED_GREEN_OFF;
	sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
	sConfigOC.OCFastMode = TIM_OCFAST_ENABLE;
	if (HAL_TIM_OC_ConfigChannel(&htim4, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
	{
		Error_Handler();
	}
	HAL_TIM_MspPostInit(&htim4);

	/* Start LED G generation ###############################*/
	if(HAL_TIM_OC_Start(&htim4, TIM_CHANNEL_2) != HAL_OK)
	{
		Error_Handler();
	}
}

/**
 * @brief  CIS blue led timer init
 * @param  Void
 * @retval None
 */
void cis_RGB888_TIM_LED_B_Init()
{
	TIM_SlaveConfigTypeDef sSlaveConfig = {0};
	TIM_MasterConfigTypeDef sMasterConfig = {0};
	TIM_OC_InitTypeDef sConfigOC = {0};

	htim8.Instance = TIM8;
	htim8.Init.Prescaler = 0;
	htim8.Init.CounterMode = TIM_COUNTERMODE_UP;
	htim8.Init.Period = (CIS_END_CAPTURE * 3) - 1;
	htim8.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	htim8.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
	htim8.Init.RepetitionCounter = 0;
	if (HAL_TIM_Base_Init(&htim8) != HAL_OK)
	{
		Error_Handler();
	}
	if (HAL_TIM_OC_Init(&htim8) != HAL_OK)
	{
		Error_Handler();
	}
	sSlaveConfig.SlaveMode = TIM_SLAVEMODE_GATED;
	sSlaveConfig.InputTrigger = TIM_TS_ITR0;
	sSlaveConfig.TriggerPolarity  = TIM_TRIGGERPOLARITY_NONINVERTED;
	sSlaveConfig.TriggerPrescaler = TIM_TRIGGERPRESCALER_DIV1;
	sSlaveConfig.TriggerFilter    = 0;
	if (HAL_TIM_SlaveConfigSynchro(&htim8, &sSlaveConfig) != HAL_OK)
	{
		Error_Handler();
	}
	sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
	sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
	if (HAL_TIMEx_MasterConfigSynchronization(&htim8, &sMasterConfig) != HAL_OK)
	{
		Error_Handler();
	}
	sConfigOC.OCMode = TIM_OCMODE_PWM1;
	sConfigOC.Pulse = CIS_LED_BLUE_OFF;
	sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
	sConfigOC.OCFastMode = TIM_OCFAST_ENABLE;
	if (HAL_TIM_OC_ConfigChannel(&htim8, &sConfigOC, TIM_CHANNEL_3) != HAL_OK)
	{
		Error_Handler();
	}
	HAL_TIM_MspPostInit(&htim8);

	/* Start LED B generation ###############################*/
	if(HAL_TIMEx_OCN_Start(&htim8, TIM_CHANNEL_3) != HAL_OK)
	{
		Error_Handler();
	}
}

/**
 * @brief  CIS adc init
 * @param  Void
 * @retval None
 */
void cis_RGB888_ADC_Init(void)
{
	ADC_MultiModeTypeDef multimode = {0};
	ADC_ChannelConfTypeDef ADCsConfig = {0};
	DAC_ChannelConfTypeDef DACsConfig = {0};

	/** DAC Initialization
	 */
	hdac1.Instance = DAC1;
	if (HAL_DAC_Init(&hdac1) != HAL_OK)
	{
		Error_Handler();
	}
	/** DAC channel OUT1 config
	 */
	DACsConfig.DAC_SampleAndHold = DAC_SAMPLEANDHOLD_DISABLE;
	DACsConfig.DAC_Trigger = DAC_TRIGGER_NONE;
	DACsConfig.DAC_OutputBuffer = DAC_OUTPUTBUFFER_DISABLE;
	DACsConfig.DAC_ConnectOnChipPeripheral = DAC_CHIPCONNECT_ENABLE;
	DACsConfig.DAC_UserTrimming = DAC_TRIMMING_FACTORY;
	if (HAL_DAC_ConfigChannel(&hdac1, &DACsConfig, DAC_CHANNEL_1) != HAL_OK)
	{
		Error_Handler();
	}

	if (HAL_ADC_DeInit(&hadc1) != HAL_OK)
	{
		/* ADC de-initialization Error */
		Error_Handler();
	}

	hopamp1.Instance = OPAMP1;
	hopamp1.Init.Mode = OPAMP_PGA_MODE;
	hopamp1.Init.NonInvertingInput = OPAMP_NONINVERTINGINPUT_DAC_CH;
	hopamp1.Init.PowerMode = OPAMP_POWERMODE_HIGHSPEED;
	hopamp1.Init.PgaConnect = OPAMP_PGA_CONNECT_INVERTINGINPUT_IO0_BIAS;
	hopamp1.Init.PgaGain = OPAMP_PGA_GAIN_4_OR_MINUS_3;
	hopamp1.Init.UserTrimming = OPAMP_TRIMMING_USER;
	hopamp1.Init.TrimmingValuePHighSpeed = 15;
	hopamp1.Init.TrimmingValueNHighSpeed = 15;
	if (HAL_OPAMP_Init(&hopamp1) != HAL_OK)
	{
		Error_Handler();
	}
	if (HAL_OPAMP_SelfCalibrate(&hopamp1) != HAL_OK)
	{
		Error_Handler();
	}

	/* Set DAC output voltage (use to change OPAMP offset */
	HAL_DAC_SetValue(&hdac1, DAC_CHANNEL_1, DAC_ALIGN_8B_R,(1.44)/(3.3/256)); //CLK 2MHZ

	/*##-3- Enable DAC Channel 1 ##########################################*/
	if(HAL_DAC_Start(&hdac1, DAC_CHANNEL_1) != HAL_OK)
	{

		Error_Handler();
	}

	/*##  Start OPAMP    #####################################################*/
	/* Enable OPAMP */
	if(HAL_OK != HAL_OPAMP_Start(&hopamp1))
	{
		Error_Handler();
	}

	/** Common config
	 */
	hadc1.Instance = ADC1;
	hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV1;
	hadc1.Init.Resolution = ADC_RESOLUTION_8B;
	hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
	hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
	hadc1.Init.LowPowerAutoWait = DISABLE;
	hadc1.Init.ContinuousConvMode = DISABLE;
	hadc1.Init.NbrOfConversion = CIS_END_CAPTURE;
	hadc1.Init.DiscontinuousConvMode = DISABLE;
	hadc1.Init.ExternalTrigConv = ADC_EXTERNALTRIG_T1_CC2;
	hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_RISING;
	hadc1.Init.ConversionDataManagement = ADC_CONVERSIONDATA_DMA_CIRCULAR;
	hadc1.Init.Overrun = ADC_OVR_DATA_PRESERVED;
	hadc1.Init.LeftBitShift = ADC_LEFTBITSHIFT_NONE;
	hadc1.Init.OversamplingMode = DISABLE;
	if (HAL_ADC_Init(&hadc1) != HAL_OK)
	{
		Error_Handler();
	}
	/** Configure the ADC multi-mode
	 */
	multimode.Mode = ADC_MODE_INDEPENDENT;
	if (HAL_ADCEx_MultiModeConfigChannel(&hadc1, &multimode) != HAL_OK)
	{
		Error_Handler();
	}
	/** Configure Regular Channel
	 */
	ADCsConfig.Channel = ADC_CHANNEL_4;
	ADCsConfig.Rank = ADC_REGULAR_RANK_1;
	ADCsConfig.SamplingTime = ADC_SAMPLETIME_2CYCLES_5;
	ADCsConfig.SingleDiff = ADC_SINGLE_ENDED;
	ADCsConfig.OffsetNumber = ADC_OFFSET_NONE;
	ADCsConfig.Offset = 0;
	if (HAL_ADC_ConfigChannel(&hadc1, &ADCsConfig) != HAL_OK)
	{
		Error_Handler();
	}

	//	HAL_ADC_Stop(&hadc1);

	/* ### Start calibration ############################################ */
	if (HAL_ADCEx_Calibration_Start(&hadc1, ADC_CALIB_OFFSET, ADC_SINGLE_ENDED) != HAL_OK)
	{
		Error_Handler();
	}
}

#ifdef CIS_RGB888
/**
 * @brief  Conversion DMA half-transfer callback in non-blocking mode
 * @param  hadc: ADC handle
 * @retval None
 */
void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef* hadc)
{
	cisBufferState = BUFFER_OFFSET_HALF;
}

/**
 * @brief  Conversion complete callback in non-blocking mode
 * @param  hadc: ADC handle
 * @retval None
 */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
	cisBufferState = BUFFER_OFFSET_FULL;
}
#endif


