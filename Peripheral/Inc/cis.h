/**
  ******************************************************************************
  * @file           : cis.h
  * @brief          : Header for cis.c file.
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __CIS_H__
#define __CIS_H__

#define HI_TIME_FREQ 		(200000)
#define DAC_TIME_FREQ 		(20) // 1Hz to 1Khz
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

enum color { RED, GREEN, BLUE };

/* Includes ------------------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/

void cisInit(void);

#endif /* __CIS_H__ */
