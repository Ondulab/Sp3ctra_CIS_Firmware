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

/**************************************************************************************/
/********************             	GUI definitions                ********************/
/**************************************************************************************/
#define BANNER_BACKGROUND_COLOR					(3)

#define DISPLAY_WIDTH							SSD1362_WIDTH
#define DISPLAY_HEIGHT							SSD1362_HEIGHT

#define DISPLAY_HEAD_HEIGHT						(9)

#define DISPLAY_AERAS1_HEIGHT					(47)
#define DISPLAY_AERAS2_HEIGHT					(16)
#define DISPLAY_INTER_AERAS_HEIGHT				(1)

#define DISPLAY_HEAD_Y1POS						(0)
#define DISPLAY_HEAD_Y2POS						(DISPLAY_HEAD_HEIGHT)

#define DISPLAY_AERA1_Y1POS						(0)//(DISPLAY_HEAD_Y2POS + DISPLAY_INTER_AERAS_HEIGHT)
#define DISPLAY_AERA1_Y2POS						(DISPLAY_AERA1_Y1POS + DISPLAY_AERAS1_HEIGHT)

#define DISPLAY_AERA2_Y1POS						(DISPLAY_AERA1_Y2POS + DISPLAY_INTER_AERAS_HEIGHT)
#define DISPLAY_AERA2_Y2POS						(DISPLAY_AERA2_Y1POS + DISPLAY_AERAS2_HEIGHT)

#define	NUMBER_OF_BUTTONS						(3)
#define	NUMBER_OF_LEDS							(3)

#endif // __GUI_CONFIG_H__

