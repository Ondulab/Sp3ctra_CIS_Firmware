/**
 ******************************************************************************
 * @file           : file_manager_fs.c
 * @brief          : FATFS shared instance for bootloader.
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

#include "ff.h"

/*
 * Bootloader needs a FATFS instance for f_mount().
 * Keep it separate from configuration/persistent data handling.
 */
FATFS fs;

#endif /* CORE_CM7 */
