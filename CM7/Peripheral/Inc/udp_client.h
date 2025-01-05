/**
 ******************************************************************************
 * @file           : udp_client.h
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

/* Exported constants --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/

/* Exported functions prototypes ---------------------------------------------*/
void udp_clientInit(void);
void udp_clientSendStartupInfoPacket(void);
void udp_clientSendPackets(struct packet_Image *rgbBuffers);

/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /*__UDP_CLIENT_H__*/
