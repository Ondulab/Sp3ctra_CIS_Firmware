/**
 ******************************************************************************
 * @file           : file_manager_boot.c
 * @brief          : File manager primitives used by bootloader (stable).
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

#include "crc.h"
#include "ff.h"

#include <string.h>
#include <stdio.h>

#include "file_manager_boot.h"

/* Private define ------------------------------------------------------------*/
#define CHUNK_SIZE 4096

/* Private function prototypes -----------------------------------------------*/
static uint32_t file_computeCRC_buffer(CRC_HandleTypeDef *hcrc, const uint8_t *pData, uint32_t length);

static uint32_t file_computeCRC_buffer(CRC_HandleTypeDef *hcrc, const uint8_t *pData, uint32_t length)
{
    __HAL_CRC_DR_RESET(hcrc);

    uint32_t totalProcessed = 0;
    uint32_t crcVal = 0;

    while (totalProcessed < length)
    {
        uint32_t chunkLen = (length - totalProcessed < CHUNK_SIZE)
                            ? (length - totalProcessed)
                            : CHUNK_SIZE;

        uint32_t remainder = chunkLen % 4U;
        uint32_t wordAlignedSize = chunkLen + (remainder ? (4U - remainder) : 0U);

        uint8_t tempBuf[CHUNK_SIZE + 3];
        memcpy(tempBuf, pData + totalProcessed, chunkLen);
        memset(tempBuf + chunkLen, 0, wordAlignedSize - chunkLen);

        crcVal = HAL_CRC_Accumulate(hcrc, (uint32_t *)tempBuf, wordAlignedSize / 4U);
        totalProcessed += chunkLen;
    }

    return crcVal;
}

fileManager_StatusTypeDef file_reliableWrite(FIL *file, const uint8_t *buffer, uint32_t length, int maxRetries)
{
    FRESULT fres;
    UINT bytesWritten;
    UINT bytesRead;
    DWORD currentPos;

    uint32_t originalCRC = file_computeCRC_buffer(&hcrc, buffer, length);
    currentPos = f_tell(file);

    for (int attempt = 1; attempt <= maxRetries; attempt++)
    {
        fres = f_write(file, buffer, length, &bytesWritten);
        if ((fres != FR_OK) || (bytesWritten != length))
        {
            printf("Error: f_write() attempt %d failed.\n", attempt);
        }
        else
        {
            fres = f_sync(file);
            if (fres != FR_OK)
            {
                printf("Error: f_sync() attempt %d failed.\n", attempt);
            }
            else
            {
                fres = f_lseek(file, currentPos);
                if (fres != FR_OK)
                {
                    printf("Error: f_lseek() attempt %d failed.\n", attempt);
                }
                else
                {
                    uint32_t totalRead = 0;
                    uint32_t readCRC = 0;

                    __HAL_CRC_DR_RESET(&hcrc);

                    while (totalRead < length)
                    {
                        uint8_t verifyBuf[CHUNK_SIZE + 3];
                        UINT chunkSize = (length - totalRead < CHUNK_SIZE)
                                            ? (length - totalRead)
                                            : CHUNK_SIZE;

                        fres = f_read(file, verifyBuf, chunkSize, &bytesRead);
                        if ((fres != FR_OK) || (bytesRead != chunkSize))
                        {
                            printf("Error: f_read() attempt %d failed.\n", attempt);
                            break;
                        }

                        uint32_t remainder = chunkSize % 4U;
                        uint32_t wordAlignedSize = chunkSize + (remainder ? (4U - remainder) : 0U);
                        memset(verifyBuf + chunkSize, 0, wordAlignedSize - chunkSize);

                        readCRC = HAL_CRC_Accumulate(&hcrc, (uint32_t *)verifyBuf, wordAlignedSize / 4U);
                        totalRead += chunkSize;
                    }

                    if ((fres == FR_OK) && (totalRead == length))
                    {
                        if (readCRC == originalCRC)
                        {
                            (void)f_lseek(file, currentPos + length);
                            return FILEMANAGER_OK;
                        }

                        printf("CRC mismatch in attempt %d. Retrying.\n", attempt);
                    }
                }
            }
        }

        (void)f_lseek(file, currentPos);
    }

    printf("Error: file_reliableWrite() failed after %d attempts.\n", maxRetries);
    return FILEMANAGER_ERROR;
}

#endif /* CORE_CM7 */
