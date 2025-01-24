/**
 ******************************************************************************
 * @file           : file_manager.c
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

#include "main.h"
#include "config.h"
#include "basetypes.h"
#include "globals.h"
#include "stdio.h"

#include "ff.h" // FATFS include
#include "diskio.h" // DiskIO include
#include "crc.h"

#include "file_manager.h"

/* Private define ------------------------------------------------------------*/
#define WORKING_BUFFER_SIZE (2 * _MAX_SS)
#define CHUNK_SIZE 4096

/* Private variables ---------------------------------------------------------*/
const struct shared_config DefaultConfig =
{
    .ui_button_delay = DEFAULT_UI_BUTTON_DELAY,
    .network_ip = DEFAULT_NETWORK_IP,
    .network_netmask = DEFAULT_NETWORK_NETMASK,
    .network_gw = DEFAULT_NETWORK_GW,
    .network_dest_ip = DEFAULT_NETWORK_DEST_IP,
    .network_udp_port = DEFAULT_NETWORK_UDP_PORT,
    .network_tcp_port = DEFAULT_NETWORK_TCP_PORT,
    .cis_print_calibration = DEFAULT_CIS_PRINT_CALIBRATION,
    .cis_dpi = DEFAULT_CIS_DPI,
    .cis_clk_freq = DEFAULT_CIS_CLK_FREQ,
    .cis_oversampling = DEFAULT_CIS_OVERSAMPLING,
    .cis_handedness = DEFAULT_CIS_HANDEDNESS
};

FATFS fs;

/* Private function prototypes -----------------------------------------------*/
static fileManager_StatusTypeDef file_parseLine(char* line, volatile struct shared_config* config);
static fileManager_StatusTypeDef print_shared_config(struct shared_config config);
static uint32_t file_computeCRC_buffer(CRC_HandleTypeDef *hcrc, const uint8_t *pData, uint32_t length);

/**
 * @brief  Reads the shared configuration from a file.
 *         This function opens the specified file and reads its content line by line,
 *         parsing each key-value pair to populate the `shared_config` structure.
 *
 * @param  filePath  Path to the configuration file.
 * @param  config    Pointer to the shared configuration structure to populate.
 *
 * @return FILEMANAGER_OK if the read operation is successful, FILEMANAGER_ERROR otherwise.
 */
fileManager_StatusTypeDef file_readConfig(const char* filePath, volatile struct shared_config* config)
{
    FIL file;
    FRESULT fr;
    char line[128];

    // Open the file in read mode
    fr = f_open(&file, filePath, FA_READ);
    if (fr != FR_OK)
    {
        return FILEMANAGER_ERROR;
    }

    // Read the file line by line and parse each entry
    while (f_gets(line, sizeof(line), &file))
    {
        file_parseLine(line, config);
    }

    // Close the file
    f_close(&file);
    return FILEMANAGER_OK;
}

/**
 * @brief  Parses a configuration line and updates the shared configuration structure.
 *         This function takes a key-value formatted string (e.g., "KEY=VALUE"),
 *         extracts the parameter name and its corresponding value, and updates
 *         the provided `shared_config` structure accordingly.
 *
 * @param  line   Pointer to the line string to parse.
 * @param  config Pointer to the shared configuration structure to update.
 *
 * @return FILEMANAGER_OK after successful parsing.
 */
fileManager_StatusTypeDef file_parseLine(char* line, volatile struct shared_config* config)
{
    char* token = strtok(line, "=");
    while (token != NULL)
    {
        char* value = strtok(NULL, "\r\n");
        if (value != NULL)
        {
            if (strcmp(token, "UI_BUTTON_DELAY") == 0)
            {
                config->ui_button_delay = strtoul(value, NULL, 10);
            }
            else if (strncmp(token, "NETWORK_IP_ADDR", 15) == 0)
            {
                int index = token[15] - '0';
                if (index >= 0 && index < 4)
                {
                    config->network_ip[index] = (uint8_t)strtoul(value, NULL, 10);
                }
            }
            else if (strncmp(token, "NETWORK_NETMASK_ADDR", 20) == 0)
            {
                int index = token[20] - '0';
                if (index >= 0 && index < 4)
                {
                    config->network_netmask[index] = (uint8_t)strtoul(value, NULL, 10);
                }
            }
            else if (strncmp(token, "NETWORK_GW_ADDR", 15) == 0)
            {
                int index = token[15] - '0';
                if (index >= 0 && index < 4)
                {
                    config->network_gw[index] = (uint8_t)strtoul(value, NULL, 10);
                }
            }
            else if (strncmp(token, "NETWORK_DEST_IP_ADDR", 20) == 0)
            {
                int index = token[20] - '0';
                if (index >= 0 && index < 4)
                {
                    config->network_dest_ip[index] = (uint8_t)strtoul(value, NULL, 10);
                }
            }
            else if (strcmp(token, "NETWORK_UDP_PORT") == 0)
            {
                config->network_udp_port = (uint16_t)strtoul(value, NULL, 10);
            }
            else if (strcmp(token, "NETWORK_TCP_PORT") == 0)
            {
                config->network_tcp_port = (uint16_t)strtoul(value, NULL, 10);
            }
            else if (strcmp(token, "CIS_PRINT_CALIBRATION") == 0)
            {
                config->cis_print_calibration = (uint8_t)strtoul(value, NULL, 10);
            }
            else if (strcmp(token, "CIS_DPI") == 0)
            {
                config->cis_dpi = (uint16_t)strtoul(value, NULL, 10);
            }
            else if (strcmp(token, "CIS_CLK_FREQ") == 0)
            {
                config->cis_clk_freq = strtoul(value, NULL, 10);
            }
            else if (strcmp(token, "CIS_OVERSAMPLING") == 0)
            {
                config->cis_oversampling = (uint8_t)strtoul(value, NULL, 10);
            }
            else if (strcmp(token, "CIS_HANDEDNESS") == 0)
            {
                config->cis_handedness = (uint8_t)strtoul(value, NULL, 10);
            }
        }
        token = strtok(NULL, "=");
    }

    return FILEMANAGER_OK;
}

/**
 * @brief  Writes the shared configuration data to a file.
 *         This function creates or overwrites the specified file and writes
 *         the configuration parameters in a key-value format.
 *
 * @param  filePath  Path to the configuration file.
 * @param  config    Pointer to the shared configuration structure.
 *
 * @return FILEMANAGER_OK if the write operation is successful, FILEMANAGER_ERROR otherwise.
 */
fileManager_StatusTypeDef file_writeConfig(const char* filePath, const volatile struct shared_config* config)
{
    FIL file;
    FRESULT fr;

    // Open the file in write mode (overwrite if exists)
    fr = f_open(&file, filePath, FA_WRITE | FA_CREATE_ALWAYS);
    if (fr != FR_OK)
    {
        return FILEMANAGER_ERROR;
    }

    // Write configuration parameters to the file
    f_printf(&file, "UI_BUTTON_DELAY=%lu\n", config->ui_button_delay);
    f_printf(&file, "NETWORK_IP_ADDR0=%u\n", config->network_ip[0]);
    f_printf(&file, "NETWORK_IP_ADDR1=%u\n", config->network_ip[1]);
    f_printf(&file, "NETWORK_IP_ADDR2=%u\n", config->network_ip[2]);
    f_printf(&file, "NETWORK_IP_ADDR3=%u\n", config->network_ip[3]);
    f_printf(&file, "NETWORK_NETMASK_ADDR0=%u\n", config->network_netmask[0]);
    f_printf(&file, "NETWORK_NETMASK_ADDR1=%u\n", config->network_netmask[1]);
    f_printf(&file, "NETWORK_NETMASK_ADDR2=%u\n", config->network_netmask[2]);
    f_printf(&file, "NETWORK_NETMASK_ADDR3=%u\n", config->network_netmask[3]);
    f_printf(&file, "NETWORK_GW_ADDR0=%u\n", config->network_gw[0]);
    f_printf(&file, "NETWORK_GW_ADDR1=%u\n", config->network_gw[1]);
    f_printf(&file, "NETWORK_GW_ADDR2=%u\n", config->network_gw[2]);
    f_printf(&file, "NETWORK_GW_ADDR3=%u\n", config->network_gw[3]);
    f_printf(&file, "NETWORK_DEST_IP_ADDR0=%u\n", config->network_dest_ip[0]);
    f_printf(&file, "NETWORK_DEST_IP_ADDR1=%u\n", config->network_dest_ip[1]);
    f_printf(&file, "NETWORK_DEST_IP_ADDR2=%u\n", config->network_dest_ip[2]);
    f_printf(&file, "NETWORK_DEST_IP_ADDR3=%u\n", config->network_dest_ip[3]);
    f_printf(&file, "NETWORK_UDP_PORT=%u\n", config->network_udp_port);
    f_printf(&file, "NETWORK_TCP_PORT=%u\n", config->network_tcp_port);
    f_printf(&file, "CIS_PRINT_CALIBRATION=%u\n", config->cis_print_calibration);
    f_printf(&file, "CIS_DPI=%u\n", config->cis_dpi);
    f_printf(&file, "CIS_CLK_FREQ=%lu\n", config->cis_clk_freq);
    f_printf(&file, "CIS_OVERSAMPLING=%u\n", config->cis_oversampling);
    f_printf(&file, "CIS_HANDEDNESS=%u\n", config->cis_handedness);

    // Close the file
    f_close(&file);
    return FILEMANAGER_OK;
}

/**
 * @brief  Prints the shared configuration parameters to the console.
 *         This function displays all configuration parameters in a formatted manner.
 *
 * @param  config  The shared configuration structure to print.
 *
 * @return FILEMANAGER_OK after successful printing.
 */
fileManager_StatusTypeDef print_shared_config(struct shared_config config)
{
    printf("=========== CONFIG ============\n");
    printf("Button Delay: %u ms\n", (unsigned int)config.ui_button_delay);

    printf("Network IP: %u.%u.%u.%u\n",
           config.network_ip[0], config.network_ip[1],
           config.network_ip[2], config.network_ip[3]);

    printf("Network Netmask: %u.%u.%u.%u\n",
           config.network_netmask[0], config.network_netmask[1],
           config.network_netmask[2], config.network_netmask[3]);

    printf("Network Gateway: %u.%u.%u.%u\n",
           config.network_gw[0], config.network_gw[1],
           config.network_gw[2], config.network_gw[3]);

    printf("Network Destination IP: %u.%u.%u.%u\n",
           config.network_dest_ip[0], config.network_dest_ip[1],
           config.network_dest_ip[2], config.network_dest_ip[3]);

    printf("Network UDP Port: %u\n", config.network_udp_port);
    printf("Network TCP Port: %u\n", config.network_tcp_port);
    printf("CIS Print Calibration: %u\n", config.cis_print_calibration);
    printf("CIS DPI: %u\n", config.cis_dpi);
    printf("CIS Clock Frequency: %u Hz\n", (unsigned int)config.cis_clk_freq);
    printf("CIS Oversampling: %u\n", config.cis_oversampling);
    printf("CIS Handedness: %u\n", config.cis_handedness);
    printf("===============================\n");

    return FILEMANAGER_OK;
}

/**
 * @brief  Resets the file system to factory settings.
 *         This function formats the QSPI flash to restore a clean filesystem.
 *
 * @return FILEMANAGER_OK if the reset operation is successful, FILEMANAGER_ERROR otherwise.
 */
fileManager_StatusTypeDef file_factoryReset(void)
{
    printf("- CONFIG FILE TO FACTORY RESET -\n");

    FRESULT fres; // Variable to store the result of FATFS operations

    printf("Attempting to format the QSPI flash...\n");

    BYTE work[WORKING_BUFFER_SIZE]; // Static allocation to simplify

    fres = f_mkfs("0:", FM_ANY, 0, work, WORKING_BUFFER_SIZE);
    if (fres != FR_OK)
    {
        printf("Failed to format the QSPI flash.\n");
        return FILEMANAGER_ERROR;
    }

    return FILEMANAGER_OK;
}

/**
 * @brief  Initializes the file system and loads the configuration.
 *         This function attempts to mount the file system. If mounting fails, it tries
 *         to format the QSPI flash and remount it. It then reads the configuration
 *         file into the provided `shared_config` structure. If the file does not exist
 *         or is unreadable, it writes a default configuration.
 *
 * @param  config  Pointer to the shared configuration structure to be initialized.
 *
 * @return FILEMANAGER_OK if the initialization is successful, FILEMANAGER_ERROR otherwise.
 */
fileManager_StatusTypeDef file_initConfig(volatile struct shared_config* config)
{
    printf("- CONFIG FILE INITIALIZATIONS -\n");

    FRESULT fres; // Variable to store the result of FATFS operations

    // Attempt to mount the file system
    fres = f_mount(&fs, "0:", 1); // 1 to mount immediately
    if (fres != FR_OK)
    {
        printf("FS mount ERROR\n");

        // If mounting fails, try to format the QSPI flash
        printf("Attempting to format the QSPI flash...\n");

        BYTE work[WORKING_BUFFER_SIZE]; // Static allocation to simplify

        fres = f_mkfs("0:", FM_ANY, 0, work, WORKING_BUFFER_SIZE);
        if (fres != FR_OK)
        {
            printf("Failed to format the QSPI flash.\n");
        }

        // Try to mount the file system again after formatting
        fres = f_mount(&fs, "0:", 1);
        if (fres != FR_OK)
        {
            printf("Failed to mount the filesystem even after formatting.\n");
        }
        else
        {
            printf("FS mount SUCCESS after formatting.\n");
        }
    }
    else
    {
        printf("FS mount SUCCESS\n");
    }

    // Attempt to read the configuration file
    if (file_readConfig(CONFIG_FILE_PATH, config) != 0)
    {
        printf("Failed to read configuration file\n");

        // Load default configuration if reading fails
        *config = DefaultConfig;

        // Attempt to write the default configuration to the file system
        if (file_writeConfig(CONFIG_FILE_PATH, config) == 0)
        {
            printf("Write configuration SUCCESS\n");
        }
        else
        {
            printf("Failed to write configuration file\n");
            return FILEMANAGER_ERROR;
        }
    }
    else
    {
        printf("Read configuration SUCCESS\n");
        print_shared_config(*config);
    }

    return FILEMANAGER_OK;
}

/**
 * @brief  Writes CIS calibration data to a file.
 *         This function creates or overwrites the specified file and writes
 *         the provided CIS calibration data into it.
 *
 * @param  filePath  Path to the file where the CIS calibration data will be stored.
 * @param  data      Pointer to the CIS calibration data to be written.
 *
 * @return FILEMANAGER_OK if the write operation is successful, FILEMANAGER_ERROR otherwise.
 */
fileManager_StatusTypeDef file_writeCisCals(const char* filePath, const struct cisCals* data)
{
    FIL file;
    UINT bw;
    FRESULT fr;

    // Open the file in write mode (overwrite if exists)
    fr = f_open(&file, filePath, FA_WRITE | FA_CREATE_ALWAYS);
    if (fr != FR_OK)
    {
        printf("Failed to create calibration file: %s\n", filePath);
        Error_Handler();
        return FILEMANAGER_ERROR;
    }

    // Write data to the file
    fr = f_write(&file, data, sizeof(cisCals), &bw);
    if (fr != FR_OK || bw != sizeof(cisCals))
    {
        printf("Failed to write calibration file\n");
        f_close(&file);
        return FILEMANAGER_ERROR;
    }

    // Close the file
    f_close(&file);
    return FILEMANAGER_OK;
}

/**
 * @brief  Reads CIS calibration data from a file.
 *         This function opens the specified file, reads its content into the
 *         `cisCals` structure, and then closes the file.
 *
 * @param  filePath  Path to the file containing the CIS calibration data.
 * @param  data      Pointer to the structure where the read data will be stored.
 *
 * @return FILEMANAGER_OK if the read operation is successful, FILEMANAGER_ERROR otherwise.
 */
fileManager_StatusTypeDef file_readCisCals(const char* filePath, struct cisCals* data)
{
    FIL file;
    UINT br;
    FRESULT fr;

    // Open the file in read mode
    fr = f_open(&file, filePath, FA_READ);
    if (fr != FR_OK)
    {
        return FILEMANAGER_ERROR;
    }

    // Read data from the file
    fr = f_read(&file, data, sizeof(cisCals), &br);
    if (fr != FR_OK || br != sizeof(cisCals))
    {
        f_close(&file);
        return FILEMANAGER_ERROR;
    }

    // Close the file
    f_close(&file);
    return FILEMANAGER_OK;
}

/**
 * @brief Computes CRC over a memory buffer in streaming mode using STM32H7 hardware CRC.
 *
 * This function:
 *  - Resets the CRC Data Register for a new calculation
 *  - Processes data in small chunks (CHUNK_SIZE)
 *  - Feeds each chunk to HAL_CRC_Accumulate()
 *  - Returns the final CRC value
 *
 * @param hcrc      Pointer to the CRC_HandleTypeDef
 * @param pData     Pointer to the data in memory
 * @param length    Number of bytes to compute CRC over
 * @return 32-bit CRC result as configured by hcrc.Init parameters, or 0 if error
 */
static uint32_t file_computeCRC_buffer(CRC_HandleTypeDef *hcrc, const uint8_t *pData, uint32_t length)
{
    // Reset the CRC Data Register for a new calculation
    __HAL_CRC_DR_RESET(hcrc);

    uint32_t totalProcessed = 0;
    uint32_t crcVal = 0;

    while (totalProcessed < length)
    {
        // Determine how many bytes in this chunk
        uint32_t chunkLen = (length - totalProcessed < CHUNK_SIZE)
                            ? (length - totalProcessed)
                            : CHUNK_SIZE;

        // Round up chunkLen to a multiple of 4
        uint32_t remainder = chunkLen % 4U;
        uint32_t wordAlignedSize = chunkLen + (remainder ? (4U - remainder) : 0U);

        // Create a small temp buffer
        uint8_t tempBuf[CHUNK_SIZE + 3]; // +3 to handle up to 3 bytes of padding
        memcpy(tempBuf, pData + totalProcessed, chunkLen);

        // Pad leftover bytes with 0x00
        memset(tempBuf + chunkLen, 0, wordAlignedSize - chunkLen);

        // Accumulate
        // The return value after each call is the running CRC so far
        crcVal = HAL_CRC_Accumulate(hcrc, (uint32_t *)tempBuf, wordAlignedSize / 4U);

        totalProcessed += chunkLen;
    }

    // crcVal is the final CRC
    return crcVal;
}

/**
 * @brief  Reliably writes data to a file with CRC verification and retries.
 *         This function writes a data block to the file, flushes it, and then
 *         verifies its integrity by reading it back and computing a CRC.
 *         If the verification fails, it retries the operation up to `maxRetries` times.
 *
 * @param  file       Pointer to the open file.
 * @param  buffer     Pointer to the data buffer to be written.
 * @param  length     Size of the data buffer in bytes.
 * @param  maxRetries Maximum number of retries in case of failure.
 *
 * @return FILEMANAGER_OK if the write operation is successful, FILEMANAGER_ERROR otherwise.
 */
fileManager_StatusTypeDef file_reliableWrite(FIL *file, const uint8_t *buffer, uint32_t length, int maxRetries)
{
    FRESULT fres;
    UINT bytesWritten;
    UINT bytesRead;
    DWORD currentPos;

    // 1) Compute CRC of the source buffer in streaming mode
    uint32_t originalCRC = file_computeCRC_buffer(&hcrc, buffer, length);

    // 2) Remember current position
    currentPos = f_tell(file);

    // Attempt multiple times
    for (int attempt = 1; attempt <= maxRetries; attempt++)
    {
        // 3) Write the entire data block
        fres = f_write(file, buffer, length, &bytesWritten);
        if ((fres != FR_OK) || (bytesWritten != length))
        {
            printf("Error: f_write() attempt %d failed.\n", attempt);
        }
        else
        {
            // 4) Flush the file data
            fres = f_sync(file);
            if (fres != FR_OK)
            {
                printf("Error: f_sync() attempt %d failed.\n", attempt);
            }
            else
            {
                // 5) Move file pointer back to where we wrote
                fres = f_lseek(file, currentPos);
                if (fres != FR_OK)
                {
                    printf("Error: f_lseek() attempt %d failed.\n", attempt);
                }
                else
                {
                    // 6) Read back in chunks, accumulate CRC
                    //    We will perform a streaming CRC on the read data
                    uint32_t totalRead = 0;
                    uint32_t readCRC = 0;

                    // Reset CRC hardware for new accumulation
                    __HAL_CRC_DR_RESET(&hcrc);

                    while (totalRead < length)
                    {
                        uint8_t verifyBuf[CHUNK_SIZE + 3];
                        UINT chunkSize = (length - totalRead < CHUNK_SIZE)
                                            ? (length - totalRead)
                                            : CHUNK_SIZE;

                        // Read chunk
                        fres = f_read(file, verifyBuf, chunkSize, &bytesRead);
                        if ((fres != FR_OK) || (bytesRead != chunkSize))
                        {
                            printf("Error: f_read() attempt %d failed.\n", attempt);
                            break;
                        }

                        // Round up to a multiple of 4
                        uint32_t remainder = chunkSize % 4U;
                        uint32_t wordAlignedSize = chunkSize + (remainder ? (4U - remainder) : 0U);

                        // Pad leftover bytes
                        memset(verifyBuf + chunkSize, 0, wordAlignedSize - chunkSize);

                        // Accumulate the chunk's CRC
                        readCRC = HAL_CRC_Accumulate(&hcrc, (uint32_t *)verifyBuf, wordAlignedSize / 4U);

                        totalRead += chunkSize;
                    }

                    if ((fres == FR_OK) && (totalRead == length))
                    {
                        // Compare CRC
                        if (readCRC == originalCRC)
                        {
                            // 7) All good; move pointer after the written block
                            f_lseek(file, currentPos + length);
                            return FILEMANAGER_OK;
                        }
                        else
                        {
                            printf("CRC mismatch in attempt %d. Retrying.\n", attempt);
                        }
                    }
                }
            }
        }

        // On failure, reset file pointer to initial position before next attempt
        f_lseek(file, currentPos);
    }

    printf("Error: file_reliableWrite() failed after %d attempts.\n", maxRetries);
    return FILEMANAGER_ERROR;
}
#endif
