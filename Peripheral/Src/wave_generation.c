/*
 * wave_generation.c
 *
 *  Created on: 24 avr. 2019
 *      Author: zhonx
 */

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"
#include "main.h"

#include "stdio.h"
#include "stdlib.h"
#include "math.h"

#include "cis.h"
#include "wave_generation.h"

/* Private includes ----------------------------------------------------------*/

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/
#define SAMPLING_FREQUENCY 	(192000)
#define WAVE_AMP_RESOLUTION (65535)
#define START_FREQUENCY     (43.654)  //FA 1
#define OCTAVE_NUMBER       (9)
#define SEMITONE_PER_OCTAVE (12)
#define COMMA_PER_SEMITONE  (4.5)
#define PIXEL_PER_COMMA     (6)

#define COMMA_PER_OCTAVE    ((SEMITONE_PER_OCTAVE) * (COMMA_PER_SEMITONE))
#define PIXEL_PER_OCTAVE    ((PIXEL_PER_COMMA) * (COMMA_PER_OCTAVE))

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/
static float calculate_frequency(uint32_t comma_cnt);

/* Private user code ---------------------------------------------------------*/

//	octave_number = ((12 / log(2)) * log(END_FREQUENCY / START_FREQUENCY) / 12);

/**
 * @brief  calculate frequency,
 * @param  comma cnt
 * @retval frequency
 */
static float calculate_frequency(uint32_t comma_cnt)
{
	float frequency = 0.0;
	frequency =  START_FREQUENCY * pow(2, (comma_cnt / (12.0 * (COMMA_PER_OCTAVE / (12.0 / (log(2)) * log((START_FREQUENCY * 2.0) / START_FREQUENCY))))));

	return frequency;
}

/**
 * @brief  build_waves,
 * @param  unitary_waveform pointer,
 * @param  waves structure pointer,
 * @retval buffer length on success, negative value otherwise
 */
uint32_t init_waves(__IO uint16_t **unitary_waveform, __IO struct wave *waves)
{
	uint32_t buffer_len = 0;
	float frequency = 0.0;
	uint32_t current_aera_size = 0;

	//compute cell number for storage all oscillators waveform
	for (uint32_t comma_cnt = 0; comma_cnt < COMMA_PER_OCTAVE; comma_cnt++)
	{
		//store only first octave frequencies ---- logarithmic distribution
		frequency = calculate_frequency(comma_cnt);
		buffer_len += (uint32_t)(SAMPLING_FREQUENCY / frequency);
	}

	//allocate the contiguous memory area for storage all waveforms for the first octave
	*unitary_waveform = malloc(buffer_len * sizeof(uint16_t*));
	if (unitary_waveform == NULL)
	{
		return -1;
	}

	//fill buffer with sinusoidal patterns for the first octave
	for (uint32_t comma_cnt = 0; comma_cnt < COMMA_PER_OCTAVE; comma_cnt++)
	{
		//store only first octave frequencies ---- logarithmic distribution
		frequency = calculate_frequency(comma_cnt);

		//current aera size is the number of char cell for storage a waveform at the current frequency (one pixel per frequency oscillator)
		current_aera_size = (uint32_t)(SAMPLING_FREQUENCY / frequency);

		//for each octave
		for (uint32_t octave = 0; octave < OCTAVE_NUMBER; octave++)
		{
			//duplicate for each pixel with same frequency value
			for (uint32_t idx = 0; idx < PIXEL_PER_COMMA; idx++)
			{
				uint32_t pix = (comma_cnt * PIXEL_PER_COMMA + idx) + (PIXEL_PER_OCTAVE * octave);
				if (pix < CIS_PIXELS_NB)
				{
					//store frequencies
					waves[pix].frequency = frequency * pow(2, octave);
					//store octave number
					waves[pix].octave = octave;
					//store aera size
					waves[pix].aera_size = current_aera_size / pow(2, octave);
					//store pointer address
					waves[pix].start_ptr = unitary_waveform[comma_cnt];
					//set current pointer at the same address
					waves[pix].current_ptr = unitary_waveform[comma_cnt];
				}
			}
		}

		for (uint32_t x = 0; x < current_aera_size; x++)
		{
			(*unitary_waveform)[comma_cnt] = (sin((x * 2.00 * M_PI) / current_aera_size) + 1.00) * WAVE_AMP_RESOLUTION / 2;
		}
	}

	//print all buffer for debug (you can see the waveform with serial tracer on arduino ide)
	//	for (uint32_t i = 0; i < buffer_len; i++)
	//	{
	//		printf("%d\n", (*unitary_waveform)[i]);
	//		HAL_Delay(1);
	//	}

	//print all structure table for debug
	uint16_t output = 0;
	for (uint32_t pix = 0; pix < CIS_PIXELS_NB; pix++)
	{
		printf("FREQ = %0.2f, SIZE = %d, OCTAVE = %d\n", waves[pix].frequency, (int)waves[pix].aera_size, (int)waves[pix].octave);
		HAL_Delay(20);
		//		for (uint32_t idx = 0; idx < waves[pix].aera_size; idx++)
		//		{
		//			output = *(waves[pix].start_ptr + idx);
		//			printf("%d\n", output);
		//		}
	}

	return buffer_len;
}

