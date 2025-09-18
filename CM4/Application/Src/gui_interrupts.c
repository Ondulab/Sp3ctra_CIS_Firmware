/**
 ******************************************************************************
 * @file           : gui_interrupts.c
 * @brief          : GUI interrupts module - interrupt handlers and sync
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
#include "stdbool.h"
#include "stdint.h"

#include "main.h"
#include "gui_interrupts.h"

/* Exported variables --------------------------------------------------------*/
volatile uint32_t transferComplete = 0;

/* Interrupt handlers --------------------------------------------------------*/

/**
 * @brief Interrupt handler for the hardware semaphore.
 *
 * Checks the semaphore flag, clears it, and sets the transferComplete flag to
 * indicate that the MDMA transfer has finished.
 */
void HSEM2_IRQHandler(void)
{
    // Check if semaphore 1 triggered the IRQ
    if (__HAL_HSEM_GET_FLAG(__HAL_HSEM_SEMID_TO_MASK(1)) != 0)
    {
        // Clear the HSEM interrupt flag
        __HAL_HSEM_CLEAR_FLAG(__HAL_HSEM_SEMID_TO_MASK(1));

        // Signal that the MDMA transfer is complete
        transferComplete = true;
    }
}
