/**
 ******************************************************************************
 * @file           : cis_scan.h
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
#ifndef CIS_SCAN_H
#define CIS_SCAN_H

extern TaskHandle_t cis_scanThreadHandle;

/* Includes ------------------------------------------------------------------*/

/* Private defines -----------------------------------------------------------*/

void cis_scanInit(void);

#endif /* CIS_SCAN_H */
