/**
 ******************************************************************************
 * @file           : gui_core.c
 * @brief          : GUI core module - main loop and coordination
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
#include "stdbool.h"
#include "stdio.h"
#include "stdlib.h"
#include "stdint.h"
#include "string.h"

#include "main.h"
#include "basetypes.h"
#include "globals.h"
#include "gui_config.h"
#include "config.h"

#include "ssd1362.h"
#include "leds.h"

#include "gui_core.h"
#include "gui_cis_display.h"
#include "gui_imu.h"
#include "gui_animations.h"
#include "gui_interaction.h"
#include "gui_calibration.h"

/* Private variables ---------------------------------------------------------*/

/**
 * @brief Main loop of the GUI.
 *
 * Initializes the scanlines and enters an infinite loop where the display is
 * updated, button inputs are processed, and the IMU data is rendered.
 *
 * @return int Always returns 0.
 */
int gui_mainLoop(void)
{
    int32_t last_refresh_tick = HAL_GetTick(); // Initialization of the last refresh tick
    int32_t last_process_count = shared_var.cis_process_cnt; // Last recorded process counter

    for (int32_t packet = 0; packet < UDP_MAX_NB_PACKET_PER_LINE; packet++)
    {
        memset((void *) scanline_CM4[packet].imageData_R, 0, sizeof(scanline_CM4[packet].imageData_R));
        memset((void *) scanline_CM4[packet].imageData_G, 0, sizeof(scanline_CM4[packet].imageData_G));
        memset((void *) scanline_CM4[packet].imageData_B, 0, sizeof(scanline_CM4[packet].imageData_B));
    }

    gui_displayWaiting();
    gui_changeHand();

    // Screensaver variables
    static uint32_t last_significant_motion_tick = 0;
    static bool screensaver_active = false;

    // Initialize motion timer
    last_significant_motion_tick = HAL_GetTick();

    /* Infinite loop */
    while (1)
    {
        int32_t current_tick = HAL_GetTick(); // Get the current tick

        // Check for significant motion or button activity and update screensaver state
        if (gui_isSignificantMotion() || gui_checkButtonActivity()) {
            last_significant_motion_tick = current_tick;
            screensaver_active = false;  // Wake up from screensaver
        }

        // Check if we should activate screensaver (timeout in seconds converted to ms)
        uint32_t screensaver_timeout_ms = (uint32_t)shared_config.screensaver_timeout_sec * 1000;
        if (!screensaver_active &&
            (current_tick - last_significant_motion_tick) >= screensaver_timeout_ms) {
            screensaver_active = true;
        }

        // Display appropriate screen
        if (screensaver_active) {
            gui_displayScreensaver();
        } else {
            // Normal operation - update the interface
            gui_displayImage();
            leds_check_update_state();

            if ((current_tick - last_refresh_tick) >= 200)
            {
                int32_t current_process_count = shared_var.cis_process_cnt;
                int32_t process_count_diff = current_process_count - last_process_count;

                if (process_count_diff > 0) // Make sure there have been processes since the last refresh
                {
                    int32_t tick_diff = current_tick - last_refresh_tick;

                    if (tick_diff > 0 && process_count_diff > 0)
                    {
                        shared_var.cis_freq = (process_count_diff * 1000000) / (tick_diff * 1000);
                    }
                }

                last_refresh_tick = current_tick; // Update the last refresh tick
                last_process_count = current_process_count; // Update the last process counter
            }

            gui_interractiveMenu();
            // Display IMU if enabled in configuration
            if (shared_config.gui_show_imu) {
                gui_displayIMU();
            }
            ssd1362_writeUpdates();
        }
    }
}
