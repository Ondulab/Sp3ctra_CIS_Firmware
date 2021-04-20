/**
 ******************************************************************************
 * @file           : buttons.c
 * @brief          : 5 key keybord driver
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
#include "buttons.h"
#include "ssd1362.h"


/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/* Private user code ---------------------------------------------------------*/

/**
 * @brief EXTI line detection callbacks
 * @param GPIO_Pin: Specifies the pins connected EXTI line
 * @retval None
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	ssd1362_drawRect(0 + 10, 63, 10 + 10, 57, 0x05, 1);
	ssd1362_drawRect(56 + 10, 63, 56 + 10 + 10, 57, 0x05, 1);
	ssd1362_drawRect(56 * 2 + 10, 63, 56 * 2 + 10 + 10, 57, 0x05, 1);
	ssd1362_drawRect(56 * 3 + 10, 63, 56 * 3 + 10 + 10, 57, 0x05, 1);
	ssd1362_drawRect(56 * 4 + 10, 63, 56 * 4 + 10 + 10, 57, 0x05, 1);

	switch (GPIO_Pin)
	{
	case SW_1_Pin:
		ssd1362_drawRect(0 + 10, 63, 10 + 10, 57, 0x0F, 1);
		break;
	case SW_2_Pin:
		ssd1362_drawRect(56 + 10, 63, 56 + 10 + 10, 57, 0x0F, 1);
		break;
	case SW_3_Pin:
		ssd1362_drawRect(56 * 2 + 10, 63, 56 * 2 + 10 + 10, 57, 0x0F, 1);
		break;
	case SW_4_Pin:
		ssd1362_drawRect(56 * 3 + 10, 63, 56 * 3 + 10 + 10, 57, 0x0F, 1);
		break;
	case SW_5_Pin:
		ssd1362_drawRect(56 * 4 + 10, 63, 56 * 4 + 10 + 10, 57, 0x0F, 1);
		break;
	}
}
