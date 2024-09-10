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

/* Definition of firmware update states */
typedef enum {
    FW_UPDATE_NONE      = 0x00000000,
    FW_UPDATE_RECEIVED  = 0xAAAAAAAA,
    FW_UPDATE_FLASHED   = 0xBBBBBBBB,
    FW_UPDATE_TO_TEST   = 0xDDDDDDDD,
    FW_UPDATE_DONE      = 0xFFFFFFFF
} FW_UpdateState;

typedef struct {
    FW_UpdateState updateState;     //(4 octets)
    uint8_t padding[24];            // Alignement pour un total de 32 octets
} PersistentData;

HAL_StatusTypeDef STM32Flash_writePersistentData(PersistentData* data);
void STM32Flash_readPersistentData(PersistentData* data);
HAL_StatusTypeDef STM32Flash_erase_app_memory(uint32_t flashBank, uint32_t flashSector, uint32_t NbSectors);
HAL_StatusTypeDef STM32Flash_write32B(uint8_t *data, uint32_t address);

#endif /* __STM32_FLASH_H__ */
