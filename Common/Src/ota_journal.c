/**
 ******************************************************************************
 * @file           : ota_journal.c
 * @brief          : Journal append-only de l'etat de mise a jour, en flash
 *                   interne. Partage par le bootloader et l'application.
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

#ifdef CORE_CM7

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"

#include "boot_config.h"
#include "ota.h"
#include "stm32_flash.h"

#include <stdio.h>
#include <string.h>

/* Private define ------------------------------------------------------------*/
#define OTA_JOURNAL_BASE  ((uint32_t)FLASH_PERSISTENT_DATA_ADDRESS)
#define OTA_JOURNAL_SLOTS ((uint32_t)(FLASH_SECTOR_SIZE / OTA_RECORD_SIZE))

/* Private functions ---------------------------------------------------------*/

static const ota_record_t *ota_journal_slot(uint32_t index)
{
    return (const ota_record_t *)(OTA_JOURNAL_BASE + index * OTA_RECORD_SIZE);
}

/**
 * @brief  Vrai si le mot de flash n'a jamais ete programme depuis l'effacement.
 */
static bool ota_journal_slotErased(const ota_record_t *rec)
{
    const uint32_t *word = (const uint32_t *)rec;

    for (uint32_t i = 0; i < OTA_RECORD_SIZE / sizeof(uint32_t); i++)
    {
        if (word[i] != 0xFFFFFFFFu)
        {
            return false;
        }
    }
    return true;
}

/**
 * @brief  Efface le secteur du journal.
 */
static bool ota_journal_eraseSector(void)
{
    FLASH_EraseInitTypeDef eraseInit;
    uint32_t sectorError = 0;

    if (HAL_FLASH_Unlock() != HAL_OK)
    {
        return false;
    }

    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ALL_ERRORS_BANK1);

    eraseInit.TypeErase = FLASH_TYPEERASE_SECTORS;
    eraseInit.Sector = stm32Flash_getSector(OTA_JOURNAL_BASE);
    eraseInit.NbSectors = 1;
    eraseInit.Banks = FLASH_BANK_1;
    eraseInit.VoltageRange = FLASH_VOLTAGE_RANGE_3;

    HAL_StatusTypeDef status = HAL_FLASHEx_Erase(&eraseInit, &sectorError);
    HAL_FLASH_Lock();

    if (status == HAL_OK)
    {
        SCB_InvalidateDCache_by_Addr((uint32_t *)OTA_JOURNAL_BASE, (int32_t)FLASH_SECTOR_SIZE);
    }
    return status == HAL_OK;
}

/**
 * @brief  Programme un enregistrement dans un emplacement efface.
 */
static bool ota_journal_program(uint32_t index, const ota_record_t *rec)
{
    const uint32_t address = OTA_JOURNAL_BASE + index * OTA_RECORD_SIZE;

    if (HAL_FLASH_Unlock() != HAL_OK)
    {
        return false;
    }

    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ALL_ERRORS_BANK1);

    HAL_StatusTypeDef status =
        HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD, address, (uint32_t)rec);

    HAL_FLASH_Lock();

    if (status != HAL_OK)
    {
        printf("OTA journal: program failed at 0x%08lX (err 0x%08lX)\n",
               (unsigned long)address, (unsigned long)HAL_FLASH_GetError());
        return false;
    }

    /* La zone vient d'etre modifiee sous la D-cache : sans invalidation, une
     * relecture dans le meme boot renverrait l'ancien contenu. */
    SCB_InvalidateDCache_by_Addr((uint32_t *)address, (int32_t)OTA_RECORD_SIZE);
    return true;
}

/* Exported functions --------------------------------------------------------*/

/**
 * @brief  Renseigne *out avec le dernier enregistrement valide du journal.
 * @retval false si le journal est vide ou entierement illisible.
 */
bool ota_journal_read(ota_record_t *out)
{
    const ota_record_t *best = NULL;

    for (uint32_t i = 0; i < OTA_JOURNAL_SLOTS; i++)
    {
        const ota_record_t *rec = ota_journal_slot(i);

        /* On ecrit toujours a la suite : le premier emplacement vierge marque
         * la fin du journal. */
        if (ota_journal_slotErased(rec))
        {
            break;
        }

        if (ota_record_valid(rec) && (best == NULL || rec->seq >= best->seq))
        {
            best = rec;
        }
    }

    if (best == NULL)
    {
        return false;
    }

    memcpy(out, best, sizeof(*out));
    return true;
}

/**
 * @brief  Ajoute un enregistrement au journal.
 * @note   Le secteur n'est efface que lorsqu'il est plein ; l'enregistrement
 *         est alors ecrit dans le premier emplacement. Une coupure secteur
 *         pendant l'ecriture laisse un enregistrement au CRC faux, et c'est le
 *         precedent qui continue de faire foi.
 */
bool ota_journal_write(ota_phase_t phase, uint8_t trial_attempts,
                       uint8_t rollback_attempts, uint8_t pending_attempts)
{
    ota_record_t previous;
    ota_record_t record;
    uint32_t index = OTA_JOURNAL_SLOTS;

    const bool hasPrevious = ota_journal_read(&previous);

    /* Premier emplacement vierge. */
    for (uint32_t i = 0; i < OTA_JOURNAL_SLOTS; i++)
    {
        if (ota_journal_slotErased(ota_journal_slot(i)))
        {
            index = i;
            break;
        }
    }

    if (index == OTA_JOURNAL_SLOTS)
    {
        printf("OTA journal: full, erasing\n");
        if (!ota_journal_eraseSector())
        {
            return false;
        }
        index = 0;
    }

    memset(&record, 0, sizeof(record));
    record.phase = (uint8_t)phase;
    record.trial_attempts = trial_attempts;
    record.rollback_attempts = rollback_attempts;
    record.pending_attempts = pending_attempts;
    record.seq = hasPrevious ? previous.seq + 1u : 1u;
    ota_record_seal(&record);

    if (!ota_journal_program(index, &record))
    {
        return false;
    }

    printf("OTA journal: %s (trial %u, rollback %u, pending %u, seq %lu, slot %lu)\n",
           ota_phase_str(record.phase), (unsigned)trial_attempts,
           (unsigned)rollback_attempts, (unsigned)pending_attempts,
           (unsigned long)record.seq, (unsigned long)index);
    return true;
}

/**
 * @brief  Change de phase en conservant les compteurs courants.
 */
bool ota_journal_set_phase(ota_phase_t phase)
{
    ota_record_t current;

    if (!ota_journal_read(&current))
    {
        return ota_journal_write(phase, 0, 0, 0);
    }
    return ota_journal_write(phase, current.trial_attempts, current.rollback_attempts,
                             current.pending_attempts);
}

/**
 * @brief  Retour a l'etat de repos, tous les compteurs remis a zero.
 */
bool ota_journal_clear(void)
{
    return ota_journal_write(OTA_PHASE_IDLE, 0, 0, 0);
}

#endif /* CORE_CM7 */
