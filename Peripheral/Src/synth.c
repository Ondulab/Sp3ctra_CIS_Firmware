/*
 * synth.c
 *
 *  Created on: 24 avr. 2019
 *      Author: zhonx
 */

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"
#include "main.h"

#include "stdio.h"
#include "stdlib.h"

#include "cis.h"
#include "wave_generation.h"
#include "synth.h"

/* Private includes ----------------------------------------------------------*/

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static __IO uint16_t *unitary_waveform = NULL;
static struct wave waves[CIS_PIXELS_NB];

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

//	buffer_len = calculate_wave_buffer_len();

//	//allocate the contiguous memory area for storage all waveforms
//	unitary_waveform = (uint16_t*)malloc(buffer_len * sizeof(uint16_t));
//	if (unitary_waveform == NULL)
//	{
//		return -1;
//	}

	if (init_waves(&unitary_waveform, waves) != SUCCESS)
	{
		Error_Handler();
	}

	printf("Buffer lengh = %d\n", (int)buffer_len);

	//print all buffer for debug (you can see the waveform with serial tracer on arduino ide)
//	for (uint32_t i = 0; i < buffer_len; i++)
//	{
//		printf("%d\n", *(unitary_waveform + i));
//		HAL_Delay(1);
//	}

	//print all structure table for debug
	uint16_t output = 0;
	for (uint32_t band = 0; band < CIS_PIXELS_NB; band++)
	{
		printf("---------- FREQUENCY = %d, BUFF LENGH = %d ----------\n", waves[band].frequency, waves[band].aera_size);
		HAL_Delay(5);
//		for (uint32_t idx = 0; idx < waves[band].aera_size; idx++)
//		{
//			output = *(waves[band].start_ptr + idx);
//			printf("%d\n", output);
//		}
	}

	while(1);

	cisInit();

	return 0;
}
