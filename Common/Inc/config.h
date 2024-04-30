/**
 ******************************************************************************
 * @file           : config.h
 ******************************************************************************
 * @attention
 *
 * Copyright (C) 2018-present Reso-nance Numerique.
 * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
 *
 ******************************************************************************
 */

#ifndef __CONFIG_H__
#define __CONFIG_H__

#include "stm32h7xx_hal.h"
/**************************************************************************************/
/********************              debug definitions               ********************/
/**************************************************************************************/
//#define SKIP_SPLASH
//#define ETHERNET_OFF
//#define CIS_DESACTIVATE_CALIBRATION

/**************************************************************************************/
/********************             	HID definitions                ********************/
/**************************************************************************************/
#define BANNER_BACKGROUND_COLOR		(3)

#define DISPLAY_WIDTH				SSD1362_WIDTH
#define DISPLAY_HEIGHT				SSD1362_HEIGHT

#define DISPLAY_HEAD_HEIGHT			(9)

#define DISPLAY_AERAS1_HEIGHT		(47)
#define DISPLAY_AERAS2_HEIGHT		(16)
#define DISPLAY_INTER_AERAS_HEIGHT	(1)

#define DISPLAY_HEAD_Y1POS			(0)
#define DISPLAY_HEAD_Y2POS			(DISPLAY_HEAD_HEIGHT)

#define DISPLAY_AERA1_Y1POS			(0)//(DISPLAY_HEAD_Y2POS + DISPLAY_INTER_AERAS_HEIGHT)
#define DISPLAY_AERA1_Y2POS			(DISPLAY_AERA1_Y1POS + DISPLAY_AERAS1_HEIGHT)

#define DISPLAY_AERA2_Y1POS			(DISPLAY_AERA1_Y2POS + DISPLAY_INTER_AERAS_HEIGHT)
#define DISPLAY_AERA2_Y2POS			(DISPLAY_AERA2_Y1POS + DISPLAY_AERAS2_HEIGHT)

#define WINDOW_IMU_AVERAGE_SIZE 	(5)  // Window size for the moving average

/**************************************************************************************/
/******************              Ethernet definitions               *******************/
/**************************************************************************************/
#define UDP_NB_PACKET_PER_LINE					(12)
#define UDP_PACKET_SIZE							((CIS_PIXELS_NB) / (UDP_NB_PACKET_PER_LINE))

#define LWIP_DEBUG 1

/**************************************************************************************/
/********************              CIS definitions                 ********************/
/**************************************************************************************/
//#define POLYNOMIAL_CALIBRATION

//#define CIS_CLK_FREQ							(2500000)
//#define CIS_CLK_FREQ							(3125000)
//#define CIS_CLK_FREQ							(3200000)
//#define CIS_CLK_FREQ							(4000000)
//#define CIS_CLK_FREQ							(5000000)

#define CIS_ADC_OUT_LANES						(3)

#define CIS_SP_WIDTH							(2)

#define CIS_INACTIVE_WIDTH						(38 + CIS_SP_WIDTH)

#ifdef CIS_400DPI
#define CIS_PIXELS_PER_LANE						(1152)
#else
#define CIS_PIXELS_PER_LANE						(576)
#endif
#define CIS_PIXELS_NB 		 					((CIS_PIXELS_PER_LANE) * (CIS_ADC_OUT_LANES))

#define CIS_PIXEL_AERA_STOP						((CIS_INACTIVE_WIDTH) + (CIS_PIXELS_PER_LANE))
#define CIS_OVER_SCAN							(12)

#define CIS_START_OFFSET	 	 				(CIS_INACTIVE_WIDTH - CIS_SP_WIDTH + 2)
#define CIS_LANE_SIZE 							(CIS_PIXEL_AERA_STOP + CIS_OVER_SCAN)
#define CIS_END_CAPTURE							(CIS_LANE_SIZE)

#define CIS_LED_RED_ON							(CIS_INACTIVE_WIDTH + 30)
#define CIS_LED_GREEN_ON						(CIS_INACTIVE_WIDTH + 30)
#define CIS_LED_BLUE_ON							(CIS_INACTIVE_WIDTH + 30)
#define CIS_LED_RED_OFF							(CIS_END_CAPTURE)//((202.0 * 2.5))
#define CIS_LED_GREEN_OFF						(CIS_END_CAPTURE)//((244.0 * 2.5))
#define CIS_LED_BLUE_OFF						(CIS_END_CAPTURE)//((243.0 * 2.5))

#define CIS_ADC_BUFF_SIZE 	 	 		 		((CIS_LANE_SIZE) * (CIS_ADC_OUT_LANES))

#define CIS_ADC_MAX_VALUE						(4095)

#define CIS_LEDS_MAX_PWM						(101)
#define CIS_LEDS_MAX_POMER						(CIS_LEDS_MAX_PWM)

/**************************************************************************************/
/********************              GYRO definitions                ********************/
/**************************************************************************************/
#define ICM42688P

#define IMU_CLKIN_FREQ							(32000)

#endif // __CONFIG_H__

