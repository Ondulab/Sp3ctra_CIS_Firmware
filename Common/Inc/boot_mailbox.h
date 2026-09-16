/**
 ******************************************************************************
 * @file           : boot_mailbox.h
 * @brief          : Boite aux lettres RAM entre l'application et le bootloader
 *                   (demande de mode flasheur reseau, cause du dernier reset).
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

#ifndef BOOT_MAILBOX_H
#define BOOT_MAILBOX_H

#include <stdbool.h>
#include <stdint.h>

#include "log_ring.h" /* BOOT_MAILBOX_ADDR : meme zone RAM retenue */

#ifdef __cplusplus
extern "C" {
#endif

#define BOOT_MAILBOX_MAGIC 0x4D424E53u /* "SNBM" */

typedef enum {
    BOOT_REQ_NONE    = 0u,
    BOOT_REQ_NETBOOT = 0x4254454Eu, /* "NETB" : rester dans le bootloader, mode flasheur reseau */
} boot_request_t;

/* 64 octets, une ligne de cache sur deux : scellee par un CRC-32 (ota_crc32).
 * Une RAM aleatoire a la mise sous tension ne passe pas le scelle. */
typedef struct {
    uint32_t magic;
    uint32_t request;       /* boot_request_t, remis a BOOT_REQ_NONE quand le bootloader l'a lue */
    uint8_t  ip[4];         /* adresse que l'application utilisait (0.0.0.0 = defaut)          */
    uint8_t  netmask[4];
    uint32_t reset_flags;   /* RCC_RSR lu par le bootloader au dernier demarrage                */
    uint32_t boot_count;    /* passages dans le bootloader depuis la mise sous tension          */
    uint8_t  reserved[36];
    uint32_t crc;           /* sur les 60 octets qui precedent                                  */
} boot_mailbox_t;

_Static_assert(sizeof(boot_mailbox_t) == BOOT_MAILBOX_SIZE, "boot_mailbox_t doit faire 64 octets");

#ifdef CORE_CM7
/* Application : demande au prochain reset d'entrer en mode flasheur reseau. */
void boot_mailbox_request_netboot(const uint8_t ip[4], const uint8_t netmask[4]);

/* Bootloader : lit ET efface une demande en attente. Faux si aucune. */
bool boot_mailbox_take_request(boot_mailbox_t *out);

/* Bootloader : conserve la cause du reset pour l'application. */
void boot_mailbox_note_reset_flags(uint32_t rcc_rsr);

/* Lecture validee (application ou bootloader). Faux si la boite est invalide. */
bool boot_mailbox_read(boot_mailbox_t *out);
#endif

#ifdef __cplusplus
}
#endif

#endif /* BOOT_MAILBOX_H */
