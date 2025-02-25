/**
 ******************************************************************************
 * @file           : cis.h
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
#ifndef __CIS_H__
#define __CIS_H__

#include "config.h"
#include "main.h"
#include "arm_math.h"
#include "globals.h"

/* Custom return type for CIS peripheral -----------------------------*/
typedef enum {
	CIS_OK = 0,
	CIS_ERROR = 1
} CIS_StatusTypeDef;

typedef enum
{
	CIS_BUFFER_OFFSET_NONE = 0,
	CIS_BUFFER_COMPLETE,
}CIS_BUFF_StateTypeDef;

typedef enum
{
	CIS_RED = 0,
	CIS_GREEN,
	CIS_BLUE,
}CIS_Color_TypeDef;

CIS_StatusTypeDef cis_init(void);
CIS_StatusTypeDef cis_configure(void);
void cis_convertRAWImageToFloatArray(float32_t* cisDataCpy_f32, struct RAWImage* RAWImage);
void cis_imageProcess_2(int32_t* cis_buff);
void cis_imageProcess(int32_t* cisDataCpy_int, struct packet_Scanline *imageBuffers);
void cis_imageProcessRGB_Calibration(int32_t *cisDataCpy, uint32_t *cisCalData, uint16_t iterationNb);
void cis_startCapture(void);
void cis_stopCapture(void);
void cis_ledsOn(void);
void cis_ledsOff(void);
void cis_ledPowerAdj(int32_t red_pwm, int32_t green_pwm, int32_t blue_pwm);
void MDMA_XferCpltCallback(MDMA_HandleTypeDef *hmdma);

#endif /* __CIS_H__ */
