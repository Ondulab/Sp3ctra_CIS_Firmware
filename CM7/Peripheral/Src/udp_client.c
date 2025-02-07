/**
 ******************************************************************************
 * @file           : udpClient_.c
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
/* Declaration of the global semaphore handle */
osSemaphoreDef(udpReadySemaphore);
osSemaphoreId udpReadySemaphoreHandle;

struct netconn *conn;
__IO uint32_t message_count = 0;

static struct packet_StartupInfo packet_StartupInfo = {0};
static struct packet_Button packet_Button = {0};

static uint32_t packetsCounter = 0;

volatile uint32_t isConnected = 0;
volatile uint8_t startupPacketSent = 0;

/* Private function prototypes -----------------------------------------------*/
static UDPCLIENT_StatusTypeDef udpClient_initUdpSemaphore(void);
static UDPCLIENT_StatusTypeDef udpClient_sendData(void *data, uint16_t length);
static UDPCLIENT_StatusTypeDef udpClient_sendStartupInfoPacket(void);
static void udpStartupTask(void const * argument);

/* Private user code ---------------------------------------------------------*/

/**
 * @brief Initializes the semaphore used to signal network availability.
 */
static UDPCLIENT_StatusTypeDef udpClient_initUdpSemaphore(void)
{
    // Create the semaphore "udpReadySemaphore" with an initial count of 1.
    udpReadySemaphoreHandle = osSemaphoreCreate(osSemaphore(udpReadySemaphore), 1);

    // Check if the semaphore was successfully created.
    if (udpReadySemaphoreHandle == NULL)
    {
        // Log an error message if the semaphore creation failed.
        printf("Failed to create UDP ready semaphore.\n");
        return UDPCLIENT_ERROR;
    }

    // Immediately acquire the semaphore in a non-blocking manner (timeout = 0)
    // to set its initial count to 0, effectively "resetting" it.
    osSemaphoreWait(udpReadySemaphoreHandle, 0);
    printf("Semaphore initialized to 0.\n");

    // Return a status indicating the semaphore was initialized successfully.
    return UDPCLIENT_OK;
}

/**
 * @brief Initialize the UDP client.
 */
UDPCLIENT_StatusTypeDef udpClient_init(void)
{
	ip_addr_t destIPaddr;

	udpClient_initUdpSemaphore();

	/* Create a new UDP connection */
	conn = netconn_new(NETCONN_UDP);
	if (conn != NULL)
	{
		//netconn_set_sendtimeout(conn, 100);

		IP4_ADDR(&destIPaddr, shared_config.network_dest_ip[0], shared_config.network_dest_ip[1], shared_config.network_dest_ip[2], shared_config.network_dest_ip[3]);
		netconn_bind(conn, NULL, 0);//        DEFAULT_NETWORK_UDP_PORT);
		netconn_connect(conn, &destIPaddr, shared_config.network_udp_port);

		//printf("UDP initialization SUCCESS\n");
	}
	else
	{
		printf("Failed to initialize UDP\n");
		return UDPCLIENT_ERROR;
	}

    memset((uint32_t *)&buffers_Scanline, 0, sizeof(buffers_Scanline));

	/* Initialize the packet_Image array based on cisConfig.pixels_nb */
	for (int32_t packet = 0; packet < UDP_MAX_NB_PACKET_PER_LINE; packet++)
	{
		// Initialize first buffer (scanline_buff1)
		buffers_Scanline.scanline_buff1[packet].type = IMAGE_DATA_HEADER;
		buffers_Scanline.scanline_buff1[packet].fragment_size = UDP_LINE_FRAGMENT_SIZE;
		buffers_Scanline.scanline_buff1[packet].total_fragments = cisConfig.udp_nb_packet_per_line;
		buffers_Scanline.scanline_buff1[packet].fragment_id = packet;

		// Initialize second buffer (scanline_buff2)
		buffers_Scanline.scanline_buff2[packet].type = IMAGE_DATA_HEADER;
		buffers_Scanline.scanline_buff2[packet].fragment_size = UDP_LINE_FRAGMENT_SIZE;
		buffers_Scanline.scanline_buff2[packet].total_fragments = cisConfig.udp_nb_packet_per_line;
		buffers_Scanline.scanline_buff2[packet].fragment_id = packet;
	}

	packet_StartupInfo.type = STARTUP_INFO_HEADER;
	packet_Button.type = BUTTON_DATA_HEADER;
	packet_IMU.type = IMU_DATA_HEADER;

	sprintf((char *)packet_StartupInfo.version_info, "CISYNTH v%s RESO-NANCE", FW_VERSION);

    osThreadDef(udpStartupTask, udpStartupTask, osPriorityHigh, 0, configMINIMAL_STACK_SIZE * 2);
    osThreadCreate(osThread(udpStartupTask), NULL);

	return UDPCLIENT_OK;
}

/**
 * @brief Send data over UDP.
 * @param data Pointer to the data to send.
 * @param length Length of the data in bytes.
 */
UDPCLIENT_StatusTypeDef udpClient_sendData(void *data, uint16_t length)
{
	if (isConnected == 0)
	{
		return UDPCLIENT_NOT_CONNECTED;
	}

	// Create a new netbuf
	struct netbuf *buf = netbuf_new();
	if (buf == NULL)
	{
		printf("Failed to allocate netbuf.\n");
		return UDPCLIENT_ERROR;
	}

	// Allocate space in the netbuf
	if (netbuf_alloc(buf, length) == NULL)
	{
		printf("Failed to allocate buffer in netbuf.\n");
		netbuf_delete(buf);
		return UDPCLIENT_ERROR;
	}

	// Copy data into the netbuf
	netbuf_take(buf, data, length);

	// Send the data
	err_t err = netconn_send(conn, buf);
	if (err != ERR_OK)
	{
		printf("Failed to send UDP data: %d\n", err);
		netbuf_delete(buf);
		return UDPCLIENT_ERROR;
	}

	// Cleanup
	netbuf_delete(buf);

	return UDPCLIENT_OK;
}

/**
 * @brief Send startup information packet.
 */
UDPCLIENT_StatusTypeDef udpClient_sendStartupInfoPacket(void)
{
	packet_StartupInfo.packet_id = packetsCounter = 1;
	if (udpClient_sendData(&packet_StartupInfo, sizeof(struct packet_StartupInfo)) != UDPCLIENT_OK)
	{
		return UDPCLIENT_ERROR;
	}
	return UDPCLIENT_OK;
}

/**
 * @brief Task that waits for the network to be ready and sends the startup packet.
 */
void udpStartupTask(void const * argument)
{
    // Infinite loop: this task continuously runs.
    for (;;)
    {
        // Wait indefinitely (or with a very long timeout) for the semaphore.
        // This semaphore indicates that the network link might be up.
        if (osSemaphoreWait(udpReadySemaphoreHandle, osWaitForever) == osOK)
        {
            // Semaphore acquired, so check if the network is connected and the startup packet has not been sent.
            if (isConnected == 1 && startupPacketSent == 0)
            {
                // Attempt to send the startup information packet.
                if (udpClient_sendStartupInfoPacket() == UDPCLIENT_OK)
                {
                    // Mark that the startup packet has been successfully sent.
                    startupPacketSent = 1;
                    printf("Startup packet sent.\n");
                }
                else
                {
                    // Log an error message if sending the startup packet fails.
                    printf("Failed to send startup packet.\n");
                }
            }
        }
        // Delay for 100 ticks before the next iteration to prevent busy-waiting.
        osDelay(100);
    }
}

/**
 * @brief Send multiple packets, including IMU and button states.
 * @param rgbBuffers Pointer to array of image packets.
 */
#pragma GCC push_options
#pragma GCC optimize ("unroll-loops")
UDPCLIENT_StatusTypeDef udpClient_sendPackets(struct packet_Scanline *rgbBuffers)
{
    static int32_t packet = 0;
    if (isConnected == 0)
    {
        return UDPCLIENT_NOT_CONNECTED;
    }

    for (packet = cisConfig.udp_nb_packet_per_line; --packet >= 0;)
    {
        rgbBuffers[packet].packet_id = packetsCounter++;
        if (udpClient_sendData(&rgbBuffers[packet], sizeof(struct packet_Scanline)) != UDPCLIENT_OK)
        {
            return UDPCLIENT_ERROR;
        }
    }

	packet_IMU.packet_id = packetsCounter++;

	packet_IMU.acc[0] = icm42688_accX();
	packet_IMU.acc[1] = icm42688_accY();
	packet_IMU.acc[2] = icm42688_accZ();

	packet_IMU.gyro[0] = icm42688_gyrX();
	packet_IMU.gyro[1] = icm42688_gyrY();
	packet_IMU.gyro[2] = icm42688_gyrZ();

	SCB_CleanDCache_by_Addr((uint32_t *)&packet_IMU, sizeof(packet_IMU));

	if (udpClient_sendData((void *)&packet_IMU, sizeof(struct packet_IMU)) != UDPCLIENT_OK)
	{
		return UDPCLIENT_ERROR;
	}

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

	if (udpClient_sendData(&packet_Button, sizeof(struct packet_Button)) != UDPCLIENT_OK)
	{
		return UDPCLIENT_ERROR;
	}

	return UDPCLIENT_OK;
}
#pragma GCC pop_options
