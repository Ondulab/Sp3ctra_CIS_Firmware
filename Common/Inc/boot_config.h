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

/**************************************************************************************/
/*******************              General definitions               *******************/
/**************************************************************************************/
#define BL_VERSION	"1.2.1"

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

#endif // __BOOT_CONFIG_H__

