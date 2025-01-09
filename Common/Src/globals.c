/**
 ******************************************************************************
 * @file           : globals.c
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

/* Includes ------------------------------------------------------------------*/
#include "globals.h"

/* Private includes ----------------------------------------------------------*/

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/**************************************************************************************/
/******************                  CM4 and CM7                    *******************/
/**************************************************************************************/

#pragma GCC push_options
#pragma GCC optimize ("O0")

__attribute__ ((section(".shared_var")))
volatile struct shared_var shared_var = {0};

__attribute__ ((section(".shared_config")))
volatile struct shared_config shared_config = {0};

__attribute__ ((section(".cisCals")))
struct cisCals cisCals = {0};

__attribute__ ((section(".rgbBuffers")))
struct packet_Image packet_Image[UDP_MAX_NB_PACKET_PER_LINE * 2] = {0};

__attribute__ ((section(".imuData")))
struct packet_IMU packet_IMU = {0};

#pragma GCC pop_options

/**************************************************************************************/
/******************                      CM7                        *******************/
/**************************************************************************************/

#ifdef CORE_CM7

#pragma GCC push_options
#pragma GCC optimize ("O0")

__attribute__ ((section(".cisDataCpy")))
float32_t cisDataCpy_f32[CIS_MAX_ADC_BUFF_SIZE * 3] = {0};

__attribute__ ((section(".cisData")))
int16_t cisData[CIS_MAX_ADC_BUFF_SIZE * 3] __attribute__ ((aligned (32))) = {0};

__attribute__ ((section(".cisRGBCalibration")))
struct cisRGB_Calibration cisRGB_Calibration = {0};

__attribute__ ((section(".cisLedsCalibration")))
struct cisLeds_Calibration cisLeds_Calibration = {0};

CIS_Config cisConfig = {0};

#pragma GCC pop_options

#endif

/* Private function prototypes -----------------------------------------------*/

/* Private user code ---------------------------------------------------------*/
