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

#define CALIBRATION_FILE_PATH "0:/calibration.bin"
#define CONFIG_FILE_PATH "0:/config.txt"

int file_readConfig(const char* filePath, struct shared_config* config);
int file_writeConfig(const char* filePath, const struct shared_config* config);
void file_parseLine(char* line, struct shared_config* config);
void file_initConfig(struct shared_config* config);
int file_writeCisCals(const char* filePath, const struct cisCals* data);
int file_readCisCals(const char* filePath, struct cisCals* data);

#endif // FILE_MANAGER_H
