/*
 * wave_generation.h
 *
 *  Created on: 24 avr. 2019
 *      Author: zhonx
 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __WAVE_GENERATION_H
#define __WAVE_GENERATION_H

/* Includes ------------------------------------------------------------------*/
#include "synth.h"

/* Private includes ----------------------------------------------------------*/

/* Exported types ------------------------------------------------------------*/

/* Exported constants --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/

/* Exported functions prototypes ---------------------------------------------*/
uint32_t init_waves(uint16_t **unitary_waveform, struct wave *waves);

/* Private defines -----------------------------------------------------------*/

#endif /* __WAVE_GENERATION_H */
