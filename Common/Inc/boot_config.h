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

/**************************************************************************************/
/****************                   Slots A / B                        ****************/
/**************************************************************************************/

/* Deux jeux d'images complets, pour que l'ecriture d'une mise a jour ne touche
 * jamais l'image en cours d'execution. Le rollback se reduit alors a continuer
 * de demarrer l'ancien slot : il devient instantane et ne peut plus echouer.
 *
 * Une image Cortex-M porte des adresses absolues (table de vecteurs, pools de
 * litteraux, table d'init de .data) : elle ne peut pas s'executer depuis deux
 * emplacements. Chaque coeur est donc lie DEUX fois, et le paquet embarque les
 * quatre images. Les linker scripts doivent rester identiques a ces valeurs.
 *
 *   0x08000000  128 Ko  bootloader
 *   0x08020000  128 Ko  journal OTA
 *   0x08040000  128 Ko  CM4 slot A
 *   0x08060000  128 Ko  CM4 slot B
 *   0x08080000  384 Ko  reserve
 *   0x080E0000  128 Ko  bootloader slot B (etape 3, non utilise)
 *   0x08100000  512 Ko  CM7 slot A
 *   0x08180000  512 Ko  CM7 slot B
 */

#define FW_SLOT_A 0u
#define FW_SLOT_B 1u
#define FW_SLOT_COUNT 2u

#define FW_CM4_SLOT_A_ADDR						(ADDR_FLASH_SECTOR_2_BANK1) /* 0x08040000 */
#define FW_CM4_SLOT_B_ADDR						(ADDR_FLASH_SECTOR_3_BANK1) /* 0x08060000 */
#define FW_CM7_SLOT_A_ADDR						(ADDR_FLASH_SECTOR_0_BANK2) /* 0x08100000 */
#define FW_CM7_SLOT_B_ADDR						(ADDR_FLASH_SECTOR_4_BANK2) /* 0x08180000 */

#define FW_CM4_MAX_SIZE							(FW_CM4_SLOT_B_ADDR - FW_CM4_SLOT_A_ADDR)   /* 128 Ko */
#define FW_CM7_MAX_SIZE							(FW_CM7_SLOT_B_ADDR - FW_CM7_SLOT_A_ADDR)   /* 512 Ko */

/* Emplacement reserve au second bootloader (etape 3 : OTA du bootloader). */
#define FW_BL_SLOT_B_ADDR						(ADDR_FLASH_SECTOR_7_BANK1) /* 0x080E0000 */

#define FW_CM4_SLOT_ADDR(slot)					(((slot) == FW_SLOT_B) ? FW_CM4_SLOT_B_ADDR : FW_CM4_SLOT_A_ADDR)
#define FW_CM7_SLOT_ADDR(slot)					(((slot) == FW_SLOT_B) ? FW_CM7_SLOT_B_ADDR : FW_CM7_SLOT_A_ADDR)
#define FW_SLOT_OTHER(slot)						(((slot) == FW_SLOT_B) ? FW_SLOT_A : FW_SLOT_B)
#define FW_SLOT_NAME(slot)						(((slot) == FW_SLOT_B) ? "B" : "A")

#define FLASH_PERSISTENT_DATA_ADDRESS 			(ADDR_FLASH_SECTOR_1_BANK1)

#endif // __BOOT_CONFIG_H__

