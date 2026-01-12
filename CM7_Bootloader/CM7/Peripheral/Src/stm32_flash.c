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

#ifdef CORE_CM7

/* Includes ------------------------------------------------------------------*/
#include "boot_config.h"

#include "stdlib.h"
#include "stdio.h"
#include "stdbool.h"

#include "arm_math.h"

#include "crc.h"

#include "stm32_flash.h"

/* Private define ------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
bool update_requested = false;

typedef struct
{
    FW_UpdateState updateState;     //(4 bytes)
    uint8_t padding[24];            // for 32 bytes alignment
} PersistentData;

/* Private function prototypes -----------------------------------------------*/
static uint32_t STM32Flash_computeCRC(const uint8_t *data, uint32_t length);

/**
 * @brief  Gets the sector of a given address.
 * @param  Address Address of the FLASH Memory.
 * @retval The sector of a given address.
 */
uint32_t stm32Flash_getSector(uint32_t Address)
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
    if (state == NULL)
    {
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

    PersistentData dataToWrite =
    {
        .updateState = updateState,
        .padding = {0}
    };

    // Unlock the Flash memory
    halStatus = HAL_FLASH_Unlock();
    if (halStatus != HAL_OK)
    {
        return STM32FLASH_ERROR;
    }

    // Configure sector erase
    eraseInitStruct.TypeErase     = FLASH_TYPEERASE_SECTORS;
    eraseInitStruct.Sector        = stm32Flash_getSector(flashAddress);
    eraseInitStruct.NbSectors     = 1;
    eraseInitStruct.Banks         = FLASH_BANK_1;
    eraseInitStruct.VoltageRange  = FLASH_VOLTAGE_RANGE_3;

    // Erase the sector
    halStatus = HAL_FLASHEx_Erase(&eraseInitStruct, &sectorError);
    if (halStatus != HAL_OK)
    {
        HAL_FLASH_Lock();
        return STM32FLASH_ERROR;
    }

    // Write the data
    uint32_t* pData = (uint32_t*)&dataToWrite;
    halStatus = HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD, flashAddress, (uint32_t)pData);
    if (halStatus != HAL_OK)
    {
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
STM32Flash_StatusTypeDef STM32Flash_write32B(const uint8_t* data, uint32_t address)
{
    // Check alignments
    if ((address % 32) != 0 || (((uint32_t)data) % 32) != 0) {
        printf("Alignment error: address=0x%08lx, data=0x%08lx\n", address, (uint32_t)data);
        return STM32FLASH_ERROR;
    }

    // Unlock flash memory
    HAL_StatusTypeDef halStatus = HAL_FLASH_Unlock();
    if (halStatus != HAL_OK) {
        printf("Failed to unlock FLASH\n");
        return STM32FLASH_ERROR;
    }

    // Clear flash error flags
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ALL_ERRORS_BANK1);
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ALL_ERRORS_BANK2);

    // Write to flash memory
    halStatus = HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD, address, (uint32_t)data);
    if (halStatus != HAL_OK) {
        uint32_t error = HAL_FLASH_GetError();
        printf("FLASH programming error: 0x%08lx at address 0x%08lx\n", error, address);
        HAL_FLASH_Lock();
        return STM32FLASH_ERROR;
    }

    // Lock flash memory
    halStatus = HAL_FLASH_Lock();
    if (halStatus != HAL_OK) {
        printf("Failed to lock FLASH\n");
        return STM32FLASH_ERROR;
    }

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
    if (halStatus != HAL_OK)
    {
        return STM32FLASH_ERROR;
    }

    halStatus = HAL_FLASHEx_Erase(&eraseInitStruct, &sectorError);
    if (halStatus != HAL_OK)
    {
        HAL_FLASH_Lock();
        return STM32FLASH_ERROR;
    }

    HAL_FLASH_Lock();
    return STM32FLASH_OK;
}

/**
 * @brief  Computes the CRC32 checksum of a given data buffer.
 *         This function processes data in 256-byte chunks (aligned to 4-byte words)
 *         to compute the CRC using the STM32 hardware CRC unit.
 *
 * @param  data   Pointer to the data buffer.
 * @param  length Length of the data buffer in bytes.
 *
 * @return Computed CRC32 value.
 */
static uint32_t STM32Flash_computeCRC(const uint8_t *data, uint32_t length)
{
    // Reset CRC for fresh calculation
    __HAL_CRC_DR_RESET(&hcrc);

    uint32_t totalProcessed = 0;
    uint32_t crcVal = 0;

    while (totalProcessed < length)
    {
        // Define chunk size (must be aligned to 4 bytes)
        uint32_t chunkSize = 256U;
        if ((length - totalProcessed) < chunkSize)
        {
            chunkSize = (length - totalProcessed);
        }

        // Round chunk size up to a multiple of 4 bytes
        uint32_t remainder = chunkSize % 4U;
        uint32_t wordAlignedSize = chunkSize + (remainder ? (4U - remainder) : 0U);

        // Copy chunk to a temporary buffer and pad if needed
        uint8_t tempBuf[256 + 3]; // Buffer large enough to accommodate padding
        memcpy(tempBuf, data + totalProcessed, chunkSize);
        memset(tempBuf + chunkSize, 0, wordAlignedSize - chunkSize);

        // Accumulate CRC
        crcVal = HAL_CRC_Accumulate(&hcrc, (uint32_t *)tempBuf, wordAlignedSize / 4U);

        totalProcessed += chunkSize;
    }

    // Return final computed CRC
    return crcVal;
}

/**
 * @brief  Reliably writes data to STM32 flash memory with CRC verification.
 *         This function writes data in 32-byte blocks, verifies each block
 *         by reading it back, and compares CRC values to ensure integrity.
 *         If verification fails, the function retries writing up to `maxRetries` times.
 *
 * @param  flashAddress  Target flash memory address for writing.
 * @param  buffer        Pointer to the data buffer to write.
 * @param  length        Length of the data in bytes.
 * @param  maxRetries    Maximum number of retries in case of CRC mismatch.
 *
 * @return STM32FLASH_OK if the write operation is successful, STM32FLASH_ERROR otherwise.
 */
STM32Flash_StatusTypeDef STM32Flash_reliableWrite(uint32_t flashAddress, const uint8_t *buffer, uint32_t length, int maxRetries)
{
    uint8_t verifyBlock[32] __attribute__((aligned(32)));
    uint32_t totalBytesWritten = 0;
    uint32_t remaining = length;

    while (remaining > 0)
    {
        uint32_t blockSize = (remaining >= 32) ? 32 : remaining;

        // Compute the reference CRC directly on the buffer
        uint32_t originalCRC = STM32Flash_computeCRC(buffer + totalBytesWritten, blockSize);

        STM32Flash_StatusTypeDef writeSuccess = STM32FLASH_ERROR;
        for (int attempt = 1; attempt <= maxRetries; attempt++)
        {
            // Write data directly to flash
            STM32Flash_StatusTypeDef status = STM32Flash_write32B(buffer + totalBytesWritten, flashAddress);
            if (status != STM32FLASH_OK)
            {
                printf("Error: flash write attempt %d failed at 0x%08lx\n", attempt, flashAddress);
                continue;
            }

            // Read back directly from flash and compare CRC
            memcpy(verifyBlock, (uint8_t *)flashAddress, 32);
            uint32_t readCRC = STM32Flash_computeCRC(verifyBlock, 32);

            if (readCRC == originalCRC)
            {
                writeSuccess = STM32FLASH_OK;
                break;
            }
            else
            {
                printf("CRC mismatch in flash write attempt %d at address 0x%08lx\n", attempt, flashAddress);
            }
        }

        if (writeSuccess != STM32FLASH_OK)
        {
            return STM32FLASH_ERROR;
        }

        flashAddress     += 32;
        totalBytesWritten += blockSize;
        remaining        -= blockSize;
    }

    return STM32FLASH_OK;
}
#endif
