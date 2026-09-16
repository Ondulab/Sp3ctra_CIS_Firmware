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
 * The band does not mask the live CIS image: gui_overlay_reservedTop() tells
 * the image renderer how many top rows the band occupies THIS frame (animated:
 * the band slides in/out, the image is squeezed into the remaining rows), then
 * gui_overlay_draw() paints the band content anchored to the sliding edge.
 *
 * Layouts (256 x 64, 16 grey levels, both fonts advance 8 px per char):
 *   1 item   : 26 px band - label 16 px face, value right-aligned, full-width bar
 *   2-3 items: 10 px rows - label (8 px face) | value | 54 px bar
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
#define OVL_CHAR_W              (8)     /* real advance of BOTH font faces */
#define OVL_COL_BG              (0)
#define OVL_COL_SEP             (6)
#define OVL_COL_DIM             (8)
#define OVL_COL_HI              (15)
#define OVL_COL_BAR_FRAME       (6)
#define OVL_ROW_BAR_X           (200)
#define OVL_ROW_BAR_W           (54)
#define OVL_ROW_VALUE_RIGHT     (196)
#define OVL_SLIDE_OPEN_MS       (140.0f) /* full band height slides IN this fast */
#define OVL_SLIDE_CLOSE_MS      (400.0f) /* … and retreats noticeably slower */

/* What the band is currently showing (kept while it slides back out). */
#define OVL_SHOW_NONE           (0)
#define OVL_SHOW_OVERLAY        (1)
#define OVL_SHOW_BANNER         (2)

/* Private variables ---------------------------------------------------------*/
static struct slp_oled_overlay ovl_local;
static uint32_t ovl_seen_seq = 0;
static uint32_t ovl_shown_tick = 0;
static uint8_t  ovl_active = 0;
static uint8_t  ovl_inited = 0;

static uint32_t link_seen_seq = 0;
static uint32_t link_tick = 0;
static char     link_msg[16] = {0};

static uint8_t  show_kind = OVL_SHOW_NONE;
static float    anim_h = 0.0f;
static int32_t  band_h = 0;          /* rounded anim_h, what this frame uses */
static uint32_t anim_tick = 0;

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

/** Label with the chain layout (SLP_OVL_TAG_INVERT): the first word (chain
 *  number) is drawn black on a plate of the row colour, the second word
 *  (module code) sits in an outlined box, the rest follows plain. The layout
 *  stays inside the plain-text envelope, so the caller's width math
 *  (ovl_fitLabel) stays valid. */
static void ovl_drawLabel(const char *label, uint8_t flags, int32_t x, int32_t y,
                          uint8_t col, uint32_t size, int32_t clip)
{
    if (flags & SLP_OVL_TAG_INVERT)
    {
        const char *sp = strchr(label, ' ');
        if (sp != NULL && sp != label)
        {
            char tok[8];
            uint32_t n = (uint32_t)(sp - label);
            if (n > sizeof(tok) - 1)
            {
                n = sizeof(tok) - 1;
            }
            memcpy(tok, label, n);
            tok[n] = '\0';

            const int32_t tw = (int32_t)n * OVL_CHAR_W;
            const int32_t h  = (size == 16) ? 16 : 8;
            /* Plate and box bounds follow the INK, not the cell: the 16 px
             * face inks rows 4..13 (2 empty rows at the bottom) and columns
             * 3..8 of its cell (1 px PAST the 8 px advance), so its plate
             * sits one px lower and two px further right than the cell to
             * keep even margins around the glyphs. The 8 px face (ink rows
             * 0..6, columns 0..6) fits the cell-based bounds. */
            const int32_t ins = (size == 16) ? 2 : 0;
            int32_t y0 = (size == 16) ? y : y - 1;
            int32_t y1 = (size == 16) ? y + h + 1 : y + h;
            if (y0 < 0)
            {
                y0 = 0;
            }
            if (y1 > clip)
            {
                y1 = clip;
            }

            /* chain number: black on a plate */
            if (y1 >= y0)
            {
                const int32_t px0 = x - 1 + ins;
                ssd1362_fillRect((uint16_t)(px0 > 0 ? px0 : 0), (uint16_t)y0,
                                 (uint16_t)(x + tw + 2 + ins), (uint16_t)y1, col, false);
            }
            ssd1362_drawStringClipped(x + 1, y, tok, 0, size, 0, clip);

            /* module code: outlined box */
            const char *p2  = sp + 1;
            const char *sp2 = strchr(p2, ' ');
            uint32_t n2 = (sp2 != NULL) ? (uint32_t)(sp2 - p2) : (uint32_t)strlen(p2);
            if (n2 == 0)
            {
                return;
            }
            if (n2 > sizeof(tok) - 1)
            {
                n2 = sizeof(tok) - 1;
            }
            memcpy(tok, p2, n2);
            tok[n2] = '\0';

            const int32_t x2  = x + tw + 7;
            const int32_t tw2 = (int32_t)n2 * OVL_CHAR_W;
            if (y1 >= y0)
            {
                ssd1362_drawRect((uint16_t)(x2 - 2 + ins), (uint16_t)y0,
                                 (uint16_t)(x2 + tw2 + 1 + ins), (uint16_t)y1, col, false);
            }
            ssd1362_drawStringClipped(x2, y, tok, col, size, 0, clip);

            if (sp2 != NULL)
            {
                ssd1362_drawStringClipped(x2 + tw2 + 6, y, sp2 + 1, col, size, 0, clip);
            }
            return;
        }
    }
    ssd1362_drawStringClipped(x, y, label, col, size, 0, clip);
}

/** Truncate label so it never runs into a value starting at value_x. */
static void ovl_fitLabel(char *label, int32_t value_x)
{
    int32_t max_chars = (value_x - 6 - 2) / OVL_CHAR_W;
    if (max_chars < 1)
    {
        max_chars = 1;
    }
    if ((int32_t)strlen(label) > max_chars)
    {
        label[max_chars] = '\0';
    }
}

/** Bar, only when it fits whole inside [0..clip] (it rides the sliding edge). */
static void ovl_drawBar(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t norm, uint8_t flags, uint8_t col, int32_t clip)
{
    if (norm == 0xFFFFU || y < 0 || (y + h - 1) > clip)
    {
        return;
    }

    ssd1362_drawRect((uint16_t)x, (uint16_t)y, (uint16_t)(x + w - 1), (uint16_t)(y + h - 1), OVL_COL_BAR_FRAME, false);

    const int32_t inner_w = w - 2;
    /* Round half-up so 65534 (the VST's clamped 100 %) still fills the bar. */
    int32_t fill = (int32_t)(((uint32_t)inner_w * (uint32_t)norm + 32767U) / 65535U);
    if (fill > inner_w)
    {
        fill = inner_w;
    }

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

/** y_shift <= 0 while the band slides; clip = last band row available to content. */
static void ovl_drawSingle(const struct slp_overlay_item *it, int32_t y_shift, int32_t clip)
{
    char label[SLP_OVERLAY_LABEL_LEN + 1];
    char value[SLP_OVERLAY_VALUE_LEN + 1];
    ovl_field(it->label, SLP_OVERLAY_LABEL_LEN, label, sizeof(label));
    ovl_field(it->value, SLP_OVERLAY_VALUE_LEN, value, sizeof(value));

    /* Value right-aligned in the 16 px face; label gets the room to its left
     * (14 label + 10 value chars = 192 px + margins: always fits at 8 px/char).
     * Texts sit at y=1 (not 2): the 16 px face inks rows 4..13 of its cell, so
     * one px up centres the digits on the chain plate (margins 4/4) - tuned on
     * the device 2026-09-02. Label starts at x=0: plate and name 2 px left. */
    const int32_t value_x = DISPLAY_WIDTH - 2 - (int32_t)strlen(value) * OVL_CHAR_W;
    ovl_fitLabel(label, value_x);

    ovl_drawLabel(label, it->flags, 0, 1 + y_shift, OVL_COL_HI, 16, clip);
    ssd1362_drawStringClipped(value_x, 1 + y_shift, value, OVL_COL_HI, 16, 0, clip);

    ovl_drawBar(2, 20 + y_shift, DISPLAY_WIDTH - 4, 5, it->norm, it->flags, OVL_COL_HI, clip);
}

static void ovl_drawRows(const struct slp_oled_overlay *o, int32_t y_shift, int32_t clip)
{
    for (uint32_t i = 0; i < o->count; i++)
    {
        const struct slp_overlay_item *it = &o->item[i];
        const int32_t y = (int32_t)i * OVL_ROW_H + y_shift;
        const uint8_t col = (it->flags & SLP_OVL_HIGHLIGHT) ? OVL_COL_HI : OVL_COL_DIM;

        char label[SLP_OVERLAY_LABEL_LEN + 1];
        char value[SLP_OVERLAY_VALUE_LEN + 1];
        ovl_field(it->label, SLP_OVERLAY_LABEL_LEN, label, sizeof(label));
        ovl_field(it->value, SLP_OVERLAY_VALUE_LEN, value, sizeof(value));

        const int32_t value_x = OVL_ROW_VALUE_RIGHT - (int32_t)strlen(value) * OVL_CHAR_W;
        ovl_fitLabel(label, value_x);

        /* Row tuning (device, 2026-09-02): texts at y+1 - one px HIGHER
         * clipped the first row's chain plate at the band top and drew its
         * bottom over the previous row. Label at x=1 so plate and module box
         * keep a left margin. */
        ovl_drawLabel(label, it->flags, 1, y + 1, col, 8, clip);
        ssd1362_drawStringClipped(value_x, y + 1, value, col, 8, 0, clip);

        ovl_drawBar(OVL_ROW_BAR_X, y + 2, OVL_ROW_BAR_W, 6, it->norm, it->flags, col, clip);
    }
}

static void ovl_drawBanner(const char *text, int32_t y_shift, int32_t clip)
{
    const int32_t w = (int32_t)strlen(text) * OVL_CHAR_W;
    ssd1362_drawStringClipped((DISPLAY_WIDTH - w) / 2, 1 + y_shift, text, OVL_COL_HI, 8, 0, clip);
}

/** Full height of the band for what it currently shows. */
static int32_t ovl_fullHeight(void)
{
    switch (show_kind)
    {
        case OVL_SHOW_OVERLAY:
            return (ovl_local.count == 1U) ? OVL_BAND1_H : (int32_t)ovl_local.count * OVL_ROW_H + 1;
        case OVL_SHOW_BANNER:
            return OVL_ROW_H + 1;
        default:
            return 0;
    }
}

/* Public functions ----------------------------------------------------------*/

uint8_t gui_overlay_hasActivity(void)
{
    static uint32_t seen_overlay = 0;
    static uint32_t seen_link = 0;
    static uint8_t  inited = 0;

    if (!inited)
    {
        seen_overlay = shared_feedback.overlay_seq;
        seen_link    = shared_feedback.link_seq;
        inited = 1;
        return 0;
    }
    if (shared_feedback.overlay_seq != seen_overlay || shared_feedback.link_seq != seen_link)
    {
        seen_overlay = shared_feedback.overlay_seq;
        seen_link    = shared_feedback.link_seq;
        return 1;
    }
    return 0;
}

uint32_t gui_overlay_reservedTop(void)
{
    const uint32_t now = HAL_GetTick();

    if (!ovl_inited)
    {
        /* Never trust the boot-time contents of the shared region: baseline only. */
        ovl_seen_seq  = shared_feedback.overlay_seq;
        link_seen_seq = shared_feedback.link_seq;
        anim_tick = now;
        ovl_inited = 1;
    }

    if (shared_feedback.overlay_seq != ovl_seen_seq)
    {
        ovl_seen_seq = shared_feedback.overlay_seq;
        const uint8_t count = shared_feedback.overlay.count;
        if (count > 0U && count <= SLP_OVERLAY_MAX_ITEMS)
        {
            memcpy(&ovl_local, (const void *)&shared_feedback.overlay, sizeof(ovl_local));
            if (ovl_local.ttl_ms == 0U)
            {
                ovl_local.ttl_ms = OVL_DEFAULT_TTL_MS;
            }
            ovl_active = 1;
            ovl_shown_tick = now;
        }
        else
        {
            /* OLED_CLEAR (or garbage): keep the previous content in ovl_local
             * so the band retracts over what it was showing. */
            ovl_active = 0;
        }
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

    /* What wants the band right now (the last content keeps drawing while the
     * band slides back out, so retracting keeps its text under the edge). */
    if (ovl_active)
    {
        show_kind = OVL_SHOW_OVERLAY;
    }
    else if (link_msg[0] != '\0' && (now - link_tick) < OVL_LINK_BANNER_MS)
    {
        show_kind = OVL_SHOW_BANNER;
    }

    const int32_t target = (ovl_active ||
                            (show_kind == OVL_SHOW_BANNER && (now - link_tick) < OVL_LINK_BANNER_MS))
                           ? ovl_fullHeight() : 0;

    /* Constant-rate slide, normalised to the band height so every band takes
     * the same time whatever its size. A time-constant ease converges in one
     * or two GUI frames when the loop runs at ~30 fps, i.e. invisibly — the
     * linear rate survives any frame rate. Opening is brisk, closing is
     * deliberately slower. */
    uint32_t dt = now - anim_tick;
    anim_tick = now;
    if (dt > 100U)
    {
        dt = 100U;   /* hiccup (screensaver wake, slow frame): don't teleport */
    }

    const int32_t full = ovl_fullHeight();
    const float   ref  = (float)((full > 0) ? full : OVL_BAND1_H);
    if (anim_h < (float)target)
    {
        anim_h += ref * (float)dt / OVL_SLIDE_OPEN_MS;
        if (anim_h > (float)target)
        {
            anim_h = (float)target;
        }
    }
    else if (anim_h > (float)target)
    {
        anim_h -= ref * (float)dt / OVL_SLIDE_CLOSE_MS;
        if (anim_h < (float)target)
        {
            anim_h = (float)target;
        }
    }

    band_h = (int32_t)(anim_h + 0.5f);
    if (band_h <= 0)
    {
        band_h = 0;
        if (target == 0)
        {
            show_kind = OVL_SHOW_NONE;
        }
    }
    return (uint32_t)band_h;
}

void gui_overlay_draw(void)
{
    if (band_h <= 0 || show_kind == OVL_SHOW_NONE)
    {
        return;
    }

    /* Band background + its sliding bottom edge. */
    if (band_h >= 2)
    {
        ssd1362_fillRect(0, 0, DISPLAY_WIDTH - 1, (uint16_t)(band_h - 2), OVL_COL_BG, false);
    }
    ssd1362_drawHLine(0, (uint16_t)(band_h - 1), DISPLAY_WIDTH, OVL_COL_SEP, false);

    /* Content rides the edge: anchored to the band bottom, clipped above it. */
    const int32_t y_shift = band_h - ovl_fullHeight();
    const int32_t clip = band_h - 2;
    if (clip < 0)
    {
        return;
    }

    if (show_kind == OVL_SHOW_OVERLAY)
    {
        if (ovl_local.count == 1U)
        {
            ovl_drawSingle(&ovl_local.item[0], y_shift, clip);
        }
        else
        {
            ovl_drawRows(&ovl_local, y_shift, clip);
        }
    }
    else
    {
        ovl_drawBanner(link_msg, y_shift, clip);
    }
}
