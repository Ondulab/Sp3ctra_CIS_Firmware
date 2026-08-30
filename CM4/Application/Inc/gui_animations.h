/**
 ******************************************************************************
 * @file           : gui_animations.h
 * @brief          : GUI animations module header - visual effects and animations
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

#ifndef __GUI_ANIMATIONS_H
#define __GUI_ANIMATIONS_H

/* Includes ------------------------------------------------------------------*/
#include "stdint.h"
#include "stdbool.h"

/* Exported constants --------------------------------------------------------*/
#define NUM_LINE 5

/* Exported types ------------------------------------------------------------*/
// Callback type for overlay content on wave animation
typedef void (*gui_overlay_callback_t)(void);

/* Exported function prototypes ----------------------------------------------*/
void gui_displayWaiting(void);
void gui_displayScreensaver(void);
float randomFloat(float min, float max);

// Enhanced bitmap drawing with noise effects
#endif /* __GUI_ANIMATIONS_H */
