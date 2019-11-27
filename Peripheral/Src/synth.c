/*
 * synth.c
 *
 *  Created on: 24 avr. 2019
 *      Author: zhonx
 */

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"
//#include "stm32h7xx_ll_dac.h"
#include "main.h"
#include "config.h"
#include "tim.h"
#include "dac.h"

#include "stdlib.h"
#include "stdio.h"

//#include "arm_math.h"

#include "cis.h"
#include "wave_generation.h"
#include "synth.h"

/* Private includes ----------------------------------------------------------*/

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static __IO uint16_t *unitary_waveform = NULL;
static struct wave waves[NUMBER_OF_NOTES];
__IO uint32_t rfft_cnt = 0;

/* Private function prototypes -----------------------------------------------*/
int32_t initDacTimer(uint32_t freq);
int32_t initSamplingTimer(uint32_t sampling_freq);

/* Private user code ---------------------------------------------------------*/

/**
 * @brief  synth init.
 * @param
 * @retval Error
 */
int32_t synth_init(void)
{
	uint32_t buffer_len = 0;

	buffer_len = init_waves(&unitary_waveform, waves);
	if (buffer_len < 0)
	{
		printf("RAM overflow");
		return -1;
	}

	printf("Buffer lengh = %d\n", (int)buffer_len);

#ifdef PRINT_FREQUENCY
	printf("FREQ = %0.2f, SIZE = %d, OCTAVE = %d\n", waves[0].frequency, (int)waves[0].aera_size, (int)waves[0].octave_coeff);
	printf("FREQ = %0.2f, SIZE = %d, OCTAVE = %d\n", waves[NUMBER_OF_NOTES - 1].frequency, (int)waves[NUMBER_OF_NOTES - 1].aera_size, (int)waves[NUMBER_OF_NOTES - 1].octave_coeff);

//	for (uint32_t pix = 0; pix < NUMBER_OF_NOTES; pix++)
//	{
//		printf("FREQ = %0.2f, SIZE = %d, OCTAVE = %d\n", waves[pix].frequency, (int)waves[pix].aera_size, (int)waves[pix].octave_coeff);
//		HAL_Delay(10);
//		//		uint16_t output = 0;
//		//		for (uint32_t idx = 0; idx < (waves[pix].aera_size / waves[pix].octave_coeff); idx++)
//		//		{
//		//			output = *(waves[pix].start_ptr + (idx *  waves[pix].octave_coeff));
//		//			printf("%d\n", output);
//		//		}
//		//				HAL_Delay(1);
//	}
//	printf("---- END ----");
//	while(1);
#endif

	if (initDacTimer(DAC_TIME_FREQ) != 0)
	{
		Error_Handler();
	}
	if (initSamplingTimer(SAMPLING_FREQUENCY) != 0)
	{
		Error_Handler();
	}

	if (HAL_DAC_Start(&hdac1, DAC_CHANNEL_2) != HAL_OK)
	{
		Error_Handler();
	}

//	HAL_DAC_SetValue(&hdac1, DAC_CHANNEL_2, DAC_ALIGN_12B_R, 1400);//1489 1,2V
//	if (HAL_DAC_Start(&hdac1, DAC_CHANNEL_2) != HAL_OK)
//	{
//		Error_Handler();
//	}
	return 0;
}

/**
 * @brief  Init and set DAC frequency
 * @param  freq
 * @retval Error
 */
int32_t initDacTimer(uint32_t freq)
{
	TIM_MasterConfigTypeDef sMasterConfig;
	uint32_t uwPrescalerValue = 0;

	/* Compute the prescaler value */
	uwPrescalerValue = (uint32_t) ((SystemCoreClock / 2) / (freq * 10));

	/*##-1- Configure the TIM peripheral #######################################*/
	/* Time base configuration */
	htim6.Instance = TIM6;
	htim6.Init.Period            = 10 - 1;
	htim6.Init.Prescaler         = uwPrescalerValue;
	htim6.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
	htim6.Init.CounterMode       = TIM_COUNTERMODE_UP;
	htim6.Init.RepetitionCounter = 0;
	if (HAL_TIM_Base_Init(&htim6) != HAL_OK)
	{
		Error_Handler();
	}

	/* TIM6 TRGO selection */
	sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE;
	sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
	if (HAL_TIMEx_MasterConfigSynchronization(&htim6, &sMasterConfig) != HAL_OK)
	{
		Error_Handler();
	}

	/*##-2- Enable TIM peripheral counter ######################################*/
	if (HAL_TIM_Base_Start(&htim6) != HAL_OK)
	{
		Error_Handler();
	}
	return 0;
}

/**
 * @brief  Init sampling frequency timer and set it
 * @param  sampling_frequency
 * @retval Error
 */
int32_t initSamplingTimer(uint32_t sampling_freq)
{
	TIM_OC_InitTypeDef sConfigOC;

	uint32_t uwPrescalerValue = 0;

	/* Compute the prescaler value */
	uwPrescalerValue = (uint32_t) ((SystemCoreClock / 2) / (sampling_freq));

	htim15.Instance = TIM15;
	htim15.Init.Prescaler = 0;
	htim15.Init.CounterMode = TIM_COUNTERMODE_UP;
	htim15.Init.Period = uwPrescalerValue;
	htim15.Init.ClockDivision = TIM_CLOCKDIVISION_DIV4;
	htim15.Init.RepetitionCounter = 0;
	htim15.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
	if (HAL_TIM_OC_Init(&htim15) != HAL_OK)
	{
		Error_Handler();
	}

	sConfigOC.OCMode = TIM_OCMODE_TOGGLE;
	sConfigOC.Pulse = uwPrescalerValue;
	sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
	sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
	sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
	sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
	sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
	if (HAL_TIM_OC_ConfigChannel(&htim15, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
	{
		Error_Handler();
	}

	//	/* Start channel 1 in Output compare mode */
	if (HAL_TIM_OC_Start_IT(&htim15, TIM_CHANNEL_1) != HAL_OK)
	{
		Error_Handler();
	}

#ifndef DEBUG_SAMPLE_RATE
	if (HAL_TIM_Base_Start_IT(&htim15) != HAL_OK)
	{
		/* Starting Error */
		return -4;
	}
#endif
	return 0;
}

/**
 * @brief  Period elapsed callback in non blocking mode
 * @param  htim : TIM handle
 * @retval None
 */
#pragma GCC push_options
#pragma GCC optimize ("unroll-loops")
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	uint32_t signal_summation = 0;
	uint32_t signal_power_summation = 0;
	uint32_t new_idx;
	uint32_t max_power = 0;

	//	if (htim != &htim15)
	//		return;

	//Summation for all pixel
	for (int32_t pix = NUMBER_OF_NOTES; --pix >= 0;)
	{
		//test for CIS presence
		if (cis_adc_data[pix] > SENSIVITY_THRESHOLD)
		{
			//octave_coeff jump current pointer into the fundamental waveform, for example : the 3th octave increment the current pointer 8 per 8 (2^3)
			//example for 17 cell waveform and 3th octave : [X][Y][Z][X][Y][Z][X][Y][Z][X][Y][[Z][X][Y][[Z][X][Y], X for the first pass, Y for second etc...
			new_idx = (waves[pix].current_idx + waves[pix].octave_coeff);
			if (new_idx > waves[pix].aera_size)
				new_idx -= waves[pix].aera_size;

			waves[pix].current_idx = new_idx;

			signal_summation += (*(waves[pix].start_ptr + waves[pix].current_idx) * cis_adc_data[pix]) >> 16;

			//read equivalent power of current pixel
			signal_power_summation += (cis_adc_data[pix]);
			if (cis_adc_data[pix] > max_power)
				max_power = cis_adc_data[pix];
		}
	}
	HAL_DAC_SetValue(&hdac1, DAC_CHANNEL_2, DAC_ALIGN_12B_R, (uint32_t)(signal_summation * ((double)max_power / signal_power_summation)) >> 4);
//	printf ("%d\n",(uint32_t)(signal_summation * ((double)max_power / signal_power_summation)) >> 4);

	++rfft_cnt;
}
#pragma GCC pop_options
