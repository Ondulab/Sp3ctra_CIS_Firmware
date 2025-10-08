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

/**************************************************************************************/
/********************             	GUI definitions                ********************/
/**************************************************************************************/
#define BANNER_BACKGROUND_COLOR					(3)

#define DISPLAY_WIDTH							SSD1362_WIDTH
#define DISPLAY_HEIGHT							SSD1362_HEIGHT

#define DISPLAY_HEAD_HEIGHT						(9)

// Conditional display area definitions based on GUI_SHOW_IMU
#if defined(GUI_SHOW_IMU) && (GUI_SHOW_IMU == 1)
	#define DISPLAY_AERAS2_HEIGHT				(17)
	/* Ensure area1 + inter + area2 exactly fit the screen.
	   Start area1 at 0 like original layout so AERA2 ends at bottom */
	#define DISPLAY_AERA1_Y1POS					(0)
	#define DISPLAY_AERAS1_HEIGHT				((DISPLAY_HEIGHT) - (DISPLAY_AERAS2_HEIGHT))
#else
	#define DISPLAY_AERAS2_HEIGHT				(0)
	#define DISPLAY_AERAS1_HEIGHT				(DISPLAY_HEIGHT)
	#define DISPLAY_AERA1_Y1POS					(0)
#endif

#define DISPLAY_HEAD_Y1POS						(0)
#define DISPLAY_HEAD_Y2POS						(DISPLAY_HEAD_HEIGHT)

#define DISPLAY_AERA1_Y2POS						(DISPLAY_AERA1_Y1POS + DISPLAY_AERAS1_HEIGHT - 1)

#define DISPLAY_AERA2_Y1POS						(DISPLAY_AERA1_Y2POS)
#define DISPLAY_AERA2_Y2POS						(DISPLAY_AERA2_Y1POS + DISPLAY_AERAS2_HEIGHT)

#define WINDOW_IMU_AVERAGE_SIZE 				(5)  // Window size for the moving average

// Color inversion macro for display
#define GUI_COLOR(c) ((GUI_INVERT_DISPLAY) ? (15 - (c)) : (c))

#endif // __GUI_CONFIG_H__
