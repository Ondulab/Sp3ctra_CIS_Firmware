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
/********************              STM32 definitions               ********************/
/**************************************************************************************/
#define FLASH_BASE_ADDR      					(uint32_t)(FLASH_BASE)
#define FLASH_END_ADDR       					(uint32_t)(0x081FFFFF)

/* Base address of the Flash sectors Bank 1 */
#define ADDR_FLASH_SECTOR_0_BANK1    			((uint32_t)0x08000000) /* Base @ of Sector 0, 128 Kbytes */
#define ADDR_FLASH_SECTOR_1_BANK1     			((uint32_t)0x08020000) /* Base @ of Sector 1, 128 Kbytes */
#define ADDR_FLASH_SECTOR_2_BANK1     			((uint32_t)0x08040000) /* Base @ of Sector 2, 128 Kbytes */
#define ADDR_FLASH_SECTOR_3_BANK1     			((uint32_t)0x08060000) /* Base @ of Sector 3, 128 Kbytes */
#define ADDR_FLASH_SECTOR_4_BANK1     			((uint32_t)0x08080000) /* Base @ of Sector 4, 128 Kbytes */
#define ADDR_FLASH_SECTOR_5_BANK1     			((uint32_t)0x080A0000) /* Base @ of Sector 5, 128 Kbytes */
#define ADDR_FLASH_SECTOR_6_BANK1     			((uint32_t)0x080C0000) /* Base @ of Sector 6, 128 Kbytes */
#define ADDR_FLASH_SECTOR_7_BANK1     			((uint32_t)0x080E0000) /* Base @ of Sector 7, 128 Kbytes */

/* Base address of the Flash sectors Bank 2 */
#define ADDR_FLASH_SECTOR_0_BANK2     			((uint32_t)0x08100000) /* Base @ of Sector 0, 128 Kbytes */
#define ADDR_FLASH_SECTOR_1_BANK2     			((uint32_t)0x08120000) /* Base @ of Sector 1, 128 Kbytes */
#define ADDR_FLASH_SECTOR_2_BANK2     			((uint32_t)0x08140000) /* Base @ of Sector 2, 128 Kbytes */
#define ADDR_FLASH_SECTOR_3_BANK2     			((uint32_t)0x08160000) /* Base @ of Sector 3, 128 Kbytes */
#define ADDR_FLASH_SECTOR_4_BANK2     			((uint32_t)0x08180000) /* Base @ of Sector 4, 128 Kbytes */
#define ADDR_FLASH_SECTOR_5_BANK2     			((uint32_t)0x081A0000) /* Base @ of Sector 5, 128 Kbytes */
#define ADDR_FLASH_SECTOR_6_BANK2     			((uint32_t)0x081C0000) /* Base @ of Sector 6, 128 Kbytes */
#define ADDR_FLASH_SECTOR_7_BANK2     			((uint32_t)0x081E0000) /* Base @ of Sector 7, 128 Kbytes */

#define ADDR_CIS_FLASH_CALIBRATION				(ADDR_FLASH_SECTOR_7_BANK1)
#define ADDR_CIS_FLASH_PARAMETERS				(ADDR_FLASH_SECTOR_7_BANK2)

/**************************************************************************************/
/********************              debug definitions               ********************/
/**************************************************************************************/
//#define PRINT_CIS_CALIBRATION
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

#define BUTTON_DELAY				(500)

#define WINDOW_IMU_AVERAGE_SIZE 	(5)  // Window size for the moving average

/**************************************************************************************/
/******************              Ethernet definitions               *******************/
/**************************************************************************************/
#define LWIP_CLK_FREQ							(1000)			//in Hz

#define UDP_NB_PACKET_PER_LINE					(12)
#define UDP_PACKET_SIZE							((CIS_PIXELS_NB) / (UDP_NB_PACKET_PER_LINE))

/* UDP local connection port */
#define UDP_SERVER_PORT    						((uint16_t)55151U)
/* UDP remote connection port */
#define UDP_CLIENT_PORT   						((uint16_t)55151U)

/*Static IP ADDRESS: */
#define IP_ADDR0   								((uint8_t) 192U)
#define IP_ADDR1   								((uint8_t) 168U)
#define IP_ADDR2   								((uint8_t) 0U)
#define IP_ADDR3   								((uint8_t) 10U)

/*Static DESTINATION IP ADDRESS: */
#define DEST_IP_ADDR0   						((uint8_t) 192U)
#define DEST_IP_ADDR1   						((uint8_t) 168U)
#define DEST_IP_ADDR2   						((uint8_t) 0U)
#define DEST_IP_ADDR3   						((uint8_t) 255U)

/*NETMASK*/
#define NETMASK_ADDR0   						((uint8_t) 255U)
#define NETMASK_ADDR1   						((uint8_t) 255U)
#define NETMASK_ADDR2   						((uint8_t) 255U)
#define NETMASK_ADDR3   						((uint8_t) 0U)

/*Gateway Address*/
#define GW_ADDR0   								((uint8_t) 0U)
#define GW_ADDR1   								((uint8_t) 0U)
#define GW_ADDR2   								((uint8_t) 0U)
#define GW_ADDR3   								((uint8_t) 0U)

/**************************************************************************************/
/********************              CIS definitions                 ********************/
/**************************************************************************************/
//#define CIS_400DPI
//#define CIS_MONOCHROME

//#define CIS_CLK_FREQ							(2500000)
#define CIS_CLK_FREQ							(3125000)
//#define CIS_CLK_FREQ							(3200000)
//#define CIS_CLK_FREQ							(4000000)
//#define CIS_CLK_FREQ							(5000000)

#define CLK_DIVIDER 							((200000000) / (CIS_CLK_FREQ))
//#define CLK_DIVIDER 							((50000000) / (CIS_CLK_FREQ))

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

//#define POLYNOMIAL_CALIBRATION

/**************************************************************************************/
/********************              GYRO definitions                ********************/
/**************************************************************************************/
#define ICM42688P

#define IMU_CLKIN_FREQ							(32000)

#endif // __CONFIG_H__

