/**
 ******************************************************************************
 * @file           : buttons.h
 * @brief          : 5 key keybord driver
 ******************************************************************************
 */

/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __BUTTONS_H__
#define __BUTTONS_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

/* Private includes ----------------------------------------------------------*/

/* Exported types ------------------------------------------------------------*/

#define KEY_NUMBER					5

typedef enum
{
 SW1  = 0x00U,
 SW2  = 0x01U,
 SW3  = 0x02U,
 SW4  = 0x03U,
 SW5  = 0x04U
}buttonTypeDef;

typedef enum
{
	SWITCH_RELEASED = 0,
	SWITCH_PRESSED
}buttonStateTypeDef;

extern buttonStateTypeDef  buttonState[5];
/* Exported constants --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/

/* Exported functions prototypes ---------------------------------------------*/

/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /*__BUTTONS_H__*/
