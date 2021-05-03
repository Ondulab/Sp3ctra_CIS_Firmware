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
#define DISPLAY_REFRESH_FPS						(20)

/**************************************************************************************/
/********************                     math                     ********************/
/**************************************************************************************/
#define PI										(3.14159265359)

/**************************************************************************************/
/********************              Synth definitions               ********************/
/**************************************************************************************/
#define SAMPLING_FREQUENCY				      	(48000)

#define IFFT_GAP_PER_MS							(10000)

/**************************************************************************************/
/********************              DAC definitions                 ********************/
/**************************************************************************************/

/**************************************************************************************/
/********************              CIS definitions                 ********************/
/**************************************************************************************/
//#define CIS_400DPI
#define CIS_BW
//#define CIS_INVERT_COLOR
//#define CIS_INVERT_COLOR_SMOOTH
//#define CIS_NORMAL_COLOR_SMOOTH

#define CIS_CLK_FREQ							(5000000)

#define CIS_SP_WIDTH							(2)
#define CIS_SP_ON								(10)
#define CIS_SP_OFF								(CIS_SP_ON + CIS_SP_WIDTH)
#define CIS_LED_ON								(CIS_SP_OFF + 4)
#define CIS_INACTIVE_WIDTH						(38)
#define CIS_INACTIVE_AERA_STOP					(CIS_INACTIVE_WIDTH + CIS_SP_ON)

#ifdef CIS_BW
#define CIS_LED_RED_OFF							(1200)//900
#define CIS_LED_GREEN_OFF						(1200)
#define CIS_LED_BLUE_OFF						(1200)
#else
#define CIS_LED_RED_OFF							(3600)
#define CIS_LED_GREEN_OFF						(3100)
#define CIS_LED_BLUE_OFF						(1600)
#endif

#ifdef CIS_400DPI
#define CIS_ACTIVE_PIXELS_PER_LINE				(1152)
#else
#define CIS_ACTIVE_PIXELS_PER_LINE				(576)
#endif
#define CIS_PIXEL_AERA_STOP						((CIS_ACTIVE_PIXELS_PER_LINE) + (CIS_INACTIVE_AERA_STOP))
#define CIS_OVER_SCAN							(64)
#define CIS_END_CAPTURE 						(CIS_PIXEL_AERA_STOP + CIS_OVER_SCAN)

#define CIS_ADC_OUT_LINES						(3)

#ifdef CIS_400DPI
#define CIS_IFFT_OVERSAMPLING_RATIO				(16)
#else
#define CIS_IFFT_OVERSAMPLING_RATIO				(8)
#endif

#define CIS_IMGPLY_OVERSAMPLING_RATIO			(16)

/**************************************************************************************/
/********************         Wave generation definitions          ********************/
/**************************************************************************************/
#define WAVE_AMP_RESOLUTION 					(65535)   	//in decimal
#define START_FREQUENCY     					(30)
#define MAX_OCTAVE_NUMBER   					(10)
#define SEMITONE_PER_OCTAVE 					(12)
#define COMMA_PER_SEMITONE  					(2)

#define NUMBER_OF_NOTES     					(((CIS_ACTIVE_PIXELS_PER_LINE) * (CIS_ADC_OUT_LINES)) / CIS_IFFT_OVERSAMPLING_RATIO)

#endif // __CONFIG_H__

