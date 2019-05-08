/* config.h */

#ifndef __CONFIG_H__
#define __CONFIG_H__

/**************************************************************************************/
/***************                 STM32 definitions                 ********************/
/**************************************************************************************/
#define ADC_VOLTAGE			(3300)                    //STM32 ADC peripheral reference is 3300mV
#define ADC_COEFF 			((ADC_VOLTAGE) / (4095))  //ADC value/mV

/**************************************************************************************/
/***************                 basic definitions                 ********************/
/**************************************************************************************/
#define PI					(3.14159265359)

/**************************************************************************************/
/***************                 Times definitions                 ********************/
/**************************************************************************************/
#define SAMPLING_FREQUENCY 	(30000)	  //in hertz
//#define HI_TIME_FREQ 		(200000)
#define DAC_TIME_FREQ 		(20) // 1Hz to 1Khz

/**************************************************************************************/
/***************            Wave generation definitions            ********************/
/**************************************************************************************/
#define WAVE_AMP_RESOLUTION (65535)   //in decimal
#define START_FREQUENCY     (43.654)  //FA 1
#define MAX_OCTAVE_NUMBER   (10)
#define SEMITONE_PER_OCTAVE (12)
#define COMMA_PER_SEMITONE  (4.5)
#define PIXEL_PER_COMMA     (6)

/**************************************************************************************/
/*********************            CIS definitions            **************************/
/**************************************************************************************/
#define SENSIVITY_THRESHOLD (10)							//threshold for detection
#define CIS_RESOLUTION      (65535)   						//in decimal
#define CIS_PIXELS_NB		(2700)
#define SEGMENT_NB			(11)
#define SEGMENT_WIDTH		((CIS_PIXELS_NB) / (SEGMENT_NB))
#define DEADZONE_WIDTH		(int)(CIS_PIXELS_NB * 0.04)
#define TOTAL_DEADZONE		(SEGMENT_NB * DEADZONE_WIDTH)
#define PIXEL_CNT_OFFSET	(11)

#define NOTE_ZONE			(200)
#define VOLUME_ZONE			(200)

#define LED_ON
//#define BLACK_AND_WITHE


#endif // __CONFIG_H__

