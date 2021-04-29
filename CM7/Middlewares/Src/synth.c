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

/* Private includes ----------------------------------------------------------*/

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/
#define AUDIO_BUFFER_SIZE             	(512)

/* Private typedef -----------------------------------------------------------*/
typedef enum {
	AUDIO_BUFFER_OFFSET_NONE = 0,
	AUDIO_BUFFER_OFFSET_HALF,
	AUDIO_BUFFER_OFFSET_FULL,
}BUFFER_AUDIO_StateTypeDef;

BUFFER_AUDIO_StateTypeDef bufferAudioState;

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static int16_t *unitary_waveform = NULL;
static struct wave waves[NUMBER_OF_NOTES];
volatile uint32_t synth_process_cnt = 0;

/* Variable containing black and white frame from CIS*/
static uint16_t *imageData = NULL;
static int16_t audioBuff[AUDIO_BUFFER_SIZE] = {0};
//ALIGN_32BYTES (static AUDIO_BufferTypeDef  buffer_ctl) = {0};

static __IO uint32_t uwVolume = AUDIO_DEFAULT_VOLUME;
//BSP_AUDIO_Init_t AudioPlayInit;

/* Private function prototypes -----------------------------------------------*/
static int32_t synth_AudioInit(void);
static void synth_IfftMode(uint16_t *imageData, int16_t *audioData, uint32_t NbrOfData);
static void synth_PlayMode(uint16_t *imageData, int16_t *audioData, uint32_t NbrOfData);

/* Private user code ---------------------------------------------------------*/

/**
 * @brief  synth play init.
 * @param
 * @retval Error
 */
int32_t synth_PlayInit(void)
{
	if (synth_AudioInit() == 0)
		return 0;
	else
		return -1;
}

/**
 * @brief  synth ifft init.
 * @param
 * @retval Error
 */
int32_t synth_IfftInit(void)
{
	int32_t buffer_len = 0;
	uint32_t aRandom32bit = 0;
	uint8_t FreqStr[256] = {0};

	//allocate the contiguous memory area for storage image data
	imageData = malloc(cis_GetEffectivePixelNb() * sizeof(uint16_t*));
	if (imageData == NULL)
	{
		Error_Handler();
	}

	memset(imageData, 0, cis_GetEffectivePixelNb() * sizeof(uint16_t*));

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

	printf("Buffer lengh = %d\n", (int)buffer_len);

#ifdef PRINT_FREQUENCY
	ssd1362_drawRect(0, 57, 256, 64, 5, false);
	sprintf((char *)FreqStr, "%dHz Sz%d Oc%d", (int)waves[0].frequency, (int)waves[0].aera_size, (int)waves[0].octave_coeff);
	ssd1362_drawString(0, 57, (int8_t*)FreqStr, 0, 8);
	sprintf((char *)FreqStr, "%dHz Sz%d Oc%d", (int)waves[NUMBER_OF_NOTES - 1].frequency, (int)waves[NUMBER_OF_NOTES - 1].aera_size / (int)sqrt(waves[NUMBER_OF_NOTES - 1].octave_coeff), (int)sqrt(waves[NUMBER_OF_NOTES - 1].octave_coeff));
	ssd1362_drawString(128, 57, (int8_t*)FreqStr, 0, 8);
	ssd1362_writeFullBuffer();

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
#endif

	if (synth_AudioInit() == 0)
		return 0;
	else
		return -1;
}

int32_t synth_AudioInit(void)
{
	bufferAudioState = AUDIO_BUFFER_OFFSET_NONE;
	HAL_SAI_Transmit_DMA(&hsai_BlockA1, (uint8_t *)&audioBuff[0], AUDIO_BUFFER_SIZE);

	return -1;
}

/**
 * @brief  Get RFFT buffer data
 * @param  Index
 * @retval Value
 */
int16_t synth_GetAudioData(uint32_t index)
{
	//	if (index >= RFFT_BUFFER_SIZE)
	//		Error_Handler();
	return audioBuff[index];
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
int32_t synth_SetImageData(uint32_t index, uint16_t value)
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
void synth_IfftMode(uint16_t *imageData, int16_t *audioData, uint32_t NbrOfData)
{
	static int32_t signal_summation;
	static uint32_t signal_power_summation;
	static int16_t rfft;
	static uint16_t new_idx;
	static uint32_t write_data_nbr;
	static int32_t max_volume;
	static int32_t current_image_data;

	write_data_nbr = 0;

	while(write_data_nbr < NbrOfData)
	{
		signal_summation = 0;
		signal_power_summation = 0;
		max_volume = 0;

		//Summation for all pixel
		for (int32_t note = NUMBER_OF_NOTES; --note >= 1;)
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
			signal_summation += ((*(waves[note].start_ptr + new_idx)) * waves[note].current_volume) >> 16;

			//read equivalent power of current pixel
			signal_power_summation += waves[note].current_volume;

			waves[note].current_idx = new_idx;
		}

		rfft = (signal_summation * ((double)max_volume) / (double)signal_power_summation);

		audioData[write_data_nbr] = rfft;		//L
		audioData[write_data_nbr + 1] = rfft;	//R
		write_data_nbr += 2;
	}

	synth_process_cnt += NbrOfData / 2;
}
#pragma GCC pop_options

/**
 * @brief  Period elapsed callback in non blocking mode
 * @param  htim : TIM handle
 * @retval None
 */
#pragma GCC push_options
#pragma GCC optimize ("unroll-loops")
void synth_PlayMode(uint16_t *imageData, int16_t *audioData, uint32_t NbrOfData)
{
	static uint32_t WriteDataNbr;
	static uint32_t CurrentPix = 0;
	static int16_t AudioDot;
	WriteDataNbr = 0;

	while(WriteDataNbr < (NbrOfData * 2))
	{
		if ((CurrentPix + 1) >= (cis_GetEffectivePixelNb()))
			CurrentPix = 0;
		AudioDot = imageData[CurrentPix] - imageData[CurrentPix + 1];
		audioData[WriteDataNbr] = AudioDot;
		audioData[WriteDataNbr + 1] = AudioDot;
		WriteDataNbr+=2;

		CurrentPix++;

		//		uint32_t aRandom32bit = 0;
		//
		//		if (HAL_RNG_GenerateRandomNumber(&hrng, &aRandom32bit) != HAL_OK)
		//		{
		//			/* Random number generation error */
		//			Error_Handler();
		//		}
		//		audioData[WriteDataNbr] = aRandom32bit % 32768;
		//		audioData[WriteDataNbr + 1] = aRandom32bit % 32768;
		//		WriteDataNbr+=2;
	}

	synth_process_cnt += NbrOfData;
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
	if(bufferAudioState == AUDIO_BUFFER_OFFSET_HALF)
	{
		bufferAudioState = AUDIO_BUFFER_OFFSET_NONE;
		cis_ImageProcessBW(imageData);
		if (mode == IFFT_MODE)
			synth_IfftMode(imageData, &audioBuff[0], AUDIO_BUFFER_SIZE / 2);
		//			else
		//				synth_PlayMode(imageData, (int16_t*)&audioBuff[0], (AUDIO_QUARTER_BUFFER_SIZE / 2));
		/* Clean Data Cache to update the content of the SRAM */
		SCB_CleanDCache_by_Addr((uint32_t *)&audioBuff[0], AUDIO_BUFFER_SIZE);
	}

	/* 2nd half buffer played; so fill it and continue playing from top */
	if(bufferAudioState == AUDIO_BUFFER_OFFSET_FULL)
	{
		bufferAudioState = AUDIO_BUFFER_OFFSET_NONE;
		cis_ImageProcessBW(imageData);
		if (mode == IFFT_MODE)
			synth_IfftMode(imageData, &audioBuff[AUDIO_BUFFER_SIZE / 2], AUDIO_BUFFER_SIZE / 2);
		//			else
		//				synth_PlayMode(imageData, (int16_t*)&audioBuff[AUDIO_QUARTER_BUFFER_SIZE / 2], (AUDIO_QUARTER_BUFFER_SIZE / 2));
		/* Clean Data Cache to update the content of the SRAM */
		SCB_CleanDCache_by_Addr((uint32_t *)&audioBuff[AUDIO_BUFFER_SIZE / 2], AUDIO_BUFFER_SIZE);
	}
	return;
}

void HAL_SAI_TxHalfCpltCallback(SAI_HandleTypeDef *hsai_BlockA1)
{
	if(hsai_BlockA1->Instance==SAI1_Block_A)
	{
		bufferAudioState = AUDIO_BUFFER_OFFSET_HALF;
	}
}

void HAL_SAI_TxCpltCallback(SAI_HandleTypeDef *hsai_BlockA1)
{
	if(hsai_BlockA1->Instance==SAI1_Block_A)
	{
		/* allows AUDIO_Process() to refill 2nd part of the buffer  */
		bufferAudioState = AUDIO_BUFFER_OFFSET_FULL;
	}
}

/**
 * @brief  Manages the DMA FIFO error event.
 * @param  None
 * @retval None
 */
void BSP_AUDIO_OUT_Error_CallBack(uint32_t Interface)
{
	/* could also generate a system reset to recover from the error */
	/* .... */
}
