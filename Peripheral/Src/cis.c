
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

#include "stdlib.h"
#include "stdio.h"
#include "stdbool.h"

#include "cis.h"

/* Private includes ----------------------------------------------------------*/

/* Private typedef -----------------------------------------------------------*/
enum cisReadStep{READ_OFF, START_PULSE, INIT_ZONE, CAL_ZONE, DEAD_ZONE, DATA_ZONE, END_ZONE};
enum cisCalStep{CAL_ON, CAL_OFF};

/* Private define ------------------------------------------------------------*/
#define CIS_SP_GPIO_Port ARD_D6_GPIO_Port
#define CIS_SP_Pin ARD_D6_Pin

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
	//	cisCalibration();

	HAL_GPIO_WritePin(CIS_LED_R_GPIO_Port, CIS_LED_R_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(CIS_LED_G_GPIO_Port, CIS_LED_G_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(CIS_LED_B_GPIO_Port, CIS_LED_B_Pin, GPIO_PIN_RESET);
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
	/* Timer Output Compare Configuration Structure declaration */
	TIM_OC_InitTypeDef sConfig;

	/* Counter Prescaler value */
	uint32_t uwPrescalerValue = 0;

	TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};
	TIM_MasterConfigTypeDef sMasterConfig = {0};

	/* Compute the prescaler value to have TIM1 counter clock equal to 20 MHz */
	uwPrescalerValue = (uint32_t)((SystemCoreClock / 2 / cis_clk_freq / 4) - 1); //cis_clk_freq


	htim1.Instance = TIM1;

	htim1.Init.Period        = 1;
	htim1.Init.Prescaler     = uwPrescalerValue;
	htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	htim1.Init.CounterMode   = TIM_COUNTERMODE_UP;
	htim1.Init.RepetitionCounter = 0;
	if(HAL_TIM_OC_Init(&htim1) != HAL_OK)
	{
		/* Initialization Error */
		Error_Handler();
	}

	/*##-2- Configure the Output Compare channels ##############################*/
	/* Common configuration for all channels */
	sConfig.OCMode     = TIM_OCMODE_TOGGLE;
	sConfig.OCPolarity = TIM_OCPOLARITY_HIGH;
	sConfig.OCFastMode = TIM_OCFAST_ENABLE;

	/* Output Compare Toggle Mode configuration: Channel1 */
	sConfig.Pulse = 1;
	if(HAL_TIM_OC_ConfigChannel(&htim1, &sConfig, TIM_CHANNEL_1) != HAL_OK)
	{
		/* Configuration Error */
		Error_Handler();
	}

	sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
	sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
	sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
	sBreakDeadTimeConfig.DeadTime = 0;
	sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
	sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
	sBreakDeadTimeConfig.BreakFilter = 0;
	sBreakDeadTimeConfig.Break2State = TIM_BREAK2_DISABLE;
	sBreakDeadTimeConfig.Break2Polarity = TIM_BREAK2POLARITY_HIGH;
	sBreakDeadTimeConfig.Break2Filter = 0;
	sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
	if (HAL_TIMEx_ConfigBreakDeadTime(&htim1, &sBreakDeadTimeConfig) != HAL_OK)
	{
		Error_Handler();
	}
	HAL_TIM_MspPostInit(&htim1);

	sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
	sMasterConfig.MasterOutputTrigger2 = TIM_TRGO2_RESET;
	sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
	if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
	{
		Error_Handler();
	}

	/*##-3- Start signals generation #######################################*/
	/* Start channel 1 in Output compare mode */
	if(HAL_TIM_OC_Start(&htim1, TIM_CHANNEL_1) != HAL_OK)
	{
		/* Starting Error */
		Error_Handler();
	}

	return 0;
}

void cisADC_Init(void)
{
	ADC_MultiModeTypeDef multimode = {0};
	ADC_ChannelConfTypeDef sConfig = {0};

	/** Common config
	 */
	hadc1.Instance = ADC1;
	hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV1;
	hadc1.Init.Resolution = ADC_RESOLUTION_8B;
	hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
	hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
	hadc1.Init.LowPowerAutoWait = DISABLE;
	hadc1.Init.ContinuousConvMode = DISABLE;
	hadc1.Init.NbrOfConversion = 1;
	hadc1.Init.DiscontinuousConvMode = ENABLE;
	hadc1.Init.ExternalTrigConv = ADC_EXTERNALTRIG_T1_CC1;
	hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_RISING;
	hadc1.Init.ConversionDataManagement = ADC_CONVERSIONDATA_DR;
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
	sConfig.Channel = ADC_CHANNEL_1;
	sConfig.Rank = ADC_REGULAR_RANK_1;
	sConfig.SamplingTime = ADC_SAMPLETIME_1CYCLE_5;
	sConfig.SingleDiff = ADC_SINGLE_ENDED;
	sConfig.OffsetNumber = ADC_OFFSET_NONE;
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

	/* ### Start conversion in IT mode ################################# */
	if (HAL_ADC_Start_IT(&hadc1) != HAL_OK)
	{
		Error_Handler();
	}
}

/**
 * @brief  End of conversion callback in non blocking mode
 * @param  hadc : hadc handle
 * @retval None
 */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
	//	if (hadc != &hadc1)
	//		return;
	HAL_GPIO_WritePin(ARD_D2_GPIO_Port, ARD_D2_Pin, GPIO_PIN_SET);
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
	static enum cisReadStep cis_step = READ_OFF;

	switch (callback_cnt)
	{
	case 0:
		HAL_GPIO_WritePin(CIS_SP_GPIO_Port, CIS_SP_Pin, GPIO_PIN_SET);
		break;
	case CIS_SP_OFF:
		HAL_GPIO_WritePin(CIS_SP_GPIO_Port, CIS_SP_Pin, GPIO_PIN_RESET);
		break;
	case CIS_LED_ON:
		HAL_GPIO_WritePin(CIS_LED_G_GPIO_Port, CIS_LED_G_Pin, GPIO_PIN_SET);
		break;
	case CIS_BLACK_PIX_AERA_START:
		cis_step = CAL_ZONE;
		break;
	case CIS_DEAD_ZONE_AERA_START:
		cis_adc_cal /= (CIS_BLACK_PIX_AERA_START - CIS_DEAD_ZONE_AERA_START);
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
	case CIS_PIXEX_AERA_START:
		cis_step = DATA_ZONE;
		break;
	case CIS_LED_RED_OFF:
		HAL_GPIO_WritePin(CIS_LED_G_GPIO_Port, CIS_LED_G_Pin, GPIO_PIN_RESET);
		break;
	case CIS_PIXEX_AERA_STOP:
		cis_step = READ_OFF;
		pixel_cnt = 0;
		cis_adc_cal = 0;
		break;
	case CIS_END_CAPTURE:
		calibration_state = CAL_OFF;
		callback_cnt = 0;
		return;
		break;
	}


	++callback_cnt;

	switch (cis_step)
	{
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
				if ((int32_t)cis_adc_offset[pixel_cnt] > (temp_data / PIXEL_PER_COMMA))
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
	HAL_GPIO_WritePin(ARD_D2_GPIO_Port, ARD_D2_Pin, GPIO_PIN_RESET);
}
