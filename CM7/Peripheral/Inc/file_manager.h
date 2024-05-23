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

#ifndef FILE_MANAGER_H
#define FILE_MANAGER_H

#include "basetypes.h"
#include "shared.h"

#include "ff.h"
#include "stdint.h"
#include "string.h"
#include "stdlib.h"

#define ROOT_PATH "0:/"
#define CALIBRATION_FILE_PATH "0:/CALIB.BIN"
#define CONFIG_FILE_PATH "0:/CONFIG.TXT"

int file_factoryReset(void);
void file_initConfig(volatile struct shared_config* config);
int file_readConfig(const char* filePath, volatile struct shared_config* config);
int file_writeConfig(const char* filePath, const volatile struct shared_config* config);
int file_writeCisCals(const char* filePath, const struct cisCals* data);
int file_readCisCals(const char* filePath, struct cisCals* data);

#endif // FILE_MANAGER_H
