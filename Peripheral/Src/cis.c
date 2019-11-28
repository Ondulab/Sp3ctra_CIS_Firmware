
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
#include "hrtim.h"
#include "adc.h"
#include "dac.h"

#include "stdlib.h"
#include "stdio.h"
#include "stdbool.h"

#include "cis.h"

/* Private includes ----------------------------------------------------------*/

/* Private typedef -----------------------------------------------------------*/
enum cisReadStep{START_PULSE, INIT_ZONE, CAL_ZONE, DATA_ZONE, END_ZONE};
enum cisCalStep{CAL_ON, CAL_OFF};

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
__IO uint32_t cis_adc_data[CIS_PIXELS_NB] = {0};
static __IO uint32_t cis_adc_offset[CIS_PIXELS_NB] = {0};
static __IO enum cisCalStep calibration_state = CAL_OFF;

#ifdef DEBUG_CIS
__IO uint32_t cis_dbg_cnt = 0;
__IO uint32_t cis_dbg_data_cal = 0;
__IO uint32_t cis_dbg_data = 0;
#endif

/* Private function prototypes -----------------------------------------------*/
int32_t cisTIM_Init(uint32_t cis_clk_freq);
void cisADC_Init(void);
void cisCalibration(void);

/* Private user code ---------------------------------------------------------*/

void cisInit(void)
{
	cisADC_Init();
	cisTIM_Init(CIS_CLK_FREQ);
	HAL_Delay(100);
	cisCalibration();
}

void cisCalibration(void)
{
	uint32_t cnt = 0;

	printf("/***** START CIS CALIBRATION *****/\n");
	calibration_state = CAL_ON;

	while (cnt < CIS_CAL_CYCLE)
	{
		if (calibration_state != CAL_ON)
		{
			cnt++;
			calibration_state = CAL_ON;
		}
	}
	calibration_state = CAL_OFF;

	for (uint32_t i = 0; i < CIS_PIXELS_NB; i++)
	{
		cis_adc_offset[i] /= CIS_CAL_CYCLE;
	}
	printf("/******** END CALIBRATION ********/\n");
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
int32_t cisTIM_Init(uint32_t cis_clk_freq)
{
	HRTIM_ADCTriggerCfgTypeDef pADCTriggerCfg = {0};
	HRTIM_TimeBaseCfgTypeDef pTimeBaseCfg = {0};
	HRTIM_SimplePWMChannelCfgTypeDef pSimplePWMChannelCfg = {0};

	uint32_t uwPrescalerValue = 0;

	uwPrescalerValue = (uint32_t) ((SystemCoreClock / 2) / (cis_clk_freq));

	hhrtim.Instance = HRTIM1;
	hhrtim.Init.HRTIMInterruptResquests = HRTIM_IT_NONE;
	hhrtim.Init.SyncOptions = HRTIM_SYNCOPTION_NONE;
	if (HAL_HRTIM_Init(&hhrtim) != HAL_OK)
	{
		Error_Handler();
	}
	pADCTriggerCfg.UpdateSource = HRTIM_ADCTRIGGERUPDATE_TIMER_A;
	pADCTriggerCfg.Trigger = HRTIM_ADCTRIGGEREVENT13_TIMERA_PERIOD;
	if (HAL_HRTIM_ADCTriggerConfig(&hhrtim, HRTIM_ADCTRIGGER_1, &pADCTriggerCfg) != HAL_OK)
	{
		Error_Handler();
	}
	/*-------------------------------------------------------------------------------------*/
	pTimeBaseCfg.Period = uwPrescalerValue;
	pTimeBaseCfg.RepetitionCounter = 0;
	pTimeBaseCfg.PrescalerRatio = HRTIM_PRESCALERRATIO_DIV1;
	pTimeBaseCfg.Mode = HRTIM_MODE_CONTINUOUS;
	if (HAL_HRTIM_TimeBaseConfig(&hhrtim, HRTIM_TIMERINDEX_TIMER_A, &pTimeBaseCfg) != HAL_OK)
	{
		Error_Handler();
	}

	pTimeBaseCfg.Period = uwPrescalerValue;

	/*-------------------------------------------------------------------------------------*/
	pSimplePWMChannelCfg.Pulse = (CIS_CLK_DUTY * uwPrescalerValue) / 100;
	pSimplePWMChannelCfg.Polarity = HRTIM_OUTPUTPOLARITY_LOW;
	pSimplePWMChannelCfg.IdleLevel = HRTIM_OUTPUTIDLELEVEL_ACTIVE;
	if (HAL_HRTIM_SimplePWMChannelConfig(&hhrtim, HRTIM_TIMERINDEX_TIMER_A, HRTIM_OUTPUT_TA1, &pSimplePWMChannelCfg) != HAL_OK)
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

	return 0;
}

void cisADC_Init(void)
{
	ADC_MultiModeTypeDef multimode = {0};
	ADC_ChannelConfTypeDef sConfig = {0};

	/* Common config */
	hadc1.Instance = ADC1;
	hadc1.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV2;
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
	/* Configure the ADC multi-mode */
	multimode.Mode = ADC_MODE_INDEPENDENT;
	if (HAL_ADCEx_MultiModeConfigChannel(&hadc1, &multimode) != HAL_OK)
	{
		Error_Handler();
	}
	/* Configure Regular Channel */
	sConfig.Channel = ADC_CHANNEL_5;
	sConfig.Rank = ADC_REGULAR_RANK_1;
	sConfig.SamplingTime = ADC_SAMPLETIME_2CYCLES_5;
	sConfig.SingleDiff = ADC_SINGLE_ENDED;
	sConfig.OffsetNumber = ADC_OFFSET_NONE;
	sConfig.Offset = 0;
	sConfig.OffsetRightShift = DISABLE;
	sConfig.OffsetSignedSaturation = DISABLE;
	if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
	{
		Error_Handler();
	}
	/* ### Start calibration ############################################ */
	if (HAL_ADCEx_Calibration_Start(&hadc1, ADC_CALIB_OFFSET, ADC_SINGLE_ENDED) != HAL_OK)
	{
		Error_Handler();
	}

	/* ### Start conversion in DMA mode ################################# */
	if (HAL_ADC_Start_IT(&hadc1) != HAL_OK)
	{
		Error_Handler();
	}
	/*******************************************************************************************/
	/** Common config
	 */
//	hadc2.Instance = ADC2;
//	hadc2.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV2;
//	hadc2.Init.Resolution = ADC_RESOLUTION_16B;
//	hadc2.Init.ScanConvMode = ADC_SCAN_DISABLE;
//	hadc2.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
//	hadc2.Init.LowPowerAutoWait = DISABLE;
//	hadc2.Init.ContinuousConvMode = DISABLE;
//	hadc2.Init.NbrOfConversion = 1;
//	hadc2.Init.DiscontinuousConvMode = DISABLE;
//	hadc2.Init.ExternalTrigConv = ADC_EXTERNALTRIG_HR1_ADCTRG1;
//	hadc2.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_FALLING;
//	hadc2.Init.ConversionDataManagement = ADC_CONVERSIONDATA_DR;
//	hadc2.Init.Overrun = ADC_OVR_DATA_OVERWRITTEN;
//	hadc2.Init.LeftBitShift = ADC_LEFTBITSHIFT_NONE;
//	hadc2.Init.OversamplingMode = DISABLE;
//	if (HAL_ADC_Init(&hadc2) != HAL_OK)
//	{
//		Error_Handler();
//	}
//	/** Configure Regular Channel
//	 */
//	sConfig.Channel = ADC_CHANNEL_9;
//	sConfig.Rank = ADC_REGULAR_RANK_1;
//	sConfig.SamplingTime = ADC_SAMPLETIME_8CYCLES_5;
//	sConfig.SingleDiff = ADC_SINGLE_ENDED;
//	sConfig.OffsetNumber = ADC_OFFSET_NONE;
//	sConfig.Offset = 0;
//	if (HAL_ADC_ConfigChannel(&hadc2, &sConfig) != HAL_OK)
//	{
//		Error_Handler();
//	}
//	/* ### Start calibration ############################################ */
//	if (HAL_ADCEx_Calibration_Start(&hadc2, ADC_CALIB_OFFSET, ADC_SINGLE_ENDED) != HAL_OK)
//	{
//		Error_Handler();
//	}

//	/* ### Start conversion in DMA mode ################################# */
//	if (HAL_ADC_Start(&hadc2) != HAL_OK)
//	{
//		Error_Handler();
//	}
}

#define CIS_SP_TICK 				(2)
#define CIS_INIT_TICK				((CIS_SP_TICK) + (CIS_INIT_CLK_CNT))
#define CIS_CAL_TICK 				((CIS_INIT_TICK) + (CIS_CAL_CLK_CNT))
#define CIS_DATA_TICK 				((CIS_CAL_TICK) + (CIS_PIXELS_NB))
#define CIS_END_TICK 				((CIS_DATA_TICK) + (CIS_END_CLK_CNT))
/**
 * @brief  End of conversion callback in non blocking mode
 * @param  hadc : hadc handle
 * @retval None
 */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
//	if (hadc != &hadc1)
//		return;

#ifdef DEBUG_CIS
	++cis_dbg_cnt;
#endif
	static int32_t temp_data = 0;
	static int32_t cis_adc_cal = 0;
	static uint32_t pixel_per_comma = 0;
	static uint32_t pixel_cnt = 0;
	static uint32_t callback_cnt = 0;
	static uint32_t cnt = 0;
	static bool cal_state = FALSE;
	static enum cisReadStep cis_read_step = START_PULSE;

	switch (callback_cnt)
	{
	case 0:
		cis_read_step = START_PULSE;
		break;
	case CIS_SP_TICK:
		cis_read_step = INIT_ZONE;
		break;
	case CIS_INIT_TICK:
		cis_read_step = CAL_ZONE;
		break;
	case CIS_CAL_TICK:
		cis_read_step = DATA_ZONE;
		cis_adc_cal /= CIS_CAL_CLK_CNT;
#ifdef DEBUG_CIS
		//		printf("C %d\n", (int)cis_adc_cal);
		cis_dbg_data_cal = cis_adc_cal;
#endif
		if (calibration_state != CAL_OFF)
		{
			cal_state = TRUE;
		}
		else
		{
			cal_state = OFF;
		}
		break;
	case CIS_DATA_TICK:
		cis_read_step = END_ZONE;
		pixel_cnt = 0;
		cis_adc_cal = 0;
		break;
	case CIS_END_TICK:
		calibration_state = CAL_OFF;
		cis_read_step = START_PULSE;
		callback_cnt = 0;
		return;
		break;
	}


	++callback_cnt;

	switch (cis_read_step)
	{
	case START_PULSE:
		HAL_GPIO_WritePin(CIS_SP_GPIO_Port, CIS_SP_Pin, GPIO_PIN_SET);
		break;
	case INIT_ZONE:
		HAL_GPIO_WritePin(CIS_SP_GPIO_Port, CIS_SP_Pin, GPIO_PIN_RESET);
		break;
	case CAL_ZONE:
		cis_adc_cal += 1;// HAL_ADC_GetValue(&hadc2);
		++cnt;
		//		if (HAL_ADC_GetValue(&hadc1) < 100) //cut tac tac bug...
		//		{
		//			calibration_state = CAL_OFF;
		//			cis_read_step = START_PULSE;
		//			callback_cnt = 0;
		//		}
		//		else
		//		{
//					++cnt;
		//		}
		break;
	case DATA_ZONE:
		temp_data += HAL_ADC_GetValue(&hadc1);
#ifdef DEBUG_CIS
		//		printf("D %d\n", (int)HAL_ADC_GetValue(&hadc1));
		cis_dbg_data = HAL_ADC_GetValue(&hadc1);
#endif
		if (pixel_per_comma < PIXEL_PER_COMMA)
		{
			++pixel_per_comma;
		}
		else
		{
			if (cal_state == TRUE)
			{
				cis_adc_offset[pixel_cnt] += (temp_data / PIXEL_PER_COMMA);
			}
			else
			{
				if (cis_adc_offset[pixel_cnt] > (temp_data / PIXEL_PER_COMMA))
				{
					cis_adc_data[pixel_cnt] = cis_adc_offset[pixel_cnt] - (temp_data / PIXEL_PER_COMMA);
				}
				else
				{
					cis_adc_data[pixel_cnt] = 0;
				}
			}
			++pixel_cnt;
			pixel_per_comma = 0;
			temp_data = 0;
		}
		break;
	case END_ZONE:
		break;
	}
}
