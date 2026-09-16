/**
 ******************************************************************************
 * @file           : netboot_protocol.h
 * @brief          : Sp3ctra Net Boot (SNB) -- protocole UDP du flasheur reseau
 *                   du bootloader. Contrat de fil, a recopier tel quel cote hote
 *                   (scripts/netboot/netboot.py l'implemente en Python).
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

#ifndef NETBOOT_PROTOCOL_H
#define NETBOOT_PROTOCOL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Transport : UDP, port 55152 (SLP occupe 55150/55151). Une requete par
 * datagramme, une reponse par requete, l'hote retransmet sur silence
 * (stop-and-wait). Tous les champs sont en petit-boutien. */
#define NB_PORT           55152u
#define NB_MAGIC          0x31424E53u /* "SNB1" */
#define NB_PROTO_VERSION  1u
#define NB_MAX_DATA       1024u       /* charge utile maximale de WRITE / DATA / LOG */

/* nb_hdr_t.flags */
#define NB_FLAG_RELEASE   0x01u       /* libere la session apres cette requete */

/* Zone de flash que le flasheur accepte d'effacer ou d'ecrire : toute la flash
 * sauf le secteur d'ou le bootloader s'execute (slot A 0x08000000 ou slot B
 * 0x080E0000, choisi par l'option byte BOOT_ADD0). Le journal OTA (secteur 1)
 * est accessible pour permettre sa remise a zero. */
#define NB_FLASH_START          0x08000000u
#define NB_FLASH_END            0x08200000u
#define NB_BL_SLOT_A            0x08000000u
#define NB_BL_SLOT_B            0x080E0000u

/* Requetes (hote -> appareil) */
enum {
    NB_DISCOVER = 0x01, /* broadcast ; ne lie pas de session                        */
    NB_ERASE    = 0x02, /* nb_range_t : addr aligne secteur, len arrondi au secteur */
    NB_WRITE    = 0x03, /* nb_write_t + donnees : addr et len multiples de 32       */
    NB_READ     = 0x04, /* nb_read_t                                                */
    NB_CRC      = 0x05, /* nb_range_t : CRC-32 (zlib) relu en place                 */
    NB_LOG      = 0x06, /* nb_log_req_t : lecture de l'anneau de logs               */
    NB_JOURNAL  = 0x07, /* nb_journal_t : ecrit un enregistrement du journal OTA    */
    NB_BOOT     = 0x08, /* nb_boot_t : ACK puis reset                               */
    NB_PING     = 0x09, /* garde la session                                         */
    NB_BOOTSEL  = 0x0A, /* nb_bootsel_t : programme BOOT_ADD0 vers l'autre slot BL   */
};

/* Reponses (appareil -> hote) */
enum {
    NB_INFO      = 0x81, /* nb_info_t                       */
    NB_ACK       = 0x82, /* nb_ack_t                        */
    NB_DATA      = 0x84, /* nb_write_t + donnees            */
    NB_CRC_REPLY = 0x85, /* nb_crc_reply_t                  */
    NB_LOG_REPLY = 0x86, /* nb_log_reply_t + texte          */
};

/* nb_ack_t.status */
enum {
    NB_OK           = 0,
    NB_E_ARG        = 1, /* argument invalide (alignement, longueur)            */
    NB_E_RANGE      = 2, /* adresse hors zone autorisee                         */
    NB_E_FLASH      = 3, /* le HAL a refuse l'operation ; code = adresse fautive */
    NB_E_BUSY       = 4, /* un autre hote tient la session (10 s de silence la libere ;
                          * le meme hote sur un autre port reprend la main)       */
    NB_E_VERIFY     = 5, /* relecture differente de ce qui a ete programme      */
    NB_E_NOT_ERASED = 6, /* mot cible non vierge et different des donnees       */
    NB_E_UNKNOWN    = 7, /* type de requete inconnu                             */
};

/* nb_info_t.reason : ce qui a fait entrer l'appareil en mode flasheur */
enum {
    NB_REASON_NONE    = 0,
    NB_REASON_MAILBOX = 1, /* demande de l'application (POST /netboot)             */
    NB_REASON_BUTTONS = 2, /* deux boutons exterieurs maintenus a la mise sous tension */
    NB_REASON_FAILED  = 3, /* journal OTA en etat FAILED : rien de bootable         */
};

typedef struct __attribute__((packed)) {
    uint32_t magic;   /* NB_MAGIC                                */
    uint8_t  type;    /* NB_*                                    */
    uint8_t  flags;   /* NB_FLAG_*                               */
    uint16_t seq;     /* choisi par l'hote, recopie dans la reponse */
} nb_hdr_t;

typedef struct __attribute__((packed)) {
    uint8_t  proto;          /* NB_PROTO_VERSION                          */
    uint8_t  state;          /* 0 libre, 1 session liee                    */
    uint16_t max_data;       /* NB_MAX_DATA                               */
    uint8_t  mac[6];
    uint8_t  ip[4];
    uint8_t  uid[12];        /* identifiant unique du MCU                  */
    char     name[16];       /* "Sp3ctra-XXXX"                             */
    char     bl_version[16]; /* BL_VERSION                                 */
    uint8_t  journal_phase;  /* ota_phase_t, 0 si journal vierge           */
    uint8_t  trial;
    uint8_t  rollback;
    uint8_t  pending;
    uint32_t reset_flags;    /* RCC_RSR au demarrage                       */
    uint32_t uptime_ms;      /* depuis l'entree en mode flasheur           */
    uint8_t  reason;         /* NB_REASON_*                                */
    uint8_t  boot_slot;      /* 0 : bootloader en slot A, 1 : en slot B    */
    uint8_t  reserved[2];
} nb_info_t;

typedef struct __attribute__((packed)) {
    uint32_t addr;
    uint32_t len;
} nb_range_t;

typedef struct __attribute__((packed)) {
    uint32_t addr;
    uint16_t len;
    uint16_t reserved;
    /* uint8_t data[len] */
} nb_write_t;

typedef struct __attribute__((packed)) {
    uint32_t addr;
    uint16_t len;
    uint16_t reserved;
} nb_read_t;

typedef struct __attribute__((packed)) {
    uint8_t  status;     /* NB_OK ou NB_E_*                                   */
    uint8_t  reserved[3];
    uint32_t code;       /* adresse fautive, ou type inconnu                  */
    uint32_t elapsed_ms; /* duree de l'operation (ERASE)                      */
} nb_ack_t;

typedef struct __attribute__((packed)) {
    uint32_t addr;
    uint32_t len;
    uint32_t crc;
} nb_crc_reply_t;

typedef struct __attribute__((packed)) {
    uint32_t since;      /* position demandee (voir log_ring_read)            */
    uint16_t max;        /* <= NB_MAX_DATA                                    */
    uint16_t src;        /* 0 = anneau CM7 (bootloader + app), 1 = anneau CM4 */
} nb_log_req_t;

typedef struct __attribute__((packed)) {
    uint32_t next;       /* position a redemander                             */
    uint16_t len;
    uint16_t reserved;
    /* char text[len] */
} nb_log_reply_t;

typedef struct __attribute__((packed)) {
    uint8_t phase;       /* ota_phase_t ; 0 = effacer le journal              */
    uint8_t trial;
    uint8_t rollback;
    uint8_t pending;
} nb_journal_t;

typedef struct __attribute__((packed)) {
    uint8_t mode;        /* 0 : reset systeme (demarrage normal)              */
    uint8_t reserved[3];
} nb_boot_t;

typedef struct __attribute__((packed)) {
    uint32_t addr;       /* NB_BL_SLOT_A ou NB_BL_SLOT_B : prend effet au reset */
} nb_bootsel_t;

#ifdef __cplusplus
}
#endif

#endif /* NETBOOT_PROTOCOL_H */
