/**
 ******************************************************************************
 * @file           : gui_config.h
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

#ifndef __GUI_CONFIG_H__
#define __GUI_CONFIG_H__

#include "config.h"
#include "globals.h"
#include "ssd1362.h"

/**************************************************************************************/
/********************             	GUI definitions                ********************/
/**************************************************************************************/
#define BANNER_BACKGROUND_COLOR					(3)

#define DISPLAY_WIDTH							SSD1362_WIDTH
#define DISPLAY_HEIGHT							SSD1362_HEIGHT

#define DISPLAY_HEAD_HEIGHT						(9)

// Static display area definitions
#define DISPLAY_AERA1_Y1POS					(0)

#define DISPLAY_HEAD_Y1POS						(0)
#define DISPLAY_HEAD_Y2POS						(DISPLAY_HEAD_HEIGHT)

// Dynamic display area calculations based on runtime GUI_SHOW_IMU config
static inline uint32_t GUI_GET_AREA2_HEIGHT(void) {
    return shared_config.gui_show_imu ? 17 : 0;
}

static inline uint32_t GUI_GET_AREA1_HEIGHT(void) {
    return DISPLAY_HEIGHT - GUI_GET_AREA2_HEIGHT();
}

static inline uint32_t GUI_GET_AREA1_Y2POS(void) {
    return DISPLAY_AERA1_Y1POS + GUI_GET_AREA1_HEIGHT() - 1;
}

static inline uint32_t GUI_GET_AREA2_Y1POS(void) {
    return GUI_GET_AREA1_Y2POS();
}

static inline uint32_t GUI_GET_AREA2_Y2POS(void) {
    uint32_t area2_height = GUI_GET_AREA2_HEIGHT();
    return area2_height > 0 ? (GUI_GET_AREA2_Y1POS() + area2_height) : 0;
}

#define WINDOW_IMU_AVERAGE_SIZE 				(4)  // Window size for the moving average (10ms @ 1000Hz ODR)

// Color inversion function for display (uses runtime configuration)
static inline uint8_t GUI_COLOR(uint8_t c) {
    return (shared_config.gui_invert_cis_image) ? (15 - (c)) : (c);
}

#endif // __GUI_CONFIG_H__
