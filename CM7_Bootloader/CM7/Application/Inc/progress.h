/**
 ******************************************************************************
 * @file           : progress.h
 * @brief          : Header for progress management in update process.
 *                   Contains prototypes and definitions for progress functionality.
 ******************************************************************************
 * @attention
 *
 * Copyright (C) 2018-present Reso-nance.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 *
 ******************************************************************************
 */

#ifndef PROGRESS_H
#define PROGRESS_H

#include <stdint.h>

typedef struct
{
    int num_steps;
    int current_step;
    int last_progress;
} ProgressManager;

/**
 * @brief Initializes the progress manager.
 * @param pm Pointer to the ProgressManager structure.
 * @param num_steps Total number of steps in the process.
 */
void progress_init(ProgressManager* pm, uint32_t num_steps);

/**
 * @brief Updates the progress manager with the current progress.
 * @param pm Pointer to the ProgressManager structure.
 * @param step_number Current step number (starting from 1).
 * @param current_value Current value of the progress metric.
 * @param total_value Total value of the progress metric.
 */
void progress_update(ProgressManager* pm, uint32_t step_number, uint32_t current_value, uint32_t total_value);

#endif // PROGRESS_H
