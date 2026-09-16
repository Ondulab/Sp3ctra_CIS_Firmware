/**
 ******************************************************************************
 * @file           : gui_menu.h
 * @brief          : GUI menu module header - on-device 3-button setup menu
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

#ifndef __GUI_MENU_H__
#define __GUI_MENU_H__

#include "stdint.h"
#include "globals.h"

/* Exported functions prototypes ---------------------------------------------*/

/** Debounced button edge from gui_interaction (both press and release). */
void gui_menu_onButton(buttonIdTypeDef id, buttonStateTypeDef state);

/** Rows [0..n-1] the menu band occupies THIS frame (animated slide, same
 *  contract as gui_overlay_reservedTop). Also advances the menu state
 *  (auto-repeat, request acknowledgements, timeouts). */
uint32_t gui_menu_reservedTop(void);

/** Paint the band content. Call after gui_overlay_draw so the menu wins. */
void gui_menu_draw(void);

/** 1 while the menu is open (inhibits the screensaver and the config popup). */
uint8_t gui_menu_isActive(void);

/** The validate button in the CURRENT orientation (for the attract pulse). */
buttonIdTypeDef gui_menu_okButton(void);

#endif /* __GUI_MENU_H__ */
