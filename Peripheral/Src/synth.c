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
#define AUDIO_DEFAULT_VOLUME    70

/* Audio file size and start address are defined here since the audio file is
   stored in Flash memory as a constant table of 16-bit data */
#define AUDIO_START_OFFSET_ADDRESS    0            /* Offset relative to audio file header size */
#define AUDIO_BUFFER_SIZE             4096
#define RFFT_BUFFER_SIZE        	  (AUDIO_BUFFER_SIZE / 4)
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
static uint32_t rfft_buff[RFFT_BUFFER_SIZE] = {0};
static AUDIO_PLAYBACK_StateTypeDef  audio_state;
__IO uint32_t uwVolume = 15;
uint8_t ReadVol = 0;
__IO uint32_t uwPauseEnabledStatus = 0;
uint32_t updown = 1;

uint32_t AudioFreq[8] = {96000, 48000, 44100, 32000, 22050, 16000, 11025, 8000};

BSP_AUDIO_Init_t AudioPlayInit;

uint32_t OutputDevice = 0;

/* Private function prototypes -----------------------------------------------*/
int32_t initDacTimer(uint32_t freq);
int32_t initSamplingTimer(uint32_t sampling_freq);
static uint32_t GetData(void *pdata, uint32_t offset, uint8_t *pbuf, uint32_t NbrOfData);

/* Private user code ---------------------------------------------------------*/

/**
 * @brief  synth init.
 * @param
 * @retval Error
 */
int32_t synth_init(void)
{
	int32_t buffer_len = 0;

	buffer_len = init_waves(&unitary_waveform, waves);

	if (buffer_len < 0)
	{
		printf("RAM overflow");
		return -1;
	}

	printf("Buffer lengh = %d\n", (int)buffer_len);

#ifdef PRINT_FREQUENCY
	printf("FREQ = %0.2fHz, SIZE = %d, OCTAVE = %d\n", waves[0].frequency, (int)waves[0].aera_size, (int)waves[0].octave_coeff);
	printf("FREQ = %0.2fHz, SIZE = %d, OCTAVE = %d\n", waves[NUMBER_OF_NOTES - 1].frequency, (int)waves[NUMBER_OF_NOTES - 1].aera_size, (int)waves[NUMBER_OF_NOTES - 1].octave_coeff);

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

	bytesread = GetData( (void *)rfft_buff, 0, &buffer_ctl.buff[0], AUDIO_BUFFER_SIZE);
	if(bytesread > 0)
	{
		BSP_AUDIO_OUT_Play(0,(uint8_t *)&buffer_ctl.buff[0], AUDIO_BUFFER_SIZE);
		audio_state = AUDIO_STATE_PLAYING;
		buffer_ctl.fptr = bytesread;

		//		if (initSamplingTimer(SAMPLING_FREQUENCY) != 0)
		//		{
		//			Error_Handler();
		//		}
		return 0;
	}

	return -1;
}

/**
 * @brief  Return buffer data
 * @param  index
 * @retval value
 */
uint16_t getBuffData(uint32_t index)
{
	return rfft_buff[index];
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

	/* Compute the prescaler value to have TIMx counter clock equal to 10000 Hz */
	uwPrescalerValue = (uint32_t)(SystemCoreClock / (4 * sampling_freq)) - 1;

	/* Set TIMx instance */
	htim12.Instance = TIM15;

	/* Initialize TIM15 peripheral as follows:
		       + Period = sampling_freq
		       + Prescaler = (SystemCoreClock/10000) - 1
		       + ClockDivision = 0
		       + Counter direction = Up
	 */
	htim12.Init.Period            = 1;
	htim12.Init.Prescaler         = uwPrescalerValue;
	htim12.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
	htim12.Init.CounterMode       = TIM_COUNTERMODE_UP;
	htim12.Init.RepetitionCounter = 0;
	htim12.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
	if (HAL_TIM_OC_Init(&htim12) != HAL_OK)
	{
		Error_Handler();
	}

	//	sConfigOC.OCMode = TIM_OCMODE_TOGGLE;
	//	sConfigOC.Pulse = uwPrescalerValue;
	//	sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
	//	sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
	//	sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
	//	sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
	//	sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
	//	if (HAL_TIM_OC_ConfigChannel(&htim12, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
	//	{
	//		Error_Handler();
	//	}

	//	/* Start channel 1 in Output compare mode */
	if (HAL_TIM_OC_Start_IT(&htim12, TIM_CHANNEL_1) != HAL_OK)
	{
		Error_Handler();
	}

#ifndef DEBUG_SAMPLE_RATE
	if (HAL_TIM_Base_Start_IT(&htim12) != HAL_OK)
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
void rfft(uint32_t *pdata, uint32_t NbrOfData)
{
	uint32_t signal_summation;
	uint32_t signal_power_summation;
	uint32_t new_idx;
	uint32_t max_power;
	uint16_t rfft;
	uint16_t reverse8bit;

	uint32_t WriteDataNbr;

	WriteDataNbr = 0;
	while(WriteDataNbr < NbrOfData)
	{
		signal_summation = 0;
		signal_power_summation = 0;
		max_power = 0;
		rfft = 0;
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

		rfft = (signal_summation * ((double)max_power / signal_power_summation));
		pdata[WriteDataNbr] = rfft | (rfft << 16); //stereo
		//	rfft_buff[audio_buff_idx] = rfft; //mono
		WriteDataNbr++;
	}
	//	printf ("%d\n",(uint32_t)(signal_summation * ((double)max_power / signal_power_summation)) >> 4);

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
uint8_t AUDIO_Process(void)
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
			bytesread = GetData((void *)buffer_ctl.SrcAddress,
					buffer_ctl.fptr,
					&buffer_ctl.buff[0],
					AUDIO_BUFFER_SIZE / 2);

			if( bytesread > 0)
			{
				buffer_ctl.state = BUFFER_OFFSET_NONE;
				buffer_ctl.fptr += bytesread;
				/* Clean Data Cache to update the content of the SRAM */
				SCB_CleanDCache_by_Addr((uint32_t*)&buffer_ctl.buff[0], AUDIO_BUFFER_SIZE / 2);
				rfft((uint32_t*)&rfft_buff[0], RFFT_BUFFER_SIZE / 2);
			}
		}

		/* 2nd half buffer played; so fill it and continue playing from top */
		if(buffer_ctl.state == BUFFER_OFFSET_FULL)
		{
			bytesread = GetData((void *)buffer_ctl.SrcAddress,
					buffer_ctl.fptr,
					&buffer_ctl.buff[AUDIO_BUFFER_SIZE / 2],
					AUDIO_BUFFER_SIZE / 2);
			if( bytesread > 0)
			{
				buffer_ctl.state = BUFFER_OFFSET_NONE;
				buffer_ctl.fptr += bytesread;
				/* Clean Data Cache to update the content of the SRAM */
				SCB_CleanDCache_by_Addr((uint32_t*)&buffer_ctl.buff[AUDIO_BUFFER_SIZE/2], AUDIO_BUFFER_SIZE / 2);
				rfft((uint32_t*)&rfft_buff[RFFT_BUFFER_SIZE / 2], RFFT_BUFFER_SIZE / 2);
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
static uint32_t GetData(void *pdata, uint32_t offset, uint8_t *pbuf, uint32_t NbrOfData)
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
