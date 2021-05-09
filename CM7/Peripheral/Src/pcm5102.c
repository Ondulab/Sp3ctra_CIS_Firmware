/**
 ******************************************************************************
 * @file           : pcm5102.c
 * @brief          : Audio Stereo DAC with 32-bit, 384kHz PCM Interface
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "stdio.h"

#include "main.h"
#include "sai.h"
#include "config.h"

/* Private includes ----------------------------------------------------------*/
#include "pcm5102.h"

/* Private typedef -----------------------------------------------------------*/
static BUFFER_AUDIO_StateTypeDef bufferAudioState;

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static int16_t audioBuff[AUDIO_BUFFER_SIZE] = {0};
//ALIGN_32BYTES (static AUDIO_BufferTypeDef  buffer_ctl) = {0};

/* Private function prototypes -----------------------------------------------*/

/* Private user code ---------------------------------------------------------*/

/**
 * @brief  Initialise audio peripheral
 * @param  void
 * @retval void
 */
void pcm5102_Init(void)
{
	printf("----------- DAC INIT ----------\n");
	printf("-------------------------------\n");

	HAL_GPIO_WritePin(DAC_FLT_GPIO_Port, DAC_FLT_Pin, GPIO_PIN_SET); //Filterselect: Normallatency(Low) / Lowlatency(High)
	HAL_GPIO_WritePin(DAC_DEMP_GPIO_Port, DAC_DEMP_Pin, GPIO_PIN_RESET); //De-emphasiscontrol for 44.1kHz sampling rate: Off(Low) / On(High)
	HAL_GPIO_WritePin(DAC_XSMT_GPIO_Port, DAC_XSMT_Pin, GPIO_PIN_SET); //Soft mute control: Softmute(Low) / soft un-mute(High)
	HAL_GPIO_WritePin(DAC_FMT_GPIO_Port, DAC_FMT_Pin, GPIO_PIN_RESET); //Audio format selection: I2S(Low)/ Left justified(High)

	bufferAudioState = AUDIO_BUFFER_OFFSET_NONE;
	HAL_SAI_Transmit_DMA(&hsai_BlockA1, (uint8_t *)&audioBuff[0], AUDIO_BUFFER_SIZE);
}

/**
 * @brief  Initialise audio peripheral
 * @param  void
 * @retval void
 */
void pcm5102_AudioPause(void)
{
	HAL_SAI_DMAPause(&hsai_BlockA1);
}

/**
 * @brief  Resume audio player
 * @param  void
 * @retval void
 */
void pcm5102_AudioResume(void)
{
	HAL_SAI_DMAResume(&hsai_BlockA1);
}

/**
 * @brief  Stop audio player
 * @param  void
 * @retval void
 */
void pcm5102_AudioStop(void)
{
	HAL_SAI_DMAStop(&hsai_BlockA1);
}

/**
 * @brief  Get audio buffer data
 * @param  Index
 * @retval Value
 */
int16_t pcm5102_GetAudioData(uint32_t index)
{
	//	if (index >= RFFT_BUFFER_SIZE)
	//		Error_Handler();
	return audioBuff[index];
}

/**
 * @brief  Get audio buffer index pointer
 * @param  index
 * @retval index address
 */
int16_t * pcm5102_GetDataPtr(uint32_t idx)
{
	return &audioBuff[idx];
}

/**
 * @brief  Get state of audio process
 * @param  void
 * @retval state
 */
__inline BUFFER_AUDIO_StateTypeDef * pcm5102_GetBufferState(void)
{
	return &bufferAudioState;
}

/**
 * @brief  Reset state of audio process
 * @param  void
 * @retval void
 */
__inline void pcm5102_ResetBufferState(void)
{
	bufferAudioState = AUDIO_BUFFER_OFFSET_NONE;
}

void HAL_SAI_TxHalfCpltCallback(SAI_HandleTypeDef *hsai_BlockA1)
{
	//	if(hsai_BlockA1->Instance==SAI1_Block_A)
	bufferAudioState = AUDIO_BUFFER_OFFSET_HALF;
//	SCB_CleanDCache_by_Addr((uint32_t *)&audioBuff[AUDIO_BUFFER_SIZE / 2], AUDIO_BUFFER_SIZE);
}

void HAL_SAI_TxCpltCallback(SAI_HandleTypeDef *hsai_BlockA1)
{
	//	if(hsai_BlockA1->Instance==SAI1_Block_A)
	bufferAudioState = AUDIO_BUFFER_OFFSET_FULL;
//	SCB_CleanDCache_by_Addr((uint32_t *)&audioBuff[0], AUDIO_BUFFER_SIZE);
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
