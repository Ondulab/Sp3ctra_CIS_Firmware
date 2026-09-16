/**
 ******************************************************************************
 * @file           : log_ring.h
 * @brief          : Journal circulaire en RAM retenue (D3 SRAM4), ecrit par
 *                   printf() sur le bootloader, le CM7 et le CM4 ; lu par le
 *                   reseau (HTTP /log sur l'application, LOG sur le flasheur).
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

#ifndef LOG_RING_H
#define LOG_RING_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Moitie haute de la SRAM4 (domaine D3). Les linker scripts des trois images
 * limitent RAM_D3/RAM_D4 a 32 Ko pour que l'editeur de liens n'y place rien.
 * Le contenu survit a tous les resets sauf la coupure d'alimentation, les deux
 * coeurs y accedent, et la zone n'est jamais touchee par le code de demarrage.
 *
 *   0x38008000  64 o     boite aux lettres de demarrage (boot_mailbox.h)
 *   0x38008040  32 o     en-tete de l'anneau CM7 (bootloader + application)
 *   0x38008060  20 Ko    donnees de l'anneau CM7
 *   0x3800D060  32 o     en-tete de l'anneau CM4
 *   0x3800D080  11,9 Ko  donnees de l'anneau CM4
 *
 * Chaque bloc est aligne sur une ligne de cache (32 o) : le CM7 nettoie ses
 * propres ecritures et invalide avant de lire l'anneau du CM4. */
#define LOG_REGION_BASE      0x38008000u
#define LOG_REGION_END       0x38010000u

#define BOOT_MAILBOX_ADDR    LOG_REGION_BASE
#define BOOT_MAILBOX_SIZE    64u

#define LOG_RING_HDR_SIZE    32u
#define LOG_RING_CM7_ADDR    (BOOT_MAILBOX_ADDR + BOOT_MAILBOX_SIZE)
#define LOG_RING_CM7_SIZE    (20u * 1024u)
#define LOG_RING_CM4_ADDR    (LOG_RING_CM7_ADDR + LOG_RING_HDR_SIZE + LOG_RING_CM7_SIZE)
#define LOG_RING_CM4_SIZE    (LOG_REGION_END - LOG_RING_CM4_ADDR - LOG_RING_HDR_SIZE)

/* Recopie du printf sur l'UART (comportement historique). A 0, printf ne
 * coute plus qu'une copie en RAM. */
#ifndef LOG_RING_UART_ECHO
#define LOG_RING_UART_ECHO 1
#endif

typedef struct {
    uint32_t magic;             /* LOG_RING_MAGIC                                    */
    uint32_t size;              /* capacite de la zone de donnees, en octets         */
    volatile uint32_t head;     /* octets ecrits depuis le formatage (monotone)      */
    uint32_t boots;             /* initialisations ayant conserve le contenu         */
    uint8_t  tag;               /* derniere image a s'etre initialisee : 'B' '7' '4' */
    uint8_t  reserved[11];
    uint32_t check;             /* scelle magic/size                                 */
} log_ring_hdr_t;

typedef enum {
    LOG_SRC_CM7 = 0,    /* bootloader et application CM7, dans le meme anneau */
    LOG_SRC_CM4 = 1
} log_src_t;

/* A appeler une fois, avant le premier printf. Conserve le contenu existant
 * quand l'en-tete est valide (reset a chaud), formate sinon (mise sous tension). */
void     log_ring_init(void);

/* Ecriture brute (c'est ce que _write() appelle). Prefixe chaque ligne de
 * "[T ssss.mmm] " ou T identifie l'image. Thread-safe (section critique). */
void     log_ring_write(const char *data, size_t len);

bool     log_ring_valid(log_src_t src);
uint32_t log_ring_head(log_src_t src);

/* Copie dans out les octets [since, head) d'un anneau, au plus max. Si since
 * est trop ancien (ecrase) la lecture reprend au plus vieil octet disponible ;
 * s'il est dans le futur (compteur remis a zero par une coupure secteur) elle
 * reprend a head. *next recoit la position a redemander. */
uint32_t log_ring_read(log_src_t src, uint32_t since, void *out, uint32_t max, uint32_t *next);

#ifdef __cplusplus
}
#endif

#endif /* LOG_RING_H */
