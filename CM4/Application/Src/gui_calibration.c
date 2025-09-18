/**
 ******************************************************************************
 * @file           : gui_calibration.c
 * @brief          : GUI calibration module - CIS calibration process
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
#include "basetypes.h"
#include "globals.h"
#include "gui_config.h"

#include "ssd1362.h"
#include "gui_calibration.h"

/* Private functions ---------------------------------------------------------*/

/**
 * @brief Initiates the CIS calibration procedure.
 *
 * Displays step-by-step instructions to guide the user through the calibration
 * process, handling different calibration states until completion.
 */
void gui_startCalibration(void)
{
    //uint8_t textData[256] = {0};
    HAL_Delay(100);
    shared_var.cis_cal_state = CIS_CAL_START;

#ifdef POLYNOMIAL_CALIBRATION
    HAL_Delay(200);
    shared_var.cis_cal_state = CIS_CAL_END;
    return;
#endif

    while (shared_var.cis_cal_state != CIS_CAL_END)
    {
        switch (shared_var.cis_cal_state)
        {
        case CIS_CAL_WHITE :
            /*-------- 1 --------*/
            ssd1362_fillRect(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, 0, false);
            ssd1362_fillRect(0, DISPLAY_HEAD_Y1POS, DISPLAY_WIDTH, DISPLAY_HEAD_Y2POS, BANNER_BACKGROUND_COLOR, true);
            ssd1362_drawString(0, DISPLAY_HEAD_Y1POS + 1, (int8_t *)" MOVE CIS ON WHITE SURFACE - HL ", 0xF, 8);
            ssd1362_writeFullBuffer();

            while (shared_var.cis_cal_progressbar < 99)
            {
                ssd1362_progressBar(30, 30, shared_var.cis_cal_progressbar, 0xF);
            }

            ssd1362_progressBar(30, 30, 99, 0xF);
            HAL_Delay(10);

            while (shared_var.cis_cal_state == CIS_CAL_WHITE);
            break;
        case CIS_CAL_INTERMEDIATE :
            /*-------- 2 --------*/
            ssd1362_fillRect(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, 0, false);
            ssd1362_fillRect(0, DISPLAY_HEAD_Y1POS, DISPLAY_WIDTH, DISPLAY_HEAD_Y2POS, BANNER_BACKGROUND_COLOR, true);
            ssd1362_drawString(0, DISPLAY_HEAD_Y1POS + 1, (int8_t *)" MOVE CIS ON GRAY SURFACE - ML ", 0xF, 8);
            ssd1362_writeFullBuffer();

            while (shared_var.cis_cal_progressbar < 99)
            {
                ssd1362_progressBar(30, 30, shared_var.cis_cal_progressbar, 0xF);
            }

            ssd1362_progressBar(30, 30, 99, 0xF);
            HAL_Delay(10);

            while (shared_var.cis_cal_state == CIS_CAL_INTERMEDIATE);
            break;
        case CIS_CAL_BLACK :
            /*-------- 3 --------*/
            ssd1362_fillRect(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, 0, false);
            ssd1362_fillRect(0, DISPLAY_HEAD_Y1POS, DISPLAY_WIDTH, DISPLAY_HEAD_Y2POS, BANNER_BACKGROUND_COLOR, true);
            ssd1362_drawString(0, DISPLAY_HEAD_Y1POS + 1, (int8_t *)" MOVE CIS ON BLACK SURFACE - LL ", 0xF, 8);
            ssd1362_writeFullBuffer();

            while (shared_var.cis_cal_progressbar < 99)
            {
                ssd1362_progressBar(30, 30, shared_var.cis_cal_progressbar, 0xF);
            }

            ssd1362_progressBar(30, 30, 99, 0xF);
            HAL_Delay(100);
            while (shared_var.cis_cal_state == CIS_CAL_BLACK);
            break;
        case CIS_CAL_EXTRACT_EXTREMUMS :
            ssd1362_fillRect(0, DISPLAY_HEAD_Y2POS, DISPLAY_WIDTH, DISPLAY_HEIGHT, 0, false);
            ssd1362_fillRect(0, DISPLAY_HEAD_Y1POS, DISPLAY_WIDTH, DISPLAY_HEAD_Y2POS, BANNER_BACKGROUND_COLOR, true);
            ssd1362_drawString(0, DISPLAY_HEAD_Y1POS + 1, (int8_t *)" EXTRACT EXTREMUMS AND DELTAS ", 0xF, 8);
            ssd1362_writeFullBuffer();

            ssd1362_writeFullBuffer();
            while (shared_var.cis_cal_state == CIS_CAL_EXTRACT_EXTREMUMS);
            break;
        case CIS_CAL_EXTRACT_OFFSETS :
            /*-------- 4 --------*/
            ssd1362_fillRect(0, DISPLAY_HEAD_Y2POS, DISPLAY_WIDTH, DISPLAY_HEIGHT, 0, false);
            ssd1362_fillRect(0, DISPLAY_HEAD_Y1POS, DISPLAY_WIDTH, DISPLAY_HEAD_Y2POS, BANNER_BACKGROUND_COLOR, true);
            ssd1362_drawString(0, DISPLAY_HEAD_Y1POS + 1, (int8_t *)"EXTRACT DIFFERENTIAL OFFSETS", 0xF, 8);
            ssd1362_writeFullBuffer();
            while (shared_var.cis_cal_state == CIS_CAL_EXTRACT_OFFSETS);
            break;
        case CIS_CAL_COMPUTE_GAINS :
            /*-------- 5 --------*/
            ssd1362_fillRect(0, DISPLAY_HEAD_Y1POS, DISPLAY_WIDTH, DISPLAY_HEAD_Y2POS, BANNER_BACKGROUND_COLOR, false);
            ssd1362_drawString(0, DISPLAY_HEAD_Y1POS + 1, (int8_t *)" COMPUTE COMPENSATIION GAINS", 0xF, 8);
            ssd1362_writeFullBuffer();
            while (shared_var.cis_cal_state == CIS_CAL_COMPUTE_GAINS);
            break;
        case CIS_CAL_COMPUTE_TRANSITIONS :
            /*-------- 6 --------*/
            ssd1362_fillRect(0, DISPLAY_HEAD_Y1POS, DISPLAY_WIDTH, DISPLAY_HEAD_Y2POS, BANNER_BACKGROUND_COLOR, false);
            ssd1362_drawString(0, DISPLAY_HEAD_Y1POS + 1, (int8_t *)" COMPUTE TRANSITION POINTS", 0xF, 8);
            ssd1362_writeFullBuffer();
            while (shared_var.cis_cal_state == CIS_CAL_COMPUTE_TRANSITIONS);
            break;
        default:
            break;
        }
    }
    ssd1362_clearBuffer();
    ssd1362_writeFullBuffer();
}

/**
 * @brief Adjusts the screen orientation based on the configured handedness.
 *
 * Calls the screen rotation function with the current handedness configuration.
 */
void gui_changeHand(void)
{
    ssd1362_screenRotation(shared_config.cis_handedness);
}
