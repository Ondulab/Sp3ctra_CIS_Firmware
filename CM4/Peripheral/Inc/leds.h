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
void leds_pressFeedback(buttonIdTypeDef button_id, buttonStateTypeDef is_pressed);
void leds_check_update_state(void);
/** Slow breathing of the three backlights while the OLED sleeps (edge-triggered). */
void leds_setScreensaverMode(bool active);
/** Sine pulse of ONE backlight (menu validate button) while the device runs
 *  without any IP link. -1 = off. Press feedback and screensaver win over it. */
void leds_setAttractMode(int32_t led_index);
void led_test(void);

#endif /* __GUI_H */
