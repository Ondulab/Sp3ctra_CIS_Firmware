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
/*******************              general definitions               *******************/
/**************************************************************************************/
#define USE_BOTLOADER
#define VERSION	"3.1"

/**************************************************************************************/
/********************              debug definitions               ********************/
/**************************************************************************************/
//#define PRINTF_CM4
//#define SKIP_SPLASH
//#define ETHERNET_OFF
//#define CIS_DESACTIVATE_CALIBRATION

/**************************************************************************************/
/********************             	HID definitions                ********************/
/**************************************************************************************/
#define DEFAULT_UI_BUTTON_DELAY 				(100)
#define DEFAULT_CIS_HANDEDNESS 1

#define BANNER_BACKGROUND_COLOR					(3)

#define DISPLAY_WIDTH							SSD1362_WIDTH
#define DISPLAY_HEIGHT							SSD1362_HEIGHT

#define DISPLAY_HEAD_HEIGHT						(9)

#define DISPLAY_AERAS1_HEIGHT					(47)
#define DISPLAY_AERAS2_HEIGHT					(16)
#define DISPLAY_INTER_AERAS_HEIGHT				(1)

#define DISPLAY_HEAD_Y1POS						(0)
#define DISPLAY_HEAD_Y2POS						(DISPLAY_HEAD_HEIGHT)

#define DISPLAY_AERA1_Y1POS						(0)//(DISPLAY_HEAD_Y2POS + DISPLAY_INTER_AERAS_HEIGHT)
#define DISPLAY_AERA1_Y2POS						(DISPLAY_AERA1_Y1POS + DISPLAY_AERAS1_HEIGHT)

#define DISPLAY_AERA2_Y1POS						(DISPLAY_AERA1_Y2POS + DISPLAY_INTER_AERAS_HEIGHT)
#define DISPLAY_AERA2_Y2POS						(DISPLAY_AERA2_Y1POS + DISPLAY_AERAS2_HEIGHT)

#define WINDOW_IMU_AVERAGE_SIZE 				(5)  // Window size for the moving average

#define	NUMBER_OF_BUTTONS						(3)
#define	NUMBER_OF_LEDS							(3)

/**************************************************************************************/
/******************              Ethernet definitions               *******************/
/**************************************************************************************/
//#define UDP_NB_PACKET_PER_LINE					(12)
//#define UDP_PACKET_SIZE							((CIS_PIXELS_NB) / (UDP_NB_PACKET_PER_LINE))

#define UDP_NB_PACKET_PER_LINE					(8)
#define UDP_LINE_FRAGMENT_SIZE					(CIS_MAX_PIXELS_PER_LANE / UDP_NB_PACKET_PER_LINE)

#define LWIP_DEBUG 1

// Network configurations
#define DEFAULT_NETWORK_IP 						{192, 168, 0, 10}
#define DEFAULT_NETWORK_NETMASK 				{255, 255, 255, 0}
#define DEFAULT_NETWORK_GW 						{0, 0, 0, 0}
#define DEFAULT_NETWORK_DEST_IP 				{192, 168, 0, 255}
#define DEFAULT_NETWORK_UDP_PORT 				(55151)
#define DEFAULT_NETWORK_TCP_PORT 				(5000)

/**************************************************************************************/
/********************              CIS definitions                 ********************/
/**************************************************************************************/
//#define POLYNOMIAL_CALIBRATION

// CIS configurations
#define DEFAULT_CIS_PRINT_CALIBRATION 0
#define DEFAULT_CIS_RAW 0
#define DEFAULT_CIS_DPI 200
#define DEFAULT_CIS_MONOCHROME 0
#define DEFAULT_CIS_MAX_LINE_FREQ 900

//#define DEFAULT_CIS_CLK_FREQ							(2500000)
#define   DEFAULT_CIS_CLK_FREQ					(3125000)
//#define DEFAULT_CIS_CLK_FREQ							(3200000)
//#define DEFAULT_CIS_CLK_FREQ							(4000000)
//#define DEFAULT_CIS_CLK_FREQ							(5000000)

#define DEFAULT_CIS_OVERSAMPLING 8



#define CIS_ADC_OUT_LANES						(3)

#define CIS_SP_WIDTH							(2)
#define CIS_INACTIVE_WIDTH						(38 + CIS_SP_WIDTH)
#define CIS_OVER_SCAN							(12)

#define CIS_MAX_PIXELS_PER_LANE					(1152)
#define CIS_MAX_PIXEL_AERA_STOP					((CIS_INACTIVE_WIDTH) + (CIS_MAX_PIXELS_PER_LANE))

#define CIS_MAX_PIXELS_NB 		 				((CIS_MAX_PIXELS_PER_LANE) * (CIS_ADC_OUT_LANES))
#define CIS_MAX_LANE_SIZE 						(CIS_MAX_PIXEL_AERA_STOP + CIS_OVER_SCAN)


#if 0
#ifdef CIS_400DPI
#define CIS_PIXELS_PER_LANE						(1152)
#else
#define CIS_PIXELS_PER_LANE						(576)
#endif
#define CIS_PIXELS_NB 		 					((CIS_PIXELS_PER_LANE) * (CIS_ADC_OUT_LANES))

#define CIS_PIXEL_AERA_STOP						((CIS_INACTIVE_WIDTH) + (CIS_PIXELS_PER_LANE))


#define CIS_START_OFFSET	 	 				(CIS_INACTIVE_WIDTH - CIS_SP_WIDTH + 2)
#define CIS_LANE_SIZE 							(CIS_PIXEL_AERA_STOP + CIS_OVER_SCAN)
#define CIS_END_CAPTURE							(CIS_LANE_SIZE)
#endif

#define CIS_LED_RED_ON							(CIS_INACTIVE_WIDTH + 30)
#define CIS_LED_GREEN_ON						(CIS_INACTIVE_WIDTH + 30)
#define CIS_LED_BLUE_ON							(CIS_INACTIVE_WIDTH + 30)

// LED illumination durations in microseconds
#define LED_RED_DURATION_US     				(178)  // Duration in microseconds for the red LED
#define LED_GREEN_DURATION_US   				(178)  // Duration in microseconds for the green LED
#define LED_BLUE_DURATION_US    				(178)  // Duration in microseconds for the blue LED

// Ensure DEFAULT_CIS_CLK_FREQ is defined to avoid issues
#define CYCLE_DURATION_US       				((float)(1000000.0f / DEFAULT_CIS_CLK_FREQ))

// Macro to calculate LED off index based on illumination duration
#define LED_OFF_INDEX(duration_us)  			((int)((duration_us) / CYCLE_DURATION_US))

// Safety check for LED illumination durations
//#define SAFE_LED_OFF_INDEX(duration_us)  		(LED_OFF_INDEX(duration_us) <= CIS_END_CAPTURE ? LED_OFF_INDEX(duration_us) : CIS_END_CAPTURE)

// Calculating off indices for each LED with safety checks
#define CIS_LED_RED_OFF       					(CIS_LED_RED_ON + LED_OFF_INDEX(LED_RED_DURATION_US))
#define CIS_LED_GREEN_OFF     					(CIS_LED_GREEN_ON + LED_OFF_INDEX(LED_GREEN_DURATION_US))
#define CIS_LED_BLUE_OFF      					(CIS_LED_BLUE_ON + LED_OFF_INDEX(LED_BLUE_DURATION_US))

#define CIS_MAX_ADC_BUFF_SIZE 	 	 		 	((CIS_MAX_LANE_SIZE) * (CIS_ADC_OUT_LANES))

#define CIS_ADC_MAX_VALUE						(4095)

#define CIS_LEDS_MAX_PWM						(101)
#define CIS_LEDS_MAX_POMER						(CIS_LEDS_MAX_PWM)

/**************************************************************************************/
/********************              GYRO definitions                ********************/
/**************************************************************************************/
#define ICM42688P

#define IMU_CLKIN_FREQ							(32000)

#endif // __CONFIG_H__

