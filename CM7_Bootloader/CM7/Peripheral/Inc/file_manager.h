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

/* Bootloader only exposes stable primitives (no CONFIG.TXT parsing here). */

#include "file_manager_boot.h"

#endif // FILE_MANAGER_H
