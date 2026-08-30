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

/* Brightest grey level of a ribbon core (0..15).
 *  - boot: readable behind the logo, a few seconds only
 *  - screensaver: deliberately dim (average panel current, burn-in) */
#define BOOT_PEAK_LEVEL         (11)
#define SCREENSAVER_PEAK_LEVEL  (6)

/* Private function prototypes -----------------------------------------------*/
static void gui_renderWaveField(gui_overlay_callback_t overlay_callback, uint8_t peak_level);
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

/* ---------------------------------------------------------------------------
 * Wave field - the background of both the boot screen and the screensaver.
 *
 * Five travelling ribbons. Each one owns three slow oscillators whose periods
 * are mutually prime (41/53/67/79/97 s for the vertical drift, 17/23/29/31/37 s
 * for the amplitude, 3..9 s for the travel): the pattern therefore never
 * repeats within a session and every ribbon sweeps the FULL height of the
 * panel. Nothing stays lit in one place - that is what protects the OLED
 * during a long screensaver, before the panel is switched off entirely.
 *
 * Cost: a 256-entry sine LUT replaces the double-precision sin() the previous
 * version called twice per pixel (the CM4 has no double-precision FPU).
 * ------------------------------------------------------------------------ */
#define WAVE_UPDATE_MS      (50)     /* 20 fps */
#define SIN_LUT_BITS        (8)
#define SIN_LUT_SIZE        (1 << SIN_LUT_BITS)
#define SIN_LUT_MASK        (SIN_LUT_SIZE - 1)

typedef struct
{
    float drift_phase, drift_inc;    /* vertical position of the ribbon */
    float wave_phase,  wave_inc;     /* travelling wave                 */
    float amp_phase,   amp_inc;      /* amplitude breathing             */
    float k;                         /* spatial cycles across the width */
    float amp_min, amp_max;          /* pixels                          */
} WaveLine;

static float    sin_lut[SIN_LUT_SIZE];
static WaveLine wave_line[NUM_LINE];
static bool     wave_ready = false;

/** sine of a phase expressed in turns, wrapped, LUT-based. */
static inline float wave_sin(float turns)
{
    const uint32_t idx = (uint32_t)(int32_t)(turns * (float)SIN_LUT_SIZE);
    return sin_lut[idx & SIN_LUT_MASK];
}

/** Advance a phase and keep it in [0, 1). */
static inline float wave_step(float phase, float inc)
{
    phase += inc;
    if (phase >= 1.0f) phase -= 1.0f;
    if (phase < 0.0f)  phase += 1.0f;
    return phase;
}

static void gui_initWaveField(void)
{
    for (int32_t i = 0; i < SIN_LUT_SIZE; i++)
    {
        sin_lut[i] = sinf((float)i * (2.0f * (float)M_PI / (float)SIN_LUT_SIZE));
    }

    /* Mutually prime periods (seconds) -> the field never repeats. */
    static const float drift_period[NUM_LINE] = { 41.0f, 53.0f, 67.0f, 79.0f, 97.0f };
    static const float amp_period[NUM_LINE]   = { 17.0f, 23.0f, 29.0f, 31.0f, 37.0f };
    static const float travel_period[NUM_LINE]= {  3.1f, -4.3f,  5.7f, -6.9f,  8.3f };  /* sign = direction */
    static const float spatial_k[NUM_LINE]    = {  1.3f,  0.8f,  2.1f,  1.7f,  0.6f };
    static const float amp_lo[NUM_LINE]       = {  3.0f,  2.0f,  4.0f,  2.5f,  1.5f };
    static const float amp_hi[NUM_LINE]       = { 10.0f,  7.0f, 12.0f,  8.0f,  6.0f };

    const float frames_per_s = 1000.0f / (float)WAVE_UPDATE_MS;

    for (int32_t i = 0; i < NUM_LINE; i++)
    {
        WaveLine *l = &wave_line[i];
        l->drift_inc = 1.0f / (drift_period[i]  * frames_per_s);
        l->amp_inc   = 1.0f / (amp_period[i]    * frames_per_s);
        l->wave_inc  = 1.0f / (travel_period[i] * frames_per_s);
        l->k         = spatial_k[i];
        l->amp_min   = amp_lo[i];
        l->amp_max   = amp_hi[i];
        /* Random start phases: two units side by side never show the same frame. */
        l->drift_phase = randomFloat(0.0f, 1.0f);
        l->amp_phase   = randomFloat(0.0f, 1.0f);
        l->wave_phase  = randomFloat(0.0f, 1.0f);
    }

    wave_ready = true;
}

/**
 * @brief Renders the evolving wave field, then the optional overlay on top.
 *
 * @param overlay_callback Drawn after the field (may be NULL).
 * @param peak_level       Brightest grey level of a ribbon core (0..15). Keep it
 *                         low for the screensaver, brighter for the boot screen.
 */
static void gui_renderWaveField(gui_overlay_callback_t overlay_callback, uint8_t peak_level)
{
    static uint32_t last_update_tick = 0;
    static float    breath_phase = 0.0f;

    if (!wave_ready)
    {
        gui_initWaveField();
    }

    const uint32_t now = HAL_GetTick();
    if ((now - last_update_tick) < WAVE_UPDATE_MS)
    {
        return;
    }
    last_update_tick = now;

    ssd1362_clearBuffer();

    /* Global brightness breathing (13 s), 60..100 % of peak_level. */
    const float breath = 0.8f + 0.2f * wave_sin(breath_phase);
    breath_phase = wave_step(breath_phase, 1.0f / (13.0f * (1000.0f / (float)WAVE_UPDATE_MS)));

    const float half_h = (float)SSD1362_HEIGHT * 0.5f;

    for (int32_t i = 0; i < NUM_LINE; i++)
    {
        WaveLine *l = &wave_line[i];

        /* Vertical drift: the ribbon crosses the whole panel and leans slightly
         * beyond both edges, so no row is favoured over a long session. */
        const float centre = half_h + (half_h + 6.0f) * wave_sin(l->drift_phase);
        const float amp    = l->amp_min + (l->amp_max - l->amp_min) * 0.5f * (1.0f + wave_sin(l->amp_phase));

        /* Ribbons far from mid-height fade out: the field breathes instead of
         * sliding a hard edge along the borders. */
        const float edge  = 1.0f - 0.55f * fabsf(centre - half_h) / (half_h + 6.0f);
        const float level = (float)peak_level * breath * edge;
        if (level < 1.0f)
        {
            goto next_line;
        }

        for (int32_t x = 0; x < SSD1362_WIDTH; x++)
        {
            const float u = (float)x / (float)SSD1362_WIDTH;
            const float y = centre + amp * wave_sin(u * l->k + l->wave_phase);

            /* Thicker where the ribbon is flat (slower vertical speed): reads as
             * a ribbon with a bright core rather than a plain sine curve. */
            const float slope     = fabsf(wave_sin(u * l->k + l->wave_phase + 0.25f));
            const int32_t half_th = 1 + (int32_t)(2.0f * slope);
            const int32_t yc      = (int32_t)(y + 0.5f);

            for (int32_t t = -half_th; t <= half_th; t++)
            {
                const int32_t py = yc + t;
                if (py < 0 || py >= SSD1362_HEIGHT)
                {
                    continue;
                }
                /* Soft falloff from the core to the edges of the ribbon. */
                const float fall = 1.0f - 0.65f * ((float)abs((int)t) / (float)(half_th + 1));
                const int32_t c  = (int32_t)(level * fall + 0.5f);
                if (c > 0)
                {
                    ssd1362_drawPixel((uint16_t)x, (uint16_t)py, (uint8_t)(c > 15 ? 15 : c), false);
                }
            }
        }

    next_line:
        l->drift_phase = wave_step(l->drift_phase, l->drift_inc);
        l->amp_phase   = wave_step(l->amp_phase,   l->amp_inc);
        l->wave_phase  = wave_step(l->wave_phase,  l->wave_inc);
    }

    if (overlay_callback != NULL)
    {
        overlay_callback();
    }

    ssd1362_writeFullBuffer();
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
    gui_renderWaveField(NULL, SCREENSAVER_PEAK_LEVEL);
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
        gui_renderWaveField(gui_drawStartupOverlay, BOOT_PEAK_LEVEL);
    }
}
