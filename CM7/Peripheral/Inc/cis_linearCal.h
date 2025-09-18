/**
 ******************************************************************************
 * @file           : cis_linearCal.h
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
#ifndef __CIS_LINEARCAL_H__
#define __CIS_LINEARCAL_H__

/* Includes ------------------------------------------------------------------*/

/* Custom return type for CIS calibration -----------------------------*/
typedef enum {
	CISCALIBRATION_OK = 0,
	CISCALIBRATION_ERROR = 1
} CISCALIBRATION_StatusTypeDef;

/* Private define ------------------------------------------------------------*/

CISCALIBRATION_StatusTypeDef cis_linearCalibrationInit(void);
void cis_applyLinearCalibration(int32_t * restrict cisDataCpy, uint32_t maxClipValue);
void cis_startLinearCalibration(int32_t *cisDataCpy, uint16_t iterationNb, uint32_t bitDepth);
void cis_printForCharacterization(float32_t* cisDataCpy_f32);

/* Global drift correction functions */
void cis_computeGlobalDriftCorrection(const int32_t * restrict cisDataCpy, int32_t globalDriftOffset[3][CIS_ADC_OUT_LANES]);
void cis_applyLinearCalibrationWithDriftCorrection(int32_t * restrict cisDataCpy, uint32_t maxClipValue);
void cis_initDriftCorrectionDefaults(void);

/* Debug functions */
void cis_enableDriftDebug(uint32_t print_interval);
void cis_disableDriftDebug(void);
void cis_printInactivePixels(const int32_t * restrict cisDataCpy, uint32_t lane, int color);
void cis_enableDetailedDebug(uint32_t print_interval);
void cis_disableDetailedDebug(void);

#endif /* __CIS_LINEARCAL_H__ */
