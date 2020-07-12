/*
 * synth.c
 *
 *  Created on: 24 avr. 2019
 *      Author: zhonx
 */

/* Includes ------------------------------------------------------------------*/
#include <BSP_example.h>
#include "stm32h7xx_hal.h"
#include "config.h"
#include "tim.h"
#include "rng.h"

#include "stdlib.h"
#include "stdio.h"

//#include "arm_math.h"

#include "cis.h"
#include "wave_generation.h"
#include "synth.h"

/* Private includes ----------------------------------------------------------*/

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/
/*Since SysTick is set to 1ms (unless to set it quicker) */
/* to run up to 48khz, a buffer around 1000 (or more) is requested*/
/* to run up to 96khz, a buffer around 2000 (or more) is requested*/
#define AUDIO_DEFAULT_VOLUME    		40

/* Audio file size and start address are defined here since the audio file is
   stored in Flash memory as a constant table of 16-bit data */
#define AUDIO_START_OFFSET_ADDRESS    	0            /* Offset relative to audio file header size */
#define AUDIO_BUFFER_SIZE             	4096
#define RFFT_BUFFER_SIZE        	  	(AUDIO_BUFFER_SIZE / 4)
/* Audio file size and start address are defined here since the audio file is
   stored in Flash memory as a constant table of 16-bit data */
#define AUDIO_START_OFFSET_ADDRESS    0            /* Offset relative to audio file header size */

/* Private typedef -----------------------------------------------------------*/
typedef enum {
	AUDIO_STATE_IDLE = 0,
	AUDIO_STATE_INIT,
	AUDIO_STATE_PLAYING,
}AUDIO_PLAYBACK_StateTypeDef;

typedef enum {
	BUFFER_OFFSET_NONE = 0,
	BUFFER_OFFSET_HALF,
	BUFFER_OFFSET_FULL,
}BUFFER_StateTypeDef;

typedef struct {
	uint8_t buff[AUDIO_BUFFER_SIZE];
	uint32_t fptr;
	BUFFER_StateTypeDef state;
	uint32_t AudioFileSize;
	uint32_t *SrcAddress;
}AUDIO_BufferTypeDef;
/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static uint16_t *unitary_waveform = NULL;
static struct wave waves[NUMBER_OF_NOTES];
volatile uint32_t rfft_cnt = 0;

uint32_t bytesread;
ALIGN_32BYTES (static AUDIO_BufferTypeDef  buffer_ctl) = {0};
static __IO uint32_t rfft_buff[RFFT_BUFFER_SIZE] = {0};
static AUDIO_PLAYBACK_StateTypeDef  audio_state;
__IO uint32_t uwVolume = AUDIO_DEFAULT_VOLUME;
uint8_t ReadVol = 0;
__IO uint32_t uwPauseEnabledStatus = 0;
uint32_t updown = 1;
uint32_t AudioFreq[8] = {96000, 48000, 44100, 32000, 22050, 16000, 11025, 8000};
BSP_AUDIO_Init_t AudioPlayInit;
uint32_t OutputDevice = 0;

/* Variable containing black and white frame from CIS*/
ALIGN_32BYTES (static uint16_t frameDataBW[NUMBER_OF_NOTES]) = {0};

/* Private function prototypes -----------------------------------------------*/
static uint32_t synthGetDataNb(void *pdata, uint32_t offset, uint8_t *pbuf, uint32_t NbrOfData);
void synthRfftMode(uint32_t *pdata, uint32_t NbrOfData);

/* Private user code ---------------------------------------------------------*/

/**
 * @brief  synth init.
 * @param
 * @retval Error
 */
int32_t synthInit(void)
{
	int32_t buffer_len = 0;
	uint32_t aRandom32bit = 0;

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
	}

	if (buffer_len < 0)
	{
		printf("RAM overflow");
		return -1;
	}

	printf("Buffer lengh = %d\n", (int)buffer_len);

#ifdef PRINT_FREQUENCY
	uint8_t FreqStr[256] = {0};
	GUI_SetTextColor(GUI_COLOR_LIGHTGRAY);
	GUI_SetBackColor(GUI_COLOR_DARKGRAY);
	GUI_SetFont(&Font12);
	sprintf((char *)FreqStr, "FIRST NOTE = %0.2fHz, SIZE = %d, OCTAVE = %d", waves[0].frequency, (int)waves[0].aera_size, (int)waves[0].octave_coeff);
	GUI_DisplayStringAt(0, LINE(20), (uint8_t*)FreqStr, LEFT_MODE);
	sprintf((char *)FreqStr, "LAST NOTE  = %0.2fHz, SIZE = %d, OCTAVE = %d", waves[NUMBER_OF_NOTES - 1].frequency, (int)waves[NUMBER_OF_NOTES - 1].aera_size / (int)sqrt(waves[NUMBER_OF_NOTES - 1].octave_coeff), (int)sqrt(waves[NUMBER_OF_NOTES - 1].octave_coeff));
	GUI_DisplayStringAt(0, LINE(21), (uint8_t*)FreqStr, LEFT_MODE);

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

	uint32_t *AudioFreq_ptr;

	AudioFreq_ptr = &AudioFreq[1]; //44K /*96K*/

	AudioPlayInit.Device = AUDIO_OUT_DEVICE_HEADPHONE;
	AudioPlayInit.ChannelsNbr = 2;
	AudioPlayInit.SampleRate = *AudioFreq_ptr;
	AudioPlayInit.BitsPerSample = AUDIO_RESOLUTION_16B;
	AudioPlayInit.Volume = uwVolume;


	if(BSP_AUDIO_OUT_Init(0, &AudioPlayInit) != 0)
	{
		GUI_SetBackColor(GUI_COLOR_WHITE);
		GUI_SetTextColor(GUI_COLOR_RED);
	}

	uint32_t bytesread;

	buffer_ctl.state = BUFFER_OFFSET_NONE;
	buffer_ctl.AudioFileSize = RFFT_BUFFER_SIZE;
	buffer_ctl.SrcAddress = (uint32_t*)rfft_buff;

	bytesread = synthGetDataNb((void *)rfft_buff, 0, &buffer_ctl.buff[0], AUDIO_BUFFER_SIZE);
	if(bytesread > 0)
	{
		BSP_AUDIO_OUT_Play(0,(uint8_t *)&buffer_ctl.buff[0], AUDIO_BUFFER_SIZE);
		audio_state = AUDIO_STATE_PLAYING;
		buffer_ctl.fptr = bytesread;

		return 0;
	}

	return -1;
}

/**
 * @brief  synth Test.
 * @param
 * @retval Error
 */
int32_t synthTest(void)
{
	uint32_t aRandom32bit = 0;

	for(int i = 0; i < NUMBER_OF_NOTES; i++)
	{
//		frameDataBW[i * PIXEL_PER_COMMA] = 30000;

		if (HAL_RNG_GenerateRandomNumber(&hrng, &aRandom32bit) != HAL_OK)
		{
			/* Random number generation error */
			Error_Handler();
		}
		frameDataBW[i] = aRandom32bit % 1000;
	}

	frameDataBW[50] = 30000;
	frameDataBW[54] = 30000;
	frameDataBW[151] = 30000;
	frameDataBW[152] = 30000;
	frameDataBW[153] = 30000;

	return 0;
}

/**
 * @brief  Get RFFT buffer data
 * @param  Index
 * @retval Value
 */
uint16_t synthGetRfftBuffData(uint32_t index)
{
//	if (index >= RFFT_BUFFER_SIZE)
//		Error_Handler();
	return rfft_buff[index];
}

/**
 * @brief  Set buffer data
 * @param  index
 * @retval None
 */
void synthSetFrameBuffData(uint32_t index, uint16_t value)
{
//	if (index >= NUMBER_OF_NOTES)
//		Error_Handler();
	frameDataBW[index] = value;
}

/**
 * @brief  Set buffer data
 * @param  index
 * @retval Value
 */
uint16_t synthGetFrameBuffData(uint32_t index)
{
	if (index >= NUMBER_OF_NOTES)
		return 0;
	return frameDataBW[index];
}

/**
 * @brief  Period elapsed callback in non blocking mode
 * @param  htim : TIM handle
 * @retval None
 */
#pragma GCC push_options
#pragma GCC optimize ("unroll-loops")
void synthRfftMode(uint32_t *pdata, uint32_t NbrOfData)
{
	uint64_t signal_summation;
	uint32_t signal_power_summation;
	uint32_t new_idx;
	uint64_t max_power;
	uint32_t rfft;
	uint32_t curr_note_val;

	uint32_t WriteDataNbr;

	WriteDataNbr = 0;
	while(WriteDataNbr < NbrOfData)
	{
		signal_summation = 0;
		signal_power_summation = 0;
		max_power = 0;
		rfft = 0;
		//Summation for all pixel
		for (int32_t note = NUMBER_OF_NOTES; --note >= 0;)
		{
			//store current pixel value
			curr_note_val = frameDataBW[(uint32_t)note];
			//test for CIS presence
			if (curr_note_val > SENSIVITY_THRESHOLD)
			{
				//octave_coeff jump current pointer into the fundamental waveform, for example : the 3th octave increment the current pointer 8 per 8 (2^3)
				//example for 17 cell waveform and 3th octave : [X][Y][Z][X][Y][Z][X][Y][Z][X][Y][[Z][X][Y][[Z][X][Y], X for the first pass, Y for second etc...
				new_idx = (waves[note].current_idx + waves[note].octave_coeff);
				if (new_idx > waves[note].aera_size)
					new_idx -= waves[note].aera_size;

				waves[note].current_idx = new_idx;

				signal_summation += (*(waves[note].start_ptr + waves[note].current_idx) * curr_note_val) >> 16;

				//read equivalent power of current pixel
				signal_power_summation += curr_note_val;
				if (curr_note_val > max_power)
					max_power = curr_note_val;
			}
		}

		rfft = (signal_summation * ((double)max_power / signal_power_summation));
		pdata[WriteDataNbr] = rfft / 2 | ((rfft / 2) << 16); //stereo
//		pdata[WriteDataNbr] = rfft / 2; //mono
		WriteDataNbr++;
	}

	rfft_cnt += WriteDataNbr;
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
uint8_t synthAudioProcess(void)
{
	uint32_t bytesread;
	AUDIO_ErrorTypeDef error_state = AUDIO_ERROR_NONE;

	switch(audio_state)
	{
	case AUDIO_STATE_PLAYING:

		if(buffer_ctl.fptr >= (buffer_ctl.AudioFileSize * 4))
		{
			/* Play audio sample again ... */
			buffer_ctl.fptr = 0;
			error_state = AUDIO_ERROR_EOF;
		}

		/* 1st half buffer played; so fill it and continue playing from bottom*/
		if(buffer_ctl.state == BUFFER_OFFSET_HALF)
		{
			bytesread = synthGetDataNb((void *)buffer_ctl.SrcAddress,
					buffer_ctl.fptr,
					&buffer_ctl.buff[0],
					AUDIO_BUFFER_SIZE / 2);

			if( bytesread > 0)
			{
				buffer_ctl.state = BUFFER_OFFSET_NONE;
				buffer_ctl.fptr += bytesread;
				/* Clean Data Cache to update the content of the SRAM */
				SCB_CleanDCache_by_Addr((uint32_t*)&buffer_ctl.buff[0], AUDIO_BUFFER_SIZE / 2);
				synthRfftMode((uint32_t*)&rfft_buff[0], RFFT_BUFFER_SIZE / 2);
			}
		}

		/* 2nd half buffer played; so fill it and continue playing from top */
		if(buffer_ctl.state == BUFFER_OFFSET_FULL)
		{
			bytesread = synthGetDataNb((void *)buffer_ctl.SrcAddress,
					buffer_ctl.fptr,
					&buffer_ctl.buff[AUDIO_BUFFER_SIZE / 2],
					AUDIO_BUFFER_SIZE / 2);
			if( bytesread > 0)
			{
				buffer_ctl.state = BUFFER_OFFSET_NONE;
				buffer_ctl.fptr += bytesread;
				/* Clean Data Cache to update the content of the SRAM */
				SCB_CleanDCache_by_Addr((uint32_t*)&buffer_ctl.buff[AUDIO_BUFFER_SIZE/2], AUDIO_BUFFER_SIZE / 2);
				synthRfftMode((uint32_t*)&rfft_buff[RFFT_BUFFER_SIZE / 2], RFFT_BUFFER_SIZE / 2);
			}
		}
		break;

	default:
		error_state = AUDIO_ERROR_NOTREADY;
		break;
	}
	return (uint8_t) error_state;
}

/**
 * @brief  Gets Data from storage unit.
 * @param  None
 * @retval None
 */
static uint32_t synthGetDataNb(void *pdata, uint32_t offset, uint8_t *pbuf, uint32_t NbrOfData)
{
	uint8_t *lptr = pdata;
	uint32_t ReadDataNbr;

	ReadDataNbr = 0;
	while(((offset + ReadDataNbr) < (buffer_ctl.AudioFileSize * 4)) && (ReadDataNbr < NbrOfData))
	{
		pbuf[ReadDataNbr]= lptr [offset + ReadDataNbr];
		ReadDataNbr++;
	}

	return ReadDataNbr;
}

/*------------------------------------------------------------------------------
       Callbacks implementation:
           the callbacks API are defined __weak in the stm32769i_discovery_audio.c file
           and their implementation should be done the user code if they are needed.
           Below some examples of callback implementations.
  ----------------------------------------------------------------------------*/
/**
 * @brief  Manages the full Transfer complete event.
 * @param  None
 * @retval None
 */
void BSP_AUDIO_OUT_TransferComplete_CallBack(uint32_t Interface)
{
	if(audio_state == AUDIO_STATE_PLAYING)
	{
		/* allows AUDIO_Process() to refill 2nd part of the buffer  */
		buffer_ctl.state = BUFFER_OFFSET_FULL;
	}
}

/**
 * @brief  Manages the DMA Half Transfer complete event.
 * @param  None
 * @retval None
 */
void BSP_AUDIO_OUT_HalfTransfer_CallBack(uint32_t Interface)
{
	if(audio_state == AUDIO_STATE_PLAYING)
	{
		/* allows AUDIO_Process() to refill 1st part of the buffer  */
		buffer_ctl.state = BUFFER_OFFSET_HALF;
	}
}

/**
 * @brief  Manages the DMA FIFO error event.
 * @param  None
 * @retval None
 */
void BSP_AUDIO_OUT_Error_CallBack(uint32_t Interface)
{
	/* Display message on the LCD screen */
	GUI_SetBackColor(GUI_COLOR_RED);
	GUI_DisplayStringAt(0, LINE(14), (uint8_t *)"       DMA  ERROR     ", CENTER_MODE);
	GUI_SetBackColor(GUI_COLOR_WHITE);

	/* Stop the program with an infinite loop */
	while (BSP_PB_GetState(BUTTON_USER) != RESET)
	{
		return;
	}

	/* could also generate a system reset to recover from the error */
	/* .... */
}
