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
 * @brief Draw the host overlay (parameters being edited in the VST) and the
 *        transient "VST LINKED / LOST" banner on top of the CIS waterfall.
 *
 * Call once per GUI frame, after gui_displayImage(). Cheap when idle.
 */
void gui_overlay_process(void);

#ifdef __cplusplus
}
#endif

#endif /* __GUI_OVERLAY_H */
