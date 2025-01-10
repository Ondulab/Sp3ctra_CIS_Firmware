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
#define USE_BOTLOADER
#define SHORT_VERSION	"3.5"
#define VERSION "3.5.0"

/**************************************************************************************/
/********************              Debug definitions               ********************/
/**************************************************************************************/
//#define PRINTF_CM4
//#define SKIP_SPLASH

//#define DEBUG_LWIP_STATS
//#define HTTP_SERVER_DEBUG

//#define CIS_DESACTIVATE_CALIBRATION
//#define CIS_PRINT_COUNTER

/**************************************************************************************/
/****************              Flash Address definitions               ****************/
/**************************************************************************************/
#define FLASH_PERSISTENT_DATA_ADDRESS 			((uint32_t)0x08020000)

#define FLASH_BASE_ADDR                      	(uint32_t)(FLASH_BASE)
#define FLASH_END_ADDR                       	(uint32_t)(0x081FFFFF)

/* Base address of the Flash sectors Bank 1 */
#define ADDR_FLASH_SECTOR_0_BANK1            	((uint32_t)0x08000000) /* Base @ of Sector 0, 128 Kbytes */
#define ADDR_FLASH_SECTOR_1_BANK1            	((uint32_t)0x08020000) /* Base @ of Sector 1, 128 Kbytes */
#define ADDR_FLASH_SECTOR_2_BANK1            	((uint32_t)0x08040000) /* Base @ of Sector 2, 128 Kbytes */
#define ADDR_FLASH_SECTOR_3_BANK1            	((uint32_t)0x08060000) /* Base @ of Sector 3, 128 Kbytes */
#define ADDR_FLASH_SECTOR_4_BANK1            	((uint32_t)0x08080000) /* Base @ of Sector 4, 128 Kbytes */
#define ADDR_FLASH_SECTOR_5_BANK1            	((uint32_t)0x080A0000) /* Base @ of Sector 5, 128 Kbytes */
#define ADDR_FLASH_SECTOR_6_BANK1            	((uint32_t)0x080C0000) /* Base @ of Sector 6, 128 Kbytes */
#define ADDR_FLASH_SECTOR_7_BANK1            	((uint32_t)0x080E0000) /* Base @ of Sector 7, 128 Kbytes */

/* Base address of the Flash sectors Bank 2 */
#define ADDR_FLASH_SECTOR_0_BANK2            	((uint32_t)0x08100000) /* Base @ of Sector 0, 128 Kbytes */
#define ADDR_FLASH_SECTOR_1_BANK2            	((uint32_t)0x08120000) /* Base @ of Sector 1, 128 Kbytes */
#define ADDR_FLASH_SECTOR_2_BANK2            	((uint32_t)0x08140000) /* Base @ of Sector 2, 128 Kbytes */
#define ADDR_FLASH_SECTOR_3_BANK2            	((uint32_t)0x08160000) /* Base @ of Sector 3, 128 Kbytes */
#define ADDR_FLASH_SECTOR_4_BANK2            	((uint32_t)0x08180000) /* Base @ of Sector 4, 128 Kbytes */
#define ADDR_FLASH_SECTOR_5_BANK2            	((uint32_t)0x081A0000) /* Base @ of Sector 5, 128 Kbytes */
#define ADDR_FLASH_SECTOR_6_BANK2            	((uint32_t)0x081C0000) /* Base @ of Sector 6, 128 Kbytes */
#define ADDR_FLASH_SECTOR_7_BANK2            	((uint32_t)0x081E0000) /* Base @ of Sector 7, 128 Kbytes */

#define FLASH_LAST_SECTOR_ADDR   (FLASH_END_ADDR - FLASH_SECTOR_SIZE + 1)

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
#define DEFAULT_CIS_CLK_FREQ					(3200000)
//#define DEFAULT_CIS_CLK_FREQ					(4000000)
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


#define CIS_ADC_MAX_VALUE						(4095)

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

