
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
#include "tim.h"
#include "hrtim.h"
#include "adc.h"
#include "dac.h"

#include "stdlib.h"
#include "stdio.h"

#include "cis.h"

/* Private includes ----------------------------------------------------------*/

/* Private typedef -----------------------------------------------------------*/
enum cisReadStep{START_PULSE, FIRST_CAL_ZONE, DATA_ZONE, END_CAL_ZONE};

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
uint32_t __IO cis_adc_data[CIS_PIXELS_NB] = {0};
__IO uint32_t cis_cnt = 0;

/* Private function prototypes -----------------------------------------------*/
int32_t cisTIM_Init(uint32_t cis_clk_freq);
void cisADC_Init(void);

/* Private user code ---------------------------------------------------------*/

void cisInit(void)
{
	cisTIM_Init(CIS_CLK_FREQ);
	cisADC_Init();

	/* ### Start conversion in DMA mode ################################# */
	if (HAL_ADC_Start_IT(&hadc1) != HAL_OK)
	{
		Error_Handler();
	}
}

/**
 * @brief  Init CIS clock Frequency
 * @param  sampling_frequency
 * @retval Error
 */
int32_t cisTIM_Init(uint32_t cis_clk_freq)
{
	HRTIM_ADCTriggerCfgTypeDef pADCTriggerCfg = {0};
	HRTIM_TimeBaseCfgTypeDef pTimeBaseCfg = {0};
	HRTIM_SimplePWMChannelCfgTypeDef pSimplePWMChannelCfg = {0};

	uint32_t uwPrescalerValue = 0;

	uwPrescalerValue = (uint32_t) ((SystemCoreClock / 20) / (cis_clk_freq));

	hhrtim.Instance = HRTIM1;
	hhrtim.Init.HRTIMInterruptResquests = HRTIM_IT_NONE;
	hhrtim.Init.SyncOptions = HRTIM_SYNCOPTION_NONE;
	if (HAL_HRTIM_Init(&hhrtim) != HAL_OK)
	{
		Error_Handler();
	}
	pADCTriggerCfg.UpdateSource = HRTIM_ADCTRIGGERUPDATE_TIMER_B;
	pADCTriggerCfg.Trigger = HRTIM_ADCTRIGGEREVENT13_TIMERB_PERIOD;
	if (HAL_HRTIM_ADCTriggerConfig(&hhrtim, HRTIM_ADCTRIGGER_1, &pADCTriggerCfg) != HAL_OK)
	{
		Error_Handler();
	}
	/*-------------------------------------------------------------------------------------*/
	pTimeBaseCfg.Period = uwPrescalerValue;
	pTimeBaseCfg.RepetitionCounter = 0;
	pTimeBaseCfg.PrescalerRatio = HRTIM_PRESCALERRATIO_DIV1;
	pTimeBaseCfg.Mode = HRTIM_MODE_CONTINUOUS;
	if (HAL_HRTIM_TimeBaseConfig(&hhrtim, HRTIM_TIMERINDEX_TIMER_B, &pTimeBaseCfg) != HAL_OK)
	{
		Error_Handler();
	}

	pTimeBaseCfg.Period = uwPrescalerValue * 2;
	if (HAL_HRTIM_TimeBaseConfig(&hhrtim, HRTIM_TIMERINDEX_TIMER_A, &pTimeBaseCfg) != HAL_OK)
	{
		Error_Handler();
	}
	/*-------------------------------------------------------------------------------------*/
	pSimplePWMChannelCfg.Pulse = uwPrescalerValue / 2;
	pSimplePWMChannelCfg.Polarity = HRTIM_OUTPUTPOLARITY_HIGH;
	pSimplePWMChannelCfg.IdleLevel = HRTIM_OUTPUTIDLELEVEL_ACTIVE;
	if (HAL_HRTIM_SimplePWMChannelConfig(&hhrtim, HRTIM_TIMERINDEX_TIMER_A, HRTIM_OUTPUT_TA1, &pSimplePWMChannelCfg) != HAL_OK)
	{
		Error_Handler();
	}
	pSimplePWMChannelCfg.Pulse = uwPrescalerValue / 2;
	pSimplePWMChannelCfg.Polarity = HRTIM_OUTPUTPOLARITY_LOW;
	pSimplePWMChannelCfg.IdleLevel = HRTIM_OUTPUTIDLELEVEL_ACTIVE
			;
	if (HAL_HRTIM_SimplePWMChannelConfig(&hhrtim, HRTIM_TIMERINDEX_TIMER_B, HRTIM_OUTPUT_TB1, &pSimplePWMChannelCfg) != HAL_OK)
	{
		Error_Handler();
	}
	/*-------------------------------------------------------------------------------------*/
	HAL_HRTIM_MspPostInit(&hhrtim);

	/*##-7- Start PWM signals generation ########################################################*/
	if (HAL_HRTIM_SimplePWMStart(&hhrtim, HRTIM_TIMERINDEX_TIMER_A, HRTIM_OUTPUT_TA1) != HAL_OK)
	{
		Error_Handler();
	}
	/*##-7- Start PWM signals generation ########################################################*/
	if (HAL_HRTIM_SimplePWMStart(&hhrtim, HRTIM_TIMERINDEX_TIMER_B, HRTIM_OUTPUT_TB1) != HAL_OK)
	{
		Error_Handler();
	}


	//	HRTIM_ADCTriggerCfgTypeDef pADCTriggerCfg = {0};
	//	HRTIM_TimeBaseCfgTypeDef pTimeBaseCfg = {0};
	//	HRTIM_SimpleOCChannelCfgTypeDef pSimpleOCChannelCfg = {0};
	//	HRTIM_SimplePWMChannelCfgTypeDef pSimplePWMChannelCfg = {0};
	//
	//	hhrtim.Instance = HRTIM1;
	//	hhrtim.Init.HRTIMInterruptResquests = HRTIM_IT_NONE;
	//	hhrtim.Init.SyncOptions = HRTIM_SYNCOPTION_NONE;
	//	if (HAL_HRTIM_Init(&hhrtim) != HAL_OK)
	//	{
	//		Error_Handler();
	//	}
	//	pADCTriggerCfg.UpdateSource = HRTIM_ADCTRIGGERUPDATE_TIMER_B;
	//	pADCTriggerCfg.Trigger = HRTIM_ADCTRIGGEREVENT13_TIMERB_PERIOD;
	//	if (HAL_HRTIM_ADCTriggerConfig(&hhrtim, HRTIM_ADCTRIGGER_1, &pADCTriggerCfg) != HAL_OK)
	//	{
	//		Error_Handler();
	//	}
	//	pTimeBaseCfg.Period = 1600;
	//	pTimeBaseCfg.RepetitionCounter = 0x00;
	//	pTimeBaseCfg.PrescalerRatio = HRTIM_PRESCALERRATIO_DIV1;
	//	pTimeBaseCfg.Mode = HRTIM_MODE_CONTINUOUS;
	//	if (HAL_HRTIM_TimeBaseConfig(&hhrtim, HRTIM_TIMERINDEX_TIMER_A, &pTimeBaseCfg) != HAL_OK)
	//	{
	//		Error_Handler();
	//	}
	//	pSimpleOCChannelCfg.Mode = HRTIM_BASICOCMODE_TOGGLE;
	//	pSimpleOCChannelCfg.Pulse = 800;
	//	pSimpleOCChannelCfg.Polarity = HRTIM_OUTPUTPOLARITY_LOW;
	//	pSimpleOCChannelCfg.IdleLevel = HRTIM_OUTPUTIDLELEVEL_INACTIVE;
	//	if (HAL_HRTIM_SimpleOCChannelConfig(&hhrtim, HRTIM_TIMERINDEX_TIMER_A, HRTIM_OUTPUT_TA1, &pSimpleOCChannelCfg) != HAL_OK)
	//	{
	//		Error_Handler();
	//	}
	//	pTimeBaseCfg.Period = 3200;
	//	if (HAL_HRTIM_TimeBaseConfig(&hhrtim, HRTIM_TIMERINDEX_TIMER_B, &pTimeBaseCfg) != HAL_OK)
	//	{
	//		Error_Handler();
	//	}
	//	pSimplePWMChannelCfg.Pulse = 10;
	//	pSimplePWMChannelCfg.Polarity = HRTIM_OUTPUTPOLARITY_LOW;
	//	pSimplePWMChannelCfg.IdleLevel = HRTIM_OUTPUTIDLELEVEL_INACTIVE;
	//	if (HAL_HRTIM_SimplePWMChannelConfig(&hhrtim, HRTIM_TIMERINDEX_TIMER_B, HRTIM_OUTPUT_TB1, &pSimplePWMChannelCfg) != HAL_OK)
	//	{
	//		Error_Handler();
	//	}
	//	HAL_HRTIM_MspPostInit(&hhrtim);
	//
	//	if (HAL_HRTIM_SimpleOCStart(&hhrtim, HRTIM_TIMERINDEX_TIMER_A, HRTIM_OUTPUT_TA1) != HAL_OK)
	//	{
	//		/* PWM Generation Error */
	//		return -3;
	//	}
	//
	//	/*##-7- Start PWM signals generation ########################################################*/
	//	if (HAL_HRTIM_SimplePWMStart(&hhrtim, HRTIM_TIMERINDEX_TIMER_B, HRTIM_OUTPUT_TB1) != HAL_OK)
	//	{
	//		/* PWM Generation Error */
	//		return -4;
	//	}
	return 0;
}

void cisADC_Init(void)
{
	ADC_MultiModeTypeDef multimode = {0};
	ADC_ChannelConfTypeDef sConfig = {0};

	/** Common config
	 */
	hadc1.Instance = ADC1;
	hadc1.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV16;
	hadc1.Init.Resolution = ADC_RESOLUTION_16B;
	hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
	hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
	hadc1.Init.LowPowerAutoWait = DISABLE;
	hadc1.Init.ContinuousConvMode = DISABLE;
	hadc1.Init.NbrOfConversion = 1;
	hadc1.Init.DiscontinuousConvMode = DISABLE;
	hadc1.Init.ExternalTrigConv = ADC_EXTERNALTRIG_HR1_ADCTRG1;
	hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_FALLING;
	hadc1.Init.ConversionDataManagement = ADC_CONVERSIONDATA_DR;
	hadc1.Init.Overrun = ADC_OVR_DATA_OVERWRITTEN;
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
	sConfig.Channel = ADC_CHANNEL_15;
	sConfig.Rank = ADC_REGULAR_RANK_1;
	sConfig.SamplingTime = ADC_SAMPLETIME_64CYCLES_5;
	sConfig.SingleDiff = ADC_SINGLE_ENDED;
	sConfig.OffsetNumber = ADC_OFFSET_NONE;
	sConfig.Offset = 1;
	sConfig.OffsetRightShift = DISABLE;
	sConfig.OffsetSignedSaturation = DISABLE;
	if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
	{
		Error_Handler();
	}

	/* ### - 2 - Start calibration ############################################ */
	if (HAL_ADCEx_Calibration_Start(&hadc1, ADC_CALIB_OFFSET, ADC_SINGLE_ENDED) != HAL_OK)
	{
		Error_Handler();
	}
}

#define CIS_SP_TICK 				(10)
#define CIS_SP_WAIT					((CIS_SP_TICK) + 2)
#define CIS_FIRST_CAL_TICK 			((CIS_SP_WAIT) + (CIS_CAL_AERA_SIZE))
#define CIS_DATA_TICK 				((CIS_FIRST_CAL_TICK) + (CIS_PIXELS_NB))
#define CIS_END_CAL_TICK 			((CIS_DATA_TICK) + (32))
/**
 * @brief  End of conversion callback in non blocking mode
 * @param  hadc : hadc handle
 * @retval None
 */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
	++cis_cnt;
	static uint32_t pixel_cnt = 0;
	static uint32_t callback_cnt = 0;
	static enum cisReadStep cis_read_step = START_PULSE;

	//	if (hadc != &hadc1)
	//		return;

	switch (callback_cnt)
	{
	case 0:
		cis_read_step = START_PULSE;
		break;
	case CIS_SP_TICK:
		cis_read_step = FIRST_CAL_ZONE;
		break;
	case CIS_FIRST_CAL_TICK:
		cis_read_step = DATA_ZONE;
		break;
	case CIS_DATA_TICK:
		cis_read_step = END_CAL_ZONE;
		pixel_cnt = 0;
		break;
	case CIS_END_CAL_TICK:
		cis_read_step = START_PULSE;
		callback_cnt = 0;
		--cis_cnt;
		return;
		break;
	}


	++callback_cnt;

	switch (cis_read_step)
	{
	case START_PULSE:
		HAL_GPIO_WritePin(CIS_SP_GPIO_Port, CIS_SP_Pin, GPIO_PIN_SET);
		break;
	case FIRST_CAL_ZONE:
		HAL_GPIO_WritePin(CIS_SP_GPIO_Port, CIS_SP_Pin, GPIO_PIN_RESET);
		break;
	case DATA_ZONE:
		cis_adc_data[pixel_cnt] = HAL_ADC_GetValue(&hadc1) - 6000;
		HAL_DAC_SetValue(&hdac1, DAC_CHANNEL_2, DAC_ALIGN_12B_L, cis_adc_data[pixel_cnt]);
		++pixel_cnt;
		break;
	case END_CAL_ZONE:
		break;
	}

	--cis_cnt;
}
