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

extern uint32_t __IO cis_adc_data[CIS_PIXELS_NB];
extern __IO uint32_t cis_cnt;

/* Includes ------------------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/

void cisInit(void);

#endif /* __CIS_H__ */
