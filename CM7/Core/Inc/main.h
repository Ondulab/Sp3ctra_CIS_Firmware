/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "config.h"
#include "shared.h"
#include "basetypes.h"

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */
extern volatile unsigned long ulHighFrequencyTimerTicks;
/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define MEMS_FSYNC_Pin GPIO_PIN_15
#define MEMS_FSYNC_GPIO_Port GPIOA
#define MEMS_CS_Pin GPIO_PIN_9
#define MEMS_CS_GPIO_Port GPIOB
#define CIS_RS_Pin GPIO_PIN_12
#define CIS_RS_GPIO_Port GPIOA
#define ETH_RST_Pin GPIO_PIN_14
#define ETH_RST_GPIO_Port GPIOC
#define CIS_CP_Pin GPIO_PIN_9
#define CIS_CP_GPIO_Port GPIOA
#define CIS_SP_Pin GPIO_PIN_8
#define CIS_SP_GPIO_Port GPIOC
#define CIS_LED_B_Pin GPIO_PIN_6
#define CIS_LED_B_GPIO_Port GPIOC
#define EN_5V_Pin GPIO_PIN_2
#define EN_5V_GPIO_Port GPIOG
#define CIS_LED_R_Pin GPIO_PIN_13
#define CIS_LED_R_GPIO_Port GPIOD
#define MEMS_INT_Pin GPIO_PIN_8
#define MEMS_INT_GPIO_Port GPIOD
#define CIS_ADC_3_Pin GPIO_PIN_3
#define CIS_ADC_3_GPIO_Port GPIOC
#define CIS_LED_G_Pin GPIO_PIN_12
#define CIS_LED_G_GPIO_Port GPIOH
#define CIS_ADC_2_Pin GPIO_PIN_1
#define CIS_ADC_2_GPIO_Port GPIOA
#define CIS_ADC_1_Pin GPIO_PIN_0
#define CIS_ADC_1_GPIO_Port GPIOA

/* USER CODE BEGIN Private defines */
#define SW1_Pin GPIO_PIN_14
#define SW1_GPIO_Port GPIOE
#define SW3_Pin GPIO_PIN_13
#define SW3_GPIO_Port GPIOE
#define SW2_Pin GPIO_PIN_15
#define SW2_GPIO_Port GPIOE

#define LED1_Pin GPIO_PIN_6
#define LED1_GPIO_Port GPIOH
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
