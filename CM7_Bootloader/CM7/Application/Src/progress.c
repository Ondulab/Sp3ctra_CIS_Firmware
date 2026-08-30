/**
 ******************************************************************************
 * @file           : progress.c
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


#include "progress.h"
#include "main.h"
#include "ota_boot.h"         // For otaBoot_refreshWatchdog
#include "ota_fault_inject.h" // For SP3CTRA_OTA_ABORT_AT_STEP
#include "update_gui.h"       // For gui_displayUpdateProcess

void progress_init(ProgressManager* pm, uint32_t num_steps)
{
    pm->num_steps = num_steps;
    pm->current_step = 1;
    pm->last_progress = -1; // Initialize to -1 to force an update on first call
}

void progress_update(ProgressManager* pm, uint32_t step_number, uint32_t current_value, uint32_t total_value)
{
    if (step_number < 1 || step_number > pm->num_steps)
    {
        // Invalid step number
        return;
    }
    if (total_value == 0)
    {
        // Prevent division by zero
        return;
    }

    /* Point de rafraichissement du chien de garde pour tout le bootloader :
     * chaque boucle longue (CRC, sauvegarde, effacement, flash) passe ici a
     * chaque bloc traite. Sans cela, un chien de garde ayant survecu au reset
     * couperait la machine en plein flash. */
    otaBoot_refreshWatchdog();

#if SP3CTRA_OTA_ABORT_AT_STEP != 0
    /* Coupure secteur simulee : un reset brutal au milieu de l'etape visee,
     * deterministe et rejouable, la ou une vraie coupure demande un banc.
     * Seulement a la premiere tentative, sinon chaque reprise se couperait au
     * meme endroit. */
    if (otaBoot_firstApplyAttempt() &&
        step_number == SP3CTRA_OTA_ABORT_AT_STEP &&
        (uint64_t)current_value * 2u >= (uint64_t)total_value)
    {
        printf("\nOTA: simulating a power cut in the middle of step %lu\n",
               (unsigned long)step_number);
        NVIC_SystemReset();
    }
#endif

    // Calculate progress percentage for the current step
    float step_progress = (current_value * 100.0f) / total_value;

    // Calculate overall progress
    float overall_progress = ((step_number - 1) + (step_progress / 100.0f)) * (100.0f / pm->num_steps);

    int32_t int_progress = (int32_t)overall_progress;

    // Only update if the progress value has changed
    if (int_progress != pm->last_progress)
    {
        pm->last_progress = int_progress;
        // Update the progress bar

        gui_displayUpdateProcess(int_progress);
    }
}
