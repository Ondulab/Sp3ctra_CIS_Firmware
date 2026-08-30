/**
 ******************************************************************************
 * @file           : sys_identity.c
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
#include <stdio.h>
#include <string.h>

#include "main.h"          /* HAL: HAL_GetUIDw0/1/2 (available on both cores) */
#include "sys_identity.h"

void sys_identity_uid(uint8_t out[12])
{
    const uint32_t w[3] = { HAL_GetUIDw0(), HAL_GetUIDw1(), HAL_GetUIDw2() };
    memcpy(out, w, 12);
}

uint32_t sys_identity_hash32(void)
{
    uint8_t uid[12];
    sys_identity_uid(uid);

    /* FNV-1a, 32-bit */
    uint32_t h = 0x811C9DC5u;
    for (int i = 0; i < 12; i++)
    {
        h ^= uid[i];
        h *= 0x01000193u;
    }
    return h;
}

void sys_identity_mac(uint8_t out[6])
{
    const uint32_t h = sys_identity_hash32();
    out[0] = 0x02;                      /* locally administered, unicast */
    out[1] = 0x53;                      /* 'S' */
    out[2] = 0x33;                      /* '3' */
    out[3] = (uint8_t)(h >> 16);
    out[4] = (uint8_t)(h >> 8);
    out[5] = (uint8_t)(h);
}

void sys_identity_name(char out[SYS_IDENTITY_NAME_LEN])
{
    snprintf(out, SYS_IDENTITY_NAME_LEN, "Sp3ctra-%04X", (unsigned)(sys_identity_hash32() & 0xFFFFu));
}

void sys_identity_serial(char out[SYS_IDENTITY_SERIAL_LEN])
{
    snprintf(out, SYS_IDENTITY_SERIAL_LEN, "S3-%08lX", (unsigned long)sys_identity_hash32());
}
