/**
 ******************************************************************************
 * @file           : gui.h
 * @brief          : Main GUI header - includes all GUI modules
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
#ifndef __GUI_H
#define __GUI_H

/* Includes ------------------------------------------------------------------*/
#include "gui_core.h"
#include "gui_cis_display.h"
#include "gui_imu.h"
#include "gui_animations.h"
#include "gui_interaction.h"
#include "gui_calibration.h"
#include "gui_interrupts.h"

/* Legacy compatibility ------------------------------------------------------*/
// For backward compatibility, expose the main loop function
#define gui_mainLoop gui_mainLoop

#endif /* __GUI_H */
