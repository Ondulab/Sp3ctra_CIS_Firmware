/*
 * synth_v2.h
 *
 *  Created on: May 31, 2020
 *      Author: zhonx
 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __SYNTH_V3_H
#define __SYNTH_V3_H

#define DISPLAY_AERAS_HEIGHT		(0x20 + 2 * DISPLAY_INTER_AERAS_HEIGHT)
#define DISPLAY_HEAD_HEIGHT			(20)
#define DISPLAY_INTER_AERAS_HEIGHT	(2)

#define DISPLAY_AERA1_YPOS			(DISPLAY_HEAD_HEIGHT + DISPLAY_INTER_AERAS_HEIGHT)
#define DISPLAY_AERA2_YPOS			(DISPLAY_AERA1_YPOS + DISPLAY_AERAS_HEIGHT + DISPLAY_INTER_AERAS_HEIGHT)
#define DISPLAY_AERA3_YPOS			(DISPLAY_AERA2_YPOS + DISPLAY_AERAS_HEIGHT + DISPLAY_INTER_AERAS_HEIGHT)
#define DISPLAY_AERA4_YPOS			(DISPLAY_AERA3_YPOS + DISPLAY_AERAS_HEIGHT + DISPLAY_INTER_AERAS_HEIGHT)
#define DISPLAY_AERA5_YPOS			(DISPLAY_AERA4_YPOS + DISPLAY_AERAS_HEIGHT + DISPLAY_INTER_AERAS_HEIGHT)

/* Includes ------------------------------------------------------------------*/

/* Private defines -----------------------------------------------------------*/

int cisynth_ifft(void);

#endif /* __SYNTH_V3_H */
