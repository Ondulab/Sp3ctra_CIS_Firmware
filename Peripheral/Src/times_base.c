/*
 * times_base.c
 *
 *  Created on: 24 avr. 2019
 *      Author: zhonx
 */

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"
#include "main.h"
#include "config.h"

#include "stdlib.h"
#include "stdio.h"
#include "math.h"

#include "hrtim.h"
#include "tim.h"

#include "cis.h"
#include "times_base.h"

/* Private includes ----------------------------------------------------------*/

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/* Private user code ---------------------------------------------------------*/

/**
 * @brief  Initialise all timers
 * @param  void
 * @retval Error code
 */
int32_t timesBaseInit(void)
{
	TIM_MasterConfigTypeDef sMasterConfig;
	TIM_OC_InitTypeDef sConfigOC;

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

	/* Compute the prescaler value */
	uwPrescalerValue = (uint32_t) ((SystemCoreClock / 2) / (SAMPLING_FREQUENCY * 2));

	htim15.Instance = TIM15;
	htim15.Init.Prescaler = 0;
	htim15.Init.CounterMode = TIM_COUNTERMODE_UP;
	htim15.Init.Period = uwPrescalerValue;
	htim15.Init.ClockDivision = TIM_CLOCKDIVISION_DIV4;
	htim15.Init.RepetitionCounter = 0;
	htim15.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
	if (HAL_TIM_OC_Init(&htim15) != HAL_OK)
	{
		return -1;
	}

	sConfigOC.OCMode = TIM_OCMODE_TOGGLE;
	sConfigOC.Pulse = uwPrescalerValue / 2;
	sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
	sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
	sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
	sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
	sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
	if (HAL_TIM_OC_ConfigChannel(&htim15, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
	{
		return -2;
	}

	//	//	/* Start channel 1 in Output compare mode */
	//	if(HAL_TIM_OC_Start_IT(&htim15, TIM_CHANNEL_1) != HAL_OK)
	//	{
	//		/* Starting Error */
	//		Error_Handler();
	//	}
	//
	//	HAL_TIM_Base_Start_IT(&htim15);

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

	//		/*##-7- Start PWM signals generation ########################################################*/
	//		if (HAL_HRTIM_WaveformOutputStart(&hhrtim, HRTIM_OUTPUT_TA1) != HAL_OK)
	//		{
	//			/* PWM Generation Error */
	//			Error_Handler();
	//		}
	//
	//		/*##-7- Start PWM signals generation ########################################################*/
	//		if (HAL_HRTIM_WaveformOutputStart(&hhrtim, HRTIM_OUTPUT_TA2) != HAL_OK)
	//		{
	//			/* PWM Generation Error */
	//			Error_Handler();
	//		}

	//		/*##-7- Start PWM signals generation ########################################################*/
	//		if (HAL_HRTIM_SimplePWMStart(&hhrtim, HRTIM_TIMERINDEX_TIMER_A, HRTIM_OUTPUT_TA2 ) != HAL_OK)
	//		{
	//			/* PWM Generation Error */
	//			Error_Handler();
	//		}
	//
	/*##-7- Start PWM signals generation ########################################################*/
	if (HAL_HRTIM_SimpleOCStart(&hhrtim, HRTIM_TIMERINDEX_TIMER_A, HRTIM_OUTPUT_TA1) != HAL_OK)
	{
		/* PWM Generation Error */
		return -3;
	}

	/*##-7- Start PWM signals generation ########################################################*/
	if (HAL_HRTIM_SimplePWMStart(&hhrtim, HRTIM_TIMERINDEX_TIMER_B, HRTIM_OUTPUT_TB1) != HAL_OK)
	{
		/* PWM Generation Error */
		return -4;
	}

	//		/*##-8- Start HRTIM counter ###################################################################*/
	//		if (HAL_HRTIM_WaveformCounterStart(&hhrtim, HRTIM_TIMERID_MASTER + HRTIM_TIMERID_TIMER_A) != HAL_OK)
	//		{
	//			/* PWM Generation Error */
	//			Error_Handler();
	//		}

	//		if (HAL_HRTIM_SimpleBaseStart(&hhrtim, HRTIM_TIMERID_MASTER + HRTIM_TIMERID_TIMER_A) != HAL_OK)
	//		{
	//			/* PWM Generation Error */
	//			Error_Handler();
	//		}
	return 0;
}
