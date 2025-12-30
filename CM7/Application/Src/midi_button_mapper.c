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

/* Private function prototypes -----------------------------------------------*/

/* Exported functions --------------------------------------------------------*/

/**
 * @brief Initialize button MIDI mapping
 */
void midi_button_mapper_init(void)
{
    printf("MIDI Button Mapper: Initialized\n");
    printf("  SW1: CH=%u CMD=%u PARAM=%u\n",
           (unsigned int)shared_config.midi_button_channel[0],
           (unsigned int)shared_config.midi_button_command[0],
           (unsigned int)shared_config.midi_button_param[0]);
    printf("  SW2: CH=%u CMD=%u PARAM=%u\n",
           (unsigned int)shared_config.midi_button_channel[1],
           (unsigned int)shared_config.midi_button_command[1],
           (unsigned int)shared_config.midi_button_param[1]);
    printf("  SW3: CH=%u CMD=%u PARAM=%u\n",
           (unsigned int)shared_config.midi_button_channel[2],
           (unsigned int)shared_config.midi_button_command[2],
           (unsigned int)shared_config.midi_button_param[2]);
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

    uint8_t channel = shared_config.midi_button_channel[button_id];
    uint8_t command = shared_config.midi_button_command[button_id];
    uint8_t param = shared_config.midi_button_param[button_id];

    // Send 127 when pressed, 0 when released (requested behavior)
    uint8_t value = pressed ? 127U : 0U;

    rtpmidi_status_t status = RTPMIDI_ERROR;

    if (command == MIDI_BUTTON_COMMAND_NOTE)
    {
        // Note: velocity=0 is Note Off.
        status = rtpmidi_send_note(channel, param, value);
    }
    else
    {
        // Default to CC
        status = rtpmidi_send_cc(channel, param, value);
    }

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
