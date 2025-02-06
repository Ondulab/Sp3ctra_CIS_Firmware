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

/* Includes ------------------------------------------------------------------*/

/* Private defines -----------------------------------------------------------*/
/* Custom return type for CIS scan functions -----------------------------*/
typedef enum {
	CISSCAN_OK = 0,
	CISSCAN_ERROR = 1
} CISSCAN_StatusTypeDef;

CISSCAN_StatusTypeDef cis_scanInit(void);

#endif /* CIS_SCAN_H */
