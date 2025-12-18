/**
 ******************************************************************************
 * @file           : midi_led_mapper.h
 * @brief          : MIDI to LED Mapping Header
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

#ifndef MIDI_LED_MAPPER_H
#define MIDI_LED_MAPPER_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include "globals.h"

/* Exported types ------------------------------------------------------------*/

/**
 * @brief LED control mode
 */
typedef enum {
    LED_MODE_SIMPLE = 0,    // 1 CC per LED (brightness only)
    LED_MODE_ADVANCED       // 7 CC per LED (full control)
} led_control_mode_t;

/* Exported constants --------------------------------------------------------*/

// Simple mode CC mapping (CC 30-32)
#define MIDI_LED1_BRIGHTNESS_CC     30
#define MIDI_LED2_BRIGHTNESS_CC     31
#define MIDI_LED3_BRIGHTNESS_CC     32

// Advanced mode CC mapping
// LED 1: CC 30-36 (MSB) + CC 62-68 (LSB for 14-bit)
// LED 2: CC 40-46 (MSB) + CC 72-78 (LSB)
// LED 3: CC 50-56 (MSB) + CC 82-88 (LSB)
#define MIDI_LED1_BASE_CC           30
#define MIDI_LED2_BASE_CC           40
#define MIDI_LED3_BASE_CC           50

/* Exported functions --------------------------------------------------------*/

/**
 * @brief Initialize MIDI LED mapper
 * @param mode Control mode (simple or advanced)
 */
void midi_led_mapper_init(led_control_mode_t mode);

/**
 * @brief Handle incoming MIDI CC message
 * Called from RTP-MIDI RX callback
 * @param channel MIDI channel (0-15)
 * @param cc Control Change number
 * @param value Control value (0-127)
 */
void midi_led_mapper_handle_cc(uint8_t channel, uint8_t cc, uint8_t value);

/**
 * @brief Set control mode at runtime
 * @param mode New control mode
 */
void midi_led_mapper_set_mode(led_control_mode_t mode);

/**
 * @brief Get current control mode
 * @return Current mode
 */
led_control_mode_t midi_led_mapper_get_mode(void);

#ifdef __cplusplus
}
#endif

#endif // MIDI_LED_MAPPER_H
