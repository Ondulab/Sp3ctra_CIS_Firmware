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

void cis_linealCalibrationInit(void);
void cis_ApplyLinearCalibration(float32_t* cisDataCpy_f32);
void cis_StartLinearCalibration(uint16_t iterationNb);

#endif /* __CIS_LINEARCAL_H__ */
