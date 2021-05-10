/*
 * synth.c
 *
 *  Created on: 24 avr. 2019
 *      Author: zhonx
 */

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"
#include "config.h"
#include "tim.h"
#include "rng.h"
#include "usart.h"

#include "basetypes.h"
#include "arm_math.h"
#include "stdlib.h"
#include "stdio.h"

#include "cis.h"
#include "wave_generation.h"
#include "synth.h"
#include "ssd1362.h"
#include "menu.h"
#include "pcm5102.h"

/* Private includes ----------------------------------------------------------*/

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private typedef -----------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static int16_t *unitary_waveform = NULL;
static struct wave waves[NUMBER_OF_NOTES];
volatile uint32_t synth_process_cnt = 0;
static int16_t* half_audio_ptr;
static int16_t* full_audio_ptr;

/* Variable containing black and white frame from CIS*/
static int32_t *imageData = NULL;
//static uint16_t imageData[((CIS_END_CAPTURE * CIS_ADC_OUT_LINES) / CIS_IFFT_OVERSAMPLING_RATIO) - 1]; // for debug

/* Private function prototypes -----------------------------------------------*/
static void synth_IfftMode(int32_t *imageData, int16_t *audioData, uint32_t NbrOfData);

/* Private user code ---------------------------------------------------------*/

/**
 * @brief  synth ifft init.
 * @param
 * @retval Error
 */
int32_t synth_IfftInit(void)
{
	int32_t buffer_len = 0;
	uint32_t aRandom32bit = 0;

	printf("---------- SYNTH INIT ---------\n");
	printf("-------------------------------\n");

	//allocate the contiguous memory area for storage image data
	imageData = malloc(NUMBER_OF_NOTES * sizeof(int32_t*));
	if (imageData == NULL)
	{
		Error_Handler();
	}

	memset(imageData, 0, NUMBER_OF_NOTES * sizeof(int32_t*));

	buffer_len = init_waves(&unitary_waveform, waves);

	// start with random index
	for (uint32_t i = 0; i < NUMBER_OF_NOTES; i++)
	{
		if (HAL_RNG_GenerateRandomNumber(&hrng, &aRandom32bit) != HAL_OK)
		{
			/* Random number generation error */
			Error_Handler();
		}
		waves[i].current_idx = aRandom32bit % waves[i].aera_size;
		waves[i].current_volume = 0;
	}

	if (buffer_len < 0)
	{
		printf("RAM overflow");
		return -1;
	}

	printf("Note number  = %d\n", (int)NUMBER_OF_NOTES);
	printf("Buffer lengh = %d uint16\n", (int)buffer_len);


	uint8_t FreqStr[256] = {0};
	ssd1362_drawRect(0, 57, 256, 64, 8, false);
	sprintf((char *)FreqStr, " %d -> %dHz      Octave:%d", (int)waves[0].frequency, (int)waves[NUMBER_OF_NOTES - 1].frequency, (int)sqrt(waves[NUMBER_OF_NOTES - 1].octave_coeff));
	ssd1362_drawString(0, 57, (int8_t*)FreqStr, 0, 8);

	printf("First note Freq = %dHz\nSize = %d\n", (int)waves[0].frequency, (int)waves[0].aera_size);
	printf("Last  note Freq = %dHz\nSize = %d\nOctave = %d\n", (int)waves[NUMBER_OF_NOTES - 1].frequency, (int)waves[NUMBER_OF_NOTES - 1].aera_size / (int)sqrt(waves[NUMBER_OF_NOTES - 1].octave_coeff), (int)sqrt(waves[NUMBER_OF_NOTES - 1].octave_coeff));

	printf("-------------------------------\n");

#ifdef PRINT_IFFT_FREQUENCY
	for (uint32_t pix = 0; pix < NUMBER_OF_NOTES; pix++)
	{
		printf("FREQ = %0.2f, SIZE = %d, OCTAVE = %d\n", waves[pix].frequency, (int)waves[pix].aera_size, (int)waves[pix].octave_coeff);
#ifdef PRINT_IFFT_FREQUENCY_FULL
							uint16_t output = 0;
							for (uint32_t idx = 0; idx < (waves[pix].aera_size / waves[pix].octave_coeff); idx++)
							{
								output = *(waves[pix].start_ptr + (idx *  waves[pix].octave_coeff));
								printf("%d\n", output);
							}
#endif
	}
	printf("-------------------------------\n");
#endif

	pcm5102_Init();
	half_audio_ptr = pcm5102_GetDataPtr(0);
	full_audio_ptr = pcm5102_GetDataPtr(AUDIO_BUFFER_SIZE / 2);

	return 0;
}

/**
 * @brief  Get Image buffer data
 * @param  Index
 * @retval Value
 */
int32_t synth_GetImageData(uint32_t index)
{
	//	if (index >= RFFT_BUFFER_SIZE)
	//		Error_Handler();
	return imageData[index];
}

/**
 * @brief  Set Image buffer data
 * @param  Index
 * @retval Value
 */
int32_t synth_SetImageData(uint32_t index, int32_t value)
{
	//	if (index >= RFFT_BUFFER_SIZE)
	//		Error_Handler();
	imageData[index] = value;
	return 0;
}

/**
 * @brief  Period elapsed callback in non blocking mode
 * @param  htim : TIM handle
 * @retval None
 */
#pragma GCC push_options
#pragma GCC optimize ("unroll-loops")
void synth_IfftMode(int32_t *imageData, int16_t *audioData, uint32_t NbrOfData)
{
	static int32_t signal_summation_R;
	static int32_t signal_summation_L;
	static uint32_t signal_power_summation;
	static int16_t rfft_R;
	static int16_t rfft_L;
	static uint16_t new_idx;
	static uint32_t write_data_nbr;
	static int32_t note;
	static int32_t max_volume;
	static int32_t current_image_data;

//	__disable_irq();

	write_data_nbr = 0;

	while(write_data_nbr < NbrOfData)
	{
		signal_summation_R = 0;
		signal_summation_L = 0;
		signal_power_summation = 0;
		max_volume = 0;

		//Summation for all pixel
		for (note = NUMBER_OF_NOTES; --note >= 1;)
		{
			//octave_coeff jump current pointer into the fundamental waveform, for example : the 3th octave increment the current pointer 8 per 8 (2^3)
			//example for 17 cell waveform and 3th octave : [X][Y][Z][X][Y][Z][X][Y][Z][X][Y][[Z][X][Y][[Z][X][Y], X for the first pass, Y for second etc...
			new_idx = (waves[note].current_idx + waves[note].octave_coeff);
			if (new_idx >= waves[note].aera_size)
				new_idx -= waves[note].aera_size;

			if (imageData[note - 1] - imageData[note] > 0)
				current_image_data = imageData[note - 1] - imageData[note];
			else
				current_image_data = 0;//imageData[note] - imageData[note - 1];

			if (waves[note].current_volume < current_image_data)
			{
				waves[note].current_volume += IFFT_GAP_PER_MS / (SAMPLING_FREQUENCY / 1000);
				if (waves[note].current_volume > current_image_data)
					waves[note].current_volume = current_image_data;
			}
			else
			{
				waves[note].current_volume -= IFFT_GAP_PER_MS / (SAMPLING_FREQUENCY / 1000);
				if (waves[note].current_volume < current_image_data)
					waves[note].current_volume = current_image_data;
			}

			if (waves[note].current_volume > max_volume)
				max_volume = waves[note].current_volume;

			//current audio point summation
			signal_summation_R += ((*(waves[note].start_ptr + new_idx)) * waves[note].current_volume) >> 16;
			signal_summation_L += ((*(waves[note - 1].start_ptr + new_idx)) * waves[note - 1].current_volume) >> 16;

			//read equivalent power of current pixel
			signal_power_summation += waves[note].current_volume;

			waves[note].current_idx = new_idx;
		}

		rfft_R = (signal_summation_R * ((double)max_volume) / (double)signal_power_summation);
		rfft_L = (signal_summation_L * ((double)max_volume) / (double)signal_power_summation);

#ifdef STEREO_1
		audioData[write_data_nbr] = rfft_L;		//L
#else
		audioData[write_data_nbr] = rfft_R;
#endif
		audioData[write_data_nbr + 1] = rfft_R;	//R
		write_data_nbr += 2;
	}

	synth_process_cnt += NbrOfData / 2;
//	__enable_irq();
}
#pragma GCC pop_options

/**
 * @brief  Manages Audio process.
 * @param  None
 * @retval Audio error
 *
 *                   |------------------------------|------------------------------|
 *                   |half rfft buffer to audio buff|                              |
 * audio buffer      |------------FILL--------------|-------------PLAY-------------|
 *                   |                              |                              |
 *                   |                              |     fill half rfft buffer    |
 *                   |                              |                              |
 *                   |------------------------------|------------------------------|
 *                                                  ^
 *                                                HALF
 *                                              COMPLETE
 *
 *                   |------------------------------|------------------------------|
 *                   |                              |full rfft buffer to audio buff|
 * audio buffer      |-------------PLAY-------------|-------------FILL-------------|
 *                   |                              |                              |
 *                   |     fill full rfft buffer    |                              |
 *                   |                              |                              |
 *                   |------------------------------|------------------------------|
 *                                                                                 ^
 *                                                                                FULL
 *                                                                              COMPLETE
 */
void synth_AudioProcess(synthModeTypeDef mode)
{
	/* 1st half buffer played; so fill it and continue playing from bottom*/
	if(*pcm5102_GetBufferState() == AUDIO_BUFFER_OFFSET_HALF)
	{
		pcm5102_ResetBufferState();
		cis_ImageProcessBW(imageData);
		synth_IfftMode(imageData, half_audio_ptr, AUDIO_BUFFER_SIZE / 2);
		SCB_CleanDCache_by_Addr((uint32_t *)half_audio_ptr, AUDIO_BUFFER_SIZE);
	}

	/* 2nd half buffer played; so fill it and continue playing from top */
	if(*pcm5102_GetBufferState() == AUDIO_BUFFER_OFFSET_FULL)
	{
		pcm5102_ResetBufferState();
		cis_ImageProcessBW(imageData);
		synth_IfftMode(imageData, full_audio_ptr, AUDIO_BUFFER_SIZE / 2);
		SCB_CleanDCache_by_Addr((uint32_t *)full_audio_ptr, AUDIO_BUFFER_SIZE);
	}
}

/**
 * @brief  synth ifft test without CIS
 * @param  void
 * @retval void
 */
void synth_Test(void)
{
	uint8_t FreqStr[256] = {0};
	uint32_t cis_color = 0;

	printf("Start BW ifft test mode \n");

	pcm5102_Init();
	synth_IfftInit();

	/* Infinite loop */
	static uint32_t start_tick;
	uint32_t latency;
	int32_t i = 0;

	while (1)
	{
		start_tick = HAL_GetTick();
		while ((synth_process_cnt) < (SAMPLING_FREQUENCY / DISPLAY_REFRESH_FPS))
		{
			synth_AudioProcess(IFFT_MODE);
		}

		static uint32_t note = 0;
		if (note > NUMBER_OF_NOTES)
		{
			note = 0;
		}

		synth_SetImageData(++note, 10000); //for testing
		synth_SetImageData(note - 1, 0);

		//	synth_SetImageData(20, 1000); //for testing
		//	synth_SetImageData(85, 5700);
		//	synth_SetImageData(120, 1000); //for testing
		//	synth_SetImageData(185, 5700);
		//	synth_SetImageData(60, 100); //for testing
		//	synth_SetImageData(105, 5700);

		latency = HAL_GetTick() - start_tick;
		sprintf((char *)FreqStr, "%dHz", (int)((synth_process_cnt * 1000) / latency));
		synth_process_cnt = 0;

		ssd1362_drawRect(0, DISPLAY_AERA1_Y1POS, DISPLAY_MAX_X_LENGTH / 2 - 1, DISPLAY_AERA1_Y2POS, 3, false);
		ssd1362_drawRect(DISPLAY_MAX_X_LENGTH / 2 + 1, DISPLAY_AERA1_Y1POS, DISPLAY_MAX_X_LENGTH, DISPLAY_AERA1_Y2POS, 4, false);
		ssd1362_drawRect(0, DISPLAY_AERA2_Y1POS, DISPLAY_MAX_X_LENGTH, DISPLAY_AERA2_Y2POS, 3, false);
		ssd1362_drawRect(0, DISPLAY_AERA3_Y1POS, DISPLAY_MAX_X_LENGTH, DISPLAY_AERA3_Y2POS, 8, false);

		for (i = 0; i < ((DISPLAY_MAX_X_LENGTH / 2) - 1); i++)
		{
			ssd1362_drawPixel(i, DISPLAY_AERA1_Y1POS + (DISPLAY_AERAS1_HEIGHT / 2) + (pcm5102_GetAudioData(i * 2) / 4096) - 1, 10, false);
			ssd1362_drawPixel(i + (DISPLAY_MAX_X_LENGTH / 2) + 1, DISPLAY_AERA1_Y1POS + (DISPLAY_AERAS1_HEIGHT / 2) + (pcm5102_GetAudioData(i * 2 + 1) / 4096) - 1, 10, false);
		}

		for (i = 0; i < (DISPLAY_MAX_X_LENGTH); i++)
		{
			cis_color = synth_GetImageData((i * ((float)NUMBER_OF_NOTES / (float)DISPLAY_MAX_X_LENGTH))) >> 12;
			ssd1362_drawPixel(DISPLAY_MAX_X_LENGTH - 1 - i, DISPLAY_AERA2_Y1POS + DISPLAY_AERAS2_HEIGHT - DISPLAY_INTER_AERAS_HEIGHT - (cis_color) - 1, 15, false);

			ssd1362_drawVLine(DISPLAY_MAX_X_LENGTH - 1 - i, DISPLAY_AERA3_Y1POS + 1, DISPLAY_AERAS3_HEIGHT - 2, cis_color, false);
		}
		ssd1362_drawRect(200, DISPLAY_HEAD_Y1POS, DISPLAY_MAX_X_LENGTH, DISPLAY_HEAD_Y2POS, 4, false);
		ssd1362_drawString(200, 1, (int8_t*)FreqStr, 15, 8);
		ssd1362_writeFullBuffer();

		HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
	}
}
