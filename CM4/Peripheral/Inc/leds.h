/**
 ******************************************************************************
 * @file           : leds.h
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __LEDS_H
#define __LEDS_H

/* Includes ------------------------------------------------------------------*/

/* Private defines -----------------------------------------------------------*/

void leds_timerInit(void);
void leds_pressFeedback(buttonTypeDef button_id, buttonStateTypeDef is_pressed);
void leds_check_update_state(void);
void led_test(void);

#endif /* __GUI_H */
