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
#ifndef SSS_SCAN_H
#define SSS_SCAN_H

typedef enum
{
	MAIN_SCAN_LOOP_FLG_RESET = 0,
	MAIN_SCAN_LOOP_FLG_SET,
}Scan_StateTypeDef;

extern volatile Scan_StateTypeDef main_loop_flg;

/* Includes ------------------------------------------------------------------*/

/* Private defines -----------------------------------------------------------*/

int cis_scan(void);

#endif /* SSS_SCAN_H */
