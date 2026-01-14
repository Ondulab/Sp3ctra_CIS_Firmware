/**
 ******************************************************************************
 * @file           : file_manager_boot.h
 * @brief          : File manager primitives used by bootloader (stable API).
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

#ifndef FILE_MANAGER_BOOT_H
#define FILE_MANAGER_BOOT_H

#include "ff.h"
#include "stdint.h"

#include "file_manager.h" /* for fileManager_StatusTypeDef */

fileManager_StatusTypeDef file_reliableWrite(FIL *file, const uint8_t *buffer, uint32_t length, int maxRetries);

#endif /* FILE_MANAGER_BOOT_H */
