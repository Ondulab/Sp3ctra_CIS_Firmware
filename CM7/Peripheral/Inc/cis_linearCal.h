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
/* Private define ------------------------------------------------------------*/

void cis_linearCalibrationInit(void);
void cis_applyLinearCalibration(uint32_t * restrict cisDataCpy, uint32_t maxClipValue);
void cis_startLinearCalibration(uint32_t *cisDataCpy, uint16_t iterationNb, uint32_t bitDepth);
void cis_printForCharacterization(float32_t* cisDataCpy_f32);

#endif /* __CIS_LINEARCAL_H__ */
