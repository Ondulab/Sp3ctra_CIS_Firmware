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

/*
 * This header is kept for backwards compatibility.
 * Public APIs are split across two headers:
 *  - file_manager_boot.h   (stable primitives shared with bootloader)
 *  - file_manager_config.h (configuration & persistent data, evolutive)
 */

#include "file_manager_boot.h"
#include "file_manager_config.h"

#endif // FILE_MANAGER_H
