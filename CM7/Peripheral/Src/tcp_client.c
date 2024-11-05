/**
 ******************************************************************************
 * @file           : tcp_client.c
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
#include "stm32h7xx_hal.h"
#include "main.h"
#include "config.h"
#include "globals.h"
#include "basetypes.h"

#include "lwip/opt.h"
#include "lwip/arch.h"
#include "lwip/api.h"
#include "lwip/apps/fs.h"

#include <stdio.h>
#include "icm42688.h"

#include "arm_math.h"

/* Private includes ----------------------------------------------------------*/
#include "tcp_client.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/* Private user code ---------------------------------------------------------*/

void print_received_data(uint8_t* data, uint16_t len)
{
	printf("Received %d bytes: ", len);
	for (uint16_t i = 0; i < len; i++) {
		printf("%02X ", data[i]);
	}
	printf("\n");
}

/**
 * @brief  TCP client task to control parameters based on incoming network data.
 *
 * This function sets up a TCP server on port and listens for incoming
 * connections. It expects packets of type `packet_HID_Leds` and updates the
 * LED states accordingly in the global `shared_var` structure.
 *
 * @param  arg: Argument passed to the task (not used).
 *
 * command line test : echo -n -e "\x04\x00\x00\x00\x01\x00\x00\x00\x01\x00\x00\x00\x64\x00\xE8\x03\x64\x00\x00\x00\xE8\x03\x64\x00\x0F\x00" | nc 192.168.0.10 5000
 */
static void tcp_client_task(void *arg)
{
	printf("----- HTTP THREAD STARTED ------\n");

	struct netconn *conn, *newconn;  // Connection and new connection pointers
	err_t err;                       // Variable for error handling
	struct netbuf *buf;              // Buffer to hold received data
	void *data;                      // Pointer to the received data
	u16_t len;                       // Length of the received data

	// Create a new TCP connection
	conn = netconn_new(NETCONN_TCP);
	netconn_bind(conn, NULL, shared_config.network_tcp_port);  // Bind to port 5000 (or any specified port)
	netconn_listen(conn);            // Start listening for incoming connections

	while (1)
	{
		// Accept a new incoming connection
		err = netconn_accept(conn, &newconn);
		if (err == ERR_OK)
		{
			// Receive data from the new connection
			while ((err = netconn_recv(newconn, &buf)) == ERR_OK)
			{
				netbuf_data(buf, &data, &len);  // Extract data and its length

				// Check if the received packet matches the expected structure size
				if (len <= sizeof(struct packet_Leds))
				{
					print_received_data(data, len);
					// Cast received data into the `packet_HID_Leds` structure
					struct packet_Leds led_packet;
					memset(&led_packet, 0, sizeof(led_packet));
					memcpy(&led_packet, data, len < sizeof(struct packet_Leds) ? len : sizeof(struct packet_Leds));

					printf("Received data: brightness_1=%d, time_1=%d, glide_1=%d, brightness_2=%d, time_2=%d, glide_2=%d, blink_count=%d\n",
							(int)led_packet.led_state.brightness_1,
							(int)led_packet.led_state.time_1,
							(int)led_packet.led_state.glide_1,
							(int)led_packet.led_state.brightness_2,
							(int)led_packet.led_state.time_2,
							(int)led_packet.led_state.glide_2,
							(int)led_packet.led_state.blink_count);

					// Verify if the packet type is HID_DATA_HEADER
					if (led_packet.type == LED_DATA_HEADER)
					{
						// Sanity check
						if (led_packet.led_id < 3)
						{
							// Update LED states in the shared global structure
							shared_var.ledState[led_packet.led_id].blink_count = led_packet.led_state.blink_count;
							shared_var.ledState[led_packet.led_id].brightness_1 = led_packet.led_state.brightness_1;
							shared_var.ledState[led_packet.led_id].brightness_2 = led_packet.led_state.brightness_2;
							shared_var.ledState[led_packet.led_id].glide_1 = led_packet.led_state.glide_1;
							shared_var.ledState[led_packet.led_id].glide_2 = led_packet.led_state.glide_2;
							shared_var.ledState[led_packet.led_id].time_1 = led_packet.led_state.time_1;
							shared_var.ledState[led_packet.led_id].time_2 = led_packet.led_state.time_2;
							shared_var.ledState[led_packet.led_id].blink_count = led_packet.led_state.blink_count;

							shared_var.led_update_requested[led_packet.led_id] = TRUE;
						}
					}
					else
					{
						printf("Invalid packet type received: %d\n", (int)led_packet.type);
					}
				}
				else
				{
					printf("Invalid packet size received: %d\n", len);
				}

				// Release the received buffer to free memory
				netbuf_delete(buf);
			}

			// Close and delete the connection after processing the data
			netconn_close(newconn);
			netconn_delete(newconn);
		}
	}
}

/**
 * @brief  Initializes the TCP client for LED control.
 *
 * This function creates a FreeRTOS task that will handle incoming TCP
 * connections and control LEDs based on received data. If the task creation
 * fails, an error message is printed.
 */
void tcp_clientInit(void)
{
	printf("--- TCP LED INITIALIZATIONS ---\n");

	// Create the TCP client task for LED control with a stack size of 1024 and normal priority
	if (xTaskCreate(tcp_client_task, "TCP_LED_Control", 1024, NULL, osPriorityNormal, NULL) == pdPASS)
	{
		printf("TCP TASK SUCCESS\n");
	}
	else
	{
		// Error handling in case the task could not be created
		printf("Failed to create HTTP task.\n");
	}
}
