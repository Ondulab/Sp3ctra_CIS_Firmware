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
#define DISPLAY_REFRESH_FPS						(10)

/**************************************************************************************/
/********************                     math                     ********************/
/**************************************************************************************/
#define PI										(3.14159265359)

/**************************************************************************************/
/********************              Synth definitions               ********************/
/**************************************************************************************/
#define AUDIO_DEFAULT_VOLUME    				(60)

//#define SAMPLING_FREQUENCY					  (96000)
//#define SAMPLING_FREQUENCY				      (48000)
#define SAMPLING_FREQUENCY					  	(44100)
//#define SAMPLING_FREQUENCY					  (32000)
//#define SAMPLING_FREQUENCY					  (22050)
//#define SAMPLING_FREQUENCY					  (16000)
//#define SAMPLING_FREQUENCY					  (11025)
//#define SAMPLING_FREQUENCY					  (8000)

#define IFFT_GAP_PER_MS							(2000)

/**************************************************************************************/
/********************              DAC definitions                 ********************/
/**************************************************************************************/

/**************************************************************************************/
/********************              CIS definitions                 ********************/
/**************************************************************************************/
#define CIS_BW
//#define CIS_INVERT_COLOR

#define CIS_CLK_FREQ							(1000000)

#define CIS_SP_OFF								(10)
#define CIS_LED_ON								(79)
#define CIS_BLACK_PIX_AERA_START				(99)
#define CIS_DEAD_ZONE_AERA_START				(157)
#define CIS_PIXEX_AERA_START					(224) //166

#ifdef CIS_BW
#define CIS_LED_RED_OFF							(3600 / 1)
#define CIS_LED_GREEN_OFF						(3100 / 1)
#define CIS_LED_BLUE_OFF						(1600 / 1)
#else
#define CIS_LED_RED_OFF							(3600)
#define CIS_LED_GREEN_OFF						(3100)
#define CIS_LED_BLUE_OFF						(1600)
#endif

#define CIS_PIXEX_AERA_STOP						(5264) //5333
#define CIS_END_CAPTURE 						(5696) //(5696)

#define CIS_IFFT_OVERSAMPLING_RATIO				(16)
#define CIS_IFFT_OVERSAMPLING_RIGHTBITSHIFT		ADC_RIGHTBITSHIFT_4

#define CIS_IMGPLY_OVERSAMPLING_RATIO			(4)
#define CIS_IMGPLY_OVERSAMPLING_RIGHTBITSHIFT	ADC_RIGHTBITSHIFT_2

#define CIS_OVERPRINT_CYCLES					(10)

/**************************************************************************************/
/********************         Wave generation definitions          ********************/
/**************************************************************************************/
#define WAVE_AMP_RESOLUTION 					(65535)   	//in decimal
#define START_FREQUENCY     					(7<W<<>0)
#define MAX_OCTAVE_NUMBER   					(20)
#define SEMITONE_PER_OCTAVE 					(12)
#define COMMA_PER_SEMITONE  					(5)	 //9

#define NUMBER_OF_NOTES     					(((CIS_PIXEX_AERA_STOP)-(CIS_PIXEX_AERA_START)) / (CIS_IFFT_OVERSAMPLING_RATIO))

#endif // __CONFIG_H__

