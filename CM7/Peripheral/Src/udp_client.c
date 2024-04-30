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
#include "shared.h"

#include "lwip/pbuf.h"
#include "lwip/udp.h"
#include "lwip/tcp.h"
#include "lwip.h"

#include "icm42688.h"

#include "arm_math.h"

/* Private includes ----------------------------------------------------------*/
#include "udp_client.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
struct udp_pcb *upcb;
__IO uint32_t message_count = 0;
int32_t udp_imageData[UDP_PACKET_SIZE] = {0};

static struct packet_StartupInfo packet_StartupInfo = {0};
static struct packet_HID packet_HID = {0};

static uint32_t packetsCounter = 0;

/* Private function prototypes -----------------------------------------------*/
static void udp_clientReceiveCallback(void *arg, struct udp_pcb *upcb, struct pbuf *p, const ip_addr_t *addr, u16_t port);
static void udp_clientSendData(void *data, uint16_t length);

/* Private user code ---------------------------------------------------------*/

void udp_clientInit(void)
{
	printf("-------- CIS UDP INIT ---------\n");
	                                          //

	ip_addr_t DestIPaddr;
	err_t err;

	//	lwiperf_start_tcp_server_default(NULL, NULL); // TCP Perf = iperf -c 192.168.0.1 -i1 -t60 -u -b 1000M UDP Perf = iperf -c 192.168.0.1 -i1 -t60

	/* Create a new UDP control block  */
	upcb = udp_new();
	ip_set_option(upcb, SOF_BROADCAST); // test for broadcast, useless at frist view
	if (upcb!=NULL)
	{
		/*assign destination IP address */
		IP4_ADDR( &DestIPaddr, shared_config.network_ip[0], shared_config.network_ip[1], shared_config.network_ip[2], 255);


		/* configure destination IP address and port */
		err = udp_connect(upcb, &DestIPaddr, shared_config.network_udp_port);

		if (err == ERR_OK)
		{
			/* Set a receive callback for the upcb */
			udp_recv(upcb, udp_clientReceiveCallback, NULL);
			printf("CIS UDP initialization SUCCESS\n");
		}
		else
		{
			printf("failed to initialize CIS UDP \n");
			Error_Handler();
		}
	}
	else
	{
		printf("failed to initialize CIS UDP \n");
		Error_Handler();
	}

	for (int32_t packet = UDP_NB_PACKET_PER_LINE; --packet >= 0;)
	{
		packet_Image[packet].fragment_size = CIS_PIXELS_NB / UDP_NB_PACKET_PER_LINE;
		packet_Image[packet].total_fragments = UDP_NB_PACKET_PER_LINE;
		packet_Image[packet].type = IMAGE_DATA_HEADER;
	}

	packet_StartupInfo.type = STARTUP_INFO_HEADER;
	packet_HID.type = HID_DATA_HEADER;
	packet_IMU.type = IMU_DATA_HEADER;

	packet_StartupInfo.packet_id = packetsCounter;
	sprintf((char *)packet_StartupInfo.version_info, "CISYNTH v3.0 RESO-NANCE");

	udp_clientSendStartupInfoPacket();
}

void udp_clientSendData(void *data, uint16_t length)
{
	struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, length, PBUF_RAM);
	if (p != NULL)
	{
		pbuf_take(p, data, length);
		udp_send(upcb, p);
		pbuf_free(p);
	}
}


void udp_clientSendStartupInfoPacket(void)
{
	udp_clientSendData(&packet_StartupInfo, sizeof(packet_StartupInfo));
}

#pragma GCC push_options
#pragma GCC optimize ("unroll-loops")
void udp_clientSendPackets(struct packet_Image *rgbBuffers)
{
	static int32_t packet = 0;

	for (packet = UDP_NB_PACKET_PER_LINE; --packet >= 0;)
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

	udp_clientSendData(&packet_IMU, sizeof(packet_IMU));

	SCB_CleanDCache_by_Addr((uint32_t *)&packet_IMU, sizeof(packet_IMU));

	packet_HID.packet_id = packetsCounter++;

	packet_HID.button_A = 0;//shared_var.buttonState[SW1];
	packet_HID.button_B = 0;//shared_var.buttonState[SW2];
	packet_HID.button_C = 0;//shared_var.buttonState[SW3];

	//udp_clientSendData(&packet_HID, sizeof(packet_HID));
}
#pragma GCC pop_options

void udp_clientReceiveCallback(void *arg, struct udp_pcb *upcb, struct pbuf *p, const ip_addr_t *addr, u16_t port)
{
	/*increment message count */
	message_count++;

	/* Free receive pbuf */
	pbuf_free(p);
}
