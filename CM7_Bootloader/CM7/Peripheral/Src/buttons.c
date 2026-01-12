/**
 ******************************************************************************
 * @file           : buttons.c
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

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
#include "buttons.h"
#include "ssd1362.h"
#include "config.h"


/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
buttonStateTypeDef  buttonState[5] = {0};

/* Private function prototypes -----------------------------------------------*/

/* Private user code ---------------------------------------------------------*/

/**
 * @brief EXTI line detection callbacks
 * @param GPIO_Pin: Specifies the pins connected EXTI line
 * @retval None
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	switch (GPIO_Pin)
	{
	case SW_1_Pin:
		buttonState[SW1] = SWITCH_PRESSED;
		break;
	case SW_2_Pin:
		buttonState[SW2] = SWITCH_PRESSED;
		break;
	case SW_3_Pin:
		buttonState[SW3] = SWITCH_PRESSED;
		break;
	}
}
