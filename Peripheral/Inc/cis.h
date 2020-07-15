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

#define CIS_SP_GPIO_Port ARD_D6_GPIO_Port
#define CIS_SP_Pin ARD_D6_Pin

#define CIS_LED_R_GPIO_Port ARD_D10_GPIO_Port
#define CIS_LED_G_GPIO_Port ARD_D0_GPIO_Port
#define CIS_LED_B_GPIO_Port ARD_D11_GPIO_Port

#define CIS_LED_R_Pin ARD_D10_Pin
#define CIS_LED_G_Pin ARD_D0_Pin
#define CIS_LED_B_Pin ARD_D11_Pin

/* Includes ------------------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/

void cis_RGB888_Init(void);
void cis_RGB888_Test(void);
uint8_t cis_RGB888_GetBuffData(uint32_t index);
void cis_RGB888_ImageProcess(void);

void cis_BW_Init(void);
void cis_BW_Test(void);
uint8_t cis_BW_GetBuffData(uint32_t index);
void cis_BW_ImageProcess(void);

#endif /* __CIS_H__ */
