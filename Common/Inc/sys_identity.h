/**
 ******************************************************************************
 * @file           : sys_identity.h
 * @brief          : Per-unit identity derived from the MCU unique id
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
#ifndef SYS_IDENTITY_H
#define SYS_IDENTITY_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SYS_IDENTITY_NAME_LEN    16
#define SYS_IDENTITY_SERIAL_LEN  16

/** 96-bit MCU unique id, little-endian word order (UIDw0, UIDw1, UIDw2). */
void     sys_identity_uid(uint8_t out[12]);

/** 32-bit FNV-1a hash of the unique id (stable across boots). */
uint32_t sys_identity_hash32(void);

/** Locally administered MAC 02:53:33:xx:xx:xx derived from the unique id. */
void     sys_identity_mac(uint8_t out[6]);

/** "Sp3ctra-XXXX" (NUL-terminated, fits SYS_IDENTITY_NAME_LEN). */
void     sys_identity_name(char out[SYS_IDENTITY_NAME_LEN]);

/** "S3-XXXXXXXX" (NUL-terminated, fits SYS_IDENTITY_SERIAL_LEN). */
void     sys_identity_serial(char out[SYS_IDENTITY_SERIAL_LEN]);

#ifdef __cplusplus
}
#endif

#endif /* SYS_IDENTITY_H */
