/*
 * synth.c
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

#include "tim.h"
#include "dac.h"
#include "cis.h"
#include "wave_generation.h"
#include "synth.h"

/* Private includes ----------------------------------------------------------*/

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static uint16_t *unitary_waveform = NULL;
static struct wave waves[CIS_PIXELS_NB];
__IO uint32_t summation_int_cnt = 0;

//#if defined ( __GNUC__ )
//ALIGN_32BYTES(uint32_t __attribute__((section (".RAM_D1"))) dac_buffer[DAC_BUFF_LEN]);
//#endif
//__IO uint32_t dac_buffer[DAC_BUFF_LEN] = {0};

/* Private function prototypes -----------------------------------------------*/
void setDacCarrier(int freq);

/* Private user code ---------------------------------------------------------*/

/**
 * @brief  synth init.
 * @param
 * @retval
 */
int32_t synth_init(void)
{
	uint32_t buffer_len = 0;

	buffer_len = init_waves(&unitary_waveform, waves);
	if (buffer_len < 0)
	{
		printf("RAM overflow");
		Error_Handler();
	}

	printf("Buffer lengh = %d\n", (int)buffer_len);

#ifdef PRINT_FREQUENCY
	for (uint32_t pix = 0; pix < CIS_PIXELS_NB; pix++)
	{
				printf("FREQ = %0.2f, SIZE = %d, OCTAVE = %d\n", waves[pix].frequency, (int)waves[pix].aera_size, (int)waves[pix].octave_coeff);
				HAL_Delay(20);
//		uint16_t output = 0;
//		for (uint32_t idx = 0; idx < (waves[pix].aera_size / waves[pix].octave_coeff); idx++)
//		{
//			output = *(waves[pix].start_ptr + (idx *  waves[pix].octave_coeff));
//			printf("%d\n", output);
//		}
//				HAL_Delay(1);
	}
	printf("---- END ----");
	while(1);
#endif

	setDacCarrier(DAC_TIME_FREQ);

	/*##-2- Enable DAC selected channel and associated DMA #############################*/
//	if (HAL_DAC_Start_DMA(&hdac1, DAC_CHANNEL_1, (uint32_t*)dac_buffer, DAC_BUFF_LEN, DAC_ALIGN_12B_L) != HAL_OK)
//	{
//		/* Start DMA Error */
//		Error_Handler();
//	}

	HAL_DAC_Start(&hdac1, DAC_CHANNEL_1);

	return 0;
}

/**
 * @brief  Set DAC read frequency
 * @param  freq
 * @retval None
 */
void setDacCarrier(int freq)
{
	uint32_t uwPrescalerValue = 0;

	/* Compute the prescaler value */
	uwPrescalerValue = (uint32_t) ((SystemCoreClock / 2) / (freq) * 100);

	HAL_TIM_Base_DeInit(&htim6);

	/*##-1- Configure the TIM peripheral #######################################*/
	/* Time base configuration */
	htim6.Instance = TIM6;
	htim6.Init.Period            = 100 - 1;
	htim6.Init.Prescaler         = uwPrescalerValue;
	htim6.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
	htim6.Init.CounterMode       = TIM_COUNTERMODE_UP;
	htim6.Init.RepetitionCounter = 0;
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
#pragma GCC push_options
#pragma GCC optimize ("unroll-loops")
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	uint64_t signal_summation = 0;
	uint64_t signal_power_summation = 0;
	uint32_t new_idx;
	uint32_t max_power = 0;

	//	if (htim != &htim15)
	//		return;

	//Summation for all pixel
	for (int32_t pix = CIS_PIXELS_NB; --pix >= 0;)
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


			//read current value of current cell and sum
			signal_summation += (*(waves[pix].start_ptr + waves[pix].current_idx) * cis_adc_data[pix]) >> 16;

			//read equivalent power of current pixel
			signal_power_summation += cis_adc_data[pix];
			if (cis_adc_data[pix] > max_power)
				max_power = cis_adc_data[pix];
		}
	}

	HAL_DAC_SetValue(&hdac1, DAC_CHANNEL_1, DAC_ALIGN_12B_L, (signal_summation * max_power) / signal_power_summation);

	++summation_int_cnt;
}
#pragma GCC pop_options
