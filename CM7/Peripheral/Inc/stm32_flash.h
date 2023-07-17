/**
  ******************************************************************************
  * @file           : stm32_flash.h
  * @brief          : Header for stm32_flash.c file.
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __STM32_FLASH_H__
#define __STM32_FLASH_H__

typedef enum
{
	CIS_READ_CAL = 0,
	CIS_WRITE_CAL,
}CIS_FlashRW_TypeDef;

/* Includes ------------------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/

void stm32_flashCalibrationRW(CIS_FlashRW_TypeDef RW);

#endif /* __STM32_FLASH_H__ */
