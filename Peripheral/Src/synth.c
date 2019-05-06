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
			output = *(waves[pix].start_ptr + (idx * (uint32_t)pow(2, waves[pix].octave)));
			printf("%d\n", output);
		}
	}
#endif

	return 0;
}
