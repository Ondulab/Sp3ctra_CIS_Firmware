/**
 ******************************************************************************
 * @file           : ota_boot.h
 * @brief          : Sequenceur de demarrage du bootloader : decide a chaque
 *                   reset s'il faut appliquer un paquet, restaurer la
 *                   sauvegarde, ou sauter dans l'application.
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

#ifndef __OTA_BOOT_H__
#define __OTA_BOOT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

/* Chien de garde ------------------------------------------------------------*/

/* Demarre l'IWDG1 (~10 s). Irreversible jusqu'au prochain reset : tout code
 * s'executant ensuite doit appeler otaBoot_refreshWatchdog() regulierement. */
void otaBoot_armWatchdog(void);

/* Recharge le compteur. Sans effet si le chien n'a pas ete arme. */
void otaBoot_refreshWatchdog(void);

/* Trace la cause du dernier reset puis efface les drapeaux. */
void otaBoot_logResetCause(void);

/* Vrai si le dernier reset a ete provoque par l'IWDG. */
bool otaBoot_lastResetWasWatchdog(void);

/* Sequencement -------------------------------------------------------------*/

/* Etape precoce, juste apres SystemClock_Config et avant l'initialisation des
 * peripheriques. Saute dans l'application et ne retourne pas quand il n'y a
 * rien a faire ; retourne quand le bootloader doit poursuivre (paquet a
 * appliquer, restauration a mener, echec a afficher). */
void otaBoot_earlyStage(void);

/* Etape tardive, apres le montage de la NOR et l'initialisation de l'ecran.
 * Ne retourne jamais : elle reboote ou saute dans l'application. */
void otaBoot_lateStage(void);

/* Saut inconditionnel dans une image flashee. Ne retourne pas. */
void otaBoot_jumpToFirmware(uint32_t flashStartAddr);

/* Reboot apres un court delai, pour laisser le temps de lire l'ecran. */
void otaBoot_reboot(void);

#ifdef __cplusplus
}
#endif

#endif /* __OTA_BOOT_H__ */
