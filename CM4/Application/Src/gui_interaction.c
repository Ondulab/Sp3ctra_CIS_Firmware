/**
 ******************************************************************************
 * @file           : gui_interaction.c
 * @brief          : GUI interaction module - user interface and buttons
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
#include "stdbool.h"
#include "stdint.h"
#include "stdio.h"

#include "main.h"
#include "basetypes.h"
#include "globals.h"
#include "config.h"
#include "gpio.h"

#include "ssd1362.h"
#include "leds.h"
#include "gui_calibration.h"
#include "gui_interaction.h"

/* Private functions ---------------------------------------------------------*/

/**
 * @brief Displays a popup window with current configuration parameters.
 *
 * Shows the DPI, oversampling rate, and current processing frequency in a popup box.
 */
void gui_displayPopUp(void)
{
    uint8_t textData[256] = {0};

    ssd1362_fillRect(10, 5, 67, 35, 15, false);
    ssd1362_drawRect(9, 4, 68, 36, 0, false);

    sprintf((char *)textData, "%d DPI", (int)shared_config.cis_dpi);
    ssd1362_drawString(12, 07, (int8_t *)textData, 0, 8);

    if (shared_config.cis_oversampling < 10)
    {
        sprintf((char *)textData, "OVS   %d", (int)shared_config.cis_oversampling);
    }
    else
    {
        sprintf((char *)textData, "OVS  %d", (int)shared_config.cis_oversampling);
    }
    ssd1362_drawString(12, 17, (int8_t *)textData, 0, 8);

    if (shared_var.cis_freq < 100)
        sprintf((char *)textData, "%d   Hz", (int)(shared_var.cis_freq));
    else if (shared_var.cis_freq < 1000)
        sprintf((char *)textData, "%d  Hz", (int)(shared_var.cis_freq));
    else
        sprintf((char *)textData, "%d Hz", (int)(shared_var.cis_freq));

    ssd1362_drawString(12, 27, (int8_t*)textData, 0, 8);
}

/**
 * @brief Checks for button activity to wake up from screensaver.
 *
 * Detects if any button is currently pressed by reading GPIO pins directly.
 * This allows the screensaver to exit immediately when a button is pressed.
 *
 * @return bool True if any button is pressed, false otherwise.
 */
bool gui_checkButtonActivity(void)
{
    // Check all three buttons directly via GPIO
    return (HAL_GPIO_ReadPin(SW1_GPIO_Port, SW1_Pin) == GPIO_PIN_RESET ||
            HAL_GPIO_ReadPin(SW2_GPIO_Port, SW2_Pin) == GPIO_PIN_RESET ||
            HAL_GPIO_ReadPin(SW3_GPIO_Port, SW3_Pin) == GPIO_PIN_RESET);
}

/**
 * @brief Processes interactive button inputs and updates the UI.
 *
 * Monitors the state of each button with robust debounce, provides visual feedback through LEDs,
 * and triggers actions (such as starting calibration) based on button events.
 * Uses edge-triggered events with sequence numbers to ensure reliable inter-core communication.
 */
void gui_interractiveMenu(void)
{
    // Button debounce state - static to maintain state between calls
    typedef struct {
        uint32_t debounce_start_tick;  // Tick when debounce started
        GPIO_PinState raw_state;       // Current raw GPIO state
        buttonStateTypeDef stable_state; // Debounced stable state
        uint32_t sequence_number;      // Event sequence counter
    } ButtonDebounceState_t;

    static ButtonDebounceState_t btn_state[NUMBER_OF_BUTTONS] = {0};

    // Configuration constants
    #define DEBOUNCE_PRESS_MS   20    // Debounce time for press (20ms standard)
    #define DEBOUNCE_RELEASE_MS 20    // Debounce time for release (20ms standard)

    // UI state tracking
    static uint8_t oldScanDir = 0;
    static uint8_t oldOversampling = 0;
    static uint16_t oldDPI = 0;
    static uint32_t start_tick = 0;

    uint32_t current_tick = HAL_GetTick();

    // Handle calibration request
    if (shared_var.cis_cal_state == CIS_CAL_REQUESTED)
    {
        gui_startCalibration();
    }

    // Handle popup display for configuration changes
    if (shared_config.cis_oversampling != oldOversampling)
    {
        start_tick = current_tick;
        oldOversampling = shared_config.cis_oversampling;
    }
    if (shared_config.cis_dpi != oldDPI)
    {
        start_tick = current_tick;
        oldDPI = shared_config.cis_dpi;
    }
    if ((current_tick - start_tick) < 3000)
    {
        gui_displayPopUp();
    }

    // Handle handedness change
    if (shared_config.cis_handedness != oldScanDir)
    {
        gui_changeHand();
        oldScanDir = shared_config.cis_handedness;
    }

    // Process each button with debounce
    for (uint8_t i = 0; i < NUMBER_OF_BUTTONS; i++)
    {
        // Get GPIO configuration for this button
        GPIO_TypeDef* button_port = (i == 0) ? SW1_GPIO_Port :
                                   (i == 1) ? SW2_GPIO_Port : SW3_GPIO_Port;
        uint16_t button_pin = (i == 0) ? SW1_Pin :
                             (i == 1) ? SW2_Pin : SW3_Pin;

        // Read current GPIO state
        GPIO_PinState current_raw = HAL_GPIO_ReadPin(button_port, button_pin);

        // Check for state change
        if (current_raw != btn_state[i].raw_state)
        {
            // State changed - start debounce timer
            btn_state[i].raw_state = current_raw;
            btn_state[i].debounce_start_tick = current_tick;
        }
        else
        {
            // State is stable - check if debounce time has elapsed
            uint32_t debounce_time = (current_raw == GPIO_PIN_RESET) ?
                                     DEBOUNCE_PRESS_MS : DEBOUNCE_RELEASE_MS;

            if ((current_tick - btn_state[i].debounce_start_tick) >= debounce_time)
            {
                // Convert GPIO state to button state
                buttonStateTypeDef new_state = (current_raw == GPIO_PIN_RESET) ?
                                                SWITCH_PRESSED : SWITCH_RELEASED;

                // Check if this is a state transition
                if (new_state != btn_state[i].stable_state)
                {
                    // State transition detected - update stable state
                    btn_state[i].stable_state = new_state;

                    // Provide LED feedback
                    leds_pressFeedback(i, new_state);

                    // Publish event to CM7 via shared memory (edge-triggered)
                    shared_var.button_events[i].state = new_state;
                    shared_var.button_events[i].pressed_time = 0; // Could be calculated if needed

                    // Increment sequence number (atomic operation on uint32_t)
                    btn_state[i].sequence_number++;
                    shared_var.button_events[i].sequence_number = btn_state[i].sequence_number;
                }
            }
        }
    }
}
