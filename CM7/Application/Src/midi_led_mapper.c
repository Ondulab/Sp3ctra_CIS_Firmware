/**
 ******************************************************************************
 * @file           : midi_led_mapper.c
 * @brief          : MIDI to LED Mapping Implementation
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
#include "midi_led_mapper.h"
#include "basetypes.h"
#include <string.h>
#include <stdio.h>

/* Private typedef -----------------------------------------------------------*/

/**
 * @brief State for 14-bit CC construction
 */
typedef struct {
    uint8_t msb_value;
    uint8_t has_msb;
} cc14_state_t;

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static led_control_mode_t current_mode = LED_MODE_SIMPLE;
static cc14_state_t cc14_state[128] = {0};  // State for 14-bit CC construction

/* Private function prototypes -----------------------------------------------*/
static void handle_simple_mode(uint8_t cc, uint8_t value);
static void handle_advanced_mode(uint8_t cc, uint8_t value);
static void update_led_state(uint8_t led_id);

/* Exported functions --------------------------------------------------------*/

/**
 * @brief Initialize MIDI LED mapper
 */
void midi_led_mapper_init(led_control_mode_t mode)
{
    current_mode = mode;
    memset(cc14_state, 0, sizeof(cc14_state));

    printf("MIDI LED Mapper: Initialized in %s mode\n",
           mode == LED_MODE_SIMPLE ? "SIMPLE" : "ADVANCED");
}

/**
 * @brief Handle incoming MIDI CC message
 */
void midi_led_mapper_handle_cc(uint8_t channel, uint8_t cc, uint8_t value)
{
    // Process based on current mode
    if (current_mode == LED_MODE_SIMPLE) {
        handle_simple_mode(cc, value);
    } else {
        handle_advanced_mode(cc, value);
    }
}

/**
 * @brief Set control mode at runtime
 */
void midi_led_mapper_set_mode(led_control_mode_t mode)
{
    current_mode = mode;
    memset(cc14_state, 0, sizeof(cc14_state));

    printf("MIDI LED Mapper: Mode changed to %s\n",
           mode == LED_MODE_SIMPLE ? "SIMPLE" : "ADVANCED");
}

/**
 * @brief Get current control mode
 */
led_control_mode_t midi_led_mapper_get_mode(void)
{
    return current_mode;
}

/* Private functions ---------------------------------------------------------*/

/**
 * @brief Handle simple mode (1 CC per LED)
 */
static void handle_simple_mode(uint8_t cc, uint8_t value)
{
    uint8_t led_id = 0xFF;

    // Map CC to LED
    if (cc == MIDI_LED1_BRIGHTNESS_CC) {
        led_id = LED_1;
    } else if (cc == MIDI_LED2_BRIGHTNESS_CC) {
        led_id = LED_2;
    } else if (cc == MIDI_LED3_BRIGHTNESS_CC) {
        led_id = LED_3;
    }

    if (led_id >= NUMBER_OF_LEDS) {
        return;  // Invalid LED
    }

    // Scale MIDI value (0-127) to LED brightness (0-1000)
    uint16_t brightness = (uint16_t)((value * 1000) / 127);

    // Update LED state (immediate brightness, no animation)
    shared_var.ledState[led_id].brightness_1 = brightness;
    shared_var.ledState[led_id].time_1 = 0;
    shared_var.ledState[led_id].glide_1 = 0;
    shared_var.ledState[led_id].brightness_2 = 0;
    shared_var.ledState[led_id].time_2 = 0;
    shared_var.ledState[led_id].glide_2 = 0;
    shared_var.ledState[led_id].blink_count = 0;

    update_led_state(led_id);
}

/**
 * @brief Handle advanced mode (7 CC per LED with 14-bit support)
 */
static void handle_advanced_mode(uint8_t cc, uint8_t value)
{
    uint8_t led_id = 0xFF;
    uint8_t param_id = 0xFF;

    // Determine LED and parameter from CC number
    if (cc >= MIDI_LED1_BASE_CC && cc <= MIDI_LED1_BASE_CC + 6) {
        led_id = LED_1;
        param_id = cc - MIDI_LED1_BASE_CC;
    } else if (cc >= MIDI_LED2_BASE_CC && cc <= MIDI_LED2_BASE_CC + 6) {
        led_id = LED_2;
        param_id = cc - MIDI_LED2_BASE_CC;
    } else if (cc >= MIDI_LED3_BASE_CC && cc <= MIDI_LED3_BASE_CC + 6) {
        led_id = LED_3;
        param_id = cc - MIDI_LED3_BASE_CC;
    }
    // Handle LSB for 14-bit CC (CC + 32)
    else if (cc >= MIDI_LED1_BASE_CC + 32 && cc <= MIDI_LED1_BASE_CC + 38) {
        // LSB for LED 1
        uint8_t msb_cc = cc - 32;
        if (cc14_state[msb_cc].has_msb) {
            led_id = LED_1;
            param_id = msb_cc - MIDI_LED1_BASE_CC;

            // Construct 14-bit value
            uint16_t value14 = (cc14_state[msb_cc].msb_value << 7) | value;

            // Apply 14-bit value to appropriate parameter
            switch (param_id) {
                case 1:  // time_1
                    shared_var.ledState[led_id].time_1 = value14;
                    break;
                case 2:  // glide_1
                    shared_var.ledState[led_id].glide_1 = value14;
                    break;
                case 4:  // time_2
                    shared_var.ledState[led_id].time_2 = value14;
                    break;
                case 5:  // glide_2
                    shared_var.ledState[led_id].glide_2 = value14;
                    break;
            }

            cc14_state[msb_cc].has_msb = 0;
            update_led_state(led_id);
        }
        return;
    } else if (cc >= MIDI_LED2_BASE_CC + 32 && cc <= MIDI_LED2_BASE_CC + 38) {
        // LSB for LED 2
        uint8_t msb_cc = cc - 32;
        if (cc14_state[msb_cc].has_msb) {
            led_id = LED_2;
            param_id = msb_cc - MIDI_LED2_BASE_CC;

            uint16_t value14 = (cc14_state[msb_cc].msb_value << 7) | value;

            switch (param_id) {
                case 1:
                    shared_var.ledState[led_id].time_1 = value14;
                    break;
                case 2:
                    shared_var.ledState[led_id].glide_1 = value14;
                    break;
                case 4:
                    shared_var.ledState[led_id].time_2 = value14;
                    break;
                case 5:
                    shared_var.ledState[led_id].glide_2 = value14;
                    break;
            }

            cc14_state[msb_cc].has_msb = 0;
            update_led_state(led_id);
        }
        return;
    } else if (cc >= MIDI_LED3_BASE_CC + 32 && cc <= MIDI_LED3_BASE_CC + 38) {
        // LSB for LED 3
        uint8_t msb_cc = cc - 32;
        if (cc14_state[msb_cc].has_msb) {
            led_id = LED_3;
            param_id = msb_cc - MIDI_LED3_BASE_CC;

            uint16_t value14 = (cc14_state[msb_cc].msb_value << 7) | value;

            switch (param_id) {
                case 1:
                    shared_var.ledState[led_id].time_1 = value14;
                    break;
                case 2:
                    shared_var.ledState[led_id].glide_1 = value14;
                    break;
                case 4:
                    shared_var.ledState[led_id].time_2 = value14;
                    break;
                case 5:
                    shared_var.ledState[led_id].glide_2 = value14;
                    break;
            }

            cc14_state[msb_cc].has_msb = 0;
            update_led_state(led_id);
        }
        return;
    }

    if (led_id >= NUMBER_OF_LEDS) {
        return;  // Invalid LED
    }

    // Handle MSB for 14-bit parameters (store and wait for LSB)
    if (param_id == 1 || param_id == 2 || param_id == 4 || param_id == 5) {
        cc14_state[cc].msb_value = value;
        cc14_state[cc].has_msb = 1;
        return;  // Wait for LSB
    }

    // Handle 7-bit parameters
    uint16_t scaled_value = 0;
    switch (param_id) {
        case 0:  // brightness_1
            scaled_value = (uint16_t)((value * 1000) / 127);
            shared_var.ledState[led_id].brightness_1 = scaled_value;
            break;
        case 3:  // brightness_2
            scaled_value = (uint16_t)((value * 1000) / 127);
            shared_var.ledState[led_id].brightness_2 = scaled_value;
            break;
        case 6:  // blink_count
            shared_var.ledState[led_id].blink_count = value;
            break;
        default:
            return;  // Unknown parameter
    }

    update_led_state(led_id);
}

/**
 * @brief Update LED state and request update
 */
static void update_led_state(uint8_t led_id)
{
    if (led_id < NUMBER_OF_LEDS) {
        shared_var.led_update_requested[led_id] = TRUE;
    }
}
