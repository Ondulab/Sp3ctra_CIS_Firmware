/**
 ******************************************************************************
 * @file           : gui_interrupts.h
 * @brief          : GUI interrupts module header - interrupt handlers and sync
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

#ifndef __GUI_INTERRUPTS_H
#define __GUI_INTERRUPTS_H

/* Includes ------------------------------------------------------------------*/
#include "stdint.h"

/* Exported variables --------------------------------------------------------*/
extern volatile uint32_t transferComplete;

/* Exported function prototypes ----------------------------------------------*/
void HSEM2_IRQHandler(void);

#endif /* __GUI_INTERRUPTS_H */
