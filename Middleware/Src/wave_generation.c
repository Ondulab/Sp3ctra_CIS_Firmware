/*
 * wave_generation.c
 *
 *  Created on: 24 avr. 2019
 *      Author: zhonx
 */

/* Includes ------------------------------------------------------------------*/
#include <BSP_example.h>
#include "stm32h7xx_hal.h"
#include "config.h"

#include "stdio.h"
#include "stdlib.h"
#include "math.h"

#include "cis.h"
#include "wave_generation.h"

/* Private includes ----------------------------------------------------------*/

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

#define COMMA_PER_OCTAVE    ((SEMITONE_PER_OCTAVE) * (COMMA_PER_SEMITONE))

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/
static float calculate_frequency(uint32_t comma_cnt);

/* Private user code ---------------------------------------------------------*/

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
uint32_t init_waves(int16_t **unitary_waveform, struct wave *waves)
{
	uint32_t buffer_len = 0;
	uint32_t current_unitary_waveform_cell = 0;
	uint32_t note = 0;

	//compute cell number for storage all oscillators waveform
	for (uint32_t comma_cnt = 0; comma_cnt < COMMA_PER_OCTAVE; comma_cnt++)
	{
		//store only first octave_coeff frequencies ---- logarithmic distribution
		float frequency = calculate_frequency(comma_cnt);
		buffer_len += (uint32_t)(SAMPLING_FREQUENCY / frequency);
	}

	//allocate the contiguous memory area for storage all waveforms for the first octave_coeff
	*unitary_waveform = malloc(buffer_len * sizeof(uint16_t*));
	if (*unitary_waveform == NULL)
	{
		Error_Handler();
	}

	//compute and store the waveform into unitary_waveform only for the first octave_coeff
	for (uint32_t current_comma_first_octave = 0; current_comma_first_octave < COMMA_PER_OCTAVE; current_comma_first_octave++)
	{
		//compute frequency for each comma into the first octave_coeff
		float frequency = calculate_frequency(current_comma_first_octave);

		//current aera size is the number of char cell for storage a waveform at the current frequency (one pixel per frequency oscillator)
		uint32_t current_aera_size = (uint32_t)(SAMPLING_FREQUENCY / frequency);

		//fill unitary_waveform buffer with sinusoidal waveform for each comma
		for (uint32_t x = 0; x < current_aera_size; x++)
		{
			//sanity check
			if (current_unitary_waveform_cell < buffer_len)
			{
				(*unitary_waveform)[current_unitary_waveform_cell] = ((sin((x * 2.00 * PI )/ (double)current_aera_size))) * (WAVE_AMP_RESOLUTION / 2.00);
				current_unitary_waveform_cell++;
			}
		}

		//for each octave (only the first octave_coeff stay in RAM, for multiple octave_coeff start_ptr stay on first octave waveform but current_ptr jump cell according to multiple frequencies)
		for (uint32_t octave = 0; octave <= MAX_OCTAVE_NUMBER; octave++)
		{
			//compute the current pixel to associate an waveform pointer,
			// *** is current pix, --- octave separation
			// *---------*---------*---------*---------*---------*---------*---------*--------- for current comma at each octave
			// ---*---------*---------*---------*---------*---------*---------*---------*------ for the second comma...
			// ------*---------*---------*---------*---------*---------*---------*---------*---
			// ---------*---------*---------*---------*---------*---------*---------*---------*
			note = current_comma_first_octave + COMMA_PER_OCTAVE * octave;
			//sanity check, if user demand is't possible
			if (note < NUMBER_OF_NOTES)
			{
#ifdef PRINT_FREQUENCY
				//store frequencies
				waves[note].frequency = frequency * pow(2, octave);
#endif
				//store octave number
				waves[note].octave_coeff = pow(2, octave);
				//store aera size
				waves[note].aera_size = current_aera_size;
				//store pointer address
				waves[note].start_ptr = &(*unitary_waveform)[current_unitary_waveform_cell - current_aera_size];
				//set current pointer at the same address
				waves[note].current_idx = 0;
			}
		}
	}

	if (note < NUMBER_OF_NOTES)
	{
		printf("Configuration fail, current pix : %d\n", (int)note);
		Error_Handler();
	}

	return buffer_len;
}

