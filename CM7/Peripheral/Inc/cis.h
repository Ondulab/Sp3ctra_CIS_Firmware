/**
  ******************************************************************************
  * @file           : cis.h
  * @brief          : Header for cis.c file.
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __CIS_H__
#define __CIS_H__

#include "config.h"
#include "main.h"
#include "arm_math.h"
#include "shared.h"

typedef enum
{
	CIS_BUFFER_OFFSET_NONE = 0,
	CIS_BUFFER_OFFSET_HALF,
	CIS_BUFFER_OFFSET_FULL,
}CIS_BUFF_StateTypeDef;

typedef enum
{
	CIS_RED = 0,
	CIS_GREEN,
	CIS_BLUE,
}CIS_Color_TypeDef;

#define CIS_RED_LANE_OFFSET 		(CIS_START_OFFSET)
#define CIS_GREEN_LANE_OFFSET 		((CIS_LANE_SIZE) + (CIS_START_OFFSET))
#define CIS_BLUE_LANE_OFFSET  		((CIS_LANE_SIZE * 2)) + (CIS_START_OFFSET)

void cis_Init(void);
void cis_getRAWImage(float32_t* cisDataCpy_f32, uint16_t overSampling);
void cis_ConvertRAWImageToFloatArray(float32_t* cisDataCpy_f32, struct RAWImage* RAWImage);
void cis_ImageProcess_2(int32_t *cis_buff);
void cis_ImageProcess(float32_t* cisDataCpy_f32, struct packet_Image *imageBuffers);
void cis_ImageProcessRGB_Calibration(float32_t *cisCalData, uint16_t iterationNb);
void cis_Start_capture(void);
void cis_Stop_capture(void);
void cis_LedsOn(void);
void cis_LedsOff(void);
void cis_LedPowerAdj(int32_t red_pwm, int32_t green_pwm, int32_t blue_pwm);

#endif /* __CIS_H__ */
