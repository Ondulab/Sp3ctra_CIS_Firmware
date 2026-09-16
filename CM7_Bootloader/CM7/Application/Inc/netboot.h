/**
 ******************************************************************************
 * @file           : netboot.h
 * @brief          : Mode flasheur reseau du bootloader (Sp3ctra Net Boot).
 *                   Remplace le ST-Link : efface, ecrit et verifie n'importe
 *                   quelle zone de flash hors du bootloader lui-meme, par UDP.
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

#ifndef NETBOOT_H
#define NETBOOT_H

#include <stdbool.h>

typedef enum {
    NETBOOT_REASON_NONE    = 0,
    NETBOOT_REASON_MAILBOX = 1, /* demande de l'application (POST /netboot)                  */
    NETBOOT_REASON_BUTTONS = 2, /* deux boutons exterieurs maintenus a la mise sous tension  */
    NETBOOT_REASON_FAILED  = 3, /* journal OTA en FAILED : rien de sain a demarrer            */
} netboot_reason_t;

/* A appeler dans l'etape precoce, apres otaBoot_logResetCause() : consomme la
 * boite aux lettres et lit les boutons. */
bool netboot_entryRequested(netboot_reason_t *reason);

/* Boucle du flasheur. Ne retourne jamais : se termine par un reset systeme. */
void netboot_run(netboot_reason_t reason) __attribute__((noreturn));

#endif /* NETBOOT_H */
