/**
 ******************************************************************************
 * @file           : update.c
 * @brief          : Implementation of firmware update functionality.
 ******************************************************************************
 * @attention
 *
 * Copyright (C) 2018-present Reso-nance.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 *
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "progress.h"
#include "main.h"
#include "config.h"
#include "basetypes.h"
#include "globals.h"
#include "boot_config.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include "fatfs.h"
#include "stm32_flash.h"
#include "file_manager.h"

#include "crc.h"

#include "update_gui.h"
#include "update.h"

/* Private define ------------------------------------------------------------*/
#define BUFFER_SIZE      2048
#define HEADER_SIZE      24
#define VERSION_STR_SIZE 9

/* Private variables ---------------------------------------------------------*/
uint8_t tempBuffer[32] __attribute__((aligned(32)));

/* Function prototypes -------------------------------------------------------*/
static uint32_t update_readUint32LE(const uint8_t *buffer);
static fwupdate_StatusTypeDef update_calculateCRC(FIL* file, ProgressManager* progressManager, uint32_t step_number);
static fwupdate_StatusTypeDef update_backupFirmware(uint32_t flashStartAddr, uint32_t size, const char* backupFilePath, ProgressManager* progressManager, uint32_t step_number);
static fwupdate_StatusTypeDef update_eraseFirmware(uint32_t flashStartAddr, uint32_t size, ProgressManager* progressManager, uint32_t step_number);
static fwupdate_StatusTypeDef update_writeFirmware(uint32_t flashStartAddr, FIL* file, uint32_t size, ProgressManager* progressManager, uint32_t step_number);
static fwupdate_StatusTypeDef update_writeExternalData(FIL* file, uint32_t external_size, ProgressManager* progressManager, uint32_t step_number);

/**
 * @brief Reads a 32-bit unsigned integer from a buffer in little-endian format.
 * @param buffer Pointer to the buffer containing the data.
 * @return The parsed 32-bit unsigned integer.
 */
static uint32_t update_readUint32LE(const uint8_t *buffer)
{
	return ((uint32_t)buffer[0]) |
			((uint32_t)buffer[1] << 8) |
			((uint32_t)buffer[2] << 16) |
			((uint32_t)buffer[3] << 24);
}

/**
 * @brief Calculates and verifies the CRC of the update package.
 * @param file Pointer to the open file.
 * @param progressManager Pointer to the progress manager.
 * @return FWUPDATE_OK if the update process is successful, FWUPDATE_ERROR otherwise.
 */
static fwupdate_StatusTypeDef update_calculateCRC(FIL* file, ProgressManager* progressManager, uint32_t step_number)
{
	FSIZE_t file_size = f_size(file);
	FRESULT res;
	UINT bytesRead;
	uint32_t crc_position = file_size - 4;
	uint32_t crc_read;
	uint32_t crc_calculated = 0;
	uint32_t totalDataRead = 0;
	uint32_t crc_length = file_size - 4; // Exclude the footer CRC
	uint8_t readBuffer[BUFFER_SIZE] __attribute__((aligned(4))); // Buffer aligned to 4 bytes
	uint8_t crc_buffer[4];

	// Read the CRC from the footer
	res = f_lseek(file, crc_position);
	if (res != FR_OK)
	{
		printf("Failed to reposition to read the CRC\n");
		gui_displayUpdateFailed();
		return FWUPDATE_ERROR;
	}

	res = f_read(file, crc_buffer, 4, &bytesRead);
	if (res != FR_OK || bytesRead != 4)
	{
		printf("Failed to read package CRC\n");
		gui_displayUpdateFailed();
		return FWUPDATE_ERROR;
	}

	// Convert the read CRC (little-endian)
	crc_read = update_readUint32LE(crc_buffer);
	printf("Read CRC from footer: 0x%08lX\n", crc_read);

	// Return to the beginning of the file for CRC calculation
	res = f_lseek(file, 0);
	if (res != FR_OK)
	{
		printf("Failed to reposition to the beginning of the file for CRC calculation\n");
		gui_displayUpdateFailed();
		return FWUPDATE_ERROR;
	}

	// Initialize the CRC calculation
	__HAL_CRC_DR_RESET(&hcrc);

	while (totalDataRead < crc_length)
	{
		uint32_t bytesToRead = (crc_length - totalDataRead > BUFFER_SIZE) ? BUFFER_SIZE : (crc_length - totalDataRead);
		res = f_read(file, readBuffer, bytesToRead, &bytesRead);
		if (res != FR_OK || bytesRead == 0)
		{
			printf("Error reading the file for CRC calculation\n");
			gui_displayUpdateFailed();
			return FWUPDATE_ERROR;
		}

		// Calculate the CRC for the read bytes
		crc_calculated = HAL_CRC_Accumulate(&hcrc, (uint32_t *)readBuffer, bytesRead);

		totalDataRead += bytesRead;

		// Update step progress
		progress_update(progressManager, step_number, totalDataRead, crc_length);
	}

	// Perform the final XOR with 0xFFFFFFFF
	crc_calculated ^= 0xFFFFFFFF;

	// Compare the calculated CRC with the CRC from the file
	if (crc_calculated != crc_read)
	{
		printf("CRC mismatch: calculated 0x%08lX, expected 0x%08lX\n", crc_calculated, crc_read);
		gui_displayUpdateFailed();
		return FWUPDATE_ERROR;
	}
	else
	{
		printf("CRC verified successfully\n");
	}

	return FWUPDATE_OK;
}

/**
 * @brief Backs up firmware from flash memory to a specified file.
 * This function reads firmware data from flash memory and writes it
 * to a backup file. Progress updates are reported during the operation.
 * @param flashStartAddr Starting address in flash memory.
 * @param size Size of the firmware to backup in bytes.
 * @param backupFilePath Path to the backup file to create.
 * @param progressManager Pointer to the progress manager for updates.
 * @param step_number Step number for the progress manager.
 * @return FWUPDATE_OK if the update process is successful, FWUPDATE_ERROR otherwise.
 */
static fwupdate_StatusTypeDef update_backupFirmware(uint32_t flashStartAddr, uint32_t size, const char* backupFilePath, ProgressManager* progressManager, uint32_t step_number)
{
	// If the final backup file already exists, skip the backup
	FILINFO fileInfo;
	if (f_stat(backupFilePath, &fileInfo) == FR_OK)
	{
		printf("File %s already exists. Skipping backup.\n", backupFilePath);
		return FWUPDATE_OK;
	}

	// Build a temporary file name by appending ".tmp"
	char tmpFilePath[256];
	snprintf(tmpFilePath, sizeof(tmpFilePath), "%s.tmp", backupFilePath);

	// Remove any existing temporary file
	f_unlink(tmpFilePath);

	// Check flash memory boundaries
	if ((flashStartAddr + size) > FLASH_END_ADDR)
	{
		printf("Error: Flash address out of range\n");
		return FWUPDATE_ERROR;
	}

	// Open the temporary backup file
	FIL backupFile;
	FRESULT res = f_open(&backupFile, tmpFilePath, FA_WRITE | FA_READ | FA_CREATE_ALWAYS);
	if (res != FR_OK)
	{
		printf("Error: Cannot open temporary backup file %s\n", tmpFilePath);
		return FWUPDATE_ERROR;
	}

	uint8_t readBuffer[32768] __attribute__((aligned(32)));
	uint32_t bytesRemaining = size;
	uint32_t flashAddress = flashStartAddr;
	uint32_t totalBytesRead = 0;

	while (bytesRemaining > 0)
	{
		uint32_t chunkSize = (bytesRemaining > sizeof(readBuffer)) ? sizeof(readBuffer) : bytesRemaining;

		// Read from flash memory
		memcpy(readBuffer, (uint8_t*)flashAddress, chunkSize);

		// Write to the temporary file
		if (file_reliableWrite(&backupFile, readBuffer, chunkSize, 5) != FILEMANAGER_OK)
		{
		    printf("Error: Reliable write failed in temporary file %s\n", tmpFilePath);
		    f_close(&backupFile);
		    return FWUPDATE_ERROR;
		}

		flashAddress += chunkSize;
		bytesRemaining -= chunkSize;
		totalBytesRead += chunkSize;

		// Update progress
		progress_update(progressManager, step_number, totalBytesRead, size);
	}

	// Close the temporary file
	f_close(&backupFile);

	// Rename the temporary file to the final backup file
	res = f_rename(tmpFilePath, backupFilePath);
	if (res != FR_OK)
	{
		printf("Error: Failed to rename %s to %s\n", tmpFilePath, backupFilePath);
		return FWUPDATE_ERROR;
	}

	return FWUPDATE_OK;
}

/**
 * @brief  Writes firmware to flash memory from a specified file.
 *         This function reads firmware data from a file and writes it to flash memory
 *         in aligned 32-byte blocks. It ensures proper alignment, handles padding,
 *         and updates the progress manager accordingly.
 *
 * @param  flashStartAddr  Starting address in flash memory.
 * @param  file            Pointer to the file containing the firmware.
 * @param  size            Size of the firmware to write in bytes.
 * @param  progressManager Pointer to the progress manager for updates.
 * @param  step_number     Step number for the progress manager.
 *
 * @return FWUPDATE_OK if the update process is successful, FWUPDATE_ERROR otherwise.
 */
static fwupdate_StatusTypeDef update_writeFirmware(uint32_t flashStartAddr, FIL* file, uint32_t size, ProgressManager* progressManager, uint32_t step_number)
{
    uint8_t readBuffer[BUFFER_SIZE] __attribute__((aligned(32)));
    uint8_t block32[32]     __attribute__((aligned(32)));

    uint32_t flashAddress = flashStartAddr;
    uint32_t totalWritten = 0;
    FRESULT res;
    UINT bytesRead;

    printf("Flashing firmware to address 0x%08lx...\n", (unsigned long)flashAddress);

    // Verify alignment
    if ((flashAddress % 32) != 0)
    {
        printf("Error: Flash address misaligned at 0x%08lx\n", (unsigned long)flashAddress);
        gui_displayUpdateFailed();
        return FWUPDATE_ERROR;
    }

    uint32_t remaining = size;
    while (remaining > 0)
    {
        // 1) Read a chunk from the file
        uint32_t chunkSize = (remaining > sizeof(readBuffer)) ? sizeof(readBuffer) : remaining;

        res = f_read(file, readBuffer, chunkSize, &bytesRead);
        if (res != FR_OK || bytesRead != chunkSize)
        {
            printf("Error: Failed to read firmware data (f_read returned %d)\n", res);
            gui_displayUpdateFailed();
            return FWUPDATE_ERROR;
        }

        remaining -= chunkSize;

        // 2) Write this chunk in 32-byte blocks to flash memory
        uint32_t offset = 0;
        while (offset < chunkSize)
        {
            // Determine the size of the next 32-byte block
            uint32_t blockSize = (chunkSize - offset >= 32) ? 32 : (chunkSize - offset);

            // Copy data into block32
            memcpy(block32, &readBuffer[offset], blockSize);

            // If block is < 32 bytes, pad with 0xFF
            if (blockSize < 32)
            {
                memset(&block32[blockSize], 0xFF, 32 - blockSize);
                blockSize = 32;  // Ensure correct size for write operation
            }

            // 3) Perform a reliable 32-byte flash write
            if (STM32Flash_reliableWrite(flashAddress, block32, blockSize, 5) != STM32FLASH_OK)
            {
                printf("Error: Reliable flash write failed at 0x%08lx\n", (unsigned long)flashAddress);
                gui_displayUpdateFailed();
                return FWUPDATE_ERROR;
            }

            // 4) Advance flash pointer by 32, offset by actual data size
            flashAddress += 32;
            offset       += (blockSize == 32) ? (uint32_t)(blockSize < (chunkSize - offset) ? 32 : (chunkSize - offset))
                                              : blockSize;

            // 5) Update the total written bytes (excluding padding)
            totalWritten += (blockSize == 32) ? ((offset <= chunkSize) ? 32 : (chunkSize - (offset - 32)))
                                              : blockSize;

            // 6) Update the progress bar
            uint32_t displayedBytes = (totalWritten > size) ? size : totalWritten;
            progress_update(progressManager, step_number, displayedBytes, size);
        }
    }

    return FWUPDATE_OK;
}

/**
 * @brief Erases necessary flash sectors for firmware.
 * @param flashStartAddr Starting address of the firmware in flash.
 * @param size Size of the firmware.
 * @param progressManager Pointer to the progress manager.
 * @param step_number The step number for progress tracking.
 * @return FWUPDATE_OK if the update process is successful, FWUPDATE_ERROR otherwise.
 */
static fwupdate_StatusTypeDef update_eraseFirmware(uint32_t flashStartAddr, uint32_t size, ProgressManager* progressManager, uint32_t step_number)
{
	// Determine flash bank and sector
	uint32_t flashBank = (flashStartAddr >= ADDR_FLASH_SECTOR_0_BANK2) ? FLASH_BANK_2 : FLASH_BANK_1;
	uint32_t flashSector = stm32Flash_getSector(flashStartAddr);
	uint32_t NbSectors = (size + FLASH_SECTOR_SIZE - 1) / FLASH_SECTOR_SIZE;
	uint32_t sectorsErased = 0;

	printf("Erasing flash sectors starting from sector %lu...\n", flashSector);
	for (uint32_t sector = flashSector; sector < flashSector + NbSectors; sector++)
	{
		if (STM32Flash_erase_sector(flashBank, sector) != STM32FLASH_OK)
		{
			printf("Failed to erase sector %lu\n", sector);
			gui_displayUpdateFailed();
			return FWUPDATE_ERROR;
		}

		sectorsErased++;

		// Update step progress
		progress_update(progressManager, step_number, sectorsErased, NbSectors);
	}

	return FWUPDATE_OK;
}

/**
 * @brief  Writes external data to the file system.
 *         This function reads data from an open package file and writes it
 *         to the file system in manageable chunks to prevent memory overload.
 *         A reliable write operation with CRC verification is performed.
 *
 * @param  file            Pointer to the open package file.
 * @param  external_size   Size of the external data in bytes.
 * @param  progressManager Pointer to the progress manager for tracking progress.
 * @param  step_number     Step number for progress tracking.
 *
 * @return FWUPDATE_OK if the update process is successful, FWUPDATE_ERROR otherwise.
 */
static fwupdate_StatusTypeDef update_writeExternalData(FIL* file, uint32_t external_size, ProgressManager* progressManager, uint32_t step_number)
{
    UINT bytesRead;
    FRESULT res;
    uint8_t readBuffer[BUFFER_SIZE] __attribute__((aligned(4)));

    uint32_t totalBytesToWrite = external_size;
    uint32_t totalBytesWritten = 0;

    printf("Writing external data to the file system...\n");

    // Open the destination file
    FIL externalFile;
    res = f_open(&externalFile, "0:/External_MAX8.tar.gz", FA_WRITE | FA_READ | FA_CREATE_ALWAYS);
    if (res != FR_OK)
    {
        printf("Failed to open the file on the file system\n");
        gui_displayUpdateFailed();
        return FWUPDATE_ERROR;
    }

    // Process data in chunks to avoid memory overload
    uint32_t bytesToWrite = external_size;
    while (bytesToWrite > 0)
    {
        // Read a chunk of data from the source file
        uint32_t chunkSize = (bytesToWrite > BUFFER_SIZE) ? BUFFER_SIZE : bytesToWrite;
        res = f_read(file, readBuffer, chunkSize, &bytesRead);
        if (res != FR_OK || bytesRead != chunkSize)
        {
            printf("Failed to read external data (error %d)\n", res);
            f_close(&externalFile);
            gui_displayUpdateFailed();
            return FWUPDATE_ERROR;
        }

        // Perform a reliable write with CRC verification
        if (file_reliableWrite(&externalFile, readBuffer, bytesRead, 5) != FILEMANAGER_OK)
        {
            printf("Error: Reliable write failed in file system\n");
            f_close(&externalFile);
            gui_displayUpdateFailed();
            return FWUPDATE_ERROR;
        }

        bytesToWrite -= bytesRead;
        totalBytesWritten += bytesRead;

        // Update progress bar
        progress_update(progressManager, step_number, totalBytesWritten, totalBytesToWrite);
    }

    // Close the file
    f_close(&externalFile);

    return FWUPDATE_OK;
}

/**
 * @brief  Search for a firmware package file in the filesystem.
 * @param  packageFilePath Buffer to store the found package file path.
 * @param  maxLen Maximum length of the buffer.
 * @return FWUPDATE_OK if the update process is successful, FWUPDATE_ERROR otherwise.
 */
fwupdate_StatusTypeDef update_findPackageFile(char *packageFilePath, size_t maxLen)
{
    FRESULT res;
    FILINFO fno;
    DIR dir;
    char *fn;

    res = f_opendir(&dir, FW_PATH); // Open the directory
    if (res == FR_OK)
    {
        for (;;)
        {
            res = f_readdir(&dir, &fno); // Read a directory item
            if ((res != FR_OK) || (fno.fname[0] == 0))
            {
                break; // End of directory or error
            }

            // Ensure it is a file and not a directory
            if (fno.fattrib & AM_DIR)
            {
                continue;
            }

            fn = fno.fname;

            // Check if the filename starts with "cis_package_" and contains ".bin"
            if ((strstr(fn, "cis_package_") == fn) && strstr(fn, ".bin"))
            {
                size_t fwPathLen = strlen(FW_PATH);
                size_t fnLen = strlen(fn);
                size_t totalLen = fwPathLen + 1 + fnLen + 1; // '/' + null terminator

                // Ensure packageFilePath buffer is large enough
                if (totalLen > maxLen)
                {
                    f_closedir(&dir);
                    return FWUPDATE_ERROR; // Avoid truncation
                }

                // Use strncpy and strncat instead of snprintf to avoid -Wformat-truncation warning
                strncpy(packageFilePath, FW_PATH, maxLen - 1);
                packageFilePath[maxLen - 1] = '\0'; // Ensure null termination

                strncat(packageFilePath, "/", maxLen - strlen(packageFilePath) - 1);
                strncat(packageFilePath, fn, maxLen - strlen(packageFilePath) - 1);

                f_closedir(&dir);
                return FWUPDATE_OK;
            }
        }
        f_closedir(&dir);
    }
    return FWUPDATE_ERROR;
}

/**
 * @brief  Restores previously backed-up firmware versions.
 *         This function erases the designated flash memory regions and writes
 *         back the backed-up firmware stored in files. It ensures each step
 *         is properly tracked with a progress manager.
 *
 * @return FWUPDATE_OK if the update process is successful, FWUPDATE_ERROR otherwise.
 */
fwupdate_StatusTypeDef update_restoreBackupFirmwares(void)
{
    const int NUM_STEPS = 4;
    const int STEP_ERASE_CM7 = 1;
    const int STEP_ERASE_CM4 = 2;
    const int STEP_FLASH_CM7 = 3;
    const int STEP_FLASH_CM4 = 4;

    char backupPath[64];
    ProgressManager progressManager;
    progress_init(&progressManager, NUM_STEPS);
    gui_displayRestorePreviousVersion();

    FIL backupFile;
    FRESULT fres;
    FSIZE_t backupSize;

    // Step 1: Erase CM7 flash region
    printf("Step 1: Erasing CM7 region\n");
    snprintf(backupPath, sizeof(backupPath), "%s/%s", FW_PATH, "backup_cm7.bin");
    fres = f_open(&backupFile, backupPath, FA_READ);
    if (fres != FR_OK)
    {
        printf("No backup found for %s (fres=%d). Skipping erase.\n", backupPath, fres);
        return FWUPDATE_ERROR;
    }
    backupSize = f_size(&backupFile);
    if (update_eraseFirmware(FW_CM7_START_ADDR, (uint32_t)backupSize, &progressManager, STEP_ERASE_CM7) != FWUPDATE_OK)
    {
        printf("Failed to erase flash region at 0x%08lX.\n", (long unsigned int)FW_CM7_START_ADDR);
        f_close(&backupFile);
        return FWUPDATE_ERROR;
    }
    f_close(&backupFile);

    // Step 2: Erase CM4 flash region
    printf("Step 2: Erasing CM4 region\n");
    snprintf(backupPath, sizeof(backupPath), "%s/%s", FW_PATH, "backup_cm4.bin");
    fres = f_open(&backupFile, backupPath, FA_READ);
    if (fres != FR_OK)
    {
        printf("No backup found for %s (fres=%d). Skipping erase.\n", backupPath, fres);
        return FWUPDATE_ERROR;
    }
    backupSize = f_size(&backupFile);
    if (update_eraseFirmware(FW_CM4_START_ADDR, (uint32_t)backupSize, &progressManager, STEP_ERASE_CM4) != FWUPDATE_OK)
    {
        printf("Failed to erase flash region at 0x%08lX.\n", (long unsigned int)FW_CM4_START_ADDR);
        f_close(&backupFile);
        return FWUPDATE_ERROR;
    }
    f_close(&backupFile);

    // Step 3: Restore CM7 firmware using `update_writeFirmware`
    printf("Step 3: Restoring CM7 backup\n");
    snprintf(backupPath, sizeof(backupPath), "%s/%s", FW_PATH, "backup_cm7.bin");
    fres = f_open(&backupFile, backupPath, FA_READ);
    if (fres != FR_OK)
    {
        printf("No backup found for %s (fres=%d). Skipping restore.\n", backupPath, fres);
        return FWUPDATE_ERROR;
    }
    backupSize = f_size(&backupFile);

    if (update_writeFirmware(FW_CM7_START_ADDR, &backupFile, (uint32_t)backupSize, &progressManager, STEP_FLASH_CM7) != FWUPDATE_OK)
    {
        printf("Error: Failed to restore CM7 firmware at 0x%08lX.\n", (long unsigned int)FW_CM7_START_ADDR);
        f_close(&backupFile);
        return FWUPDATE_ERROR;
    }

    f_close(&backupFile);
    printf("Successfully restored %s to 0x%08lX.\n", backupPath, (long unsigned int)FW_CM7_START_ADDR);

    // Step 4: Restore CM4 firmware using `update_writeFirmware`
    printf("Step 4: Restoring CM4 backup\n");
    snprintf(backupPath, sizeof(backupPath), "%s/%s", FW_PATH, "backup_cm4.bin");
    fres = f_open(&backupFile, backupPath, FA_READ);
    if (fres != FR_OK)
    {
        printf("No backup found for %s (fres=%d). Skipping restore.\n", backupPath, fres);
        return FWUPDATE_ERROR;
    }
    backupSize = f_size(&backupFile);

    if (update_writeFirmware(FW_CM4_START_ADDR, &backupFile, (uint32_t)backupSize, &progressManager, STEP_FLASH_CM4) != FWUPDATE_OK)
    {
        printf("Error: Failed to restore CM4 firmware at 0x%08lX.\n", (long unsigned int)FW_CM4_START_ADDR);
        f_close(&backupFile);
        return FWUPDATE_ERROR;
    }

    f_close(&backupFile);
    printf("Successfully restored %s to 0x%08lX.\n", backupPath, (long unsigned int)FW_CM4_START_ADDR);

    return FWUPDATE_OK;
}

/**
 * @brief Processes a firmware update package file.
 * This function handles the full update process including CRC verification,
 * backup, erasing, flashing, and external data handling.
 * @param packageFilePath Path to the firmware package file.
 * @return FWUPDATE_OK if the update process is successful, FWUPDATE_ERROR otherwise.
 */
fwupdate_StatusTypeDef update_processPackageFile(const TCHAR* packageFilePath)
{
    const int NUM_STEPS = 8;
    const int STEP_CRC_CALCULATION = 1;
    const int STEP_BACKUP_CM7 = 2;
    const int STEP_BACKUP_CM4 = 3;
    const int STEP_ERASE_CM7 = 4;
    const int STEP_ERASE_CM4 = 5;
    const int STEP_FLASH_CM7 = 6;
    const int STEP_FLASH_CM4 = 7;
    const int STEP_SAVE_EXTERNAL = 8;

	FIL file;
	UINT bytesRead;
	FRESULT res;
	uint8_t header[HEADER_SIZE];
	uint32_t cm7_size, cm4_size, external_size;
	char version[VERSION_STR_SIZE] = {0};
	uint8_t magic[4];
	char backupPath[64];
	ProgressManager progressManager;

	// Initialize progress manager
	progress_init(&progressManager, NUM_STEPS);

	// Open the update package file
	res = f_open(&file, packageFilePath, FA_READ);
	if (res != FR_OK)
	{
		printf("Error: Failed to open the package file: %s\n", packageFilePath);
		gui_displayUpdateFailed();
		return FWUPDATE_ERROR;
	}

	// Read the header
	res = f_read(&file, header, sizeof(header), &bytesRead);
	if (res != FR_OK || bytesRead != sizeof(header))
	{
		printf("Error: Failed to read the package header\n");
		f_close(&file);
		gui_displayUpdateFailed();
		return FWUPDATE_ERROR;
	}

	// Parse the header
	memcpy(magic, header, 4);
	if (memcmp(magic, "BOOT", 4) != 0)
	{
		printf("Error: Invalid package magic number\n");
		f_close(&file);
		gui_displayUpdateFailed();
		return FWUPDATE_ERROR;
	}

	cm7_size = update_readUint32LE(header + 4);
	cm4_size = update_readUint32LE(header + 8);
	external_size = update_readUint32LE(header + 12);
	memcpy(version, header + 16, 8);

	printf("Package version: %s\n", version);
	printf("CM7 firmware size: %lu bytes\n", cm7_size);
	printf("CM4 firmware size: %lu bytes\n", cm4_size);
	printf("External data size: %lu bytes\n", external_size);

	// Display version
	gui_displayVersion(version);

	// Step 1: Calculate and verify CRC
	printf("Step 1: Calculate and verify CRC\n");
	if (update_calculateCRC(&file, &progressManager, STEP_CRC_CALCULATION) != FWUPDATE_OK)
	{
		printf("Error: Failed to calculate or verify CRC\n");
		f_close(&file);
		return FWUPDATE_ERROR;
	}

	// Step 2: Backup current CM7 firmware
	printf("Step 2: Backup current CM7 firmware\n");
	snprintf(backupPath, sizeof(backupPath), "%s/%s", FW_PATH, "backup_cm7.bin");
	if (update_backupFirmware(FW_CM7_START_ADDR, FW_CM7_MAX_SIZE, backupPath, &progressManager, STEP_BACKUP_CM7) != FWUPDATE_OK)
	{
	    printf("Error: Failed to backup current CM7 firmware\n");
	    f_close(&file);
	    return FWUPDATE_ERROR;
	}

	// Step 3: Backup current CM4 firmware
	printf("Step 3: Backup current CM4 firmware\n");
	snprintf(backupPath, sizeof(backupPath), "%s/%s", FW_PATH, "backup_cm4.bin");
	if (update_backupFirmware(FW_CM4_START_ADDR, FW_CM4_MAX_SIZE, backupPath, &progressManager, STEP_BACKUP_CM4) != FWUPDATE_OK)
	{
	    printf("Error: Failed to backup current CM4 firmware\n");
	    f_close(&file);
	    return FWUPDATE_ERROR;
	}

	// Step 4: Erase CM7 firmware
	printf("Step 4: Erase CM7 firmware\n");
	if (update_eraseFirmware(FW_CM7_START_ADDR, cm7_size, &progressManager, STEP_ERASE_CM7) != FWUPDATE_OK)
	{
		printf("Error: Failed to erase CM7 firmware\n");
		f_close(&file);
		return FWUPDATE_ERROR;
	}

	// Step 5: Erase CM4 firmware
	printf("Step 5: Erase CM4 firmware\n");
	if (update_eraseFirmware(FW_CM4_START_ADDR, cm4_size, &progressManager, STEP_ERASE_CM4) != FWUPDATE_OK)
	{
		printf("Error: Failed to erase CM4 firmware\n");
		f_close(&file);
		return FWUPDATE_ERROR;
	}

	// Step 6: Flash new CM7 firmware
	printf("Step 6: Flash new CM7 firmware\n");
	res = f_lseek(&file, HEADER_SIZE);
	if (res != FR_OK)
	{
		printf("Error: Failed to reposition after the header\n");
		gui_displayUpdateFailed();
		f_close(&file);
		return FWUPDATE_ERROR;
	}

	if (update_writeFirmware(FW_CM7_START_ADDR, &file, cm7_size, &progressManager, STEP_FLASH_CM7) != FWUPDATE_OK)
	{
		printf("Error: Failed to flash new CM7 firmware\n");
		f_close(&file);
		return FWUPDATE_ERROR;
	}

	// Step 7: Flash new CM4 firmware
	printf("Step 7: Flash new CM4 firmware\n");
	res = f_lseek(&file, HEADER_SIZE + cm7_size);
	if (res != FR_OK)
	{
		printf("Error: Failed to reposition to CM4 firmware data\n");
		gui_displayUpdateFailed();
		f_close(&file);
		return FWUPDATE_ERROR;
	}

	if (update_writeFirmware(FW_CM4_START_ADDR, &file, cm4_size, &progressManager, STEP_FLASH_CM4) != FWUPDATE_OK)
	{
		printf("Error: Failed to flash new CM4 firmware\n");
		f_close(&file);
		return FWUPDATE_ERROR;
	}

	// Step 8: Save external data
	printf("Step 8: Save external data\n");
	res = f_lseek(&file, HEADER_SIZE + cm7_size + cm4_size);
	if (res != FR_OK)
	{
		printf("Error: Failed to reposition to external data\n");
		gui_displayUpdateFailed();
		f_close(&file);
		return FWUPDATE_ERROR;
	}

	if (update_writeExternalData(&file, external_size, &progressManager, STEP_SAVE_EXTERNAL) != FWUPDATE_OK)
	{
		printf("Error: Failed to save external data\n");
		f_close(&file);
		return FWUPDATE_ERROR;
	}

	// Close the package file
	f_close(&file);

	// Display success message
	gui_displayUpdateSuccess();

	return FWUPDATE_OK;
}
