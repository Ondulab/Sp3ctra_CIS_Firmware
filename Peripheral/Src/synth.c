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
__IO uint32_t left_output = 0;
__IO uint32_t right_output = 0;

/* Private function prototypes -----------------------------------------------*/

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

#ifdef DEBUG
	for (uint32_t pix = 0; pix < CIS_PIXELS_NB; pix++)
	{
		//		printf("FREQ = %0.2f, SIZE = %d, OCTAVE = %d\n", waves[pix].frequency, (int)waves[pix].aera_size, (int)waves[pix].octave);
		//		HAL_Delay(20);
		uint16_t output = 0;
		for (uint32_t idx = 0; idx < waves[pix].aera_size; idx++)
		{
			output = *(waves[pix].start_ptr + (idx * (uint32_t)pow(2, waves[pix].octave_coeff)));
			printf("%d\n", output);
		}
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
	static uint16_t pix = 0;
	static uint32_t signal_summation = 0;
	static uint32_t signal_power_summation = 0;

	//	if (htim != &htim15)
	//		return;

	signal_summation = 0;
	signal_power_summation = 0;
	//Summation for all pixel
	for (pix = 0; pix < 2700; pix+=6) //CIS_PIXELS_NB
	{
		//test for CIS presence
//		if (cis_adc_data[pix] > SENSIVITY_THRESHOLD)
//		{
			//octave_coeff jump current pointer into the fundamental waveform, for example : the 3th octave increment the current pointer 8 per 8 (2^3)
			//modulo example for 17 cell waveform and 3th octave : [X][Y][Z][X][Y][Z][X][Y][Z][X][Y][[Z][X][Y][[Z][X][Y], X for the first pass, Y for second etc...
			waves[pix].current_idx = (waves[pix].current_idx + waves[pix].octave_coeff) % waves[pix].aera_size;

			//read current value of current cell and sum
			signal_summation += (*(waves[pix].start_ptr + waves[pix].current_idx) * cis_adc_data[pix]) / CIS_RESOLUTION;

			//read equivalent power of current pixel
			signal_power_summation += cis_adc_data[pix];
//		}
	}

	left_output = signal_summation / (signal_power_summation / CIS_RESOLUTION);

	summation_int_cnt++;
}
#pragma GCC pop_options
