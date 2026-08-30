/**
 ******************************************************************************
 * @file           : ota_crc32.c
 * @brief          : CRC-32 logiciel partage par le bootloader, l'application
 *                   et le generateur de paquets.
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

#include "ota.h"

/* Table par quartet : 64 octets de rodata au lieu du kilo-octet de la table par
 * octet. Le bootloader n'a que 19 Ko de marge et le surcout en temps est
 * negligeable devant la lecture QSPI qui alimente le calcul. */
static const uint32_t ota_crc32_table[16] = {
    0x00000000u, 0x1DB71064u, 0x3B6E20C8u, 0x26D930ACu,
    0x76DC4190u, 0x6B6B51F4u, 0x4DB26158u, 0x5005713Cu,
    0xEDB88320u, 0xF00F9344u, 0xD6D6A3E8u, 0xCB61B38Cu,
    0x9B64C2B0u, 0x86D3D2D4u, 0xA00AE278u, 0xBDBDF21Cu,
};

/**
 * @brief  CRC-32 reflechi, compatible zlib.crc32().
 * @param  crc    OTA_CRC32_INIT au premier appel, la valeur precedente ensuite.
 * @param  data   bloc a accumuler.
 * @param  length taille du bloc en octets.
 * @retval CRC courant, directement comparable a la valeur produite par Python.
 */
uint32_t ota_crc32(uint32_t crc, const void *data, uint32_t length)
{
    const uint8_t *p = (const uint8_t *)data;

    crc = ~crc;
    while (length--)
    {
        crc ^= *p++;
        crc = (crc >> 4) ^ ota_crc32_table[crc & 0x0Fu];
        crc = (crc >> 4) ^ ota_crc32_table[crc & 0x0Fu];
    }

    return ~crc;
}

/**
 * @brief  Renseigne magic, version et crc32 d'un enregistrement de journal.
 */
void ota_record_seal(ota_record_t *rec)
{
    rec->magic = OTA_RECORD_MAGIC;
    rec->version = OTA_RECORD_VERSION;
    rec->crc32 = ota_crc32(OTA_CRC32_INIT, rec, OTA_RECORD_SIZE - sizeof(uint32_t));
}

/**
 * @brief  Vrai si l'enregistrement est lisible et intact.
 */
bool ota_record_valid(const ota_record_t *rec)
{
    if (rec->magic != OTA_RECORD_MAGIC || rec->version != OTA_RECORD_VERSION)
    {
        return false;
    }
    return rec->crc32 == ota_crc32(OTA_CRC32_INIT, rec, OTA_RECORD_SIZE - sizeof(uint32_t));
}

/**
 * @brief  Libelle court d'une phase, pour les traces UART.
 */
const char *ota_phase_str(uint8_t phase)
{
    switch (phase)
    {
    case OTA_PHASE_IDLE:     return "IDLE";
    case OTA_PHASE_PENDING:  return "PENDING";
    case OTA_PHASE_TRIAL:    return "TRIAL";
    case OTA_PHASE_ROLLBACK: return "ROLLBACK";
    case OTA_PHASE_FAILED:   return "FAILED";
    default:                 return "?";
    }
}
