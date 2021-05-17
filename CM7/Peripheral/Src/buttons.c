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
buttonStateTypeDef  buttonState[5] = {0};

static GPIO_TypeDef* BUTTON_PORT[KEY_NUMBER] = {SW_1_GPIO_Port,
												    SW_2_GPIO_Port,
												    SW_3_GPIO_Port,
												    SW_4_GPIO_Port,
												    SW_5_GPIO_Port
                                                   };

static const uint16_t BUTTON_PIN[KEY_NUMBER] = {SW_1_Pin,
												    SW_2_Pin,
												    SW_3_Pin,
												    SW_4_Pin,
											        SW_5_Pin
                                                   };

/* Private function prototypes -----------------------------------------------*/

/* Private user code ---------------------------------------------------------*/

/**
* @brief  Returns the current joystick status.
* @retval Code of the joystick key pressed
*          This code can be one of the following values:
*            @arg  JOY_NONE
*            @arg  JOY_SEL
*            @arg  JOY_DOWN
*            @arg  JOY_LEFT
*            @arg  JOY_RIGHT
*            @arg  JOY_UP
*/
int32_t BSP_JOY_GetState(buttonTypeDef KEY,uint32_t keyPin)
{

  uint32_t key, key_pressed;
  for (key = 0U; key < KEY_NUMBER ; key++)
  {
    key_pressed = 1UL << key;
    if(key_pressed == key_pressed)  //    if((key_pressed & JoyPinsMask) == key_pressed)
    {
      if(KEY == keyPin)
      {
        if (HAL_GPIO_ReadPin(BUTTON_PORT[key], BUTTON_PIN[key]) != GPIO_PIN_SET)
        {
          /* Return Code Joystick key pressed */
          return (int32_t)key_pressed;
        }
      }
    }
  }
  /* No Joystick key pressed */
  return (int32_t)SWITCH_RELEASED;
}

/**
 * @brief EXTI line detection callbacks
 * @param GPIO_Pin: Specifies the pins connected EXTI line
 * @retval None
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	if (buttonState[SW1] == SWITCH_RELEASED)
	{
		ssd1362_drawRect(0 + 10, 63, 10 + 10, 57, 0x05, 1);
	}
	if (buttonState[SW2] == SWITCH_RELEASED)
	{
		ssd1362_drawRect(56 + 10, 63, 56 + 10 + 10, 57, 0x05, 1);
	}
	if (buttonState[SW3] == SWITCH_RELEASED)
	{
		ssd1362_drawRect(56 * 2 + 10, 63, 56 * 2 + 10 + 10, 57, 0x05, 1);
	}
	if (buttonState[SW4] == SWITCH_RELEASED)
	{
		ssd1362_drawRect(56 * 3 + 10, 63, 56 * 3 + 10 + 10, 57, 0x05, 1);
	}
	if (buttonState[SW5] == SWITCH_RELEASED)
	{
		ssd1362_drawRect(56 * 4 + 10, 63, 56 * 4 + 10 + 10, 57, 0x05, 1);
	}

	switch (GPIO_Pin)
	{
	case SW_1_Pin:
		ssd1362_drawRect(0 + 10, 63, 10 + 10, 57, 0x0F, 1);
		buttonState[SW1] = SWITCH_PRESSED;
		break;
	case SW_2_Pin:
		ssd1362_drawRect(56 + 10, 63, 56 + 10 + 10, 57, 0x0F, 1);
		buttonState[SW2] = SWITCH_PRESSED;
		break;
	case SW_3_Pin:
		ssd1362_drawRect(56 * 2 + 10, 63, 56 * 2 + 10 + 10, 57, 0x0F, 1);
		buttonState[SW3] = SWITCH_PRESSED;
		break;
	case SW_4_Pin:
		ssd1362_drawRect(56 * 3 + 10, 63, 56 * 3 + 10 + 10, 57, 0x0F, 1);
		buttonState[SW4] = SWITCH_PRESSED;
		break;
	case SW_5_Pin:
		ssd1362_drawRect(56 * 4 + 10, 63, 56 * 4 + 10 + 10, 57, 0x0F, 1);
		buttonState[SW5] = SWITCH_PRESSED;
		break;
	}
}
