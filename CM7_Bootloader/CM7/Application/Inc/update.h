/**
 ******************************************************************************
 * @file           : update.h
 * @brief          : Header for update.c file.
 *                   Contains prototypes and definitions for update functionality.
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

#ifndef UPDATE_H
#define UPDATE_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>
#include "ff.h"       // For FatFS types


/* Private define ------------------------------------------------------------*/

/* Custom return type for update operations -----------------------------*/
typedef enum {
    FWUPDATE_OK = 0,
	FWUPDATE_ERROR = 1,
	FWUPDATE_CRCMISMATCH = 2
} fwupdate_StatusTypeDef;

/* Exported constants --------------------------------------------------------*/

/* Exported types ------------------------------------------------------------*/

/* Exported functions --------------------------------------------------------*/

fwupdate_StatusTypeDef update_findPackageFile(char *packageFilePath, size_t maxLen);
fwupdate_StatusTypeDef update_restoreBackupFirmwares(void);
fwupdate_StatusTypeDef update_processPackageFile(const TCHAR* packageFilePath);

/* Exported macros -----------------------------------------------------------*/
/* Add any necessary macros here */

#ifdef __cplusplus
}
#endif

#endif /* UPDATE_H */
