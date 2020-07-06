
/**
 ******************************************************************************
 * @file           : cis.c
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
enum cisCalStep{CAL_ON, CAL_OFF};

typedef enum
{
	BUFFER_OFFSET_NONE = 0,
	BUFFER_OFFSET_HALF,
	BUFFER_OFFSET_FULL,
}CIS_BUFF_StateTypeDef;

/* Private define ------------------------------------------------------------*/
#define CIS_SP_GPIO_Port ARD_D6_GPIO_Port
#define CIS_SP_Pin ARD_D6_Pin
/* Definition of ADCx conversions data table size */
#define ADC_CONVERTED_DATA_BUFFER_SIZE   ((uint32_t) CIS_END_CAPTURE) /* Size of array aADCxConvertedData[] */

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
/* Variable containing ADC conversions data */
ALIGN_32BYTES (static uint8_t   cisData[ADC_CONVERTED_DATA_BUFFER_SIZE]);

static CIS_BUFF_StateTypeDef  cisBufferState = {0};

#ifdef DEBUG_CIS
__IO uint32_t cis_dbg_cnt = 0;
__IO uint32_t cis_dbg_data_cal = 0;
__IO uint32_t cis_dbg_data = 0;
#endif

/* Private function prototypes -----------------------------------------------*/
int32_t cisTIM_CLK_Init(uint32_t cis_clk_freq);
int32_t cisTIM_SP_Init(void);
int32_t cisTIM_LED_G_Init(void);

void cisADC_Init(void);
void cisDisplayLine(void);

/* Private user code ---------------------------------------------------------*/

void cisInit(void)
{
	cisADC_Init();
	cisTIM_SP_Init();
	cisTIM_LED_G_Init();
	cisTIM_CLK_Init(CIS_CLK_FREQ);

	while(1)
	{
		cisDisplayLine();
	}
}

void cisDisplayLine(void)
{
	static uint32_t j = 24;
	uint32_t x_size, y_size;

	BSP_LCD_GetXSize(0, &x_size);
	BSP_LCD_GetYSize(0, &y_size);

	/* 1st half buffer played; so fill it and continue playing from bottom*/
	if(cisBufferState == BUFFER_OFFSET_HALF)
	{
		cisBufferState = BUFFER_OFFSET_NONE;
		/* Invalidate Data Cache to get the updated content of the SRAM on the first half of the ADC converted data buffer: 32 bytes */
		SCB_InvalidateDCache_by_Addr((uint32_t *) &cisData[0], ADC_CONVERTED_DATA_BUFFER_SIZE/2);
	}

	/* 2nd half buffer played; so fill it and continue playing from top */
	if(cisBufferState == BUFFER_OFFSET_FULL)
	{
		cisBufferState = BUFFER_OFFSET_NONE;
		/* Invalidate Data Cache to get the updated content of the SRAM on the second half of the ADC converted data buffer: 32 bytes */
		SCB_InvalidateDCache_by_Addr((uint32_t *) &cisData[ADC_CONVERTED_DATA_BUFFER_SIZE/2], ADC_CONVERTED_DATA_BUFFER_SIZE/2);
		for (uint32_t i = 0; i < x_size; i++)
		{
			GUI_SetPixel(i, j, (0xFF000000 | (cisData[i + CIS_PIXEX_AERA_START] << 8)));
		}
		j++;
		if (j >= (y_size - 24))
			j = 24;
	}
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	if (GPIO_Pin != GPIO_PIN_13)
		return;

	//	cisCalibration();
}

/**
 * @brief  Init CIS clock Frequency
 * @param  sampling_frequency
 * @retval Error
 */
int32_t cisTIM_CLK_Init(uint32_t cis_clk_freq)
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

	cisTIM_SP_Init();
	cisTIM_LED_G_Init();

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

	return 0;
}

int32_t cisTIM_SP_Init()
{
	TIM_SlaveConfigTypeDef sSlaveConfig = {0};
	TIM_MasterConfigTypeDef sMasterConfig = {0};
	TIM_OC_InitTypeDef sConfigOC = {0};
	TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};

	htim15.Instance = TIM15;
	htim15.Init.CounterMode = TIM_COUNTERMODE_UP;
	htim15.Init.Prescaler = 0;
	htim15.Init.Period = CIS_FINAL_WAIT - 1;
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
	return 0;
}

int32_t cisTIM_LED_G_Init()
{
	TIM_SlaveConfigTypeDef sSlaveConfig = {0};
	TIM_MasterConfigTypeDef sMasterConfig = {0};
	TIM_OC_InitTypeDef sConfigOC = {0};

	htim3.Instance = TIM3;
	htim3.Init.Prescaler = 0;
	htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
	htim3.Init.Period = CIS_FINAL_WAIT - 1;
	htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
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
	sConfigOC.Pulse = CIS_LED_GREEN_OFF;
	sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
	sConfigOC.OCFastMode = TIM_OCFAST_ENABLE;
	if (HAL_TIM_OC_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
	{
		Error_Handler();
	}
	HAL_TIM_MspPostInit(&htim3);

	/* Start LED G generation ###############################*/
	if(HAL_TIM_OC_Start(&htim3, TIM_CHANNEL_1) != HAL_OK)
	{
		Error_Handler();
	}

	return 0;
}

void cisADC_Init(void)
{
	ADC_MultiModeTypeDef multimode = {0};
	ADC_ChannelConfTypeDef sConfig = {0};

	if (HAL_ADC_DeInit(&hadc1) != HAL_OK)
	{
		/* ADC de-initialization Error */
		Error_Handler();
	}

	/* Set DAC output voltage (use to change OPAMP offset */
	HAL_DAC_SetValue(&hdac1, DAC_CHANNEL_1, DAC_ALIGN_8B_R, 93);//(0.64/(3.3/256)));

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
	sConfig.Channel = ADC_CHANNEL_4;
	sConfig.Rank = ADC_REGULAR_RANK_1;
	sConfig.SamplingTime = ADC_SAMPLETIME_1CYCLE_5;
	sConfig.SingleDiff = ADC_SINGLE_ENDED;
	sConfig.OffsetNumber = ADC_OFFSET_1;
	sConfig.Offset = 0;
	if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
	{
		Error_Handler();
	}

	//	HAL_ADC_Stop(&hadc1);

	/* ### Start calibration ############################################ */
	if (HAL_ADCEx_Calibration_Start(&hadc1, ADC_CALIB_OFFSET, ADC_SINGLE_ENDED) != HAL_OK)
	{
		Error_Handler();
	}

	if (HAL_ADC_Start_DMA(&hadc1, (uint32_t *)cisData, ADC_CONVERTED_DATA_BUFFER_SIZE) != HAL_OK)
	{
		Error_Handler();
	}
}

/**
 * @brief  Conversion complete callback in non-blocking mode
 * @param  hadc: ADC handle
 * @retval None
 */
void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef* hadc)
{
	cisBufferState = BUFFER_OFFSET_HALF;
}

/**
 * @brief  Conversion DMA half-transfer callback in non-blocking mode
 * @param  hadc: ADC handle
 * @retval None
 */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
	cisBufferState = BUFFER_OFFSET_FULL;

	__HAL_TIM_SET_COUNTER(&htim15, 0);
	__HAL_TIM_SET_COUNTER(&htim3, 0);
}



