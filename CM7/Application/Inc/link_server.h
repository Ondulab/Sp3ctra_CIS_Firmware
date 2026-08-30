/**
 ******************************************************************************
 * @file           : link_server.h
 * @brief          : Sp3ctra Link (SLP) control channel - device side
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
#ifndef __LINK_SERVER_H__
#define __LINK_SERVER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef enum
{
    LINKSERVER_OK = 0,
    LINKSERVER_ERROR = 1
} LINKSERVER_StatusTypeDef;

/** Create the link task (call once lwIP is initialised, before the CIS starts). */
LINKSERVER_StatusTypeDef link_serverInit(void);

/** 1 while a host session is bound. Readable from any task. */
uint8_t  link_isBound(void);

/** Negotiated HID rate (Hz). Defaults to SLP_DEFAULT_HID_RATE_HZ when unbound. */
uint16_t link_getHidRateHz(void);

/** Bound peer address (0.0.0.0 when unbound). */
void     link_getPeerIp(uint8_t out[4]);

/** Re-apply the stream target after a configuration change (dest ip / port / fallback). */
void     link_refreshStreamTarget(void);

#ifdef __cplusplus
}
#endif

#endif /* __LINK_SERVER_H__ */
