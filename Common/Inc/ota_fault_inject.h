/**
 ******************************************************************************
 * @file           : ota_fault_inject.h
 * @brief          : Injection de fautes pour la validation du rollback.
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

#ifndef __OTA_FAULT_INJECT_H__
#define __OTA_FAULT_INJECT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* Familles de panne
 * -----------------
 * Chacune verifie un filet different. Une image saine doit valoir 0 : c'est la
 * seule valeur autorisee pour une version diffusee, et scripts/ota/make_package.py
 * refuse d'empaqueter autre chose sans --allow-fault.
 *
 *  0  aucune faute
 *  1  HardFault des l'entree de main(), avant toute initialisation
 *     -> le gestionnaire de faute boucle, c'est le chien de garde qui reprend
 *        la main, et le compteur de tentatives finit par declencher le rollback
 *  2  boucle infinie des l'entree de main()
 *     -> teste le chien de garde seul, sans exception
 *  3  demarrage complet mais serveur HTTP en echec
 *     -> teste le controle de sante : l'image tourne mais a perdu le seul canal
 *        permettant d'en envoyer une autre, elle ne doit pas etre confirmee
 *  4  demarrage complet puis HardFault avant la fin du delai de confirmation
 *     -> teste la fenetre OTA_CONFIRM_SETTLE_MS
 *  5  tache de garde absente : le chien de garde n'est jamais recharge
 *     -> teste le chien de garde en regime etabli, image par ailleurs saine
 */
#define OTA_FAULT_NONE           0
#define OTA_FAULT_HARDFAULT_BOOT 1
#define OTA_FAULT_HANG_BOOT      2
#define OTA_FAULT_NO_HTTP        3
#define OTA_FAULT_LATE_CRASH     4
#define OTA_FAULT_NO_GUARD       5

/* Faute active. Cette ligne est reecrite par scripts/ota/build_broken_fw.sh,
 * qui la restaure a 0 apres la construction du paquet empoisonne.
 * NE JAMAIS COMMITTER UNE AUTRE VALEUR. */
#define SP3CTRA_OTA_FAULT OTA_FAULT_NONE

/* Delai, apres satisfaction des criteres de sante, avant la panne tardive de
 * la faute 4. Doit rester inferieur a OTA_CONFIRM_SETTLE_MS pour que l'image
 * tombe avant d'avoir ete confirmee. */
#define OTA_FAULT_LATE_CRASH_MS 15000u

/* Coupure secteur simulee dans le bootloader, au milieu de l'etape indiquee de
 * update_processPackageFile() :
 *
 *   1 CRC   2 sauvegarde CM7   3 sauvegarde CM4   4 effacement CM7
 *   5 effacement CM4   6 flash CM7   7 flash CM4   8 donnees externes
 *
 * 0 desactive le mecanisme. La coupure ne se produit qu'a la PREMIERE tentative
 * d'application (pending_attempts == 0), sinon chaque reprise se couperait au
 * meme endroit et l'on testerait le compteur de tentatives plutot que la
 * reprise elle-meme.
 *
 * Reecrit par scripts/ota/build_abort_bl.sh, qui le restaure a 0 ensuite.
 * NE JAMAIS COMMITTER UNE AUTRE VALEUR. */
#define SP3CTRA_OTA_ABORT_AT_STEP 0

#if SP3CTRA_OTA_ABORT_AT_STEP != 0
#warning "SP3CTRA_OTA_ABORT_AT_STEP est actif : bootloader de test, ne pas diffuser"
#endif

#if SP3CTRA_OTA_FAULT != OTA_FAULT_NONE
#warning "SP3CTRA_OTA_FAULT est actif : image de test, ne pas diffuser"
#endif

static inline void otaFault_hardFault(void)
{
    __asm volatile("udf #0");
}

static inline void otaFault_hang(void)
{
    for (;;)
    {
        __asm volatile("nop");
    }
}

/* Points d'injection ------------------------------------------------------- */

#if SP3CTRA_OTA_FAULT == OTA_FAULT_HARDFAULT_BOOT
#define OTA_FAULT_EARLY_BOOT() otaFault_hardFault()
#elif SP3CTRA_OTA_FAULT == OTA_FAULT_HANG_BOOT
#define OTA_FAULT_EARLY_BOOT() otaFault_hang()
#else
#define OTA_FAULT_EARLY_BOOT() ((void)0)
#endif

#if SP3CTRA_OTA_FAULT == OTA_FAULT_NO_HTTP
#define OTA_FAULT_HTTP_INIT_FAILS() (1)
#else
#define OTA_FAULT_HTTP_INIT_FAILS() (0)
#endif

#if SP3CTRA_OTA_FAULT == OTA_FAULT_LATE_CRASH
#define OTA_FAULT_LATE_CRASH_ENABLED() (1)
#else
#define OTA_FAULT_LATE_CRASH_ENABLED() (0)
#endif

#if SP3CTRA_OTA_FAULT == OTA_FAULT_NO_GUARD
#define OTA_FAULT_SKIP_GUARD_TASK() (1)
#else
#define OTA_FAULT_SKIP_GUARD_TASK() (0)
#endif

#ifdef __cplusplus
}
#endif

#endif /* __OTA_FAULT_INJECT_H__ */
