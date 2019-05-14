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
//#define PRINT_FREQUENCY
//#define DEBUG_SAMPLE_RATE

/**************************************************************************************/
/********************                     math                     ********************/
/**************************************************************************************/
#define PI					(3.14159265359)

/**************************************************************************************/
/********************              Synth definitions               ********************/
/**************************************************************************************/
#define SAMPLING_FREQUENCY 	(30100)	  	//in hertz

/**************************************************************************************/
/********************              DAC definitions                 ********************/
/**************************************************************************************/
#define DAC_TIME_FREQ 		(40000)//SAMPLING_FREQUENCY)
//#define DAC_MAX_LATENCY   (1000)   	//in millisecond
//#define DAC_BUFF_LEN		((SAMPLING_FREQUENCY) / (1000) * (DAC_MAX_LATENCY))

/**************************************************************************************/
/********************              CIS definitions                 ********************/
/**************************************************************************************/
#define CIS_CLK_FREQ		(100000)	//in hertz
#define SENSIVITY_THRESHOLD (10000)		//threshold for detection
#define CIS_RESOLUTION      (65535)   	//in decimal
#define CIS_PIXELS_NB		(2579)		//2579 active pixels

#define CIS_CAL_AERA_SIZE   (32)  		//fixed by manufacturer

#define LED_ON
//#define BLACK_AND_WITHE

/**************************************************************************************/
/********************         Wave generation definitions          ********************/
/**************************************************************************************/
#define WAVE_AMP_RESOLUTION (65535)   	//in decimal
#define START_FREQUENCY     (43.654)  	//FA 1
#define MAX_OCTAVE_NUMBER   (10)
#define SEMITONE_PER_OCTAVE (12)
#define COMMA_PER_SEMITONE  (4.5)		//4.5
#define PIXEL_PER_COMMA     (9)         //6

#define NUMBER_OF_NOTES     ((CIS_PIXELS_NB) / (PIXEL_PER_COMMA))

#endif // __CONFIG_H__

