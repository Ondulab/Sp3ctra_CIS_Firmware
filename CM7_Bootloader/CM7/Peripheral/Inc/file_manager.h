/**
 ******************************************************************************
 * @file           : file_manager.h
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
#ifndef FILE_MANAGER_H
#define FILE_MANAGER_H

/* Includes ------------------------------------------------------------------*/
#include "basetypes.h"
#include "globals.h"

#include "ff.h"
#include "stdint.h"
#include "string.h"
#include "stdlib.h"

/* Private define ------------------------------------------------------------*/

/* Custom return type for STM32 file operations -----------------------------*/
typedef enum {
    FILEMANAGER_OK = 0,
	FILEMANAGER_ERROR = 1
} fileManager_StatusTypeDef;

extern FATFS fs;

fileManager_StatusTypeDef file_factoryReset(void);
fileManager_StatusTypeDef file_initConfig(volatile struct shared_config* config);
fileManager_StatusTypeDef file_readConfig(const char* filePath, volatile struct shared_config* config);
fileManager_StatusTypeDef file_writeConfig(const char* filePath, const volatile struct shared_config* config);
fileManager_StatusTypeDef file_writeCisCals(const char* filePath, const struct cisCals* data);
fileManager_StatusTypeDef file_readCisCals(const char* filePath, struct cisCals* data);
fileManager_StatusTypeDef file_reliableWrite(FIL *file, const uint8_t *buffer, uint32_t length, int maxRetries);

#endif // FILE_MANAGER_H
