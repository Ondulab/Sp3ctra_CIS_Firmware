/**
  ******************************************************************************
  * @file    Display/LTDC_Paint/Inc/paint.h
  * @author  MCD Application Team
  * @brief   Header for paint.c module
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */
  
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __PAINT_H
#define __PAINT_H

/* Includes ------------------------------------------------------------------*/
#include "stm32h750b_discovery.h"
#include "stm32h750b_discovery_lcd.h"
#include "stm32h750b_discovery_ts.h"
#include "stm32h750b_discovery_sdram.h"
#include "stm32h7xx_hal_conf.h"
#include <stdlib.h>
#include "basic_gui.h"
#include "lcd_trace.h"

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
#define CONVERTED_LINE_BUFFER                    0x24000000

/* Exported macro ------------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
int paint(void);
void Touchscreen_Calibration (void);
uint16_t Calibration_GetX(uint16_t x);
uint16_t Calibration_GetY(uint16_t y);
uint8_t IsCalibrationDone(void);

#endif /* __PAINT_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
