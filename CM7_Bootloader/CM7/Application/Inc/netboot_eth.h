/**
 ******************************************************************************
 * @file           : netboot_eth.h
 * @brief          : Glue HAL Ethernet + PHY LAN8742 pour le flasheur reseau,
 *                   en scrutation, sans OS ni interruption.
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

#ifndef NETBOOT_ETH_H
#define NETBOOT_ETH_H

#include <stdbool.h>
#include <stdint.h>

/* Horloges, broches RMII, MAC, PHY. Ne demarre pas la reception : voir
 * nb_eth_service(). Le D-cache doit etre coupe avant l'appel. */
bool     nb_eth_init(const uint8_t mac[6]);

/* A appeler periodiquement (~200 ms) : suit l'etat du lien, demarre ou arrete
 * le MAC en consequence. Retourne 1 quand le lien est etabli et le MAC actif. */
int      nb_eth_service(void);

/* Copie la prochaine trame recue dans out (0 si aucune). */
uint16_t nb_eth_poll(uint8_t *out, uint16_t max);

/* Emission bloquante d'une trame complete sans FCS (le MAC l'ajoute). */
bool     nb_eth_send(const uint8_t *frame, uint16_t len);

#endif /* NETBOOT_ETH_H */
