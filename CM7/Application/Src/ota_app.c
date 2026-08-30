/**
 ******************************************************************************
 * @file           : ota_app.c
 * @brief          : Cote application de la mise a jour.
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

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os2.h"

#include "boot_config.h"
#include "ff.h"
#include "ota.h"
#include "ota_app.h"
#include "ota_fault_inject.h"

#include <stdio.h>
#include <string.h>

/* Private define ------------------------------------------------------------*/
#define OTA_WATCHDOG_PERIOD_MS 500u
#define OTA_IWDG_KEY_RELOAD    0x0000AAAAu

#define OTA_PACKAGE_HEADER_SIZE 24u
#define OTA_PACKAGE_MAGIC       "BOOT"
#define OTA_VERIFY_CHUNK        2048u

/* Private variables ---------------------------------------------------------*/
static volatile uint32_t otaApp_health;
static bool otaApp_underTrial;
static bool otaApp_confirmed;

static osThreadId_t otaApp_taskHandle;
static const osThreadAttr_t otaApp_taskAttributes = {
    .name = "otaGuard",
    .stack_size = 1024 * 4,
    /* Au-dessus des taches applicatives : le chien de garde doit continuer a
     * etre recharge meme si une tache de traitement sature le processeur, sans
     * quoi une simple surcharge passerait pour un firmware defaillant.
     *
     * Contrepartie assumee : un blocage cantonne a une tache de priorite
     * inferieure ne coupera pas la machine, puisque cette tache-ci continue de
     * tourner. Le chien de garde couvre donc les plantages, les fautes
     * materielles et la mort de l'ordonnanceur, pas les interblocages
     * applicatifs -- ceux-la demanderaient un battement par tache, hors du
     * perimetre de l'etape 0. La priorite haute est le choix prudent : elle ne
     * produit aucune annulation a tort.
     *
     * osPriorityRealtime vaut 48, sous configMAX_PRIORITIES (56). */
    .priority = (osPriority_t)osPriorityRealtime,
};

/* Private function prototypes -----------------------------------------------*/
static void otaApp_task(void *argument);
static uint32_t otaApp_readUint32LE(const uint8_t *buffer);

/* Exported functions --------------------------------------------------------*/

void otaApp_refreshWatchdog(void)
{
    IWDG1->KR = OTA_IWDG_KEY_RELOAD;
}

void otaApp_reportHealth(uint32_t flag, bool ok)
{
    if (ok)
    {
        otaApp_health |= flag;
    }
    else
    {
        otaApp_health &= ~flag;
    }
}

bool otaApp_isUnderTrial(void)
{
    return otaApp_underTrial;
}

bool otaApp_init(void)
{
    ota_record_t record;

    otaApp_health = 0;
    otaApp_confirmed = false;
    otaApp_underTrial = ota_journal_read(&record) && record.phase == OTA_PHASE_TRIAL;

    if (otaApp_underTrial)
    {
        printf("OTA: running under trial (attempt %u/%u)\n",
               (unsigned)record.trial_attempts, (unsigned)OTA_MAX_TRIAL_BOOTS);
    }

    if (OTA_FAULT_SKIP_GUARD_TASK())
    {
        /* Faute injectee : personne ne rechargera le chien de garde. L'image
         * demarre normalement puis se fait couper. */
        printf("OTA: guard task skipped (fault injection)\n");
        return true;
    }

    otaApp_taskHandle = osThreadNew(otaApp_task, NULL, &otaApp_taskAttributes);
    return otaApp_taskHandle != NULL;
}

/* Private functions ---------------------------------------------------------*/

/**
 * @brief  Recharge le chien de garde et confirme l'image lorsqu'elle a fait ses
 *         preuves.
 *
 * La confirmation ne survient qu'apres OTA_CONFIRM_SETTLE_MS passees avec tous
 * les criteres OTA_HEALTH_REQUIRED satisfaits sans interruption. L'ancienne
 * sequence, elle, declarait l'image bonne avant meme d'avoir demarre le serveur
 * HTTP : une version cassant ce serveur etait validee et coupait du meme coup
 * le seul moyen d'en envoyer une autre.
 */
static void otaApp_task(void *argument)
{
    (void)argument;

    uint32_t healthySince = 0;

    for (;;)
    {
        otaApp_refreshWatchdog();

        if (otaApp_underTrial && !otaApp_confirmed)
        {
            const bool healthy = (otaApp_health & OTA_HEALTH_REQUIRED) == OTA_HEALTH_REQUIRED;

            if (!healthy)
            {
                healthySince = 0;
            }
            else
            {
                /* configTICK_RATE_HZ vaut 1000 : un tic vaut une milliseconde. */
                const uint32_t now = osKernelGetTickCount();

                if (healthySince == 0)
                {
                    healthySince = now;
                    printf("OTA: health criteria met, confirming in %lu ms\n",
                           (unsigned long)OTA_CONFIRM_SETTLE_MS);
                }
                else if (OTA_FAULT_LATE_CRASH_ENABLED() &&
                         (now - healthySince) >= OTA_FAULT_LATE_CRASH_MS)
                {
                    /* Faute injectee : l'image tombe apres avoir tout initialise
                     * mais avant d'avoir ete confirmee. */
                    printf("OTA: injecting a late crash\n");
                    otaFault_hardFault();
                }
                else if ((now - healthySince) >= OTA_CONFIRM_SETTLE_MS)
                {
                    if (ota_journal_clear())
                    {
                        printf("OTA: image confirmed\n");
                        otaApp_confirmed = true;
                    }
                    else
                    {
                        /* Nouvelle tentative au tour suivant : ne pas confirmer
                         * vaut mieux que croire l'avoir fait. */
                        printf("OTA: failed to confirm, will retry\n");
                    }
                }
            }
        }

        osDelay(OTA_WATCHDOG_PERIOD_MS);
    }
}

static uint32_t otaApp_readUint32LE(const uint8_t *buffer)
{
    return ((uint32_t)buffer[0]) | ((uint32_t)buffer[1] << 8) |
           ((uint32_t)buffer[2] << 16) | ((uint32_t)buffer[3] << 24);
}

/**
 * @brief  Verifie un paquet fraichement recu, puis demande son application.
 *
 * La verification a lieu ici, avant le redemarrage, pour deux raisons : le
 * refus peut etre explique dans la reponse HTTP, et un paquet invalide ne
 * provoque aucun reboot. Le bootloader refait ces controles avant d'effacer
 * quoi que ce soit -- la NOR peut s'etre degradee entre-temps.
 *
 * @retval NULL si le paquet est accepte, sinon un libelle court du refus.
 */
const char *otaApp_acceptPackage(const char *packageFilePath)
{
    FIL file;
    UINT bytesRead;
    uint8_t header[OTA_PACKAGE_HEADER_SIZE];

    if (f_open(&file, packageFilePath, FA_READ) != FR_OK)
    {
        return "cannot reopen the uploaded file";
    }

    const uint32_t fileSize = (uint32_t)f_size(&file);

    if (fileSize <= OTA_PACKAGE_HEADER_SIZE + 4u)
    {
        f_close(&file);
        return "package too small";
    }

    if (f_read(&file, header, sizeof(header), &bytesRead) != FR_OK || bytesRead != sizeof(header))
    {
        f_close(&file);
        return "cannot read the package header";
    }

    if (memcmp(header, OTA_PACKAGE_MAGIC, 4) != 0)
    {
        f_close(&file);
        return "bad magic number";
    }

    const uint32_t cm7Size = otaApp_readUint32LE(header + 4);
    const uint32_t cm4Size = otaApp_readUint32LE(header + 8);
    const uint32_t externalSize = otaApp_readUint32LE(header + 12);

    if (cm7Size == 0 || cm7Size > FW_CM7_MAX_SIZE)
    {
        f_close(&file);
        return "CM7 image size out of range";
    }

    if (cm4Size == 0 || cm4Size > FW_CM4_MAX_SIZE)
    {
        f_close(&file);
        return "CM4 image size out of range";
    }

    if ((uint64_t)OTA_PACKAGE_HEADER_SIZE + cm7Size + cm4Size + externalSize + 4u != (uint64_t)fileSize)
    {
        f_close(&file);
        return "package size does not match its header";
    }

    /* CRC-32 sur tout le fichier sauf les quatre octets de pied. */
    uint32_t crcExpected;
    uint8_t footer[4];

    if (f_lseek(&file, fileSize - 4u) != FR_OK ||
        f_read(&file, footer, 4, &bytesRead) != FR_OK || bytesRead != 4)
    {
        f_close(&file);
        return "cannot read the package checksum";
    }
    crcExpected = otaApp_readUint32LE(footer);

    if (f_lseek(&file, 0) != FR_OK)
    {
        f_close(&file);
        return "cannot rewind the package";
    }

    static uint8_t chunk[OTA_VERIFY_CHUNK] __attribute__((aligned(4)));
    uint32_t remaining = fileSize - 4u;
    uint32_t crc = OTA_CRC32_INIT;

    while (remaining > 0)
    {
        const uint32_t want = (remaining > OTA_VERIFY_CHUNK) ? OTA_VERIFY_CHUNK : remaining;

        if (f_read(&file, chunk, want, &bytesRead) != FR_OK || bytesRead != want)
        {
            f_close(&file);
            return "cannot read the package body";
        }

        crc = ota_crc32(crc, chunk, bytesRead);
        remaining -= bytesRead;

        otaApp_refreshWatchdog();
    }

    f_close(&file);

    if (crc != crcExpected)
    {
        printf("OTA: CRC mismatch (computed 0x%08lX, expected 0x%08lX)\n",
               (unsigned long)crc, (unsigned long)crcExpected);
        return "checksum mismatch";
    }

    printf("OTA: package verified (CM7 %lu B, CM4 %lu B, external %lu B)\n",
           (unsigned long)cm7Size, (unsigned long)cm4Size, (unsigned long)externalSize);

    /* Les compteurs repartent de zero : ce paquet a droit a son quota complet
     * de tentatives, quel que soit l'historique. */
    if (!ota_journal_write(OTA_PHASE_PENDING, 0, 0, 0))
    {
        return "cannot record the update request";
    }

    return NULL;
}
