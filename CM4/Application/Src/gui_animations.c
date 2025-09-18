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

/**
 * @brief Draws startup overlay content (logo + version number).
 */
static void gui_drawStartupOverlay(void)
{
    char shortVersion[8];
    strcpy(shortVersion, FW_VERSION);

    // Locate the first dot
    char *p = strchr(shortVersion, '.');
    if (p != NULL)
    {
        // From there, look for the second dot
        p = strchr(p + 1, '.');
        if (p != NULL)
        {
            // Truncate at the second dot
            *p = '\0';
        }
    }

    // Display logo
    ssd1362_drawBmp(Sp3ctra_img, 2, 0, 250, 64, 0xF, 0);

    // Calculate position for right-aligned version number
    int textWidth = strlen(shortVersion) * 8; // 8 pixels per character
    int rightAlignedX = SSD1362_WIDTH - textWidth - 2; // Screen width - text width - margin
    ssd1362_drawString(rightAlignedX, 1, (signed char *)shortVersion, 0xF, 8);
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

static void gui_drawScreensaverOverlay(void)
{
    static uint32_t frame = 0;

    // Parameters to make it pop
    const uint8_t base   = 14;   // un peu en-dessous du blanc pour laisser de la marge vers le haut et bas
    const uint8_t amp    = 2;    // swing fort
    const int8_t  bias   = -1;   // globalement un peu plus sombre (accentue le contraste)
    const uint8_t dens   = 4;   // ~16% de "coups" forts (plus petit = plus fréquent)
    const uint8_t tstr   = 80;  // pulsation temporelle sensible

    ssd1362_drawBmpNoisyFx(Sp3ctra_img,
                           2, 0, 250, 64,
                           base, amp, bias,
                           dens, tstr,
                           frame++,
                           false);
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
