/**
 ******************************************************************************
 * @file           : gui_overlay.c
 * @brief          : Host-driven OLED overlay (SLP OLED_OVERLAY) + link banner
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
 *
 * The CM7 link server copies each OLED_OVERLAY datagram into
 * shared_feedback.overlay and bumps overlay_seq. This module watches the
 * sequence number (robust to garbage in the NOLOAD shared region at boot: only
 * CHANGES are acted upon), keeps a local copy and draws it until ttl_ms
 * elapses.
 *
 * Layouts (256 x 64, 16 grey levels):
 *   1 item   : 26 px band - label in 16 px font, value right-aligned, full-width bar
 *   2-3 items: 10 px rows - label (8 px font) | value | 54 px bar
 * A link state change shows "VST LINKED" / "VST LOST" for 1.5 s when no host
 * overlay is active.
 */
/* Includes ------------------------------------------------------------------*/
#include <stdio.h>
#include <string.h>

#include "main.h"
#include "globals.h"
#include "config.h"
#include "gui_config.h"
#include "ssd1362.h"
#include "sp3ctra_link.h"
#include "gui_overlay.h"

/* Private define ------------------------------------------------------------*/
#define OVL_DEFAULT_TTL_MS      (1500U)
#define OVL_LINK_BANNER_MS      (1500U)
#define OVL_ROW_H               (10)
#define OVL_BAND1_H             (26)
#define OVL_COL_BG              (0)
#define OVL_COL_SEP             (6)
#define OVL_COL_DIM             (8)
#define OVL_COL_HI              (15)
#define OVL_COL_BAR_FRAME       (6)
#define OVL_ROW_BAR_X           (200)
#define OVL_ROW_BAR_W           (54)
#define OVL_ROW_VALUE_RIGHT     (196)

/* Private variables ---------------------------------------------------------*/
static struct slp_oled_overlay ovl_local;
static uint32_t ovl_seen_seq = 0;
static uint32_t ovl_shown_tick = 0;
static uint8_t  ovl_active = 0;
static uint8_t  ovl_inited = 0;

static uint32_t link_seen_seq = 0;
static uint32_t link_tick = 0;
static char     link_msg[16] = {0};

/* Private functions ---------------------------------------------------------*/

/** Copy a fixed-width, possibly non NUL-terminated field into a C string. */
static void ovl_field(const char *src, uint32_t max, char *dst, uint32_t dst_size)
{
    uint32_t n = 0;
    while (n < max && n + 1 < dst_size && src[n] != '\0')
    {
        dst[n] = src[n];
        n++;
    }
    dst[n] = '\0';
}

static void ovl_drawBar(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t norm, uint8_t flags, uint8_t col)
{
    if (norm == 0xFFFFU)
    {
        return;
    }

    ssd1362_drawRect((uint16_t)x, (uint16_t)y, (uint16_t)(x + w - 1), (uint16_t)(y + h - 1), OVL_COL_BAR_FRAME, false);

    const int32_t inner_w = w - 2;
    const int32_t fill = (int32_t)(((uint32_t)inner_w * (uint32_t)norm) / 65535U);

    if (flags & SLP_OVL_BIPOLAR)
    {
        /* Bar grows from the centre towards the value. */
        const int32_t mid = x + 1 + inner_w / 2;
        const int32_t pos = x + 1 + fill;
        if (pos > mid)
        {
            ssd1362_fillRect((uint16_t)mid, (uint16_t)(y + 1), (uint16_t)pos, (uint16_t)(y + h - 2), col, false);
        }
        else if (pos < mid)
        {
            ssd1362_fillRect((uint16_t)pos, (uint16_t)(y + 1), (uint16_t)mid, (uint16_t)(y + h - 2), col, false);
        }
        ssd1362_drawVLine((uint16_t)mid, (uint16_t)y, (int16_t)h, OVL_COL_HI, false);
    }
    else if (fill > 0)
    {
        ssd1362_fillRect((uint16_t)(x + 1), (uint16_t)(y + 1), (uint16_t)(x + fill), (uint16_t)(y + h - 2), col, false);
    }
}

static void ovl_drawSingle(const struct slp_overlay_item *it)
{
    char label[SLP_OVERLAY_LABEL_LEN + 1];
    char value[SLP_OVERLAY_VALUE_LEN + 1];
    ovl_field(it->label, SLP_OVERLAY_LABEL_LEN, label, sizeof(label));
    ovl_field(it->value, SLP_OVERLAY_VALUE_LEN, value, sizeof(value));

    ssd1362_fillRect(0, 0, DISPLAY_WIDTH - 1, OVL_BAND1_H - 1, OVL_COL_BG, false);
    ssd1362_drawHLine(0, OVL_BAND1_H - 1, DISPLAY_WIDTH, OVL_COL_SEP, false);

    /* Value first (right-aligned, 16 px font when it fits next to the label). */
    const int32_t value_w16 = (int32_t)strlen(value) * 16;
    const int32_t label_w16 = (int32_t)strlen(label) * 16;
    int32_t value_x, label_max_w;

    if (label_w16 + value_w16 + 8 <= DISPLAY_WIDTH)
    {
        value_x = DISPLAY_WIDTH - 2 - value_w16;
        ssd1362_drawString((uint16_t)value_x, 2, (int8_t *)value, OVL_COL_HI, 16);
        label_max_w = value_x - 6;
    }
    else
    {
        const int32_t value_w8 = (int32_t)strlen(value) * 8;
        value_x = DISPLAY_WIDTH - 2 - value_w8;
        ssd1362_drawString((uint16_t)value_x, 6, (int8_t *)value, OVL_COL_HI, 8);
        label_max_w = value_x - 6;
    }

    /* Label, truncated to the room left. */
    int32_t max_chars = label_max_w / 16;
    if (max_chars < 1)
    {
        max_chars = 1;
    }
    if ((int32_t)strlen(label) > max_chars)
    {
        label[max_chars] = '\0';
    }
    ssd1362_drawString(2, 2, (int8_t *)label, OVL_COL_HI, 16);

    ovl_drawBar(2, 20, DISPLAY_WIDTH - 4, 5, it->norm, it->flags, OVL_COL_HI);
}

static void ovl_drawRows(const struct slp_oled_overlay *o)
{
    const int32_t band_h = (int32_t)o->count * OVL_ROW_H + 1;

    ssd1362_fillRect(0, 0, DISPLAY_WIDTH - 1, (uint16_t)(band_h - 1), OVL_COL_BG, false);
    ssd1362_drawHLine(0, (uint16_t)(band_h - 1), DISPLAY_WIDTH, OVL_COL_SEP, false);

    for (uint32_t i = 0; i < o->count; i++)
    {
        const struct slp_overlay_item *it = &o->item[i];
        const int32_t y = (int32_t)i * OVL_ROW_H;
        const uint8_t col = (it->flags & SLP_OVL_HIGHLIGHT) ? OVL_COL_HI : OVL_COL_DIM;

        char label[SLP_OVERLAY_LABEL_LEN + 1];
        char value[SLP_OVERLAY_VALUE_LEN + 1];
        ovl_field(it->label, SLP_OVERLAY_LABEL_LEN, label, sizeof(label));
        ovl_field(it->value, SLP_OVERLAY_VALUE_LEN, value, sizeof(value));

        ssd1362_drawString(2, (uint16_t)(y + 1), (int8_t *)label, col, 8);

        const int32_t value_w = (int32_t)strlen(value) * 8;
        ssd1362_drawString((uint16_t)(OVL_ROW_VALUE_RIGHT - value_w), (uint16_t)(y + 1), (int8_t *)value, col, 8);

        ovl_drawBar(OVL_ROW_BAR_X, y + 2, OVL_ROW_BAR_W, 6, it->norm, it->flags, col);
    }
}

static void ovl_drawBanner(const char *text)
{
    ssd1362_fillRect(0, 0, DISPLAY_WIDTH - 1, OVL_ROW_H, OVL_COL_BG, false);
    ssd1362_drawHLine(0, OVL_ROW_H, DISPLAY_WIDTH, OVL_COL_SEP, false);
    const int32_t w = (int32_t)strlen(text) * 8;
    ssd1362_drawString((uint16_t)((DISPLAY_WIDTH - w) / 2), 1, (int8_t *)text, OVL_COL_HI, 8);
}

/* Public functions ----------------------------------------------------------*/

void gui_overlay_process(void)
{
    const uint32_t now = HAL_GetTick();

    if (!ovl_inited)
    {
        /* Never trust the boot-time contents of the shared region: baseline only. */
        ovl_seen_seq  = shared_feedback.overlay_seq;
        link_seen_seq = shared_feedback.link_seq;
        ovl_inited = 1;
    }

    if (shared_feedback.overlay_seq != ovl_seen_seq)
    {
        ovl_seen_seq = shared_feedback.overlay_seq;
        memcpy(&ovl_local, (const void *)&shared_feedback.overlay, sizeof(ovl_local));
        if (ovl_local.ttl_ms == 0U)
        {
            ovl_local.ttl_ms = OVL_DEFAULT_TTL_MS;
        }
        ovl_active = (ovl_local.count > 0U && ovl_local.count <= SLP_OVERLAY_MAX_ITEMS) ? 1U : 0U;
        ovl_shown_tick = now;
    }

    if (shared_feedback.link_seq != link_seen_seq)
    {
        link_seen_seq = shared_feedback.link_seq;
        snprintf(link_msg, sizeof(link_msg), "%s", shared_feedback.link_state ? "VST LINKED" : "VST LOST");
        link_tick = now;
    }

    if (ovl_active && (now - ovl_shown_tick) >= ovl_local.ttl_ms)
    {
        ovl_active = 0;
    }

    if (ovl_active)
    {
        if (ovl_local.count == 1U)
        {
            ovl_drawSingle(&ovl_local.item[0]);
        }
        else
        {
            ovl_drawRows(&ovl_local);
        }
    }
    else if (link_msg[0] != '\0' && (now - link_tick) < OVL_LINK_BANNER_MS)
    {
        ovl_drawBanner(link_msg);
    }
}
