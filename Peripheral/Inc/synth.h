/*
 * synth.h
 *
 *  Created on: 24 avr. 2019
 *      Author: zhonx
 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __SYNTH_H
#define __SYNTH_H

/* Includes ------------------------------------------------------------------*/
#include "config.h"
#include "stdint.h"
#include "synth.h"

/* Private includes ----------------------------------------------------------*/

/* Exported types ------------------------------------------------------------*/
struct wave {
	uint16_t *start_ptr;
    uint16_t current_idx;
	uint16_t aera_size;
	uint16_t octave_coeff;
#ifdef PRINT_FREQUENCY
	float frequency;
#endif
};

#define  AUDIO_BUFFER_SIZE 4410

extern volatile uint32_t rfft_cnt;
extern volatile uint32_t audio_buff[AUDIO_BUFFER_SIZE];
/* Exported constants --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/

/* Exported functions prototypes ---------------------------------------------*/
int32_t synth_init(void);

/* Private defines -----------------------------------------------------------*/

#endif /* __SYNTH_H */
