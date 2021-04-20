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
	HAL_GPIO_WritePin(DAC_FLT_GPIO_Port, DAC_FLT_Pin, GPIO_PIN_RESET); //Filterselect: Normallatency(Low) / Lowlatency(High)
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

//void HAL_I2S_TxHalfCpltCallback(I2S_HandleTypeDef *hi2s)
//{
//	HalfTransfer_CallBack_FS();
//}
void HAL_SAI_TxHalfCpltCallback(SAI_HandleTypeDef *hsai_BlockA1)
{
	if(hsai_BlockA1->Instance==SAI1_Block_A)
	{

	}
}

//void HAL_I2S_TxCpltCallback(I2S_HandleTypeDef *hi2s)
//{
//	TransferComplete_CallBack_FS();
//}

void HAL_SAI_TxCpltCallback(SAI_HandleTypeDef *hsai_BlockA1)
{
	if(hsai_BlockA1->Instance==SAI1_Block_A)
	{

	}
}

///**
//  * @brief  Main program
//  * @param  None
//  * @retval None
//  */
//int main(void)
//{
//  /* Enable the CPU Cache */
//  CPU_CACHE_Enable();
//
//  /* STM32H7xx HAL library initialization:
//       - Systick timer is configured by default as source of time base, but user
//         can eventually implement his proper time base source (a general purpose
//         timer for example or other time source), keeping in mind that Time base
//         duration should be kept 1ms since PPP_TIMEOUT_VALUEs are defined and
//         handled in milliseconds basis.
//       - Set NVIC Group Priority to 4
//       - Low Level Initialization
//     */
//  HAL_Init();
//
//  /* Configure the system clock to have a frequency of 400 MHz */
//  SystemClock_Config();
//
//  /* Configure LED1 */
//  BSP_LED_Init(LED1);
//
//  /* Configure LED3 */
//  BSP_LED_Init(LED3);
//
//  /* Initialize playback */
//  Playback_Init();
//  WM8994_Init_t codec_init;
//
//  codec_init.Resolution   = 0;
//
//  /* Fill codec_init structure */
//  codec_init.Frequency    = 16000;
//  codec_init.InputDevice  = AUDIO_IN_DEVICE_DIGITAL_MIC;
//  codec_init.OutputDevice = AUDIO_OUT_DEVICE_HEADPHONE;
//
//  /* Convert volume before sending to the codec */
//  codec_init.Volume       = VOLUME_OUT_CONVERT(100);
//
//  /* Start the playback */
//  if(Audio_Drv->Init(Audio_CompObj, &codec_init) != 0)
//  {
//    Error_Handler();
//  }
//
//  /* Start the PDM data reception process */
//  if(HAL_OK != HAL_SAI_Receive_DMA(&SaiInputHandle, (uint8_t*)audioPdmBuf, AUDIO_BUFFER_SIZE))
//  {
//    Error_Handler();
//  }
//
//  /* Start the PCM data transmission process */
//  if(HAL_OK != HAL_SAI_Transmit_DMA(&SaiOutputHandle, (uint8_t *)audioPcmBuf, AUDIO_BUFFER_SIZE))
//  {
//    Error_Handler();
//  }
//    /* Start the playback */
//  if(Audio_Drv->Play(Audio_CompObj) < 0)
//  {
//    Error_Handler();
//  }
//  /* Initialize Rx buffer status */
//  bufferStatus &= BUFFER_OFFSET_NONE;
//
//  /* Start loopback */
//  while(1)
//  {
//    /* Wait Rx half transfer event */
//    while(bufferStatus != BUFFER_OFFSET_HALF);
//
//    /* Convert the first half of PDM data to PCM */
//    AUDIO_IN_PDMToPCM((uint16_t*)&audioPdmBuf[0], &audioPcmBuf[pcmPtr], AUDIO_CHANNEL_NUMBER);
//
//    /* Update pcmPtr */
//    pcmPtr += AUDIO_PCM_CHUNK_SIZE;
//
//    /* Initialize Rx buffer status */
//    bufferStatus &= BUFFER_OFFSET_NONE;
//
//    /* Wait Rx transfer complete event */
//    while(bufferStatus != BUFFER_OFFSET_FULL);
//
//    /* Convert the second half of PDM data to PCM */
//    AUDIO_IN_PDMToPCM((uint16_t*)&audioPdmBuf[AUDIO_BUFFER_SIZE/2], &audioPcmBuf[pcmPtr], AUDIO_CHANNEL_NUMBER);
//
//    /* Update pcmPtr */
//    pcmPtr += AUDIO_PCM_CHUNK_SIZE;
//    if(pcmPtr >= AUDIO_BUFFER_SIZE)
//    {
//      pcmPtr = 0;
//    }
//
//    /* Initialize Rx buffer status */
//    bufferStatus &= BUFFER_OFFSET_NONE;
//
//    /* Toggle LED1 */
//    BSP_LED_Toggle(LED1);
//  }
//}
