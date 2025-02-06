/**
 ******************************************************************************
 * @file           : tcp_client.h
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
#ifndef __TCP_CLIENT_H__
#define __TCP_CLIENT_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

/* Private includes ----------------------------------------------------------*/

/* Exported types ------------------------------------------------------------*/
/* Custom return type for TCP client -----------------------------*/
typedef enum {
	TCPCLIENT_OK = 0,
	TCPCLIENT_ERROR = 1
} TCPCLIENT_StatusTypeDef;

/* Exported constants --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/

/* Exported functions prototypes ---------------------------------------------*/
TCPCLIENT_StatusTypeDef tcp_clientInit(void);

/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /*__TCP_CLIENT_H__*/
