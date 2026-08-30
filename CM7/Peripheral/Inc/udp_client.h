/**
 ******************************************************************************
 * @file           : udp_client.h
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
 */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __UDP_CLIENT_H__
#define __UDP_CLIENT_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "globals.h"
#include "lwip/ip_addr.h"

/* Exported types ------------------------------------------------------------*/
extern volatile uint32_t isConnected;

typedef enum {
	UDPCLIENT_OK = 0,
	UDPCLIENT_ERROR = 1,
	UDPCLIENT_NOT_CONNECTED = 2
} UDPCLIENT_StatusTypeDef;

/* Exported constants --------------------------------------------------------*/
extern osSemaphoreId_t udpReadySemaphoreHandle;

/* Exported functions prototypes ---------------------------------------------*/
/** Create the UDP netconn and pre-fill the LINE headers. Applies the default target. */
UDPCLIENT_StatusTypeDef udpClient_init(void);

/** Point the STREAM flow at ip:port (NULL / 0 = stop streaming). Any task. */
UDPCLIENT_StatusTypeDef udpClient_setTarget(const ip_addr_t *ip, uint16_t port);

/** Unbound fallback: shared_config.network_dest_ip:network_udp_port when stream_when_unbound, else off. */
void udpClient_applyDefaultTarget(void);

/** 1 when a target is set and the Ethernet link is up. */
uint8_t udpClient_isStreaming(void);

/** Send one raw datagram on the STREAM flow (retries on netbuf pressure). */
UDPCLIENT_StatusTypeDef udpClient_sendData(const void *data, uint16_t length);

/** Send the fragments of one scan line (called by cis_sendTask). */
UDPCLIENT_StatusTypeDef udpClient_sendPackets(struct slp_line_cis *rgbBuffers);

/** Lines sent since boot (PONG statistics). */
uint32_t udpClient_linesSent(void);

#ifdef __cplusplus
}
#endif

#endif /*__UDP_CLIENT_H__*/
