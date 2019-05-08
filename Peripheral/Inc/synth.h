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

/* Private includes ----------------------------------------------------------*/

/* Exported types ------------------------------------------------------------*/
struct wave {
	uint16_t *start_ptr;
    uint16_t current_idx;
	uint16_t aera_size;
	uint16_t octave_coeff;
#ifdef DEBUG
	float frequency;
#endif
};

extern __IO uint32_t left_output;
extern __IO uint32_t right_output;
/* Exported constants --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/

/* Exported functions prototypes ---------------------------------------------*/
int32_t synth_init(void);

/* Private defines -----------------------------------------------------------*/

#endif /* __SYNTH_H */
