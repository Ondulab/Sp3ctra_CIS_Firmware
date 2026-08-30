/**
 ******************************************************************************
 * @file           : gui_cis_display.c
 * @brief          : GUI CIS display module - CIS image rendering
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
#include "stdint.h"
#include "math.h"

#include "basetypes.h"
#include "globals.h"
#include "gui_config.h"
#include "config.h"

#include "ssd1362.h"
#include "gui_interrupts.h"
#include "gui_cis_display.h"

/* Private defines -----------------------------------------------------------*/
// Use M_PI from math.h instead of redefining PI

/* Private functions ---------------------------------------------------------*/

/**
 * @brief Renders the CIS image on the display.
 *
 * Processes the RGB data from the scanline buffers, converts the values into a
 * grayscale or pseudo-color representation, and draws the corresponding pixels.
 *
 * @note This function returns immediately if the transfer is not complete.
 */
void gui_displayImage(void)
{
    uint8_t cis_rgb[3] = {0};
    int32_t cis_color = 0;
    int32_t i = 0;
    int32_t y = 0;
    float32_t packet, index;
    uint32_t area1_height = GUI_GET_AREA1_HEIGHT();
    uint32_t area1_y2pos = GUI_GET_AREA1_Y2POS();
    int32_t line_Ypos = DISPLAY_AERA1_Y1POS + (area1_height / 2);
    int32_t half_height_above = (area1_height + 1) / 2;  // Round up for odd heights
    int32_t half_height_below = area1_height / 2;        // Round down for odd heights
    int32_t pixel_intensity = 0;
    float64_t angle = 0;

    if (!transferComplete)
    {
        return;
    }

    transferComplete = false;

    ssd1362_fillRect(0, DISPLAY_AERA1_Y1POS, DISPLAY_WIDTH, area1_y2pos, GUI_COLOR(0), false);

    // CIS DISPLAY
    for (i = 0; i < (DISPLAY_WIDTH); i++)
    {
        if (shared_config.cis_dpi > 200)
        {
            packet = (float32_t)(i * UDP_MAX_NB_PACKET_PER_LINE - 1.0)/(DISPLAY_WIDTH - 1.0);
            index = (packet - (uint32_t)packet) * (CIS_400DPI_PIXELS_NB / UDP_MAX_NB_PACKET_PER_LINE);
        }
        else
        {
            packet = (float32_t)(i * (UDP_MAX_NB_PACKET_PER_LINE / 2) - 1.0)/(DISPLAY_WIDTH - 1.0);
            index = (packet - (uint32_t)packet) * (CIS_200DPI_PIXELS_NB / (UDP_MAX_NB_PACKET_PER_LINE / 2));
        }

        cis_rgb[0] = scanline_CM4[(uint32_t)packet].r[(uint32_t)index];
        cis_rgb[1] = scanline_CM4[(uint32_t)packet].g[(uint32_t)index];
        cis_rgb[2] = scanline_CM4[(uint32_t)packet].b[(uint32_t)index];

        // Convert the RGB values to a single brightness value. The numbers 299, 587, and 114
        // are weights given to the R, G, and B components respectively,
        // according to the ITU-R BT.601 standard for converting color to grayscale.
        // This standard assumes that human eyes are less sensitive to the blue component as compared to red and green.
        // Note that cis_rgb[0], cis_rgb[1] and cis_rgb[2] are assumed to be the R, G, B values respectively.
        // cis_color = (299 * (uint32_t)cis_rgb[0]) + 587 * ((uint32_t)cis_rgb[1]) + (114 * (uint32_t)cis_rgb[2]);
        cis_color = cis_rgb[0] + cis_rgb[1] + cis_rgb[2];

        cis_color = cis_color < 0 ? 0 : cis_color > 765 ? 765 : cis_color;

        angle = cis_color * (M_PI / 2) / 765.00;

        // Draw pixels above the center line
        for (y = 0; y < half_height_above; y++)
        {
            if (angle < (M_PI / 2))
                pixel_intensity = tan(angle) * (y + 1);

            pixel_intensity = pixel_intensity < 0 ? 0 : pixel_intensity > 15 ? 15 : pixel_intensity;

            ssd1362_drawPixel(i, line_Ypos + y, GUI_COLOR(pixel_intensity), false);
        }

        // Draw pixels below the center line
        for (y = 1; y <= half_height_below; y++)
        {
            if (angle < (M_PI / 2))
                pixel_intensity = tan(angle) * (y + 1);

            pixel_intensity = pixel_intensity < 0 ? 0 : pixel_intensity > 15 ? 15 : pixel_intensity;

            ssd1362_drawPixel(i, line_Ypos - y, GUI_COLOR(pixel_intensity), false);
        }
    }
}
