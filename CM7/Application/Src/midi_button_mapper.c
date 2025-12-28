/**
 ******************************************************************************
 * @file           : midi_button_mapper.c
 * @brief          : Button to MIDI Mapping Implementation
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
#include "midi_button_mapper.h"
#include "rtpmidi.h"
#include <stdio.h>

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

// Button to CC mapping table
static const uint8_t button_to_cc[NUMBER_OF_BUTTONS] = {
    MIDI_BUTTON1_CC,  // SW1 → CC 20
    MIDI_BUTTON2_CC,  // SW2 → CC 21
    MIDI_BUTTON3_CC   // SW3 → CC 22
};

/* Private function prototypes -----------------------------------------------*/

/* Exported functions --------------------------------------------------------*/

/**
 * @brief Initialize button MIDI mapping
 */
void midi_button_mapper_init(void)
{
    printf("MIDI Button Mapper: Initialized\n");
    printf("  SW1 → CC %d\n", MIDI_BUTTON1_CC);
    printf("  SW2 → CC %d\n", MIDI_BUTTON2_CC);
    printf("  SW3 → CC %d\n", MIDI_BUTTON3_CC);
}

/**
 * @brief Handle button state change
 */
void midi_button_mapper_on_change(uint8_t button_id, uint8_t pressed)
{
    // Validate button ID
    if (button_id >= NUMBER_OF_BUTTONS) {
        return;
    }

    // Get CC number for this button
    uint8_t cc = button_to_cc[button_id];

    // Send MIDI CC: 127 when pressed, 0 when released
    uint8_t value = pressed ? 127 : 0;

    // Send via RTP-MIDI
    rtpmidi_status_t status = rtpmidi_send_cc(MIDI_BUTTON_CHANNEL, cc, value);

    if (status == RTPMIDI_OK) {
        /* Intentionally silent here to avoid double logging.
         * Button event logging (including seq) is handled in StartMidiTask
         * and can be enabled via DEBUG_MIDI_BUTTONS.
         */
    } else if (status == RTPMIDI_NOT_CONNECTED) {
        // Silently ignore if not connected (avoid spam)
    } else {
        printf("MIDI: Failed to send button %d (error %d)\n", button_id, status);
    }
}

/* Private functions ---------------------------------------------------------*/
