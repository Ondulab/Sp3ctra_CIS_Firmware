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
#define DEST_IP_ADDR0 192
#define DEST_IP_ADDR1 168
#define DEST_IP_ADDR2 1
#define DEST_IP_ADDR3 206
#define DEST_PORT 7
#define OLED_RESET_Pin GPIO_PIN_8
#define OLED_RESET_GPIO_Port GPIOB
#define MEMS_FSYNC_Pin GPIO_PIN_15
#define MEMS_FSYNC_GPIO_Port GPIOA
#define MEMS_CS_Pin GPIO_PIN_9
#define MEMS_CS_GPIO_Port GPIOB
#define CIS_RS_Pin GPIO_PIN_12
#define CIS_RS_GPIO_Port GPIOA
#define DAC_FLT_Pin GPIO_PIN_13
#define DAC_FLT_GPIO_Port GPIOC
#define CIS_CP_Pin GPIO_PIN_9
#define CIS_CP_GPIO_Port GPIOA
#define CIS_SP_Pin GPIO_PIN_8
#define CIS_SP_GPIO_Port GPIOC
#define CIS_VLED_G_Pin GPIO_PIN_6
#define CIS_VLED_G_GPIO_Port GPIOC
#define EN_12V_Pin GPIO_PIN_5
#define EN_12V_GPIO_Port GPIOG
#define EN_5V_Pin GPIO_PIN_2
#define EN_5V_GPIO_Port GPIOG
#define DAC_FMT_Pin GPIO_PIN_6
#define DAC_FMT_GPIO_Port GPIOF
#define CIS_VLED_R_Pin GPIO_PIN_13
#define CIS_VLED_R_GPIO_Port GPIOD
#define MEMS_INT_Pin GPIO_PIN_8
#define MEMS_INT_GPIO_Port GPIOD
#define DAC_XSMT_Pin GPIO_PIN_2
#define DAC_XSMT_GPIO_Port GPIOC
#define SW_4_Pin GPIO_PIN_14
#define SW_4_GPIO_Port GPIOE
#define CIS_ADC_3_Pin GPIO_PIN_3
#define CIS_ADC_3_GPIO_Port GPIOC
#define CIS_VLED_B_Pin GPIO_PIN_12
#define CIS_VLED_B_GPIO_Port GPIOH
#define CIS_ADC_2_Pin GPIO_PIN_1
#define CIS_ADC_2_GPIO_Port GPIOA
#define SW_5_Pin GPIO_PIN_12
#define SW_5_GPIO_Port GPIOE
#define SW_1_Pin GPIO_PIN_13
#define SW_1_GPIO_Port GPIOE
#define SW_3_Pin GPIO_PIN_15
#define SW_3_GPIO_Port GPIOE
#define LED_Pin GPIO_PIN_6
#define LED_GPIO_Port GPIOH
#define CIS_ADC_1_Pin GPIO_PIN_0
#define CIS_ADC_1_GPIO_Port GPIOA
#define DAC_DEMP_Pin GPIO_PIN_3
#define DAC_DEMP_GPIO_Port GPIOH
#define SW_2_Pin GPIO_PIN_11
#define SW_2_GPIO_Port GPIOE
/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
