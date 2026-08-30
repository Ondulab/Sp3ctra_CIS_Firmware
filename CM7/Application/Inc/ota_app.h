/**
 ******************************************************************************
 * @file           : ota_app.h
 * @brief          : Cote application de la mise a jour : chien de garde,
 *                   controle de sante et confirmation de l'image a l'essai.
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

#ifndef __OTA_APP_H__
#define __OTA_APP_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

/* Etapes d'initialisation prises en compte par le controle de sante ---------*/

#define OTA_HEALTH_CONFIG  (1u << 0) /* configuration lue depuis la NOR        */
#define OTA_HEALTH_NETWORK (1u << 1) /* lien Ethernet etabli                   */
#define OTA_HEALTH_HTTP    (1u << 2) /* serveur HTTP en ecoute                 */
#define OTA_HEALTH_LINK    (1u << 3) /* serveur Sp3ctra Link demarre           */
#define OTA_HEALTH_CIS     (1u << 4) /* capteur d'image operationnel           */

/* Ce qu'une image doit prouver pour etre confirmee.
 *
 * Volontairement limite a la configuration et au serveur HTTP : ce dernier est
 * le seul canal permettant d'envoyer une nouvelle version, donc une image qui
 * le casse doit etre annulee. Le lien reseau en est exclu a dessein -- un cable
 * debranche pendant l'essai provoquerait sinon une annulation a tort. */
#define OTA_HEALTH_REQUIRED (OTA_HEALTH_CONFIG | OTA_HEALTH_HTTP)

/* Duree pendant laquelle l'image doit tenir, une fois tous les criteres
 * satisfaits, avant d'etre confirmee. Fixe la frontiere entre une panne
 * precoce (rattrapee) et une panne tardive (non rattrapee). */
#define OTA_CONFIRM_SETTLE_MS 30000u

/* API ----------------------------------------------------------------------*/

/* Recharge le chien de garde. Sans effet s'il n'a pas ete arme par le
 * bootloader (cas d'une image deja confirmee). */
void otaApp_refreshWatchdog(void);

/* Cree la tache qui recharge le chien de garde et confirme l'image a l'essai.
 * A appeler des la creation des taches, avant toute initialisation longue. */
bool otaApp_init(void);

/* Signale le resultat d'une etape d'initialisation. */
void otaApp_reportHealth(uint32_t flag, bool ok);

/* Vrai si le bootloader nous a demarres a l'essai. */
bool otaApp_isUnderTrial(void);

/* Verifie un paquet recu (en-tete + CRC) et, s'il est valide, demande son
 * application au prochain demarrage. Retourne NULL si tout va bien, sinon un
 * libelle court decrivant le refus, destine a la reponse HTTP. */
const char *otaApp_acceptPackage(const char *packageFilePath);

#ifdef __cplusplus
}
#endif

#endif /* __OTA_APP_H__ */
