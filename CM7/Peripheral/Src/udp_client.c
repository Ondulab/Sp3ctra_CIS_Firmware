/**
 ******************************************************************************
 * @file           : udp_client.c
 * @brief          : SLP STREAM flow sender (LINE + HID datagrams)
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
 *
 * One connected UDP netconn shared by cis_sendTask (LINE) and hidTask (HID):
 * netconn_send() is serialised by the tcpip thread, so both may call it.
 * The target is set by the link server (BIND -> host, expiry -> fallback).
 */
/* Includes ------------------------------------------------------------------*/
#include "globals.h"
#include "config.h"

#include "lwip/opt.h"
#include "lwip/arch.h"
#include "lwip/api.h"
#include "lwip/netif.h"
#include "lwip/err.h"

#include <stdio.h>
#include <string.h>

#include "sp3ctra_link.h"
#include "udp_client.h"

/* Private variables ---------------------------------------------------------*/
osSemaphoreId_t udpReadySemaphoreHandle;

static struct netconn *conn = NULL;
static volatile uint8_t streamEnabled = 0;
static uint32_t lineSeq = 0;
static uint32_t linesSent = 0;
static uint32_t lastLineTick = 0;

volatile uint32_t isConnected = 0;

/* Private function prototypes -----------------------------------------------*/
static UDPCLIENT_StatusTypeDef udpClient_initUdpSemaphore(void);
static void udpClient_initLineHeaders(struct slp_line_cis *buf);

/* Private user code ---------------------------------------------------------*/

/**
 * @brief Initializes the semaphore used to signal network availability.
 */
static UDPCLIENT_StatusTypeDef udpClient_initUdpSemaphore(void)
{
    // Released by the lwIP link callback once the stack is ready.
    udpReadySemaphoreHandle = osSemaphoreNew(1U, 0U, NULL);

    if (udpReadySemaphoreHandle == NULL)
    {
        printf("Failed to create UDP ready semaphore.\n");
        return UDPCLIENT_ERROR;
    }

    return UDPCLIENT_OK;
}

/**
 * @brief Pre-fill the constant part of every LINE fragment header of one buffer set.
 */
static void udpClient_initLineHeaders(struct slp_line_cis *buf)
{
	for (int32_t packet = 0; packet < UDP_MAX_NB_PACKET_PER_LINE; packet++)
	{
		buf[packet].h.hdr.magic      = SLP_MAGIC;
		buf[packet].h.hdr.version    = SLP_VERSION;
		buf[packet].h.hdr.type       = SLP_LINE;
		buf[packet].h.hdr.length     = (uint16_t)sizeof(struct slp_line_cis);
		buf[packet].h.hdr.flags      = 0;
		buf[packet].h.pixel_offset   = (uint16_t)(packet * UDP_LINE_FRAGMENT_SIZE);
		buf[packet].h.pixel_count    = UDP_LINE_FRAGMENT_SIZE;
		buf[packet].h.fragment_index = (uint8_t)packet;
		buf[packet].h.fragment_count = (uint8_t)UDP_MAX_NB_PACKET_PER_LINE;   /* refined by cis_init() */
		buf[packet].h.line_period_us = 0;
	}
}

/**
 * @brief Initialize the UDP client.
 */
UDPCLIENT_StatusTypeDef udpClient_init(void)
{
	udpClient_initUdpSemaphore();

	conn = netconn_new(NETCONN_UDP);
	if (conn == NULL)
	{
		printf("Failed to initialize UDP\n");
		return UDPCLIENT_ERROR;
	}
	netconn_bind(conn, NULL, 0);   /* ephemeral source port */

    memset((uint32_t *)&buffers_Scanline, 0, sizeof(buffers_Scanline));
	udpClient_initLineHeaders(buffers_Scanline.scanline_buff1);
	udpClient_initLineHeaders(buffers_Scanline.scanline_buff2);

	udpClient_applyDefaultTarget();

	return UDPCLIENT_OK;
}

UDPCLIENT_StatusTypeDef udpClient_setTarget(const ip_addr_t *ip, uint16_t port)
{
	if (conn == NULL)
	{
		return UDPCLIENT_ERROR;
	}

	if (ip == NULL || port == 0U)
	{
		streamEnabled = 0;
		printf("STREAM: off\n");
		return UDPCLIENT_OK;
	}

	const err_t err = netconn_connect(conn, ip, port);
	if (err != ERR_OK)
	{
		streamEnabled = 0;
		printf("STREAM: connect to %s:%u failed (%d)\n", ipaddr_ntoa(ip), (unsigned)port, (int)err);
		return UDPCLIENT_ERROR;
	}

	streamEnabled = 1;
	printf("STREAM: -> %s:%u\n", ipaddr_ntoa(ip), (unsigned)port);
	return UDPCLIENT_OK;
}

void udpClient_applyDefaultTarget(void)
{
	if (shared_config.stream_when_unbound)
	{
		ip_addr_t dest;
		IP4_ADDR(ip_2_ip4(&dest), shared_config.network_dest_ip[0], shared_config.network_dest_ip[1],
		         shared_config.network_dest_ip[2], shared_config.network_dest_ip[3]);
		udpClient_setTarget(&dest, shared_config.network_udp_port);
	}
	else
	{
		udpClient_setTarget(NULL, 0);
	}
}

uint8_t udpClient_isStreaming(void)
{
	return (streamEnabled && isConnected) ? 1U : 0U;
}

uint32_t udpClient_linesSent(void)
{
	return linesSent;
}

/**
 * @brief Send data over UDP with simple retry mechanism.
 */
UDPCLIENT_StatusTypeDef udpClient_sendData(const void *data, uint16_t length)
{
	if (isConnected == 0 || streamEnabled == 0)
	{
		return UDPCLIENT_NOT_CONNECTED;
	}

	if (conn == NULL)
	{
		return UDPCLIENT_ERROR;
	}

	// Up to 3 attempts with a small exponential backoff: a failed netbuf
	// allocation means the lwIP pools are under pressure, don't hammer them.
	for (int retry = 0; retry < 3; retry++)
	{
		struct netbuf *buf = netbuf_new();
		if (buf == NULL)
		{
			osDelay((uint32_t)(10U << (uint32_t)retry));
			continue;
		}

		if (netbuf_alloc(buf, length) == NULL)
		{
			netbuf_delete(buf);
			osDelay((uint32_t)(10U << (uint32_t)retry));
			continue;
		}

		netbuf_take(buf, data, length);

		err_t err = netconn_send(conn, buf);
		netbuf_delete(buf);

		if (err == ERR_OK)
		{
			return UDPCLIENT_OK;
		}

		if (retry == 2)
		{
			printf("Failed to send UDP data after %d retries: %d\n", retry + 1, err);
			return UDPCLIENT_ERROR;
		}
		osDelay((uint32_t)(10U << (uint32_t)retry));
	}

	return UDPCLIENT_ERROR;
}

/**
 * @brief Send the fragments of one scan line, in natural order.
 * @param rgbBuffers Array of UDP_MAX_NB_PACKET_PER_LINE fragments (line_id / fragment_count set by cis_imageProcess).
 */
#pragma GCC push_options
#pragma GCC optimize ("unroll-loops")
UDPCLIENT_StatusTypeDef udpClient_sendPackets(struct slp_line_cis *rgbBuffers)
{
    if (!udpClient_isStreaming())
    {
        return UDPCLIENT_NOT_CONNECTED;
    }

    const uint32_t now = HAL_GetTick();
    uint32_t period_us = (lastLineTick != 0U) ? (now - lastLineTick) * 1000U : 0U;
    if (period_us > 65535U)
    {
        period_us = 65535U;
    }
    lastLineTick = now;

    for (int32_t packet = 0; packet < cisConfig.udp_nb_packet_per_line; packet++)
    {
        rgbBuffers[packet].h.hdr.seq = lineSeq++;
        rgbBuffers[packet].h.line_period_us = (uint16_t)period_us;
        if (udpClient_sendData(&rgbBuffers[packet], sizeof(struct slp_line_cis)) != UDPCLIENT_OK)
        {
            return UDPCLIENT_ERROR;
        }
    }

    linesSent++;
	return UDPCLIENT_OK;
}
#pragma GCC pop_options
