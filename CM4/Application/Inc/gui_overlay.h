/**
 ******************************************************************************
 * @file           : gui_overlay.h
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
 */
#ifndef __GUI_OVERLAY_H
#define __GUI_OVERLAY_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Advance the overlay state (poll the CM7 shared region, TTL, slide
 *        animation) and return the number of top display rows the band
 *        occupies THIS frame. Call once per GUI frame, BEFORE
 *        gui_displayImage(reserved): the live image is squeezed below the
 *        band instead of being masked by it. 0 when idle.
 */
uint32_t gui_overlay_reservedTop(void);

/**
 * @brief Paint the band (host overlay of the parameters being edited in the
 *        VST, or the transient "VST LINKED / LOST" banner) into the rows
 *        reserved by gui_overlay_reservedTop(). Call after gui_displayImage().
 */
void gui_overlay_draw(void);

/**
 * @brief 1 when the host pushed a new overlay or the link state changed since
 *        the last call - used by the main loop to wake up from the screensaver.
 */
uint8_t gui_overlay_hasActivity(void);

#ifdef __cplusplus
}
#endif

#endif /* __GUI_OVERLAY_H */
