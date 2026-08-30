/**
 ******************************************************************************
 * @file           : gui_animations.c
 * @brief          : GUI animations module - visual effects and animations
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
#include "stdint.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "math.h"

#include "main.h"
#include "basetypes.h"
#include "globals.h"
#include "config.h"

#include "pictures.h"
#include "ssd1362.h"
#include "gui_animations.h"

/* Private defines -----------------------------------------------------------*/

/* Anti burn-in pixel shift: the whole wave field slides down by one line
 * spacing every BURNIN_SHIFT_PERIOD_S. At 16 px / 60 s = 0.27 px/s the motion
 * is invisible frame to frame, and because the shift wraps on the SPACING the
 * field looks statistically identical at any moment - but no row keeps the
 * same average illumination, which is what damages an OLED. */
#define BURNIN_SHIFT_PERIOD_S   (60.0f)

/* Private function prototypes -----------------------------------------------*/
static void gui_renderWaveAnimation(gui_overlay_callback_t overlay_callback);
static void gui_drawStartupOverlay(void);

/* Private functions ---------------------------------------------------------*/

/**
 * @brief Generates a random float within the specified range.
 *
 * @param min The minimum value of the range.
 * @param max The maximum value of the range.
 * @return float A random floating-point value between min and max.
 */
float randomFloat(float min, float max)
{
    return min + ((float)rand() / RAND_MAX) * (max - min);
}

/**
 * @brief Renders the core wave animation with optional overlay content.
 *
 * This function handles the wave animation rendering and calls the provided
 * overlay callback to draw additional content on top of the animation.
 *
 * @param overlay_callback Function pointer to draw overlay content (can be NULL)
 */
static void gui_renderWaveAnimation(gui_overlay_callback_t overlay_callback)
{
    static uint32_t lastUpdateTime = 0;
    static const uint32_t updateInterval = 50; // Update every 50 ms
    static int offset = 0;
    static int lightOffset = 0;
    static const uint16_t screenWidth = SSD1362_WIDTH;
    static const uint16_t screenHeight = SSD1362_HEIGHT;
    static const uint8_t waveHeight = 8;
    static const float lineSpacing = 16.0f;   /* waveHeight * 2 */
    static float burninShift = 0.0f;          /* px, wraps on lineSpacing */

    // Frequency and increment min/max values
    static const float modFreqMin[] = {0.03, 0.01, 0.03, 0.02, 0.04};
    static const float modFreqMax[] = {0.5, 0.3, 0.5, 0.35, 0.45};
    static const float modIncMin[] = {-0.01, -0.01, -0.03, -0.02, -0.04};
    static const float modIncMax[] = {0.15, 0.03, 0.05, 0.08, 0.12};

    // Dynamic modulation frequencies and increments
    static float modFreqLine[NUM_LINE] = {0};
    static float modIncLine[NUM_LINE] = {0};
    static bool initialized = false;

    if (!initialized) {
        for (int i = 0; i < NUM_LINE; i++) {
            modFreqLine[i] = randomFloat(modFreqMin[i], modFreqMax[i]);
            modIncLine[i] = randomFloat(modIncMin[i], modIncMax[i]);
        }
        initialized = true;
    }

    static const float contrastFreqMin = 0.01;
    static const float contrastFreqMax = 0.05;
    static float contrastFreq = 0;
    static const float contrastIncMin = -0.003;
    static const float contrastIncMax = 0.003;
    static float contrastInc = 0;
    static bool contrast_initialized = false;

    if (!contrast_initialized) {
        contrastFreq = randomFloat(contrastFreqMin, contrastFreqMax);
        contrastInc = randomFloat(contrastIncMin, contrastIncMax);
        contrast_initialized = true;
    }

    uint32_t currentTime = HAL_GetTick();

    if (currentTime - lastUpdateTime >= updateInterval)
    {
        lastUpdateTime = currentTime;
        ssd1362_clearBuffer();

        // Render wave animation.
        // The loop runs one line beyond each edge (-1 .. NUM_LINE): combined with
        // the burn-in shift below, the field stays continuous top and bottom.
        for (int idx = -1; idx <= NUM_LINE; idx++)
        {
            const int i = (idx + NUM_LINE) % NUM_LINE;   /* per-line modulation set */
            int y = (int)((float)(waveHeight * (idx + 1) * 2 - 16) + burninShift);

            for (int x = 0; x < screenWidth; x++)
            {
                int wave = (int)(waveHeight * sinf((x + offset) * 0.1f));
                int yPos = y + wave;

                int modulation = (int)(5 * sinf((x + offset) * modFreqLine[i]));
                int thickness = 1 + abs(modulation);

                uint8_t contrast = 5 + (uint8_t)(5 * (1 + sinf((x + lightOffset * i) * contrastFreq)));
                contrast = contrast > 5 ? 5 : contrast;

                if (yPos < screenHeight)
                {
                    for (int t = -thickness; t <= thickness; t++)
                    {
                        if (yPos + t >= 0 && yPos + t < screenHeight)
                        {
                            ssd1362_drawPixel(x, yPos + t, contrast, false);
                        }
                    }
                }
            }
        }

        // Update animation parameters
        offset = (offset + 1) % screenWidth;
        lightOffset = (lightOffset - 5) % screenWidth;

        burninShift += lineSpacing / (BURNIN_SHIFT_PERIOD_S * (1000.0f / (float)updateInterval));
        if (burninShift >= lineSpacing)
        {
            burninShift -= lineSpacing;
        }

        for (int i = 0; i < NUM_LINE; i++)
        {
            modFreqLine[i] += modIncLine[i];
            if (modFreqLine[i] > modFreqMax[i] || modFreqLine[i] < modFreqMin[i]) {
                modFreqLine[i] = randomFloat(modFreqMin[i], modFreqMax[i]);
            }
        }

        contrastFreq += contrastInc;
        if (contrastFreq > contrastFreqMax) contrastFreq = randomFloat(contrastFreqMin, contrastFreqMax);
        if (contrastFreq < contrastFreqMin) contrastFreq = randomFloat(contrastFreqMin, contrastFreqMax);

        // Draw overlay content if callback provided
        if (overlay_callback != NULL) {
            overlay_callback();
        }

        ssd1362_writeFullBuffer();
    }
}

/* Boot screen layout (256 x 64):
 *   y  0..45  wave animation + logo scaled 250x64 -> 180x46, centred
 *   y 46      separator
 *   y 47..54  device name (left)                     firmware version (right)
 *   y 55..63  IP address (left, once configured)     CM7 boot stage + dots (right)
 */
#define BOOT_LOGO_NUM       (18)
#define BOOT_LOGO_DEN       (25)
#define BOOT_LOGO_W         (250 * BOOT_LOGO_NUM / BOOT_LOGO_DEN)   /* 180 */
#define BOOT_BAND_Y         (46)
#define BOOT_LINE1_Y        (47)
#define BOOT_LINE2_Y        (56)
#define BOOT_COL_ID         (11)
#define BOOT_COL_DIM        (7)
#define BOOT_COL_STAGE      (15)

static const char *gui_bootStageLabel(uint8_t stage)
{
    switch (stage)
    {
        case BOOT_STAGE_STARTING: return "STARTING";
        case BOOT_STAGE_CONFIG:   return "CONFIG";
        case BOOT_STAGE_NETWORK:  return "NETWORK";
        case BOOT_STAGE_LINK:     return "LINK";
        case BOOT_STAGE_IMU:      return "IMU";
        case BOOT_STAGE_CIS:      return "SENSOR";
        case BOOT_STAGE_READY:    return "READY";
        default:                  return "BOOT";
    }
}

/**
 * @brief Draws the boot screen overlay: scaled logo, identity band and CM7 boot stage.
 */
static void gui_drawStartupOverlay(void)
{
    char text[24];

    ssd1362_drawBmpScaled(Sp3ctra_img, (SSD1362_WIDTH - BOOT_LOGO_W) / 2, 0, 250, 64,
                          BOOT_LOGO_NUM, BOOT_LOGO_DEN, 0xF);

    // Opaque band: the waves never run through the text
    ssd1362_fillRect(0, BOOT_BAND_Y, SSD1362_WIDTH - 1, SSD1362_HEIGHT - 1, 0, false);
    ssd1362_drawHLine(0, BOOT_BAND_Y, SSD1362_WIDTH, 3, false);

    // Line 1 - device name (published by the CM7 before it releases this core)
    if (shared_feedback.device_name[0] == 'S')
    {
        memcpy(text, (const void *)shared_feedback.device_name, sizeof(shared_feedback.device_name));
        text[sizeof(shared_feedback.device_name)] = '\0';
        ssd1362_drawString(2, BOOT_LINE1_Y, (signed char *)text, BOOT_COL_ID, 8);
    }

    // Line 1 - firmware version, right aligned
    snprintf(text, sizeof(text), "v%s", FW_VERSION);
    ssd1362_drawString((uint16_t)(SSD1362_WIDTH - 2 - strlen(text) * 8), BOOT_LINE1_Y, (signed char *)text, BOOT_COL_ID, 8);

    // Line 2 - IP address once the CM7 has loaded the configuration
    const uint8_t stage = shared_feedback.boot_stage;
    if (stage >= BOOT_STAGE_NETWORK && shared_config.network_ip[0] != 0)
    {
        snprintf(text, sizeof(text), "%u.%u.%u.%u",
                 (unsigned)shared_config.network_ip[0], (unsigned)shared_config.network_ip[1],
                 (unsigned)shared_config.network_ip[2], (unsigned)shared_config.network_ip[3]);
        ssd1362_drawString(2, BOOT_LINE2_Y, (signed char *)text, BOOT_COL_DIM, 8);
    }

    // Line 2 - boot stage with animated dots, right aligned (fixed width: no jitter)
    const uint32_t dots = (HAL_GetTick() / 400U) % 4U;
    snprintf(text, sizeof(text), "%s%.*s", gui_bootStageLabel(stage), (int)dots, "...");
    ssd1362_drawString((uint16_t)(SSD1362_WIDTH - 2 - (strlen(gui_bootStageLabel(stage)) + 3) * 8), BOOT_LINE2_Y,
                       (signed char *)text, BOOT_COL_STAGE, 8);
}

/**
 * @brief Displays the screensaver.
 *
 * Background ONLY: no logo, no text, nothing that could stay in one place -
 * a lit OLED pixel that never moves burns in. After
 * DEFAULT_SCREENSAVER_DISPLAY_OFF_SEC the caller (gui_mainLoop) switches the
 * panel off altogether.
 */
void gui_displayScreensaver(void)
{
    gui_renderWaveAnimation(NULL);
}

/**
 * @brief Displays the boot screen until the CM7 signals the CIS is ready.
 *
 * Same wave field, brighter, with the identity / boot-stage overlay on top.
 */
void gui_displayWaiting(void)
{
    while (shared_var.cis_process_rdy != TRUE)
    {
        gui_renderWaveAnimation(gui_drawStartupOverlay);
    }
}
