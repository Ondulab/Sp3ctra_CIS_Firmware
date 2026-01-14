/**
 ******************************************************************************
 * @file           : file_manager_config.h
 * @brief          : Configuration and persistent data file services (evolutive).
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

#ifndef FILE_MANAGER_CONFIG_H
#define FILE_MANAGER_CONFIG_H

#include "basetypes.h"
#include "globals.h"

#include "file_manager.h" /* for fileManager_StatusTypeDef and FATFS fs */

fileManager_StatusTypeDef file_factoryReset(void);
fileManager_StatusTypeDef file_initConfig(volatile struct shared_config* config);
fileManager_StatusTypeDef file_readConfig(const char* filePath, volatile struct shared_config* config);
fileManager_StatusTypeDef file_writeConfig(const char* filePath, const volatile struct shared_config* config);

fileManager_StatusTypeDef file_writeCisCals(const char* filePath, const struct cisCals* data);
fileManager_StatusTypeDef file_readCisCals(const char* filePath, struct cisCals* data);
fileManager_StatusTypeDef file_writeImuCals(const char* filePath, const struct imuCals* data);
fileManager_StatusTypeDef file_readImuCals(const char* filePath, struct imuCals* data);

#endif /* FILE_MANAGER_CONFIG_H */
