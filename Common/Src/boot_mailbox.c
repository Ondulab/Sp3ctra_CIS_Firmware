/**
 ******************************************************************************
 * @file           : boot_mailbox.c
 * @brief          : Boite aux lettres RAM application <-> bootloader.
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

#include "boot_mailbox.h"

#ifdef CORE_CM7

#include <stddef.h>
#include <string.h>

#include "ota.h" /* ota_crc32 */
#include "stm32h7xx.h"

static boot_mailbox_t *const mb = (boot_mailbox_t *)BOOT_MAILBOX_ADDR;

static uint32_t mb_crc(const boot_mailbox_t *m)
{
    return ota_crc32(OTA_CRC32_INIT, m, offsetof(boot_mailbox_t, crc));
}

/* Ecriture puis nettoyage de cache : le reset qui suit ne recopierait pas une
 * ligne sale. Sans effet quand le D-cache est coupe (mode flasheur). */
static void mb_seal(void)
{
    mb->crc = mb_crc(mb);
    SCB_CleanDCache_by_Addr((uint32_t *)mb, (int32_t)sizeof(*mb));
}

static bool mb_valid(void)
{
    return (mb->magic == BOOT_MAILBOX_MAGIC) && (mb->crc == mb_crc(mb));
}

static void mb_reset(void)
{
    memset(mb, 0, sizeof(*mb));
    mb->magic = BOOT_MAILBOX_MAGIC;
    mb_seal();
}

bool boot_mailbox_read(boot_mailbox_t *out)
{
    if (!mb_valid())
    {
        return false;
    }
    memcpy(out, mb, sizeof(*out));
    return true;
}

void boot_mailbox_request_netboot(const uint8_t ip[4], const uint8_t netmask[4])
{
    if (!mb_valid())
    {
        mb_reset();
    }
    mb->request = BOOT_REQ_NETBOOT;
    memcpy(mb->ip, ip, 4);
    memcpy(mb->netmask, netmask, 4);
    mb_seal();
}

bool boot_mailbox_take_request(boot_mailbox_t *out)
{
    if (!mb_valid())
    {
        mb_reset();
        return false;
    }
    if (mb->request == BOOT_REQ_NONE)
    {
        return false;
    }
    memcpy(out, mb, sizeof(*out));
    mb->request = BOOT_REQ_NONE;
    mb_seal();
    return true;
}

void boot_mailbox_note_reset_flags(uint32_t rcc_rsr)
{
    if (!mb_valid())
    {
        mb_reset();
    }
    mb->reset_flags = rcc_rsr;
    mb->boot_count++;
    mb_seal();
}

#endif /* CORE_CM7 */
