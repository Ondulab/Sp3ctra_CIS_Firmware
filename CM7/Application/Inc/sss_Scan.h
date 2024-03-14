/*
 * sss_Scan.h
 *
 *  Created on: May 31, 2020
 *      Author: zhonx
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

int sss_Scan(void);

#endif /* SSS_SCAN_H */
