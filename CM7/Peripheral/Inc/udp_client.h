/**
 ******************************************************************************
 * @file           : udpClient_.h
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

/* Private includes ----------------------------------------------------------*/

/* Exported types ------------------------------------------------------------*/
extern volatile uint32_t isConnected;
extern volatile uint8_t startupPacketSent;

/* Custom return type for UDP client -----------------------------*/
typedef enum {
	UDPCLIENT_OK = 0,
	UDPCLIENT_ERROR = 1,
	UDPCLIENT_NOT_CONNECTED = 2
} UDPCLIENT_StatusTypeDef;

/* Exported constants --------------------------------------------------------*/
extern osSemaphoreId udpReadySemaphoreHandle;

/* Exported macro ------------------------------------------------------------*/

/* Exported functions prototypes ---------------------------------------------*/
UDPCLIENT_StatusTypeDef udpClient_init(void);
UDPCLIENT_StatusTypeDef udpClient_sendPackets(struct packet_Scanline *rgbBuffers);

/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /*__UDP_CLIENT_H__*/
