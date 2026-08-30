/**
 ******************************************************************************
 * @file           : update_gui.h
 * @brief          : Header for update.c file.
 *                   Contains prototypes and definitions for update functionality.
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

#ifndef UPDATE_GUI_H
#define UPDATE_GUI_H

#ifdef __cplusplus
extern "C" {
#endif

void gui_init(void);
void gui_displayVersion(const char* version);
void gui_displayUpdateProcess(int32_t progressBar);
void gui_displayRestorePreviousVersion(void);
void gui_displayUpdateFailed(void);
void gui_displayUpdateTesting(void);
void gui_displayUpdateSuccess(void);

/* Message generique sur deux lignes, pour les etats qui n'ont pas d'ecran
 * dedie (restauration impossible, intervention requise). Les chaines sont
 * affichees telles quelles : 32 caracteres par ligne. */
void gui_displayMessage(const char* line1, const char* line2);

/* Exported macros -----------------------------------------------------------*/
/* Add any necessary macros here */

#ifdef __cplusplus
}
#endif

#endif /* UPDATE_GUI_H */
