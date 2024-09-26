/**
 ******************************************************************************
 * @file           : leds.c
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
#include "stdio.h"
#include "stdlib.h"
#include "basetypes.h"

#include "shared.h"
#include "config.h"

#include "tim.h"

#include "leds.h"

/* Private define ------------------------------------------------------------*/

/* Private typedef -----------------------------------------------------------*/
// Structure containing the current state of the LED
typedef struct
{
    uint32_t start_time;
    volatile int current_time;    // If modified by ISR
    int phase;
    int remaining_blinks;
} ledStateExtendedTypeDef;

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
// Declaration of commands and states for 3 LEDs
static ledStateTypeDef ledCommands[3];
static ledStateExtendedTypeDef ledState[3];
static volatile uint32_t system_time = 0; // Global timer in milliseconds

/* Variable containing black and white frame from CIS*/

/* Private function prototypes -----------------------------------------------*/
static void leds_timerInit(void);
static int leds_interpolate(int start, int end, int duration, int elapsed_time);
static void leds_initCommand(int led_index, int brightness_1, int time_1, int glide_1, int brightness_2, int time_2, int glide_2, int blink_count);
static void leds_handle(ledStateTypeDef *cmd, ledStateExtendedTypeDef *state, GPIO_TypeDef *GPIO_Port, uint16_t GPIO_Pin);

// Timer initialization function for htim12
static void leds_timerInit(void)
{
    printf("Initializing Timer...\n");
    HAL_TIM_Base_Start_IT(&htim12);  // Start timer with interrupt
}

// Function for linear interpolation between two values
static int leds_interpolate(int start, int end, int duration, int elapsed_time)
{
    if (elapsed_time >= duration)
    {
        return end; // Return the final value if the duration is reached
    }
    return start + ((end - start) * elapsed_time) / duration; // Linear interpolation
}

// Function to initialize the LED command parameters
static void leds_initCommand(int led_index, int brightness_1, int time_1, int glide_1, int brightness_2, int time_2, int glide_2, int blink_count)
{
    printf("Initializing LED %d with brightness_1: %d, time_1: %d, glide_1: %d, brightness_2: %d, time_2: %d, glide_2: %d, blink_count: %d\n",
           led_index, brightness_1, time_1, glide_1, brightness_2, time_2, glide_2, blink_count);

    ledStateTypeDef *cmd = &ledCommands[led_index];
    cmd->brightness_1 = brightness_1;
    cmd->time_1 = time_1 * 10;
    cmd->glide_1 = glide_1;
    cmd->brightness_2 = brightness_2;
    cmd->time_2 = time_2 * 10;
    cmd->glide_2 = glide_2;
    cmd->blink_count = blink_count;

    // Initialize the state of the LED
    ledStateExtendedTypeDef *state = &ledState[led_index];
    state->start_time = system_time;
    state->current_time = 0;
    state->phase = 1;  // Start with phase 1
    state->remaining_blinks = (blink_count > 0) ? blink_count : -1;  // -1 for infinite loop
}

// Function to handle each LED based on its state
static void leds_handle(ledStateTypeDef *cmd, ledStateExtendedTypeDef *state, GPIO_TypeDef *GPIO_Port, uint16_t GPIO_Pin)
{
    int target_brightness;
    uint32_t elapsed_time = system_time - state->start_time;

    if (state->remaining_blinks == 0)
    {
        HAL_GPIO_WritePin(GPIO_Port, GPIO_Pin, GPIO_PIN_SET);  // Turn off the LED
        return;  // Stop processing if no blinks are remaining
    }

    if (state->phase == 1)
    {
        // Infinite duration for time_1
        if (cmd->time_1 == 0)
        {
            target_brightness = cmd->brightness_1;
        }
        else
        {
            // Calculate interpolated brightness using glide_1
            target_brightness = leds_interpolate(cmd->brightness_2, cmd->brightness_1, (cmd->time_1 * cmd->glide_1) / 100, elapsed_time);
        }

        // Handle PWM based on target brightness
        if ((system_time % 100) < target_brightness)
        {
            HAL_GPIO_WritePin(GPIO_Port, GPIO_Pin, GPIO_PIN_RESET);  // Turn on the LED
        }
        else
        {
            HAL_GPIO_WritePin(GPIO_Port, GPIO_Pin, GPIO_PIN_SET);    // Turn off the LED
        }

        if (elapsed_time >= cmd->time_1 && cmd->time_1 != 0)
        {
            state->phase = 2;
            state->start_time = system_time;  // Reset time for phase 2
        }
    }
    else if (state->phase == 2)
    {
        // Infinite duration for time_2
        if (cmd->time_2 == 0)
        {
            target_brightness = cmd->brightness_2;
        }
        else
        {
            // Calculate interpolated brightness using glide_2
            target_brightness = leds_interpolate(cmd->brightness_1, cmd->brightness_2, (cmd->time_2 * cmd->glide_2) / 100, elapsed_time);
        }

        // Handle PWM based on target brightness
        if ((system_time % 100) < target_brightness)
        {
            HAL_GPIO_WritePin(GPIO_Port, GPIO_Pin, GPIO_PIN_RESET);  // Turn on the LED
        }
        else
        {
            HAL_GPIO_WritePin(GPIO_Port, GPIO_Pin, GPIO_PIN_SET);    // Turn off the LED
        }

        if (elapsed_time >= cmd->time_2 && cmd->time_2 != 0)
        {
            if (state->remaining_blinks > 0)
            {
                state->remaining_blinks--;
            }
            if (state->remaining_blinks != 0)
            {
                state->phase = 1;
                state->start_time = system_time;  // Reset to restart
            }
            else
            {
                HAL_GPIO_WritePin(GPIO_Port, GPIO_Pin, GPIO_PIN_SET);  // Turn off the LED
                //printf("LED blinking finished\n");
            }
        }
    }
}

// Interrupt handler updated to manage multiple LEDs
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM12)
    {
        leds_handle(&ledCommands[0], &ledState[0], LED1_GPIO_Port, LED1_Pin);  // Manage LED 1
        leds_handle(&ledCommands[1], &ledState[1], LED2_GPIO_Port, LED2_Pin);  // Manage LED 2
        leds_handle(&ledCommands[2], &ledState[2], LED3_GPIO_Port, LED3_Pin);  // Manage LED 3
        system_time++; // Increment global system time for PWM
    }
}

/*
 * Function to control LED behavior with the following parameters:
 *
 * Parameters:
 * 1. led_index: LED to control.
 *      - 0 = LED 1
 *      - 1 = LED 2
 *      - 2 = LED 3
 *
 * 2. brightness_1: Initial brightness level (0-100).
 *      - Default: 0
 *
 * 3. time_1: Duration for brightness_1 in milliseconds.
 *      - Use 0 for infinite duration.
 *      - Default: 0
 *
 * 4. glide_1: Transition percentage to reach brightness_1 over time_1.
 *      - Range: 0-100
 *      - Use 0 for instant transition.
 *      - Default: 0
 *
 * 5. brightness_2: Brightness level for the second phase (0-100).
 *      - Default: 0
 *
 * 6. time_2: Duration for brightness_2 in milliseconds.
 *      - Use 0 for infinite duration.
 *      - Default: 0
 *
 * 7. glide_2: Transition percentage to reach brightness_2 over time_2.
 *      - Range: 0-100
 *      - Use -1 for instant transition.
 *      - Default: 0
 *
 * 8. blink_count: Number of cycles to repeat the sequence.
 *      - 0 = infinite cycles
 *      - Default: 0
 */
void led_test(void)
{
    printf("Starting LED test...\n");
    leds_timerInit();

    // Initialization of LED states
    leds_initCommand(0, 100, 1000, 50, 10, 1000, 0, 0);    // LED1: Constant lighting at 10% without blinking
    HAL_Delay(500);
    leds_initCommand(1, 30, 1000, 100, 0, 1000, 100, 20);  // LED2: Blinking with transitions
    HAL_Delay(500);
    leds_initCommand(2, 100, 1000, 80, 30, 1000, 0, 20);   // LED3: Transition from 100% to 30%
}
