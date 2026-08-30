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

/* Private function prototypes -----------------------------------------------*/
static void gui_renderWaveAnimation(gui_overlay_callback_t overlay_callback);
static void gui_drawStartupOverlay(void);
static void gui_drawScreensaverOverlay(void);

// Hash and utility functions for noise effects
static inline uint32_t wang_hash(uint32_t x);
static inline uint8_t clamp4(int v);
static inline int8_t tri8(uint8_t t);

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

        // Render wave animation
        for (int i = 0; i < NUM_LINE; i++)
        {
            int y = waveHeight * (i + 1) * 2 - 16;

            for (int x = 0; x < screenWidth; x++)
            {
                int wave = (int)(waveHeight * sin((x + offset) * 0.1));
                int yPos = y + wave;

                int modulation = (int)(5 * sin((x + offset) * modFreqLine[i]));
                int thickness = 1 + abs(modulation);

                uint8_t contrast = 5 + (uint8_t)(5 * (1 + sin((x + lightOffset * i) * contrastFreq)));
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

// --- Fast hash (stable), and frame source you control elsewhere ---
static inline uint32_t wang_hash(uint32_t x)
{
    x = (x ^ 61u) ^ (x >> 16);
    x = x + (x << 3);
    x = x ^ (x >> 4);
    x = x * 0x27d4eb2dU;
    x = x ^ (x >> 15);
    return x;
}

static inline uint8_t clamp4(int v)
{
    return (uint8_t)(v < 0 ? 0 : (v > 15 ? 15 : v));
}

// Triangle wave in [-128..+127] from an integer phase (wrap 0..255)
static inline int8_t tri8(uint8_t t)
{
    // 0..127 up, 128..255 down
    int v = (t < 128) ? t : (255 - t);
    // scale to 0..127 then center -> [-128..+127] approx
    return (int8_t)((v << 1) - 128);
}

/*
    Stronger, animated noisy draw:
    - bitmap: 1 bpp, same format as your current function (Data hor, Bit ver)
    - base_color: 0..15
    - amp: 0..15       (max random swing magnitude)
    - bias: -15..+15   (negative = darker on average)
    - density: 0..255  (apply strong deltas when (hash&0xFF) < density)
    - temporal_strength: 0..255 (adds a global pulsation component)
    - frame: increments every call to animate (e.g., from HAL_GetTick()/N or a counter)
*/
void ssd1362_drawBmpNoisyFx(const uint8_t *bitmap,
                            uint16_t x,
                            uint16_t y,
                            uint16_t w,
                            uint16_t h,
                            uint8_t base_color,
                            uint8_t amp,
                            int8_t  bias,
                            uint8_t density,
                            uint8_t temporal_strength,
                            uint32_t frame,
                            bool display)
{
    // Precompute a temporal term in [-A..+A]
    // Map tri8() [-128..127] to a signed amplitude scaled by temporal_strength and amp/2
    int t = (int)tri8((uint8_t)(frame & 0xFF));
    int temporal = (t * (int)temporal_strength) / 255;     // [-128..+127] scaled
    temporal = (temporal * (int)amp) / 2 / 128;            // keep it subtle vs. random

    for (uint16_t j = 0; j < h; j++)
    {
        const uint8_t *row = bitmap + (j / 8u) * w;

        for (uint16_t i = 0; i < w; i++)
        {
            if (row[i] & (1u << (j & 7u)))
            {
                // Hash per-pixel with slow-changing frame component
                uint32_t h32 = wang_hash((uint32_t)i * 73856093u
                                       ^ (uint32_t)j * 19349663u
                                       ^ (uint32_t)(frame * 83492791u));

                // Random swing in [-amp..+amp]
                int r = (int)((h32 >> 8) & 0xFFFF);
                int rnd = (r % (amp + 1));            // 0..amp
                if ((h32 >> 24) & 1u) { rnd = -rnd; } // sign

                // Occasionally push harder (density gate)
                // If the low byte is below density, add an extra kick (dark-biased for visibility)
                if (((h32) & 0xFFu) < density)
                {
                    int kick = (amp * 3) / 2;         // 1.5x amp
                    rnd -= kick;                      // darker kick
                }

                // Combine: base + bias + temporal + rnd, clamp to 4-bit
                int c = (int)base_color + (int)bias + temporal + rnd;
                uint8_t gray = clamp4(c);

                ssd1362_drawPixel((int)x + (int)i, (int)y + (int)j, gray, display);
            }
        }
    }
}

/* Same effect on a nearest-neighbour downscaled bitmap (num/den <= 1). */
void ssd1362_drawBmpScaledNoisyFx(const uint8_t *bitmap,
                                  int16_t x, int16_t y, uint16_t w, uint16_t h,
                                  uint16_t num, uint16_t den,
                                  uint8_t base_color, uint8_t amp, int8_t bias,
                                  uint8_t density, uint8_t temporal_strength,
                                  uint32_t frame)
{
    int t = (int)tri8((uint8_t)(frame & 0xFF));
    int temporal = (t * (int)temporal_strength) / 255;
    temporal = (temporal * (int)amp) / 2 / 128;

    const uint16_t dw = (uint16_t)((uint32_t)w * num / den);
    const uint16_t dh = (uint16_t)((uint32_t)h * num / den);

    for (uint16_t dj = 0; dj < dh; dj++)
    {
        const uint16_t sj = (uint16_t)((uint32_t)dj * den / num);
        const uint8_t *row = bitmap + (sj / 8u) * w;
        const uint8_t  bit = (uint8_t)(1u << (sj & 7u));
        const int32_t  py  = (int32_t)y + dj;
        if (py < 0 || py >= SSD1362_HEIGHT) continue;

        for (uint16_t di = 0; di < dw; di++)
        {
            const uint16_t si = (uint16_t)((uint32_t)di * den / num);
            if ((row[si] & bit) == 0) continue;
            const int32_t px = (int32_t)x + di;
            if (px < 0 || px >= SSD1362_WIDTH) continue;

            uint32_t h32 = wang_hash((uint32_t)di * 73856093u ^ (uint32_t)dj * 19349663u ^ (uint32_t)(frame * 83492791u));
            int rnd = (int)(((h32 >> 8) & 0xFFFF) % (amp + 1));
            if ((h32 >> 24) & 1u) rnd = -rnd;
            if ((h32 & 0xFFu) < density) rnd -= (amp * 3) / 2;

            ssd1362_drawPixel((uint16_t)px, (uint16_t)py, clamp4((int)base_color + bias + temporal + rnd), false);
        }
    }
}

/* Screensaver: the logo is downscaled (250x64 -> 150x38), dimmed and drifts on a
 * slow Lissajous path whose two periods (~37 s and ~53 s at 20 fps) are
 * incommensurate, so no pixel stays lit at the same place. The panel is then
 * switched off entirely by gui_core after DEFAULT_SCREENSAVER_DISPLAY_OFF_SEC. */
#define SAVER_LOGO_NUM      (3)
#define SAVER_LOGO_DEN      (5)
#define SAVER_LOGO_W        (250 * SAVER_LOGO_NUM / SAVER_LOGO_DEN)   /* 150 */
#define SAVER_LOGO_H        (64 * SAVER_LOGO_NUM / SAVER_LOGO_DEN)    /* 38 */

static void gui_drawScreensaverOverlay(void)
{
    static uint32_t frame = 0;

    const float   t  = (float)frame;
    const int32_t ax = (SSD1362_WIDTH  - SAVER_LOGO_W) / 2 - 2;   /* 51 px of travel each side */
    const int32_t ay = (SSD1362_HEIGHT - SAVER_LOGO_H) / 2 - 2;   /* 11 px */
    const int32_t x  = (SSD1362_WIDTH  - SAVER_LOGO_W) / 2 + (int32_t)((float)ax * sinf(t * 0.0085f));
    const int32_t y  = (SSD1362_HEIGHT - SAVER_LOGO_H) / 2 + (int32_t)((float)ay * sinf(t * 0.0059f + 1.3f));

    ssd1362_drawBmpScaledNoisyFx(Sp3ctra_img, (int16_t)x, (int16_t)y, 250, 64,
                                 SAVER_LOGO_NUM, SAVER_LOGO_DEN,
                                 9,    /* base: well below white */
                                 2,    /* amp */
                                 -1,   /* bias */
                                 4,    /* density of strong kicks */
                                 80,   /* temporal pulsation */
                                 frame);
    frame++;
}

/**
 * @brief Displays screensaver animation.
 *
 * Uses the modular wave animation system without version number overlay.
 * Used when no significant motion is detected for the configured timeout period.
 */
void gui_displayScreensaver(void)
{
    gui_renderWaveAnimation(gui_drawScreensaverOverlay);
}

/**
 * @brief Displays an animated waiting screen.
 *
 * Uses the modular wave animation system with version number overlay.
 * Renders a dynamic wave animation on the screen to indicate that the system is
 * currently waiting for a process to complete.
 */
void gui_displayWaiting(void)
{
    while (shared_var.cis_process_rdy != TRUE)
    {
        gui_renderWaveAnimation(gui_drawStartupOverlay);
    }
}
