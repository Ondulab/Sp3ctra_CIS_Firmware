/**
 ******************************************************************************
 * @file           : update_gui.c
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

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "gui_config.h"
#include "basetypes.h"

#include "stdlib.h"
#include "stdio.h"
#include "stdbool.h"

#include "ssd1362.h"
#include "pictures.h"

/* Functions -----------------------------------------------------------------*/

/**
 * @brief Initializes the graphical user interface (GUI).
 * @param None
 * @retval None
 */
void gui_init(void)
{
	ssd1362_init();
	ssd1362_clearBuffer();
	ssd1362_writeFullBuffer();
}

/**
 * @brief Displays the firmware version during update.
 * @param version Pointer to a string containing the firmware version.
 */
void gui_displayVersion(const char* version)
{
	ssd1362_clearBuffer();

	// Draw border
	ssd1362_fillRect(0, DISPLAY_HEAD_Y1POS, DISPLAY_WIDTH, DISPLAY_HEAD_Y2POS, BANNER_BACKGROUND_COLOR, true);
	ssd1362_drawRect(0, 0, 255, 63, BANNER_BACKGROUND_COLOR, false);

	char versionString[32];

	ssd1362_drawString(0, DISPLAY_HEAD_Y1POS + 1, (int8_t *)"         FIRMWARE UPDATE        ", 0xF, 8);

	snprintf(versionString, sizeof(versionString),  (char *)"        Updating -> %s    	  ", version);

	ssd1362_drawString(0, 15, (int8_t *)versionString, 0xF, 8);

	ssd1362_drawString(0, 45, (int8_t *)					"        DO NOT POWER OFF        ", 0xF, 8);

	// Display the frame buffer
	ssd1362_writeFullBuffer();
}

/**
 * @brief Displays the update process with a progress bar.
 * @param progressBar Integer indicating the progress percentage (0 to 100).
 */
void gui_displayUpdateProcess(int32_t progressBar)
{
	ssd1362_progressBar(26, 27, progressBar, 0xF);
}

/**
 * @brief Displays an error message when the update process fails.
 */
void gui_displayRestorePreviousVersion(void)
{
	ssd1362_clearBuffer();

	// Draw border
	ssd1362_fillRect(0, DISPLAY_HEAD_Y1POS, DISPLAY_WIDTH, DISPLAY_HEAD_Y2POS, BANNER_BACKGROUND_COLOR, false);
	ssd1362_drawRect(0, 0, 255, 63, BANNER_BACKGROUND_COLOR, false);

	ssd1362_drawString(0, DISPLAY_HEAD_Y1POS + 1, (int8_t *)"         FIRMWARE UPDATE        ", 0xF, 8);

	ssd1362_drawString(0, 15, (int8_t *)					"  RESTORE THE PREVIOUS VERSION  ", 0xF, 8);

	ssd1362_drawString(0, 45, (int8_t *)					"        DO NOT POWER OFF        ", 0xF, 8);

	// Display the frame buffer
	ssd1362_writeFullBuffer();
}

/**
 * @brief Displays an error message when the update process fails.
 */
void gui_displayUpdateFailed(void)
{
	ssd1362_clearBuffer();

	// Draw border
	ssd1362_fillRect(0, DISPLAY_HEAD_Y1POS, DISPLAY_WIDTH, DISPLAY_HEAD_Y2POS, BANNER_BACKGROUND_COLOR, false);
	ssd1362_drawRect(0, 0, 255, 63, BANNER_BACKGROUND_COLOR, false);

	ssd1362_drawString(0, DISPLAY_HEAD_Y1POS + 1, (int8_t *)"         FIRMWARE UPDATE        ", 0xF, 8);

	for (uint32_t i = 0; i < 10; i++)
	{
		ssd1362_drawString(0, 25, (int8_t *)				"          UPDATE FAILED         ", 0xF, 8);
		ssd1362_writeFullBuffer();
		HAL_Delay(200);

		ssd1362_fillRect(2, 25, 254, 33, 0, false);
		ssd1362_writeFullBuffer();
		HAL_Delay(200);
	}
}

/**
 * @brief Displays a message indicating the update process is complete.
 */
void gui_displayUpdateTesting(void)
{
	ssd1362_clearBuffer();

	// Draw border
	ssd1362_fillRect(0, DISPLAY_HEAD_Y1POS, DISPLAY_WIDTH, DISPLAY_HEAD_Y2POS, BANNER_BACKGROUND_COLOR, false);
	ssd1362_drawRect(0, 0, 255, 63, BANNER_BACKGROUND_COLOR, false);

	ssd1362_drawString(0, DISPLAY_HEAD_Y1POS + 1, (int8_t *)"         FIRMWARE UPDATE        ", 0xF, 8);

	ssd1362_drawString(0, 25, (int8_t *)					"     START FIRMWARE TESTING     ", 0xF, 8);

	ssd1362_drawString(0, 45, (int8_t *)					"              REBOOT            ", 0xF, 8);

	// Display the frame buffer
	ssd1362_writeFullBuffer();
}

/**
 * @brief Displays a success message after a successful update.
 */
void gui_displayUpdateSuccess(void)
{
	ssd1362_clearBuffer();

	// Draw border
	ssd1362_fillRect(0, DISPLAY_HEAD_Y1POS, DISPLAY_WIDTH, DISPLAY_HEAD_Y2POS, BANNER_BACKGROUND_COLOR, true);
	ssd1362_drawRect(0, 0, 255, 63, BANNER_BACKGROUND_COLOR, false);

	ssd1362_drawString(0, DISPLAY_HEAD_Y1POS + 1, (int8_t *)"         FIRMWARE UPDATE        ", 0xF, 8);

ssd1362_drawString(0, 25, (int8_t *)						"    FIRMWARE TESTING SUCCESS    ", 0xF, 8);

	ssd1362_drawString(0, 45, (int8_t *)					"              REBOOT            ", 0xF, 8);

	// Display the frame buffer
	ssd1362_writeFullBuffer();
}
