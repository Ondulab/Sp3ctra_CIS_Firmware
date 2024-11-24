/**
 ******************************************************************************
 * @file           : stm32_flash.c
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

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "config.h"
#include "basetypes.h"
#include "globals.h"

#include "stdlib.h"
#include "stdio.h"
#include "stdbool.h"

#include "arm_math.h"

#include "stm32_flash.h"

/* Private define ------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
bool update_requested = false;

typedef struct {
    FW_UpdateState updateState;     //(4 bytes)
    uint8_t padding[24];            // for 32 bytes alignment
} PersistentData;

/* Private function prototypes -----------------------------------------------*/
static uint32_t stm32_flashGetSector(uint32_t Address);

/**
 * @brief  Gets the sector of a given address.
 * @param  Address Address of the FLASH Memory.
 * @retval The sector of a given address.
 */
static uint32_t stm32_flashGetSector(uint32_t Address)
{
    if (((Address < ADDR_FLASH_SECTOR_1_BANK1) && (Address >= ADDR_FLASH_SECTOR_0_BANK1)) ||
        ((Address < ADDR_FLASH_SECTOR_1_BANK2) && (Address >= ADDR_FLASH_SECTOR_0_BANK2))) {
        return FLASH_SECTOR_0;
    }
    if (((Address < ADDR_FLASH_SECTOR_2_BANK1) && (Address >= ADDR_FLASH_SECTOR_1_BANK1)) ||
        ((Address < ADDR_FLASH_SECTOR_2_BANK2) && (Address >= ADDR_FLASH_SECTOR_1_BANK2))) {
        return FLASH_SECTOR_1;
    }
    if (((Address < ADDR_FLASH_SECTOR_3_BANK1) && (Address >= ADDR_FLASH_SECTOR_2_BANK1)) ||
        ((Address < ADDR_FLASH_SECTOR_3_BANK2) && (Address >= ADDR_FLASH_SECTOR_2_BANK2))) {
        return FLASH_SECTOR_2;
    }
    if (((Address < ADDR_FLASH_SECTOR_4_BANK1) && (Address >= ADDR_FLASH_SECTOR_3_BANK1)) ||
        ((Address < ADDR_FLASH_SECTOR_4_BANK2) && (Address >= ADDR_FLASH_SECTOR_3_BANK2))) {
        return FLASH_SECTOR_3;
    }
    if (((Address < ADDR_FLASH_SECTOR_5_BANK1) && (Address >= ADDR_FLASH_SECTOR_4_BANK1)) ||
        ((Address < ADDR_FLASH_SECTOR_5_BANK2) && (Address >= ADDR_FLASH_SECTOR_4_BANK2))) {
        return FLASH_SECTOR_4;
    }
    if (((Address < ADDR_FLASH_SECTOR_6_BANK1) && (Address >= ADDR_FLASH_SECTOR_5_BANK1)) ||
        ((Address < ADDR_FLASH_SECTOR_6_BANK2) && (Address >= ADDR_FLASH_SECTOR_5_BANK2))) {
        return FLASH_SECTOR_5;
    }
    if (((Address < ADDR_FLASH_SECTOR_7_BANK1) && (Address >= ADDR_FLASH_SECTOR_6_BANK1)) ||
        ((Address < ADDR_FLASH_SECTOR_7_BANK2) && (Address >= ADDR_FLASH_SECTOR_6_BANK2))) {
        return FLASH_SECTOR_6;
    }
    if (((Address < ADDR_FLASH_SECTOR_0_BANK2) && (Address >= ADDR_FLASH_SECTOR_7_BANK1)) ||
        ((Address < FLASH_END_ADDR) && (Address >= ADDR_FLASH_SECTOR_7_BANK2))) {
        return FLASH_SECTOR_7;
    }
    return FLASH_SECTOR_7;
}

/**
 * @brief  Reads the firmware update state from the flash memory.
 * @param  state Pointer to the FW_UpdateState variable to fill.
 * @retval STM32Flash_StatusTypeDef Status of the flash operation.
 */
STM32Flash_StatusTypeDef STM32Flash_readPersistentData(FW_UpdateState* state)
{
    if (state == NULL) {
        return STM32FLASH_ERROR; // Return error if the input pointer is invalid
    }

    const uint32_t flashAddress = FLASH_PERSISTENT_DATA_ADDRESS;
    const FW_UpdateState* flashState = (const FW_UpdateState*)flashAddress;

    *state = *flashState;
    return STM32FLASH_OK;
}

/**
 * @brief  Updates the firmware state in flash memory directly.
 * @param  updateState The new firmware update state to be written.
 * @retval STM32Flash_StatusTypeDef.
 */
STM32Flash_StatusTypeDef STM32Flash_writePersistentData(FW_UpdateState updateState)
{
    HAL_StatusTypeDef halStatus;
    FLASH_EraseInitTypeDef eraseInitStruct;
    uint32_t sectorError = 0;

    const uint32_t flashAddress = FLASH_PERSISTENT_DATA_ADDRESS;

    PersistentData dataToWrite = {
        .updateState = updateState,
        .padding = {0}
    };

    // Unlock the Flash memory
    halStatus = HAL_FLASH_Unlock();
    if (halStatus != HAL_OK) {
        return STM32FLASH_ERROR;
    }

    // Configure sector erase
    eraseInitStruct.TypeErase     = FLASH_TYPEERASE_SECTORS;
    eraseInitStruct.Sector        = stm32_flashGetSector(flashAddress);
    eraseInitStruct.NbSectors     = 1;
    eraseInitStruct.Banks         = FLASH_BANK_1;
    eraseInitStruct.VoltageRange  = FLASH_VOLTAGE_RANGE_3;

    // Erase the sector
    halStatus = HAL_FLASHEx_Erase(&eraseInitStruct, &sectorError);
    if (halStatus != HAL_OK) {
        HAL_FLASH_Lock();
        return STM32FLASH_ERROR;
    }

    // Write the data
    uint32_t* pData = (uint32_t*)&dataToWrite;
    halStatus = HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD, flashAddress, (uint32_t)pData);
    if (halStatus != HAL_OK) {
        HAL_FLASH_Lock();
        return STM32FLASH_ERROR;
    }

    // Lock the Flash memory
    HAL_FLASH_Lock();

    return STM32FLASH_OK;
}

/**
 * @brief  Writes 32 bytes of data into the flash at the specified address.
 * @param  data Pointer to the data to write (must be aligned to 32 bytes).
 * @param  address Flash address to write (must be aligned to 32 bytes).
 * @retval STM32Flash_StatusTypeDef.
 */
STM32Flash_StatusTypeDef STM32Flash_write32B(uint8_t* data, uint32_t address)
{
    if ((address % 32) != 0 || (((uint32_t)data) % 32) != 0) {
        return STM32FLASH_ERROR;
    }

    HAL_StatusTypeDef halStatus = HAL_FLASH_Unlock();
    if (halStatus != HAL_OK) {
        return STM32FLASH_ERROR;
    }

    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ALL_ERRORS_BANK1);
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ALL_ERRORS_BANK2);

    halStatus = HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD, address, (uint32_t)data);
    if (halStatus != HAL_OK) {
        HAL_FLASH_Lock();
        return STM32FLASH_ERROR;
    }

    HAL_FLASH_Lock();
    return STM32FLASH_OK;
}

/**
 * @brief  Erases a single sector of the flash memory.
 * @param  flashBank Flash bank (FLASH_BANK_1 or FLASH_BANK_2).
 * @param  sector Sector number to erase.
 * @retval STM32Flash_StatusTypeDef.
 */
STM32Flash_StatusTypeDef STM32Flash_erase_sector(uint32_t flashBank, uint32_t sector)
{
    FLASH_EraseInitTypeDef eraseInitStruct;
    uint32_t sectorError = 0;

    eraseInitStruct.TypeErase     = FLASH_TYPEERASE_SECTORS;
    eraseInitStruct.Banks         = flashBank;
    eraseInitStruct.Sector        = sector;
    eraseInitStruct.NbSectors     = 1;
    eraseInitStruct.VoltageRange  = FLASH_VOLTAGE_RANGE_3;

    HAL_StatusTypeDef halStatus = HAL_FLASH_Unlock();
    if (halStatus != HAL_OK) {
        return STM32FLASH_ERROR;
    }

    halStatus = HAL_FLASHEx_Erase(&eraseInitStruct, &sectorError);
    if (halStatus != HAL_OK) {
        HAL_FLASH_Lock();
        return STM32FLASH_ERROR;
    }

    HAL_FLASH_Lock();
    return STM32FLASH_OK;
}
