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
#include "shared.h"

#include "stdlib.h"
#include "stdio.h"
#include "stdbool.h"

#include "arm_math.h"

#include "stm32_flash.h"

/* Private define ------------------------------------------------------------*/

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

#define FLASH_LAST_SECTOR_ADDR   (FLASH_END_ADDR - FLASH_SECTOR_SIZE + 1)

/* Private variables ---------------------------------------------------------*/
bool update_requested = false;

/* Private function prototypes -----------------------------------------------*/
static uint32_t stm32_flashGetSector(uint32_t Address);

/**
 * @brief  Gets the sector of a given address
 * @param  Address Address of the FLASH Memory
 * @retval The sector of a given address
 */
uint32_t stm32_flashGetSector(uint32_t Address)
{
	uint32_t sector = 0;

	if(((Address < ADDR_FLASH_SECTOR_1_BANK1) && (Address >= ADDR_FLASH_SECTOR_0_BANK1)) || \
			((Address < ADDR_FLASH_SECTOR_1_BANK2) && (Address >= ADDR_FLASH_SECTOR_0_BANK2)))
	{
		sector = FLASH_SECTOR_0;
	}
	else if(((Address < ADDR_FLASH_SECTOR_2_BANK1) && (Address >= ADDR_FLASH_SECTOR_1_BANK1)) || \
			((Address < ADDR_FLASH_SECTOR_2_BANK2) && (Address >= ADDR_FLASH_SECTOR_1_BANK2)))
	{
		sector = FLASH_SECTOR_1;
	}
	else if(((Address < ADDR_FLASH_SECTOR_3_BANK1) && (Address >= ADDR_FLASH_SECTOR_2_BANK1)) || \
			((Address < ADDR_FLASH_SECTOR_3_BANK2) && (Address >= ADDR_FLASH_SECTOR_2_BANK2)))
	{
		sector = FLASH_SECTOR_2;
	}
	else if(((Address < ADDR_FLASH_SECTOR_4_BANK1) && (Address >= ADDR_FLASH_SECTOR_3_BANK1)) || \
			((Address < ADDR_FLASH_SECTOR_4_BANK2) && (Address >= ADDR_FLASH_SECTOR_3_BANK2)))
	{
		sector = FLASH_SECTOR_3;
	}
	else if(((Address < ADDR_FLASH_SECTOR_5_BANK1) && (Address >= ADDR_FLASH_SECTOR_4_BANK1)) || \
			((Address < ADDR_FLASH_SECTOR_5_BANK2) && (Address >= ADDR_FLASH_SECTOR_4_BANK2)))
	{
		sector = FLASH_SECTOR_4;
	}
	else if(((Address < ADDR_FLASH_SECTOR_6_BANK1) && (Address >= ADDR_FLASH_SECTOR_5_BANK1)) || \
			((Address < ADDR_FLASH_SECTOR_6_BANK2) && (Address >= ADDR_FLASH_SECTOR_5_BANK2)))
	{
		sector = FLASH_SECTOR_5;
	}
	else if(((Address < ADDR_FLASH_SECTOR_7_BANK1) && (Address >= ADDR_FLASH_SECTOR_6_BANK1)) || \
			((Address < ADDR_FLASH_SECTOR_7_BANK2) && (Address >= ADDR_FLASH_SECTOR_6_BANK2)))
	{
		sector = FLASH_SECTOR_6;
	}
	else if(((Address < ADDR_FLASH_SECTOR_0_BANK2) && (Address >= ADDR_FLASH_SECTOR_7_BANK1)) || \
			((Address < FLASH_END_ADDR) && (Address >= ADDR_FLASH_SECTOR_7_BANK2)))
	{
		sector = FLASH_SECTOR_7;
	}
	else
	{
		sector = FLASH_SECTOR_7;
	}

	return sector;
}

/**
 * @brief  Writes the firmware update state to the last address of the flash
 * @param  state Firmware update state to store in the flash (FW_UPDATE_NONE, FW_UPDATE_RECEIVED, etc.)
 * @retval None
 */
HAL_StatusTypeDef STM32Flash_writePersistentData(PersistentData* data)
{
    HAL_StatusTypeDef status;
    FLASH_EraseInitTypeDef eraseInitStruct;
    uint32_t sectorError = 0;

    const uint32_t flashAddress = 0x08020000;  // Fixed flash address for the data

    // Step 1: Unlock the Flash
    status = HAL_FLASH_Unlock();
    if (status != HAL_OK)
    {
        return status;
    }

    // Step 2: Erase the target sector (make sure the sector is correct)
    eraseInitStruct.TypeErase     = FLASH_TYPEERASE_SECTORS;
    eraseInitStruct.Sector        = stm32_flashGetSector(flashAddress);  // Get sector based on address
    eraseInitStruct.NbSectors     = 1;
    eraseInitStruct.Banks         = FLASH_BANK_1;
    eraseInitStruct.VoltageRange  = FLASH_VOLTAGE_RANGE_3;

    status = HAL_FLASHEx_Erase(&eraseInitStruct, &sectorError);
    if (status != HAL_OK)
    {
        HAL_FLASH_Lock();
        return status;
    }

    // Step 3: Write the 32-byte structure to Flash
    // FLASH_TYPEPROGRAM_FLASHWORD expects 32-byte alignment
    uint32_t* pData = (uint32_t*)data;
    status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD, flashAddress, (uint32_t)pData);
    if (status != HAL_OK)
    {
        HAL_FLASH_Lock();
        return status;
    }

    // Step 4: Lock the Flash after writing
    HAL_FLASH_Lock();

    return HAL_OK;
}

/**
 * @brief  Reads the firmware update state from the last address of the flash
 * @retval Firmware update state read from the flash (FW_UPDATE_NONE, FW_UPDATE_RECEIVED, etc.)
 */
void STM32Flash_readPersistentData(PersistentData* data)
{
    // Adresse fixe pour la Flash (doit être la même que pour l'écriture)
    const uint32_t flashAddress = 0x08020000;

    // Read the structure directly from the specified Flash address
    PersistentData* flashData = (PersistentData*)flashAddress;
    *data = *flashData;  // Copie les données dans la structure passée en paramètre
}

HAL_StatusTypeDef STM32Flash_erase_app_memory(uint32_t flashBank, uint32_t flashSector, uint32_t NbSectors)
{
    FLASH_EraseInitTypeDef EraseInitStruct;
    uint32_t SectorError = 0;
    HAL_StatusTypeDef status;

    EraseInitStruct.TypeErase = FLASH_TYPEERASE_SECTORS;
    EraseInitStruct.Banks = flashBank;
    EraseInitStruct.Sector = flashSector;
    EraseInitStruct.NbSectors = NbSectors;
    EraseInitStruct.VoltageRange = FLASH_VOLTAGE_RANGE_3;

    // Unlock the Flash to enable the flash control register access
    status = HAL_FLASH_Unlock();
    if (status != HAL_OK) {
        return status;
    }

    // Erase the specified flash sectors
    status = HAL_FLASHEx_Erase(&EraseInitStruct, &SectorError);
    if (status != HAL_OK) {
        HAL_FLASH_Lock();
        return status;
    }

    // Lock the Flash to disable the flash control register access
    status = HAL_FLASH_Lock();
    return status;
}

HAL_StatusTypeDef STM32Flash_write32B(uint8_t *data, uint32_t address)
{
    HAL_StatusTypeDef status;
    uint32_t i;

    // Unlock the Flash to enable the flash control register access
    status = HAL_FLASH_Unlock();
    if (status != HAL_OK) {
        return status;
    }

    // Write 32 bytes (8 words) to the flash memory
    for (i = 0; i < 32; i += 4) {
        status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD, address + i, (uint32_t)(data + i));
       // status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD, flashAddress, (uint32_t)pData);

        if (status != HAL_OK) {
            HAL_FLASH_Lock();
            return status;
        }
    }

    // Lock the Flash to disable the flash control register access
    status = HAL_FLASH_Lock();
    return status;
}
