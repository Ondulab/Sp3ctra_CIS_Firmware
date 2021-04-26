/**
 ******************************************************************************
 * @file           : pcm5102.c
 * @brief          : Audio Stereo DAC with 32-bit, 384kHz PCM Interface
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
#include "pcm5102.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/
#define DMA_MAX_SZE                     0xFFFF
#define DMA_MAX(_X_)                (((_X_) <= DMA_MAX_SZE)? (_X_):DMA_MAX_SZE)
#define AUDIODATA_SIZE                  2   /* 16-bits audio data size */

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/* Private user code ---------------------------------------------------------*/

void pcm5102_Init(void)
{
	HAL_GPIO_WritePin(DAC_FLT_GPIO_Port, DAC_FLT_Pin, GPIO_PIN_SET); //Filterselect: Normallatency(Low) / Lowlatency(High)
	HAL_GPIO_WritePin(DAC_DEMP_GPIO_Port, DAC_DEMP_Pin, GPIO_PIN_RESET); //De-emphasiscontrol for 44.1kHz sampling rate: Off(Low) / On(High)
	HAL_GPIO_WritePin(DAC_XSMT_GPIO_Port, DAC_XSMT_Pin, GPIO_PIN_SET); //Soft mute control: Softmute(Low) / soft un-mute(High)
	HAL_GPIO_WritePin(DAC_FMT_GPIO_Port, DAC_FMT_Pin, GPIO_PIN_RESET); //Audio format selection: I2S(Low)/ Left justified(High)
}

void Audio_Player_Play(uint8_t* pBuffer, uint32_t Size)
{
	if(Size > 0xFFFF) {
		Size = 0xFFFF;
		HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
	}
	HAL_SAI_Transmit_DMA(&hsai_BlockA1, (uint8_t*)pBuffer, Size);

//	HAL_SAI_Transmit_DMA(&hsai_BlockA1, (uint8_t*)pBuffer, DMA_MAX(Size/AUDIODATA_SIZE));
}

void Audio_Player_Pause(void)
{
//	HAL_I2S_DMAPause(&hi2s2);
	HAL_SAI_DMAPause(&hsai_BlockA1);
}

void Audio_Player_Resume(void)
{
//	HAL_I2S_DMAResume(&hi2s2);
	HAL_SAI_DMAResume(&hsai_BlockA1);
}

void Audio_Player_Stop(void)
{
//	HAL_I2S_DMAStop(&hi2s2);
	HAL_SAI_DMAStop(&hsai_BlockA1);
}

void Audio_Player_VolumeCtl(uint8_t vol)
{
//	WM8978_VolumeCtl(vol);
}

////void HAL_I2S_TxHalfCpltCallback(I2S_HandleTypeDef *hi2s)
////{
////	HalfTransfer_CallBack_FS();
////}
//void HAL_SAI_TxHalfCpltCallback(SAI_HandleTypeDef *hsai_BlockA1)
//{
//	if(hsai_BlockA1->Instance==SAI1_Block_A)
//	{
//
//	}
//}
//
////void HAL_I2S_TxCpltCallback(I2S_HandleTypeDef *hi2s)
////{
////	TransferComplete_CallBack_FS();
////}
//
//void HAL_SAI_TxCpltCallback(SAI_HandleTypeDef *hsai_BlockA1)
//{
//	if(hsai_BlockA1->Instance==SAI1_Block_A)
//	{
//
//	}
//}
