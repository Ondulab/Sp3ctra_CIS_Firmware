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
#define FW_VERSION "3.10.0"

/**************************************************************************************/
/********************              Debug definitions               ********************/
/**************************************************************************************/
//#define PRINTF_CM4

//#define DEBUG_LWIP_STATS
//#define HTTP_SERVER_DEBUG

//#define CIS_PRINT_COUNTER

//#define DEBUG_ICM42688

//#define USE_WDG

// CIS Drift Correction Configuration
#define CIS_DRIFT_DEBUG_ENABLED                 (0)     // Debug drift correction (0=disabled, 1=enabled)
#define CIS_DRIFT_DEBUG_INTERVAL                (100)   // Print debug info every N lines
#define CIS_DETAILED_DEBUG_ENABLED              (0)     // Detailed debug with pixel values (0=disabled, 1=enabled)
#define CIS_DETAILED_DEBUG_INTERVAL             (100)   // Print detailed debug every N lines

/**************************************************************************************/
/********************              GUI definitions                 ********************/
/**************************************************************************************/
#define GUI_SHOW_IMU 0   		// Set to 0 to hide IMU panel and use full image area
#define GUI_INVERT_DISPLAY 1  	// Set to 1 to invert display colors (white background, dark content)

// Screensaver configuration
#define SCREENSAVER_TIMEOUT_MS                  (6000)  // 60 seconds timeout for testing
#define MOTION_THRESHOLD_ACC                    (0.08f)  // Accelerometer motion threshold (g)
#define MOTION_THRESHOLD_GYRO                   (2.0f)   // Gyroscope motion threshold (dps)

/**************************************************************************************/
/*******************              Storage definitions               *******************/
/**************************************************************************************/
#define FILE_NAME_MAX_LENGTH                    (256)  //Max filename length

#define CALIBRATION_FILE_PATH_FORMAT 			"0:/CIS_CALIB_%ddpi.BIN"
#define CONFIG_FILE_PATH 						"0:/CONFIG.TXT"
#define IMU_CALIBRATION_FILE_PATH 				"0:/IMU_CALIB.BIN"

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
#define DEFAULT_NETWORK_IP 						{192, 168, 100, 1}
#define DEFAULT_NETWORK_NETMASK 				{255, 255, 255, 0}
#define DEFAULT_NETWORK_GW 						{0, 0, 0, 0}
#define DEFAULT_NETWORK_DEST_IP 				{192, 168, 100, 255}
#define DEFAULT_NETWORK_UDP_PORT 				(55151)
#define DEFAULT_NETWORK_TCP_PORT 				(5000)

/**************************************************************************************/
/********************              CIS definitions                 ********************/
/**************************************************************************************/
//#define POLYNOMIAL_CALIBRATION

// CIS configurations
#define DEFAULT_CIS_PRINT_CALIBRATION 0
#define DEFAULT_CIS_RAW 0
#define DEFAULT_CIS_DPI 400
#define DEFAULT_CIS_MAX_LINE_FREQ 900
#define DEFAULT_CIS_OVERSAMPLING 2

//#define DEFAULT_CIS_CLK_FREQ					(2500000)
//#define   DEFAULT_CIS_CLK_FREQ				(3125000)
//#define DEFAULT_CIS_CLK_FREQ					(3200000)
#define DEFAULT_CIS_CLK_FREQ					(4000000)
//#define DEFAULT_CIS_CLK_FREQ					(5000000)

#define CIS_CAPTURE_TIMEOUT 					(100)

#define CIS_ADC_OUT_LANES						(3)
#define COLOR_CHANNELS        			 		(3)
#define CIS_SP_WIDTH							(2)
#define CIS_BLACK_PIXELS						(38)
#define CIS_IGNORE_FIRST_BLACK_PIXELS			(8)
#define CIS_USEFUL_BLACK_PIXELS					(24)
#define CIS_INACTIVE_WIDTH						((CIS_BLACK_PIXELS)+ (CIS_SP_WIDTH))
#define CIS_OVER_SCAN							(12)

#define CIS_400DPI_PIXELS_PER_LANE		        (1152)
#define CIS_200DPI_PIXELS_PER_LANE		        (576)

#define CIS_400DPI_PIXELS_NB					(3456)
#define CIS_200DPI_PIXELS_NB					(1728)

#define CIS_MAX_PIXELS_PER_LANE					(CIS_400DPI_PIXELS_PER_LANE)
#define CIS_MAX_PIXELS_NB 		 				(CIS_400DPI_PIXELS_NB)

#define CIS_MAX_PIXEL_AERA_STOP					((CIS_INACTIVE_WIDTH) + (CIS_MAX_PIXELS_PER_LANE))

#define CIS_MAX_LANE_SIZE 						((CIS_MAX_PIXEL_AERA_STOP) + (CIS_OVER_SCAN))

#define CIS_LED_ON								(CIS_SP_WIDTH)

// LED illumination durations in microseconds
#define CIS_400DPI_LED_DURATION_US              (300)  // Duration in microseconds
#define CIS_200DPI_LED_DURATION_US              (150)  // Duration in microseconds

#define CIS_MAX_ADC_BUFF_SIZE 	 	 		    ((CIS_MAX_LANE_SIZE) * (COLOR_CHANNELS))
#define CIS_MAX_USEFUL_DATA_SIZE 			    (((CIS_BLACK_PIXELS) + (CIS_400DPI_PIXELS_PER_LANE)) * (COLOR_CHANNELS))

#define CIS_ADC_MAX_VALUE                       (1024)

#define CIS_LEDS_MAX_PWM                        (101)
#define CIS_LEDS_MAX_POMER		                (CIS_LEDS_MAX_PWM)

#define CIS_DRIFT_THRESHOLD                     (100)   // Maximum allowed drift in ADC counts

// Calibration segmentée
#define CIS_INTERMEDIATE_LED_POWER              (30)    // Puissance LED intermédiaire (50%)

// Format Q8.8 pour les gains (optimisation mémoire)
#define UNITY_Q8_8                              (1 << 8)    // 1.0 en format Q8.8
#define CLIP_INT16(x)                           ((x) > 32767 ? 32767 : ((x) < -32768 ? -32768 : (x)))

/**************************************************************************************/
/***                            Packet Management Definitions                      ***/
/**************************************************************************************/

// Number of UDP packets per line
#define UDP_MAX_NB_PACKET_PER_LINE              (12)

// Ensure UDP_LINE_FRAGMENT_SIZE is an integer
#if (CIS_MAX_PIXELS_NB % UDP_MAX_NB_PACKET_PER_LINE) != 0
  #error "CIS_MAX_PIXELS_NB must be divisible by UDP_NB_PACKET_PER_LINE."
#endif

// Size of each UDP line fragment (number of pixels per packet)
#define UDP_LINE_FRAGMENT_SIZE                  (CIS_MAX_PIXELS_NB / UDP_MAX_NB_PACKET_PER_LINE)

/**************************************************************************************/
/********************              GYRO definitions                ********************/
/**************************************************************************************/
#define ICM42688P

#define IMU_CLKIN_FREQ			                (32000)

// Gyroscope sensitivity configuration for handheld usage
// Available options: dps2000, dps1000, dps500, dps250, dps125, dps62_5, dps31_25, dps15_625
#define DEFAULT_GYRO_SENSITIVITY                dps250  // Lower sensitivity for better precision in handheld use

// Accelerometer sensitivity configuration for handheld usage
// Available options: gpm16, gpm8, gpm4, gpm2
#define DEFAULT_ACCEL_SENSITIVITY		        gpm4    // Moderate sensitivity for handheld movement detection

// Calibration sample count for handheld usage (reduced for faster startup)
#define HANDHELD_CALIB_SAMPLES				    (50)      // Reduced from 100 for faster calibration

#endif // __CONFIG_H__
