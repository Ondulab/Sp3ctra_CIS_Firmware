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
/*******************              General definitions               *******************/
/**************************************************************************************/
#define FW_VERSION "3.8.0"

/**************************************************************************************/
/********************              Debug definitions               ********************/
/**************************************************************************************/
//#define PRINTF_CM4
//#define SKIP_SPLASH

//#define DEBUG_LWIP_STATS
//#define HTTP_SERVER_DEBUG

//#define CIS_DESACTIVATE_CALIBRATION
//#define CIS_PRINT_COUNTER

//#define USE_WDG

/**************************************************************************************/
/*******************              Storage definitions               *******************/
/**************************************************************************************/
#define FILE_NAME_MAX_LENGTH 256  //Max filename length

#define CALIBRATION_FILE_PATH_FORMAT "0:/CALIB_%ddpi.BIN"
#define CONFIG_FILE_PATH "0:/CONFIG.TXT"

/**************************************************************************************/
/********************             	HID definitions                ********************/
/**************************************************************************************/
#define DEFAULT_UI_BUTTON_DELAY 				(100)
#define DEFAULT_CIS_HANDEDNESS 					(1)

#define	NUMBER_OF_BUTTONS						(3)
#define	NUMBER_OF_LEDS							(3)

/**************************************************************************************/
/******************              Ethernet definitions               *******************/
/**************************************************************************************/
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
#define DEFAULT_CIS_OVERSAMPLING 8

//#define DEFAULT_CIS_CLK_FREQ					(2500000)
//#define   DEFAULT_CIS_CLK_FREQ				(3125000)
//#define DEFAULT_CIS_CLK_FREQ					(3200000)
#define DEFAULT_CIS_CLK_FREQ					(4000000)
//#define DEFAULT_CIS_CLK_FREQ					(5000000)

#define CIS_CAPTURE_TIMEOUT 					(100)

#define CIS_ADC_OUT_LANES						(3)

#define CIS_SP_WIDTH							(2)
#define CIS_INACTIVE_WIDTH						(38 + CIS_SP_WIDTH)
#define CIS_OVER_SCAN							(12)

#define CIS_400DPI_PIXELS_PER_LANE				(1152)
#define CIS_200DPI_PIXELS_PER_LANE				(576)

#define CIS_400DPI_PIXELS_NB					(3456)
#define CIS_200DPI_PIXELS_NB					(1728)

#define CIS_MAX_PIXELS_PER_LANE					(CIS_400DPI_PIXELS_PER_LANE)
#define CIS_MAX_PIXELS_NB 		 				(CIS_400DPI_PIXELS_NB)

#define CIS_MAX_PIXEL_AERA_STOP					((CIS_INACTIVE_WIDTH) + (CIS_MAX_PIXELS_PER_LANE))

#define CIS_MAX_LANE_SIZE 						(CIS_MAX_PIXEL_AERA_STOP + CIS_OVER_SCAN)

#define CIS_LED_ON								(CIS_INACTIVE_WIDTH + 30)

// LED illumination durations in microseconds
#define CIS_400DPI_LED_DURATION_US     			(356)  // Duration in microseconds
#define CIS_200DPI_LED_DURATION_US     			(178)  // Duration in microseconds

#define CIS_MAX_ADC_BUFF_SIZE 	 	 		 	((CIS_MAX_LANE_SIZE) * (CIS_ADC_OUT_LANES))


#define CIS_ADC_MAX_VALUE						(4096)

#define CIS_LEDS_MAX_PWM						(101)
#define CIS_LEDS_MAX_POMER						(CIS_LEDS_MAX_PWM)

/**************************************************************************************/
/***                            Packet Management Definitions                      ***/
/**************************************************************************************/

// Number of UDP packets per line
#define UDP_MAX_NB_PACKET_PER_LINE         		(12)

// Ensure UDP_LINE_FRAGMENT_SIZE is an integer
#if (CIS_MAX_PIXELS_NB % UDP_MAX_NB_PACKET_PER_LINE) != 0
  #error "CIS_MAX_PIXELS_NB must be divisible by UDP_NB_PACKET_PER_LINE."
#endif

// Size of each UDP line fragment (number of pixels per packet)
#define UDP_LINE_FRAGMENT_SIZE         			(CIS_MAX_PIXELS_NB / UDP_MAX_NB_PACKET_PER_LINE)

/**************************************************************************************/
/********************              GYRO definitions                ********************/
/**************************************************************************************/
#define ICM42688P

#define IMU_CLKIN_FREQ							(32000)

#endif // __CONFIG_H__

