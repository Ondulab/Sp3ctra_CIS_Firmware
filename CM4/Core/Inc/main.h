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

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

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
#define OLED_RESET_Pin GPIO_PIN_8
#define OLED_RESET_GPIO_Port GPIOB
#define EN_12V_Pin GPIO_PIN_5
#define EN_12V_GPIO_Port GPIOG
#define SW_3_Pin GPIO_PIN_14
#define SW_3_GPIO_Port GPIOE
#define LED3_Pin GPIO_PIN_12
#define LED3_GPIO_Port GPIOE
#define SW_2_Pin GPIO_PIN_13
#define SW_2_GPIO_Port GPIOE
#define SW_1_Pin GPIO_PIN_15
#define SW_1_GPIO_Port GPIOE
#define LED1_Pin GPIO_PIN_6
#define LED1_GPIO_Port GPIOH
#define LED2_Pin GPIO_PIN_11
#define LED2_GPIO_Port GPIOE

/* USER CODE BEGIN Private defines */
#define MEMS_FSYNC_Pin GPIO_PIN_15
#define MEMS_FSYNC_GPIO_Port GPIOA
#define MEMS_CS_Pin GPIO_PIN_9
#define MEMS_CS_GPIO_Port GPIOB
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
