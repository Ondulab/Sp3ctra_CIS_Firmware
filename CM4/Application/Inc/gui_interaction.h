/**
 ******************************************************************************
 * @file           : gui_interaction.h
 * @brief          : GUI interaction module header - user interface and buttons
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

#ifndef __GUI_INTERACTION_H
#define __GUI_INTERACTION_H

/* Includes ------------------------------------------------------------------*/
#include "stdbool.h"
#include "stdint.h"

/* Exported function prototypes ----------------------------------------------*/
void gui_interractiveMenu(void);
void gui_displayPopUp(void);
bool gui_checkButtonActivity(void);

#endif /* __GUI_INTERACTION_H */
