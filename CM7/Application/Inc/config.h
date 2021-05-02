/* config.h */

#ifndef __CONFIG_H__
#define __CONFIG_H__

#include "stm32h7xx_hal.h"

/**************************************************************************************/
/********************              STM32 definitions               ********************/
/**************************************************************************************/

/**************************************************************************************/
/********************              debug definitions               ********************/
/**************************************************************************************/
#define PRINT_FREQUENCY

/**************************************************************************************/
/********************             Display definitions              ********************/
/**************************************************************************************/
#define DISPLAY_REFRESH_FPS						(40)

/**************************************************************************************/
/********************                     math                     ********************/
/**************************************************************************************/
#define PI										(3.14159265359)

/**************************************************************************************/
/********************              Synth definitions               ********************/
/**************************************************************************************/
#define AUDIO_DEFAULT_VOLUME    				(80)

#define SAMPLING_FREQUENCY				      	(48000)

#define IFFT_GAP_PER_MS							(2000)

/**************************************************************************************/
/********************              DAC definitions                 ********************/
/**************************************************************************************/

/**************************************************************************************/
/********************              CIS definitions                 ********************/
/**************************************************************************************/
#define CIS_BW
#define CIS_INVERT_COLOR

#define CIS_CLK_FREQ							(1000000)

#define CIS_SP_OFF								(12)
#define CIS_LED_ON								(4)
#define CIS_INACTIVE_AERA_STOP					(48) //38

#ifdef CIS_BW
#define CIS_LED_RED_OFF							(1200)//900
#define CIS_LED_GREEN_OFF						(1200)
#define CIS_LED_BLUE_OFF						(1200)
#else
#define CIS_LED_RED_OFF							(3600)
#define CIS_LED_GREEN_OFF						(3100)
#define CIS_LED_BLUE_OFF						(1600)
#endif

#define CIS_ACTIVE_PIXELS_PER_LINE				(1152) //576
#define CIS_PIXEL_AERA_STOP						((CIS_ACTIVE_PIXELS_PER_LINE) + (CIS_INACTIVE_AERA_STOP))
#define CIS_OVER_SCAN							(64)
#define CIS_END_CAPTURE 						(CIS_PIXEL_AERA_STOP + CIS_OVER_SCAN)

#define CIS_ADC_OUT_LINES						(3)

#define CIS_IFFT_OVERSAMPLING_RATIO				(16)

#define CIS_IMGPLY_OVERSAMPLING_RATIO			(16)

/**************************************************************************************/
/********************         Wave generation definitions          ********************/
/**************************************************************************************/
#define WAVE_AMP_RESOLUTION 					(65535)   	//in decimal
#define START_FREQUENCY     					(70)
#define MAX_OCTAVE_NUMBER   					(10)
#define SEMITONE_PER_OCTAVE 					(12)
#define COMMA_PER_SEMITONE  					(5)

#define NUMBER_OF_NOTES     					(((CIS_ACTIVE_PIXELS_PER_LINE) * (CIS_ADC_OUT_LINES)) / CIS_IFFT_OVERSAMPLING_RATIO)

#endif // __CONFIG_H__

