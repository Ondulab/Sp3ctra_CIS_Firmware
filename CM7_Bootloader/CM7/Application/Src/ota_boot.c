/**
 ******************************************************************************
 * @file           : ota_boot.c
 * @brief          : Sequenceur de demarrage du bootloader.
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

#include "boot_config.h"
#include "ota.h"
#include "ota_boot.h"
#include "update.h"
#include "update_gui.h"

#include <stdio.h>

/* Private define ------------------------------------------------------------*/

/* LSI ~32 kHz / 128 = 250 Hz ; 2500 pas => environ 10 s.
 * Il faut tenir la sequence d'initialisation de l'application, qui attend le
 * lien reseau : c'est la tache de rafraichissement, creee des le demarrage de
 * l'ordonnanceur, qui couvre cette attente. */
#define OTA_IWDG_PRESCALER 5u    /* 0=/4, 1=/8, ... 5=/128 */
#define OTA_IWDG_RELOAD    2500u /* 12 bits, 4095 maximum  */

#define OTA_IWDG_KEY_RELOAD 0x0000AAAAu
#define OTA_IWDG_KEY_ENABLE 0x0000CCCCu
#define OTA_IWDG_KEY_WRITE  0x00005555u

/* Private variables ---------------------------------------------------------*/
static uint32_t otaBoot_resetFlags;

/* L'etape precoce s'execute avant MX_FMC_Init() et gui_init() : y toucher a
 * l'ecran accede a un FMC non horloge, donc a une faute de bus. Ce drapeau est
 * pose a l'entree de l'etape tardive, seul endroit ou l'ecran existe. */
static bool otaBoot_displayReady;

/* Chien de garde ------------------------------------------------------------*/

void otaBoot_refreshWatchdog(void)
{
    IWDG1->KR = OTA_IWDG_KEY_RELOAD;
}

void otaBoot_armWatchdog(void)
{
    IWDG1->KR = OTA_IWDG_KEY_ENABLE;
    IWDG1->KR = OTA_IWDG_KEY_WRITE;
    IWDG1->PR = OTA_IWDG_PRESCALER;
    IWDG1->RLR = OTA_IWDG_RELOAD;

    /* Attente de la prise en compte de PR/RLR. Bornee : si la LSI ne demarre
     * pas, mieux vaut poursuivre sans chien de garde que rester bloque ici. */
    for (uint32_t guard = 0; guard < 1000000u && IWDG1->SR != 0u; guard++)
    {
        __NOP();
    }

    IWDG1->KR = OTA_IWDG_KEY_RELOAD;
    printf("Watchdog armed (~%lu ms)\n",
           (unsigned long)((OTA_IWDG_RELOAD * 1000u) / 250u));
}

void otaBoot_logResetCause(void)
{
    otaBoot_resetFlags = RCC->RSR;

    printf("Reset cause: 0x%08lX%s%s%s%s%s%s\n",
           (unsigned long)otaBoot_resetFlags,
           (otaBoot_resetFlags & RCC_RSR_PORRSTF) ? " POR" : "",
           (otaBoot_resetFlags & RCC_RSR_BORRSTF) ? " BOR" : "",
           (otaBoot_resetFlags & RCC_RSR_PINRSTF) ? " PIN" : "",
           (otaBoot_resetFlags & RCC_RSR_SFT1RSTF) ? " SOFT" : "",
           (otaBoot_resetFlags & RCC_RSR_IWDG1RSTF) ? " IWDG" : "",
           (otaBoot_resetFlags & RCC_RSR_WWDG1RSTF) ? " WWDG" : "");

    /* Sans effacement, les drapeaux s'accumulent et la cause du reset suivant
     * devient illisible. */
    __HAL_RCC_CLEAR_RESET_FLAGS();
}

bool otaBoot_lastResetWasWatchdog(void)
{
    return (otaBoot_resetFlags & RCC_RSR_IWDG1RSTF) != 0u;
}

/**
 * @brief  Message a l'operateur, sur la liaison serie et, si elle existe deja,
 *         sur la dalle OLED.
 */
static void otaBoot_message(const char *line1, const char *line2)
{
    printf("OTA: %s | %s\n", line1, line2);

    if (otaBoot_displayReady)
    {
        gui_displayMessage(line1, line2);
        HAL_Delay(3000);
    }
}

/* Saut et reboot ------------------------------------------------------------*/

typedef void (*pFunction)(void);

void otaBoot_jumpToFirmware(uint32_t flashStartAddr)
{
    HAL_MPU_Disable();
    HAL_SuspendTick();

    __disable_irq();

    SysTick->CTRL = 0;
    SysTick->LOAD = 0;
    SysTick->VAL = 0;

    HAL_RCC_DeInit();

    for (uint8_t i = 0; i < 8; i++)
    {
        NVIC->ICER[i] = 0xFFFFFFFF;
        NVIC->ICPR[i] = 0xFFFFFFFF;
    }

    uint32_t appStack = *((volatile uint32_t *)flashStartAddr);
    uint32_t appEntry = *((volatile uint32_t *)(flashStartAddr + 4));

    SCB_DisableICache();
    SCB_DisableDCache();

    __enable_irq();

    __DMB();
    SCB->VTOR = flashStartAddr;
    __DSB();

    HAL_DeInit();
    __set_MSP(appStack);

    pFunction jumpToApplication = (pFunction)appEntry;
    jumpToApplication();
}

void otaBoot_reboot(void)
{
    printf("Rebooting\n");

    for (uint32_t i = 0; i < 20u; i++)
    {
        otaBoot_refreshWatchdog();
        HAL_Delay(100);
    }

    NVIC_SystemReset();
}

/* Sequencement --------------------------------------------------------------*/

/**
 * @brief  Saut vers l'application, chien de garde arme ou non.
 * @param  underTrial  vrai quand l'image n'a pas encore ete confirmee : le
 *                     chien de garde est alors arme pour que la machine se
 *                     reinitialise seule si elle se fige, ce qui fait avancer
 *                     le compteur de tentatives au reset suivant.
 */
static void otaBoot_startApplication(bool underTrial)
{
    if (underTrial)
    {
        otaBoot_armWatchdog();
    }

    otaBoot_jumpToFirmware(FW_CM7_START_ADDR);
}

/**
 * @brief  Verifie qu'une transition d'etat a bien ete enregistree.
 *
 * Toute la sequence repose sur la progression des compteurs en flash. Si le
 * journal devient inecrivable, le compteur ne bouge plus et chaque branche qui
 * reboote se met a boucler : essai perpetuel, restauration perpetuelle. On
 * arrete donc la mecanique et on demarre l'application telle quelle, sans
 * chien de garde -- une machine figee vaut mieux qu'une machine qui se
 * reinitialise indefiniment et qu'on ne peut plus joindre pour la reparer.
 */
static void otaBoot_requireJournal(bool written)
{
    if (written)
    {
        return;
    }

    printf("OTA: the journal is unwritable, giving up on the update sequence\n");
    otaBoot_message("        JOURNAL FAILURE         ",
                    "     SERVICE REQUIRED (SWD)     ");
    otaBoot_startApplication(false);
}

void otaBoot_earlyStage(void)
{
    ota_record_t record;

    if (!ota_journal_read(&record))
    {
        /* Journal vierge : machine neuve, ou premier demarrage apres la
         * migration depuis l'ancien mot d'etat. Rien a appliquer. */
        otaBoot_startApplication(false);
        return;
    }

    printf("OTA phase: %s (trial %u/%u, rollback %u/%u, pending %u/%u)\n",
           ota_phase_str(record.phase),
           (unsigned)record.trial_attempts, (unsigned)OTA_MAX_TRIAL_BOOTS,
           (unsigned)record.rollback_attempts, (unsigned)OTA_MAX_ROLLBACK_ATTEMPTS,
           (unsigned)record.pending_attempts, (unsigned)OTA_MAX_PENDING_ATTEMPTS);

    switch (record.phase)
    {
    case OTA_PHASE_IDLE:
        otaBoot_startApplication(false);
        break;

    case OTA_PHASE_TRIAL:
        if (record.trial_attempts >= OTA_MAX_TRIAL_BOOTS)
        {
            /* L'image a epuise ses essais sans jamais se confirmer : on passe
             * la main a l'etape tardive, qui restaurera la sauvegarde. */
            printf("OTA: trial image exhausted its attempts, rolling back\n");
            otaBoot_requireJournal(ota_journal_write(OTA_PHASE_ROLLBACK,
                                                     record.trial_attempts, 0,
                                                     record.pending_attempts));
            break;
        }

        /* Le compteur est incremente AVANT le saut : si l'image se fige, se
         * plante ou se fait couper par le chien de garde, la tentative reste
         * comptee. C'est ce qui manquait a l'ancienne sequence. */
        otaBoot_requireJournal(ota_journal_write(OTA_PHASE_TRIAL,
                                                 record.trial_attempts + 1u,
                                                 record.rollback_attempts,
                                                 record.pending_attempts));
        otaBoot_startApplication(true);
        break;

    case OTA_PHASE_FAILED:
        /* Plus rien a tenter : on demarre ce qui se trouve en flash, sans
         * toucher a quoi que ce soit. L'ecran affichera la raison. */
        break;

    case OTA_PHASE_PENDING:
    case OTA_PHASE_ROLLBACK:
    default:
        /* Ces phases demandent la NOR et l'ecran : etape tardive. */
        break;
    }
}

/**
 * @brief  Applique le paquet en attente.
 */
static void otaBoot_applyPendingPackage(const ota_record_t *record)
{
    char packageFilePath[64];

    if (record->pending_attempts >= OTA_MAX_PENDING_ATTEMPTS)
    {
        printf("OTA: package application failed %u times, rolling back\n",
               (unsigned)record->pending_attempts);
        otaBoot_requireJournal(ota_journal_write(OTA_PHASE_ROLLBACK, 0, 0,
                                                 record->pending_attempts));
        otaBoot_reboot();
        return;
    }

    if (update_findPackageFile(packageFilePath, sizeof(packageFilePath)) != FWUPDATE_OK)
    {
        /* Rien a appliquer et rien d'efface : l'application en place est
         * intacte, on repart proprement. */
        printf("OTA: no package found in %s, aborting\n", FW_PATH);
        gui_displayUpdateFailed();
        otaBoot_requireJournal(ota_journal_clear());
        otaBoot_reboot();
        return;
    }

    printf("OTA: applying %s\n", packageFilePath);

    /* La tentative est comptee avant d'effacer quoi que ce soit : une coupure
     * secteur en plein flash sera retentee, mais un nombre borne de fois. */
    otaBoot_requireJournal(ota_journal_write(OTA_PHASE_PENDING, 0,
                                             record->rollback_attempts,
                                             record->pending_attempts + 1u));

    const fwupdate_StatusTypeDef status = update_processPackageFile(packageFilePath);

    if (status == FWUPDATE_OK)
    {
        printf("OTA: package applied, starting trial\n");
        gui_displayUpdateTesting();
        otaBoot_requireJournal(ota_journal_write(OTA_PHASE_TRIAL, 0, 0, 0));
        otaBoot_reboot();
        return;
    }

    if (status == FWUPDATE_CRCMISMATCH)
    {
        /* Paquet invalide : refuse avant tout effacement, l'application en
         * place n'a pas ete touchee. */
        printf("OTA: package rejected, application left untouched\n");
        gui_displayUpdateFailed();
        otaBoot_requireJournal(ota_journal_clear());
        otaBoot_reboot();
        return;
    }

    /* Echec apres le debut de l'effacement : la zone applicative est dans un
     * etat indetermine. On reboote pour retenter, le compteur bornera. */
    printf("OTA: package application failed, will retry\n");
    otaBoot_reboot();
}

/**
 * @brief  Restaure la sauvegarde apres un essai infructueux.
 */
static void otaBoot_rollback(const ota_record_t *record)
{
    if (record->rollback_attempts >= OTA_MAX_ROLLBACK_ATTEMPTS)
    {
        /* Fin des tentatives. On ne detruit plus rien et on affiche la raison :
         * c'est ce qui remplace l'ancienne boucle de restauration infinie. */
        printf("OTA: rollback failed %u times, giving up\n",
               (unsigned)record->rollback_attempts);
        otaBoot_requireJournal(ota_journal_write(OTA_PHASE_FAILED, 0,
                                                 record->rollback_attempts, 0));
        otaBoot_message("       RECOVERY FAILED          ",
                        "     SERVICE REQUIRED (SWD)     ");
        otaBoot_startApplication(false);
        return;
    }

    printf("OTA: restoring the previous firmware\n");
    otaBoot_requireJournal(ota_journal_write(OTA_PHASE_ROLLBACK, 0,
                                             record->rollback_attempts + 1u, 0));

    if (update_restoreBackupFirmwares() != FWUPDATE_OK)
    {
        printf("OTA: restore failed, rebooting to retry\n");
        gui_displayUpdateFailed();
        otaBoot_reboot();
        return;
    }

    printf("OTA: previous firmware restored\n");
    gui_displayUpdateSuccess();
    otaBoot_requireJournal(ota_journal_clear());
    otaBoot_reboot();
}

void otaBoot_lateStage(void)
{
    ota_record_t record;

    /* main.c a appele gui_init() juste avant : l'ecran est utilisable. */
    otaBoot_displayReady = true;

    if (!ota_journal_read(&record))
    {
        otaBoot_startApplication(false);
        return;
    }

    switch (record.phase)
    {
    case OTA_PHASE_PENDING:
        otaBoot_applyPendingPackage(&record);
        break;

    case OTA_PHASE_ROLLBACK:
        otaBoot_rollback(&record);
        break;

    case OTA_PHASE_FAILED:
        otaBoot_message("       RECOVERY FAILED          ",
                        "     SERVICE REQUIRED (SWD)     ");
        otaBoot_startApplication(false);
        break;

    default:
        otaBoot_startApplication(false);
        break;
    }

    /* Aucune branche ne revient ici : toutes rebootent ou sautent. */
    otaBoot_reboot();
}
