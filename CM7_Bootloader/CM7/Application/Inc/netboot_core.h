/**
 ******************************************************************************
 * @file           : netboot_core.h
 * @brief          : Coeur portable du flasheur reseau : pile ARP/IPv4/ICMP/UDP
 *                   minimale et protocole SNB, sans dependance au HAL.
 *                   Le portage (netboot.c sur cible, netboot_sim.c sur l'hote)
 *                   fournit les fonctions nbp_*.
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

#ifndef NETBOOT_CORE_H
#define NETBOOT_CORE_H

#include <stdbool.h>
#include <stdint.h>

#include "netboot_protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t mac[6];
    uint8_t ip[4];
    uint8_t netmask[4];
} nb_netcfg_t;

typedef struct {
    uint8_t  uid[12];
    char     name[16];
    char     bl_version[16];
    uint8_t  journal_phase;
    uint8_t  trial;
    uint8_t  rollback;
    uint8_t  pending;
    uint32_t reset_flags;
    uint8_t  reason;
    uint8_t  boot_slot;
} nb_devinfo_t;

/* ---- Interface de portage ------------------------------------------------ */

/* Emet une trame Ethernet complete (sans FCS). Bloquant. */
void     nbp_send_frame(const uint8_t *frame, uint16_t len);
uint32_t nbp_tick_ms(void);
void     nbp_get_info(nb_devinfo_t *info);

/* Retournent NB_OK ou un code NB_E_*. fail_addr recoit l'adresse fautive. */
int      nbp_flash_erase(uint32_t addr, uint32_t len, uint32_t *fail_addr);
int      nbp_flash_write(uint32_t addr, const uint8_t *data, uint32_t len, uint32_t *fail_addr);
int      nbp_mem_read(uint32_t addr, uint8_t *out, uint32_t len);
int      nbp_crc32(uint32_t addr, uint32_t len, uint32_t *crc);
uint32_t nbp_log_read(uint16_t src, uint32_t since, uint8_t *out, uint16_t max, uint32_t *next);
int      nbp_journal_write(uint8_t phase, uint8_t trial, uint8_t rollback, uint8_t pending);
int      nbp_boot_select(uint32_t addr);
void     nbp_reboot(void);

/* ---- Coeur --------------------------------------------------------------- */

void     nb_core_init(const nb_netcfg_t *cfg);
void     nb_core_input(const uint8_t *frame, uint16_t len);
void     nb_core_poll(void);
bool     nb_core_client_bound(void);
uint32_t nb_core_last_activity(void);
uint32_t nb_core_requests(void);

#ifdef __cplusplus
}
#endif

#endif /* NETBOOT_CORE_H */
