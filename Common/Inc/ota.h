/**
 ******************************************************************************
 * @file           : ota.h
 * @brief          : Journal de mise a jour partage entre le bootloader et
 *                   l'application, et CRC-32 logiciel commun.
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

#ifndef __OTA_H__
#define __OTA_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

/**************************************************************************************/
/*******************                    CRC-32                      *******************/
/**************************************************************************************/

/* Polynome 0x04C11DB7, entree et sortie reflechies, init 0xFFFFFFFF, XOR final.
 * Autrement dit exactement zlib.crc32() cote Python.
 *
 * Implemente en logiciel a dessein : le peripherique CRC est configure
 * differemment dans les deux projets (INVERSION_BYTE + FORMAT_BYTES dans le
 * bootloader, INVERSION_NONE + FORMAT_WORDS dans l'application), et rien
 * n'empeche un autre module de le reconfigurer entre deux appels. Le CRC d'un
 * paquet ne doit dependre d'aucun etat partage. */
#define OTA_CRC32_INIT 0u

uint32_t ota_crc32(uint32_t crc, const void *data, uint32_t length);

/**************************************************************************************/
/*******************                Journal OTA                     *******************/
/**************************************************************************************/

/* Phase courante de la sequence de mise a jour.
 *
 * Aucune valeur ne vaut 0x00 ni 0xFF : un octet de flash efface (0xFF) ou une
 * page a zero ne doit jamais pouvoir se faire passer pour une phase valide.
 * C'est precisement le defaut de l'ancien FW_UPDATE_DONE = 0xFFFFFFFF. */
typedef enum {
    OTA_PHASE_IDLE     = 0x11, /* rien en attente : demarrer l'application       */
    OTA_PHASE_PENDING  = 0x22, /* un paquet attend dans /firmware : l'appliquer  */
    OTA_PHASE_TRIAL    = 0x33, /* image fraichement flashee, a l'essai           */
    OTA_PHASE_ROLLBACK = 0x44, /* essai echoue : restaurer la sauvegarde         */
    OTA_PHASE_FAILED   = 0x55, /* restauration impossible : arret des tentatives */
} ota_phase_t;

#define OTA_RECORD_MAGIC   0x4F335053u /* "SP3O" en little-endian */
#define OTA_RECORD_VERSION 1u
#define OTA_RECORD_SIZE    32u /* = FLASH_TYPEPROGRAM_FLASHWORD sur STM32H7 */

/* Nombre de demarrages accordes a une image en essai avant de la declarer
 * mauvaise, puis nombre de restaurations tentees avant d'abandonner. */
#define OTA_MAX_TRIAL_BOOTS       3u
#define OTA_MAX_ROLLBACK_ATTEMPTS 3u
#define OTA_MAX_PENDING_ATTEMPTS  3u

typedef struct __attribute__((packed, aligned(4))) {
    uint32_t magic;             /* OTA_RECORD_MAGIC                             */
    uint32_t seq;               /* monotone : le plus eleve valide fait foi      */
    uint8_t version;            /* OTA_RECORD_VERSION                            */
    uint8_t phase;              /* ota_phase_t                                   */
    uint8_t trial_attempts;     /* sauts deja effectues dans l'image a l'essai   */
    uint8_t rollback_attempts;  /* restaurations deja tentees                    */
    uint8_t pending_attempts;   /* applications du paquet deja tentees           */
    uint8_t reserved[15];
    uint32_t crc32;             /* sur les 28 octets qui precedent               */
} ota_record_t;

_Static_assert(sizeof(ota_record_t) == OTA_RECORD_SIZE, "ota_record_t doit faire 32 octets");

/* Renseigne magic / version / crc32. seq est laisse a l'appelant. */
void ota_record_seal(ota_record_t *rec);

/* Vrai si l'enregistrement porte le bon magic, la bonne version et un CRC juste. */
bool ota_record_valid(const ota_record_t *rec);

const char *ota_phase_str(uint8_t phase);

#ifdef CORE_CM7

/* Journal append-only occupant le secteur FLASH_PERSISTENT_DATA_ADDRESS.
 *
 * Un enregistrement de 32 octets est ajoute a la suite du precedent sans
 * effacer quoi que ce soit ; le secteur n'est efface que lorsqu'il est plein,
 * et le nouvel enregistrement est alors ecrit immediatement apres. Une coupure
 * secteur pendant l'ecriture laisse un enregistrement au CRC faux : c'est le
 * precedent qui continue de faire foi. L'atomicite est structurelle, sans
 * secteur miroir.
 *
 * Effet de bord bienvenu : le secteur n'est plus efface a chaque transition
 * (l'ancien code l'effacait trois fois par mise a jour) mais toutes les
 * ~1000 mises a jour. */

/* Renseigne *out avec le dernier enregistrement valide.
 * Retourne false si le journal est vide ou entierement illisible : l'appelant
 * doit alors se comporter comme en OTA_PHASE_IDLE. */
bool ota_journal_read(ota_record_t *out);

/* Ajoute un enregistrement portant cette phase et ces compteurs.
 * seq, magic, version et crc32 sont calcules ici. */
bool ota_journal_write(ota_phase_t phase, uint8_t trial_attempts,
                       uint8_t rollback_attempts, uint8_t pending_attempts);

/* Raccourci : change de phase en conservant les compteurs courants. */
bool ota_journal_set_phase(ota_phase_t phase);

/* Retour a l'etat de repos : phase IDLE, tous les compteurs remis a zero. */
bool ota_journal_clear(void);

#endif /* CORE_CM7 */

#ifdef __cplusplus
}
#endif

#endif /* __OTA_H__ */
