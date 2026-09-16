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
#include "gui_overlay.h"
#include "gui_menu.h"

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
        memset((void *) scanline_CM4[packet].r, 0, sizeof(scanline_CM4[packet].r));
        memset((void *) scanline_CM4[packet].g, 0, sizeof(scanline_CM4[packet].g));
        memset((void *) scanline_CM4[packet].b, 0, sizeof(scanline_CM4[packet].b));
    }

    gui_displayWaiting();
    gui_changeHand();

    // Screensaver variables
    static uint32_t last_significant_motion_tick = 0;
    static uint32_t screensaver_start_tick = 0;
    static bool screensaver_active = false;
    static bool display_off = false;   // OLED panel switched off (burn-in protection)

    // Initialize motion timer
    last_significant_motion_tick = HAL_GetTick();

    /* Infinite loop */
    while (1)
    {
        int32_t current_tick = HAL_GetTick(); // Get the current tick

        // Check for significant motion or button activity and update screensaver state
        // (an open device menu keeps the panel awake; it closes by itself after
        // 30 s without a button press, re-arming the screensaver countdown)
        if (gui_isSignificantMotion() || gui_checkButtonActivity() || gui_overlay_hasActivity() || gui_menu_isActive()) {
            last_significant_motion_tick = current_tick;
            screensaver_active = false;  // Wake up from screensaver
            leds_setScreensaverMode(false);
            if (display_off) {
                ssd1362_displayOn(true);
                display_off = false;
            }
        }

        // Check if we should activate screensaver (timeout in seconds converted to ms)
        uint32_t screensaver_timeout_ms = (uint32_t)shared_config.screensaver_timeout_sec * 1000;
        if (!screensaver_active &&
            (current_tick - last_significant_motion_tick) >= screensaver_timeout_ms) {
            screensaver_active = true;
            screensaver_start_tick = current_tick;
            leds_setScreensaverMode(true);    // buttons keep breathing while the panel sleeps
        }

        // Display appropriate screen
        if (screensaver_active) {
            // Drifting, dimmed logo first; then the panel itself goes dark (OLED burn-in).
            if (!display_off &&
                (current_tick - screensaver_start_tick) >= (uint32_t)DEFAULT_SCREENSAVER_DISPLAY_OFF_SEC * 1000U) {
                ssd1362_displayOn(false);
                display_off = true;
            }
            if (!display_off) {
                gui_displayScreensaver();
            }
        } else {
            // Normal operation - update the interface. The overlay and menu
            // bands do not mask the live image: they reserve the top rows
            // (animated slide) and the image is squeezed into the rows below -
            // which keeps the live feedback visible on the calibration pages.
            uint32_t overlay_top = gui_overlay_reservedTop();
            uint32_t menu_top = gui_menu_reservedTop();
            gui_displayImage((menu_top > overlay_top) ? menu_top : overlay_top);
            gui_overlay_draw();      // host overlay + link banner, in the reserved rows
            gui_menu_draw();         // device menu wins over the overlay band
            leds_check_update_state();

            // No host session bound: pulse the validate button's backlight so
            // the user thinks of pressing it (menu entry). Deliberately NOT
            // gated on the Ethernet carrier - a USB-ETH adapter provides a
            // link even with no computer behind it (seen on device). The LED
            // index equals the button index (leds_handle convention).
            leds_setAttractMode((shared_feedback.link_state == 0U &&
                                 !gui_menu_isActive())
                                ? (int32_t)gui_menu_okButton() : -1);

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
