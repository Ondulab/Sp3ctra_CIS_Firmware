/**
 ******************************************************************************
 * @file           : boot_config.h
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

#ifndef __BOOT_CONFIG_H__
#define __BOOT_CONFIG_H__

//#include "stm32h7xx_hal.h"

/**************************************************************************************/
/*******************              General definitions               *******************/
/**************************************************************************************/
#define BL_VERSION	"1.2.0"

/**************************************************************************************/
/********************              Debug definitions               ********************/
/**************************************************************************************/

/**************************************************************************************/
/*******************              Storage definitions               *******************/
/**************************************************************************************/
#define FW_PATH "0:/firmware"

/**************************************************************************************/
/****************              Flash Address definitions               ****************/
/**************************************************************************************/
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

#define FLASH_LAST_SECTOR_ADDR   				(FLASH_END_ADDR - FLASH_SECTOR_SIZE + 1)

#define FW_CM4_START_ADDR 						(ADDR_FLASH_SECTOR_2_BANK1)
#define FW_CM7_START_ADDR 						(ADDR_FLASH_SECTOR_0_BANK2)
#define FW_CM4_MAX_SIZE							(ADDR_FLASH_SECTOR_7_BANK1 - FW_CM4_START_ADDR)
#define FW_CM7_MAX_SIZE							(ADDR_FLASH_SECTOR_7_BANK2 - FW_CM7_START_ADDR)
#define FLASH_PERSISTENT_DATA_ADDRESS 			(ADDR_FLASH_SECTOR_1_BANK1)

/**************************************************************************************/
/********************             	HID definitions                ********************/
/**************************************************************************************/
#if 0
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
#endif

#endif // __BOOT_CONFIG_H__

