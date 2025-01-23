/**
 ******************************************************************************
 * @file           : udp_client.c
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

#include "lwip/opt.h"
#include "lwip/arch.h"
#include "lwip/api.h"
#include "lwip/apps/fs.h"
#include "lwip/netif.h"

#include "lwip/udp.h"
#include "lwip/api.h"
#include "lwip/err.h"

#include "stm32h7xx_hal_eth.h"
#include "lan8742.h"

#include <stdio.h>
#include "icm42688.h"

#include "arm_math.h"

/* Private includes ----------------------------------------------------------*/
#include "udp_client.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
struct netconn *conn;
__IO uint32_t message_count = 0;

static struct packet_StartupInfo packet_StartupInfo = {0};
static struct packet_Button packet_Button = {0};

static uint32_t packetsCounter = 0;

volatile uint32_t isConnected = 0;

/* Private function prototypes -----------------------------------------------*/
static void udp_clientSendData(void *data, uint16_t length);
void udp_clientSendStartupInfoPacket(void);

/* Private user code ---------------------------------------------------------*/

/**
 * @brief Initialize the UDP client.
 */
void udp_clientInit(void)
{
    printf("---------- UDP INIT -----------\n");
    ip_addr_t destIPaddr;

    /* Create a new UDP connection */
    conn = netconn_new(NETCONN_UDP);
    if (conn != NULL) {

        //netconn_set_sendtimeout(conn, 100);

        IP4_ADDR(&destIPaddr, shared_config.network_dest_ip[0], shared_config.network_dest_ip[1], shared_config.network_dest_ip[2], shared_config.network_dest_ip[3]);
        netconn_bind(conn, NULL, 0);//        DEFAULT_NETWORK_UDP_PORT);
        netconn_connect(conn, &destIPaddr, shared_config.network_udp_port);

        printf("UDP initialization SUCCESS\n");
    } else {
        printf("Failed to initialize UDP\n");
        Error_Handler();
    }

    /* Initialize the packet_Image array based on cisConfig.pixels_nb */
    for (int32_t packet = UDP_MAX_NB_PACKET_PER_LINE; --packet >= 0;)
    {
        packet_Image[packet].fragment_size = UDP_LINE_FRAGMENT_SIZE;
        packet_Image[packet].total_fragments = cisConfig.udp_nb_packet_per_line;
        packet_Image[packet].type = IMAGE_DATA_HEADER;
        packet_Image[packet].fragment_id = packet;
    }

    packet_StartupInfo.type = STARTUP_INFO_HEADER;
    packet_Button.type = BUTTON_DATA_HEADER;
    packet_IMU.type = IMU_DATA_HEADER;

    packet_StartupInfo.packet_id = packetsCounter++;
    sprintf((char *)packet_StartupInfo.version_info, "CISYNTH v%s RESO-NANCE", FW_VERSION);

    udp_clientSendStartupInfoPacket();
}

/**
 * @brief Send data over UDP.
 * @param data Pointer to the data to send.
 * @param length Length of the data in bytes.
 */
void udp_clientSendData(void *data, uint16_t length)
{
    if (isConnected == 0)
    {
        return;
    }

    // Create a new netbuf
    struct netbuf *buf = netbuf_new();
    if (buf == NULL)
    {
        printf("Failed to allocate netbuf.\n");
        return;
    }

    // Allocate space in the netbuf
    if (netbuf_alloc(buf, length) == NULL)
    {
        printf("Failed to allocate buffer in netbuf.\n");
        netbuf_delete(buf);
        return;
    }

    // Copy data into the netbuf
    netbuf_take(buf, data, length);

    // Send the data
    err_t err = netconn_send(conn, buf);
    if (err != ERR_OK)
    {
        printf("Failed to send UDP data: %d\n", err);
    }

    // Cleanup
    netbuf_delete(buf);
}

/**
 * @brief Send startup information packet.
 */
void udp_clientSendStartupInfoPacket(void)
{
	udp_clientSendData(&packet_StartupInfo, sizeof(packet_StartupInfo));
}


/**
 * @brief Send multiple packets, including IMU and button states.
 * @param rgbBuffers Pointer to array of image packets.
 */
#pragma GCC push_options
#pragma GCC optimize ("unroll-loops")
void udp_clientSendPackets(struct packet_Image *rgbBuffers)
{
	static int32_t packet = 0;

	for (packet = cisConfig.udp_nb_packet_per_line; --packet >= 0;)
	{
		rgbBuffers[packet].packet_id = packetsCounter++;
		udp_clientSendData(&rgbBuffers[packet], sizeof(struct packet_Image));
	}

	packet_IMU.packet_id = packetsCounter++;

	packet_IMU.acc[0] = icm42688_accX();
	packet_IMU.acc[1] = icm42688_accY();
	packet_IMU.acc[2] = icm42688_accZ();

	packet_IMU.gyro[0] = icm42688_gyrX();
	packet_IMU.gyro[1] = icm42688_gyrY();
	packet_IMU.gyro[2] = icm42688_gyrZ();

	SCB_CleanDCache_by_Addr((uint32_t *)&packet_IMU, sizeof(packet_IMU));

	udp_clientSendData(&packet_IMU, sizeof(struct packet_IMU));

	icm42688_TIM_Callback();

	for (int i = 0; i < NUMBER_OF_BUTTONS; i++)
	{
		// Check if an update was requested for this Button
		if (shared_var.button_update_requested[i] == TRUE)
		{
			packet_Button.packet_id = packetsCounter++;
			// Update the LED state
			packet_Button.button_id = i;
			packet_Button.button_state = shared_var.buttonState[i];
			// Clear the update request flag after processing
			shared_var.button_update_requested[i] = FALSE;
		}
	}

	udp_clientSendData(&packet_Button, sizeof(packet_Button));
}
#pragma GCC pop_options
