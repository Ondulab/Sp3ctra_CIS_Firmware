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

enum color { RED, GREEN, BLUE };

__IO uint32_t cis_adc_data[CIS_PIXELS_NB];

#ifdef DEBUG_CIS
extern __IO uint32_t cis_dbg_cnt;
extern __IO uint32_t cis_dbg_data_cal;
extern __IO uint32_t cis_dbg_data;
#endif

/* Includes ------------------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/

void cisInit(void);

#endif /* __CIS_H__ */
