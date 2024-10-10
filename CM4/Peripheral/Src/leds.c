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
struct ledStateExtended
{
	uint32_t start_time;
	volatile int32_t current_time;    // If modified by ISR
	int32_t phase;
	int32_t remaining_blinks;
	int32_t led_state;    // Current LED state (0 = OFF, 1 = ON)
	int32_t button_state; // Current button state (0 = Released, 1 = Pressed)
};

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
// Declaration of commands and states for 3 LEDs
static struct led_State ledCommands[NUMBER_OF_LEDS];
static struct ledStateExtended ledState[NUMBER_OF_LEDS];
static volatile uint32_t system_time = 0; // Global timer in milliseconds

// Global variables for button and LED states
int32_t led_states[NUMBER_OF_LEDS];    // Array to store the state of each LED (0 = OFF, 1 = ON)
int32_t button_states[NUMBER_OF_BUTTONS]; // Array to store the state of each button (0 = Released, 1 = Pressed)

/* Variable containing black and white frame from CIS*/

/* Private function prototypes -----------------------------------------------*/
static int32_t leds_interpolate(int32_t start, int32_t end, int32_t duration, int32_t elapsed_time);
static void leds_initCommand(int32_t led_index, int32_t brightness_1, int32_t time_1, int32_t glide_1, int32_t brightness_2, int32_t time_2, int32_t glide_2, int32_t blink_count);
static void leds_handle(struct led_State *cmd, struct ledStateExtended *state, GPIO_TypeDef *GPIO_Port, uint16_t GPIO_Pin, int32_t button_id);

// Timer initialization function for htim12
void leds_timerInit(void)
{
	printf("Initializing Timer...\n");
	HAL_TIM_Base_Start_IT(&htim12);  // Start timer with interrupt
}

// Function for linear interpolation between two values
static int32_t leds_interpolate(int32_t start, int32_t end, int32_t duration, int32_t elapsed_time)
{
	if (elapsed_time >= duration)
	{
		return end; // Return the final value if the duration is reached
	}
	return start + ((end - start) * elapsed_time) / duration; // Linear interpolation
}

// Function to initialize the LED command parameters
static void leds_initCommand(int32_t led_index, int32_t brightness_1, int32_t time_1, int32_t glide_1, int32_t brightness_2, int32_t time_2, int32_t glide_2, int32_t blink_count)
{
	printf("Initializing LED %d with brightness_1: %d, time_1: %d, glide_1: %d, brightness_2: %d, time_2: %d, glide_2: %d, blink_count: %d\n",
			(int)led_index, (int)brightness_1, (int)time_1, (int)glide_1, (int)brightness_2, (int)time_2, (int)glide_2, (int)blink_count);

	struct led_State *cmd = &ledCommands[led_index];
	cmd->brightness_1 = brightness_1;
	cmd->time_1 = time_1 * 10;
	cmd->glide_1 = glide_1;
	cmd->brightness_2 = brightness_2;
	cmd->time_2 = time_2 * 10;
	cmd->glide_2 = glide_2;
	cmd->blink_count = blink_count;

	// Initialize the state of the LED
	struct ledStateExtended *state = &ledState[led_index];
	state->start_time = system_time;
	state->current_time = 0;
	state->phase = 1;  // Start with phase 1
	state->remaining_blinks = (blink_count > 0) ? blink_count : -1;  // -1 for infinite loop
}

// Function to handle each LED based on its state
static void leds_handle(struct led_State *cmd, struct ledStateExtended *state, GPIO_TypeDef *GPIO_Port, uint16_t GPIO_Pin, int32_t button_id)
{
	int32_t target_brightness;
	uint32_t elapsed_time = system_time - state->start_time;
	int32_t brightness_start, brightness_end, time, glide;

	// Check if the button associated with the LED is pressed
	if (button_states[button_id] == 1)
	{
		state->led_state = 1;  // Force LED ON
		HAL_GPIO_WritePin(GPIO_Port, GPIO_Pin, GPIO_PIN_RESET);  // Turn on the LED
		return;  // Skip normal LED handling if the button is pressed
	}

	if (state->remaining_blinks == 0)
	{
		HAL_GPIO_WritePin(GPIO_Port, GPIO_Pin, GPIO_PIN_SET);  // Turn off the LED
		return;  // Stop processing if no blinks are remaining
	}

	// Determine phase-specific values
	if (state->phase == 1)
	{
		brightness_start = cmd->brightness_2;
		brightness_end = cmd->brightness_1;
		time = cmd->time_1;
		glide = cmd->glide_1;
	}
	else
	{
		brightness_start = cmd->brightness_1;
		brightness_end = cmd->brightness_2;
		time = cmd->time_2;
		glide = cmd->glide_2;
	}

	// Infinite duration check
	if (time == 0)
	{
		target_brightness = brightness_end;
	}
	else
	{
		// Calculate interpolated brightness using glide
		target_brightness = leds_interpolate(brightness_start, brightness_end, (time * glide) / 100, elapsed_time);
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

	// Check if the phase is complete
	if (elapsed_time >= time && time != 0)
	{
		if (state->phase == 1)
		{
			state->phase = 2;
		}
		else
		{
			if (state->remaining_blinks > 0)
			{
				state->remaining_blinks--;
			}
			if (state->remaining_blinks != 0)
			{
				state->phase = 1;
			}
			else
			{
				HAL_GPIO_WritePin(GPIO_Port, GPIO_Pin, GPIO_PIN_SET);  // Turn off the LED
			}
		}
		state->start_time = system_time;  // Reset start time for the next phase
	}
}

// Interrupt handler updated to manage multiple LEDs
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	if (htim->Instance == TIM12)
	{
        for (int i = 0; i < NUMBER_OF_LEDS; i++)
        {
            GPIO_TypeDef *ledPort;
            uint16_t ledPin;

            switch (i)
            {
                case 0:
                    ledPort = LED1_GPIO_Port;
                    ledPin = LED1_Pin;
                    break;
                case 1:
                    ledPort = LED2_GPIO_Port;
                    ledPin = LED2_Pin;
                    break;
                case 2:
                    ledPort = LED3_GPIO_Port;
                    ledPin = LED3_Pin;
                    break;
            }

            leds_handle(&ledCommands[i], &ledState[i], ledPort, ledPin, i);
        }
		system_time++; // Increment global system time for PWM
	}
}

void leds_pressFeedback(buttonIdTypeDef button_id, buttonStateTypeDef is_pressed)
{
	button_states[button_id] = is_pressed;  // Update the button state

	if (is_pressed == SWITCH_PRESSED)
	{
		// Forcing the LED associated with the button to stay on
		led_states[button_id] = 1;  // Set the LED state to ON
	}
	else
	{
		// Releasing the button, restore normal LED behavior
		led_states[button_id] = 0;  // Set the LED state to OFF or restore previous behavior
	}
}

void leds_check_update_state(void)
{
	for (int i = 0; i < NUMBER_OF_LEDS; i++)
	{
		// Check if an update was requested for this LED
		if (shared_var.led_update_requested[i] == TRUE)
		{
			// Update the LED state
			leds_initCommand(i,
					shared_var.ledState[i].brightness_1,
					shared_var.ledState[i].time_1,
					shared_var.ledState[i].glide_1,
					shared_var.ledState[i].brightness_2,
					shared_var.ledState[i].time_2,
					shared_var.ledState[i].glide_2,
					shared_var.ledState[i].blink_count);
					// Clear the update request flag after processing
			shared_var.led_update_requested[i] = FALSE;
		}
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

	// Initialization of LED states
	leds_initCommand(0, 10, 1000, 80, 79, 200, 0, 0);    // LED1: Constant lighting at 10% without blinking
	HAL_Delay(500);
	leds_initCommand(1, 30, 1000, 100, 0, 1000, 100, 20);  // LED2: Blinking with transitions
	HAL_Delay(500);
	leds_initCommand(2, 100, 1000, 80, 30, 1000, 0, 20);   // LED3: Transition from 100% to 30%
}
