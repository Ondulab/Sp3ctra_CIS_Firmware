/**
 ******************************************************************************
 * @file           : midi_button_mapper.h
 * @brief          : Button to MIDI Mapping Header
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

#ifndef MIDI_BUTTON_MAPPER_H
#define MIDI_BUTTON_MAPPER_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include "globals.h"

/* Exported types ------------------------------------------------------------*/

/* Exported constants --------------------------------------------------------*/

// Button to CC mapping (CC 20-22)
#define MIDI_BUTTON1_CC     20
#define MIDI_BUTTON2_CC     21
#define MIDI_BUTTON3_CC     22

// MIDI channel for buttons
#define MIDI_BUTTON_CHANNEL 0

/* Exported functions --------------------------------------------------------*/

/**
 * @brief Initialize button MIDI mapping
 */
void midi_button_mapper_init(void);

/**
 * @brief Handle button state change
 * Called from button ISR or task
 * @param button_id Button ID (SW1, SW2, SW3)
 * @param pressed 1 if pressed, 0 if released
 */
void midi_button_mapper_on_change(uint8_t button_id, uint8_t pressed);

#ifdef __cplusplus
}
#endif

#endif // MIDI_BUTTON_MAPPER_H
