/* config.h */

#ifndef __CONFIG_H__
#define __CONFIG_H__

/**************************************************************************************/
/********************              STM32 definitions               ********************/
/**************************************************************************************/
#define ADC_VOLTAGE			(3300)                    //STM32 ADC peripheral reference is 3300mV
#define ADC_COEFF 			((ADC_VOLTAGE) / (4095))  //ADC value/mV

/**************************************************************************************/
/********************              debug definitions               ********************/
/**************************************************************************************/
#define PRINT_FREQUENCY
//#define DEBUG_CIS
#define DEBUG_SYNTH
//#define DEBUG_SAMPLE_RATE

/**************************************************************************************/
/********************                     math                     ********************/
/**************************************************************************************/
#define PI					(3.14159265359)

/**************************************************************************************/
/********************              Synth definitions               ********************/
/**************************************************************************************/
#define SAMPLING_FREQUENCY 	(44100)	  	//in hertz

/**************************************************************************************/
/********************              DAC definitions                 ********************/
/**************************************************************************************/
#define DAC_TIME_FREQ 		(44100)//SAMPLING_FREQUENCY)

/**************************************************************************************/
/********************           Generation definitions             ********************/
/**************************************************************************************/
#define SENSIVITY_THRESHOLD (100)		    //threshold for detection

/**************************************************************************************/
/********************              CIS definitions                 ********************/
/**************************************************************************************/
#define CIS_CLK_FREQ					(1000000)	//3,5MHz in hertz
#define CIS_RESOLUTION      			(65535)   	//in decimal
#define CIS_PIXELS_NB					(5148)		//5148 active pixels
#define CIS_CLK_DUTY					(10)

#define CIS_SP_OFF						(10)
#define CIS_LED_ON						(79)
#define CIS_BLACK_PIX_AERA_START		(99)
#define CIS_DEAD_ZONE_AERA_START		(157)
#define CIS_PIXEX_AERA_START			(166)
#define CIS_LED_RED_OFF				(1578)
#define CIS_LED_BLUE_OFF				(2127)
#define CIS_LED_GREEN_OFF				(2475)
#define CIS_PIXEX_AERA_STOP				(5333)
#define CIS_END_CAPTURE 				(5696)

#define CIS_CAL_CYCLE		(10)		//number of cis cycle to read max pixel value

/**************************************************************************************/
/********************         Wave generation definitions          ********************/
/**************************************************************************************/
#define WAVE_AMP_RESOLUTION (65535)   	//in decimal
#define START_FREQUENCY     (60)  	//FA 1
#define MAX_OCTAVE_NUMBER   (10)
#define SEMITONE_PER_OCTAVE (12)
#define COMMA_PER_SEMITONE  (4.5)		//4.5
#define PIXEL_PER_COMMA     (14)        //12

#define NUMBER_OF_NOTES     ((CIS_PIXELS_NB) / (PIXEL_PER_COMMA))

#endif // __CONFIG_H__

