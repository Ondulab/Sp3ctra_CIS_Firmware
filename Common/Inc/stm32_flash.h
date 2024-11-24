/**
 ******************************************************************************
 * @file           : stm32_flash.h
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __STM32_FLASH_H__
#define __STM32_FLASH_H__

/* Includes ------------------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/

/* Custom return type for STM32 Flash operations -----------------------------*/
typedef enum {
    STM32FLASH_OK = 0,
    STM32FLASH_ERROR = 1
} STM32Flash_StatusTypeDef;

/* Definition of firmware update states */
typedef enum {
    FW_UPDATE_NONE      = 0x00000000,
    FW_UPDATE_RECEIVED  = 0xAAAAAAAA,
    FW_UPDATE_TO_TEST   = 0xDDDDDDDD,
    FW_UPDATE_DONE      = 0xFFFFFFFF
} FW_UpdateState;

STM32Flash_StatusTypeDef STM32Flash_readPersistentData(FW_UpdateState* state);
STM32Flash_StatusTypeDef STM32Flash_writePersistentData(FW_UpdateState updateState);
STM32Flash_StatusTypeDef STM32Flash_erase_app_memory(uint32_t flashBank, uint32_t flashSector, uint32_t NbSectors);
STM32Flash_StatusTypeDef STM32Flash_write32B(uint8_t *data, uint32_t address);
STM32Flash_StatusTypeDef STM32Flash_erase_sector(uint32_t flashBank, uint32_t sector);

#endif /* __STM32_FLASH_H__ */
