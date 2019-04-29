/*
 * wave_generation.h
 *
 *  Created on: 24 avr. 2019
 *      Author: zhonx
 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __WAVE_GENERATION_H
#define __WAVE_GENERATION_H


#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "synth.h"

/* Private includes ----------------------------------------------------------*/

/* Exported types ------------------------------------------------------------*/

/* Exported constants --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/

/* Exported functions prototypes ---------------------------------------------*/
uint32_t init_waves(__IO uint16_t **unitary_waveform, __IO struct wave *waves);

/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* __WAVE_GENERATION_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
