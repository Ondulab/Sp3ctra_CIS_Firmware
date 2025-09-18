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
 * Monitors the state of each button, provides visual feedback through LEDs,
 * and triggers actions (such as starting calibration) based on button events.
 */
void gui_interractiveMenu(void)
{
    static uint32_t button_current_tick[NUMBER_OF_BUTTONS] = {0, 0, 0};
    static uint32_t button_initial_tick[NUMBER_OF_BUTTONS] = {0, 0, 0};
    static uint8_t clear_button[NUMBER_OF_BUTTONS] = {0, 0, 0};

    static uint8_t oldScanDir = 0;
    static uint8_t oldOversampling = 0;
    static uint16_t oldDPI = 0;
    static uint32_t start_tick = 0;

    if (shared_var.cis_cal_state == CIS_CAL_REQUESTED)
    {
        gui_startCalibration();
    }

    if (shared_config.cis_oversampling != oldOversampling)
    {
        start_tick = HAL_GetTick();
        oldOversampling = shared_config.cis_oversampling;
    }
    if (shared_config.cis_dpi != oldDPI)
    {
        start_tick = HAL_GetTick();
        oldDPI = shared_config.cis_dpi;
    }
    if ((HAL_GetTick() - start_tick) < 3000)
    {
        gui_displayPopUp();
    }

    if (shared_config.cis_handedness != oldScanDir)
    {
        gui_changeHand();
        oldScanDir = shared_config.cis_handedness;
    }

    for (int i = 0; i < NUMBER_OF_BUTTONS; i++)
    {
        GPIO_TypeDef* button_port = (i == 0) ? SW1_GPIO_Port : (i == 1) ? SW2_GPIO_Port : SW3_GPIO_Port;
        uint16_t button_pin = (i == 0) ? SW1_Pin : (i == 1) ? SW2_Pin : SW3_Pin;

        if (HAL_GPIO_ReadPin(button_port, button_pin) == GPIO_PIN_RESET)
        {
            button_current_tick[i] = HAL_GetTick();
            if (clear_button[i] == 1)
            {
                clear_button[i] = 0;
                button_initial_tick[i] = button_current_tick[i];
            }

            leds_pressFeedback(i, SWITCH_PRESSED);
            shared_var.buttonState[i].state = SWITCH_PRESSED;
            shared_var.buttonState[i].pressed_time = button_current_tick[i] - button_initial_tick[i];
            shared_var.button_update_requested[i] = TRUE;
        }

        if (HAL_GetTick() > (button_current_tick[i] + shared_config.ui_button_delay) && clear_button[i] != 1)
        {
            clear_button[i] = 1;

            leds_pressFeedback(i, SWITCH_RELEASED);
            shared_var.buttonState[i].state = SWITCH_RELEASED;
            shared_var.button_update_requested[i] = TRUE;
        }
    }
}
