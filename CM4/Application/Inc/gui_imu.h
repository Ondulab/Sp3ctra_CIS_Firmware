/**
 ******************************************************************************
 * @file           : gui_imu.h
 * @brief          : GUI IMU module header - IMU data display and motion detection
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

#ifndef __GUI_IMU_H
#define __GUI_IMU_H

/* Includes ------------------------------------------------------------------*/
#include "stdbool.h"
#include "stdint.h"
#include "basetypes.h"

/* Exported types ------------------------------------------------------------*/
// Structure to store IMU data and moving average
struct IMU_average
{
    float32_t acc[3];  // Moving average of accelerometer
    float32_t gyro[3]; // Moving average of gyroscope
};

/* Exported function prototypes ----------------------------------------------*/
void gui_displayIMU(void);
void update_IMU_average(void);
bool gui_isSignificantMotion(void);

#endif /* __GUI_IMU_H */
