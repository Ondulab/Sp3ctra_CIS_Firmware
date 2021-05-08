/**
 ******************************************************************************
 * @file           : pcm5102.h
 * @brief          : Audio Stereo DAC with 32-bit, 384kHz PCM Interface
 ******************************************************************************
 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __PCM5102_H__
#define __PCM5102_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "config.h"

/* Private includes ----------------------------------------------------------*/

/* Exported types ------------------------------------------------------------*/
typedef enum {
	AUDIO_BUFFER_OFFSET_NONE = 0,
	AUDIO_BUFFER_OFFSET_HALF,
	AUDIO_BUFFER_OFFSET_FULL,
}BUFFER_AUDIO_StateTypeDef;

/* Exported constants --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/

/* Exported functions prototypes ---------------------------------------------*/
void pcm5102_Init(void);
void pcm5102_AudioPause(void);
void pcm5102_AudioResume(void);
void pcm5102_AudioStop(void);
int16_t pcm5102_GetAudioData(uint32_t index);
int16_t * pcm5102_GetDataPtr(uint32_t idx);
BUFFER_AUDIO_StateTypeDef * pcm5102_GetBufferState(void);
void pcm5102_ResetBufferState(void);

/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /*__PCM5102_H__*/
