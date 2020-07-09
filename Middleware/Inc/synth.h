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

extern volatile uint32_t rfft_cnt;

/* Exported constants --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/

/* Exported functions prototypes ---------------------------------------------*/
int32_t synthInit(void);
int32_t synthTest(void);
uint16_t getBuffData(uint32_t index);
void rfft(uint32_t *pdata, uint32_t NbrOfData);
uint8_t AUDIO_Process(void);

/* Private defines -----------------------------------------------------------*/

#endif /* __SYNTH_H */
