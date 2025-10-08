/**
 ******************************************************************************
 * @file           : gui_core.h
 * @brief          : GUI core module header - main loop and coordination
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

#ifndef __GUI_CORE_H
#define __GUI_CORE_H

/* Includes ------------------------------------------------------------------*/
#include "stdbool.h"
#include "stdint.h"

/* Exported constants --------------------------------------------------------*/
// Screensaver and motion detection constants are defined in config.h

/* Exported function prototypes ----------------------------------------------*/
int gui_mainLoop(void);

#endif /* __GUI_CORE_H */
