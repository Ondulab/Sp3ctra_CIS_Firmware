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
#include "update_gui.h" // For gui_displayUpdateProcess

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
