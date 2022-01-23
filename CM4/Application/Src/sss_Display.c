/*
 * sss_Display.c
 *
 *  Created on: May 31, 2020
 *      Author: zhonx
 */

/* Includes ------------------------------------------------------------------*/
#include "stdbool.h"
#include "stdio.h"
#include "arm_math.h"

#include "shared.h"
#include "config.h"

#include "ssd1362.h"
#include "buttons.h"

#include "sss_Display.h"

/* Private includes ----------------------------------------------------------*/

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private typedef -----------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Variable containing black and white frame from CIS*/

/* Private function prototypes -----------------------------------------------*/
static void cisynth_eth_SetHint(void);
static void cisynth_interractiveMenu(void);
void cis_StartCalibration(void);

/* Private user code ---------------------------------------------------------*/
/**
 * @brief  The application entry point.
 * @retval int
 */
int sss_Display(void)
{
	uint8_t FreqStr[256] = {0};
	uint8_t *cis_rgb = NULL;
	uint32_t cis_color = 0;
	int32_t start_tick = 0;
	int32_t old_cis_process_cnt = 0;
	uint32_t cis_line_freq = 0;
	int32_t i = 0;
	int32_t scroll = 0;

	printf("----- ETHERNET MODE START -----\n");
	printf("-------------------------------\n");

	cisynth_eth_SetHint();
	old_cis_process_cnt = shared_var.cis_process_cnt;

	start_tick = HAL_GetTick();

	/* Infinite loop */
	while (1)
	{
		cisynth_interractiveMenu();

		if ((shared_var.cis_process_cnt - old_cis_process_cnt) > 100)
		{
			cis_line_freq = 1000000 / (((HAL_GetTick() - start_tick) * 1000) / (shared_var.cis_process_cnt - old_cis_process_cnt));
			start_tick = HAL_GetTick();
			old_cis_process_cnt = shared_var.cis_process_cnt;
		}

		sprintf((char *)FreqStr, "%dHz", (int)(cis_line_freq));

		//		ssd1362_drawRect(0, DISPLAY_AERA1_Y1POS, DISPLAY_WIDTH, DISPLAY_AERA1_Y2POS, 3, false);

		scroll++;
		if (scroll > (DISPLAY_AERAS1_HEIGHT - 2))
			scroll = 0;

		for (i = 0; i < (DISPLAY_WIDTH); i++)
		{
			cis_rgb = (uint8_t*)&(imageData[(uint32_t)(i * ((float)CIS_PIXELS_NB / (float)DISPLAY_WIDTH)) + UDP_HEADER_SIZE]);
			cis_color = cis_rgb[0] * cis_rgb[1] * cis_rgb[2];
			cis_color >>= 20;

			//			ssd1362_drawVLine(DISPLAY_WIDTH - 1 - i, DISPLAY_AERA1_Y1POS + 1, (DISPLAY_AERAS1_HEIGHT - 2) / 2, cis_color, false);
			ssd1362_drawPixel(DISPLAY_WIDTH - 1 - i, DISPLAY_AERA1_Y1POS + 1 + scroll, cis_color, false);

		}

		ssd1362_drawRect(0, DISPLAY_HEAD_Y1POS, 100, DISPLAY_HEAD_Y2POS, 4, false);
		ssd1362_drawString(0, 1, (int8_t*)FreqStr, 15, 8);

		ssd1362_writeUpdates();
		//		HAL_Delay(1);
	}
}

/**
 * @brief  CIS start calibration
 * @param  calibration iteration
 * @retval None
 */
void cis_StartCalibration()
{
	uint8_t textData[256] = {0};
	shared_var.cis_cal_state = CIS_CAL_START;
	while (shared_var.cis_cal_state != CIS_CAL_END)
	{
		switch (shared_var.cis_cal_state)
		{
		case CIS_CAL_PLACE_ON_WHITE_LED_ON :
			/*-------- 1 --------*/
			ssd1362_drawRect(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, 0, false);
			ssd1362_drawRect(0, DISPLAY_HEAD_Y1POS, DISPLAY_WIDTH, DISPLAY_HEAD_Y2POS, 4, true);
			ssd1362_drawString(10, DISPLAY_HEAD_Y1POS + 1, (int8_t *)" MOVE CIS ON WHITE SURFACE ", 0xF, 8);
			ssd1362_writeFullBuffer();

			while (shared_var.cis_cal_progressbar < 99)
			{
				ssd1362_progressBar(30, 30, shared_var.cis_cal_progressbar, 0xF);
			}

			ssd1362_progressBar(30, 30, 99, 0xF);

			while (shared_var.cis_cal_state == CIS_CAL_PLACE_ON_WHITE_LED_ON);
			break;
		case CIS_CAL_PLACE_ON_WHITE_LED_OFF :
			/*-------- 2 --------*/
			ssd1362_drawRect(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, 0, false);
			ssd1362_drawRect(0, DISPLAY_HEAD_Y1POS, DISPLAY_WIDTH, DISPLAY_HEAD_Y2POS, 4, true);
			ssd1362_drawString(10, DISPLAY_HEAD_Y1POS + 1, (int8_t *)"          LED OFF           ", 0xF, 8);
			ssd1362_writeFullBuffer();
			ssd1362_writeFullBuffer();

			while (shared_var.cis_cal_progressbar < 99)
			{
				ssd1362_progressBar(30, 30, shared_var.cis_cal_progressbar, 0xF);
			}

			ssd1362_progressBar(30, 30, 99, 0xF);
			HAL_Delay(100);

			// Print curves
			ssd1362_drawRect(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, 0, false);
			ssd1362_drawString(0, 28, (int8_t *)"  R1 G1 B1  R2 G2 B2  R3 G3 B3", 5, 8);
			for (uint32_t i = 1; i < 9; i++)
			{
				ssd1362_drawVLine(DISPLAY_WIDTH / 9 * i, 0, DISPLAY_HEIGHT, 4, false);
			}
			for (uint32_t i = 0; i < (DISPLAY_WIDTH); i++)
			{
				int32_t cis_color = cisCals.whiteCal.data[(uint32_t)(i * ((float)(CIS_ADC_BUFF_SIZE * 3) / (float)DISPLAY_WIDTH))] >> 6;
				ssd1362_drawPixel(DISPLAY_WIDTH - 1 - i, DISPLAY_HEIGHT - cis_color - 1, 14, false);

				cis_color = cisCals.blackCal.data[(uint32_t)(i * ((float)(CIS_ADC_BUFF_SIZE * 3) / (float)DISPLAY_WIDTH))] >> 6;
				ssd1362_drawPixel(DISPLAY_WIDTH - 1 - i, DISPLAY_HEIGHT - cis_color -1, 8, false);
			}
			ssd1362_writeFullBuffer();
			while (shared_var.cis_cal_state == CIS_CAL_PLACE_ON_WHITE_LED_OFF);
			break;
		case CIS_CAL_EXTRACT_EXTREMUMS :
			ssd1362_drawRect(0, DISPLAY_HEAD_Y2POS, DISPLAY_WIDTH, DISPLAY_HEIGHT, 0, false);
			ssd1362_drawRect(0, DISPLAY_HEAD_Y1POS, DISPLAY_WIDTH, DISPLAY_HEAD_Y2POS, 4, true);
			ssd1362_drawString(10, DISPLAY_HEAD_Y1POS + 1, (int8_t *)" EXTRACT EXTREMUMS AND DELTAS ", 0xF, 8);
			ssd1362_writeFullBuffer();

//			uint8_t textData[256] = {0};
			sprintf((char *)textData, "WH MIN R%d G%d B%d", (int)cisCals.whiteCal.red.minPix, (int)cisCals.whiteCal.green.minPix, (int)cisCals.whiteCal.blue.minPix);
			ssd1362_drawString(0, 10, (int8_t*)textData, 15, 8);
			sprintf((char *)textData, "WH MAX R%d G%d B%d", (int)cisCals.whiteCal.red.maxPix, (int)cisCals.whiteCal.green.maxPix, (int)cisCals.whiteCal.blue.maxPix);
			ssd1362_drawString(0, 19, (int8_t*)textData, 15, 8);
			sprintf((char *)textData, "WH DLT R%d G%d B%d", (int)cisCals.whiteCal.red.deltaPix, (int)cisCals.whiteCal.green.deltaPix, (int)cisCals.whiteCal.blue.deltaPix);
			ssd1362_drawString(0, 28, (int8_t*)textData, 15, 8);

			sprintf((char *)textData, "BL MIN R%d G%d B%d", (int)cisCals.blackCal.red.minPix, (int)cisCals.blackCal.green.minPix, (int)cisCals.blackCal.blue.minPix);
			ssd1362_drawString(0, 37, (int8_t*)textData, 15, 8);
			sprintf((char *)textData, "BL MAX R%d G%d B%d", (int)cisCals.blackCal.red.maxPix, (int)cisCals.blackCal.green.maxPix, (int)cisCals.blackCal.blue.maxPix);
			ssd1362_drawString(0, 46, (int8_t*)textData, 15, 8);
			sprintf((char *)textData, "BL DLT R%d G%d B%d", (int)cisCals.blackCal.red.deltaPix, (int)cisCals.blackCal.green.deltaPix, (int)cisCals.blackCal.blue.deltaPix);
			ssd1362_drawString(0, 55, (int8_t*)textData, 15, 8);

			ssd1362_writeFullBuffer();
			while (shared_var.cis_cal_state == CIS_CAL_EXTRACT_EXTREMUMS);
			break;
		case CIS_CAL_EXTRACT_OFFSETS :
			/*-------- 3 --------*/
			ssd1362_drawRect(0, DISPLAY_HEAD_Y2POS, DISPLAY_WIDTH, DISPLAY_HEIGHT, 0, false);
			ssd1362_drawRect(0, DISPLAY_HEAD_Y1POS, DISPLAY_WIDTH, DISPLAY_HEAD_Y2POS, 4, true);
			ssd1362_drawString(10, DISPLAY_HEAD_Y1POS + 1, (int8_t *)"EXTRACT DIFFERENTIAL OFFSETS", 0xF, 8);
			ssd1362_drawString(0, 18, (int8_t *)"  R1 G1 B1  R2 G2 B2  R3 G3 B3", 10, 8);
			for (uint32_t i = 1; i < 9; i++)
			{
				ssd1362_drawVLine(DISPLAY_WIDTH / 9 * i, 28, DISPLAY_HEIGHT, 4, false);
			}
			ssd1362_writeFullBuffer();

			// Print curve
			for (uint32_t i = 0; i < (DISPLAY_WIDTH); i++)
			{
				uint32_t index = i * ((float)(CIS_ADC_BUFF_SIZE * 3) / (float)DISPLAY_WIDTH);
				int32_t cis_color = cisCals.offsetData[index] >> 6;
				ssd1362_drawPixel(DISPLAY_WIDTH - 1 - i, DISPLAY_HEIGHT - cis_color, 15, false);
			}
			ssd1362_writeFullBuffer();
			while (shared_var.cis_cal_state == CIS_CAL_EXTRACT_OFFSETS);
			break;
		case CIS_CAL_COMPUTE_GAINS :
			/*-------- 4 --------*/
			ssd1362_drawRect(0, DISPLAY_HEAD_Y1POS, DISPLAY_WIDTH, DISPLAY_HEAD_Y2POS, 4, false);
			ssd1362_drawString(10, DISPLAY_HEAD_Y1POS + 1, (int8_t *)" COMPUTE COMPENSATIION GAINS", 0xF, 8);
			ssd1362_writeFullBuffer();
			while (shared_var.cis_cal_state == CIS_CAL_COMPUTE_GAINS);
			break;
		}
	}
	cisynth_eth_SetHint();
}

#define BUTTON_DELAY			500

/**
 * @brief
 * @param
 * @retval None
 */
void cisynth_interractiveMenu()
{
	static uint32_t button_tick = 0;
	static uint8_t clear_button = 0;

	if (buttonState[SW1] == SWITCH_PRESSED)
	{
		ssd1362_drawRect(0 + 10, 60, 10 + 10, 54, 0x0F, false);
		buttonState[SW1] = SWITCH_RELEASED;
		button_tick = HAL_GetTick();
		clear_button = 0;
		cis_StartCalibration();
	}
	if (buttonState[SW2] == SWITCH_PRESSED)
	{
		ssd1362_drawRect(56 + 10, 60, 56 + 10 + 10, 54, 0x0F, false);
		buttonState[SW2] = SWITCH_RELEASED;
		button_tick = HAL_GetTick();
		clear_button = 0;
	}
	if (buttonState[SW3] == SWITCH_PRESSED)
	{
		ssd1362_drawRect(56 * 2 + 10, 60, 56 * 2 + 10 + 10, 54, 0x0F, false);
		buttonState[SW3] = SWITCH_RELEASED;
		button_tick = HAL_GetTick();
		clear_button = 0;
	}
	if (buttonState[SW4] == SWITCH_PRESSED)
	{
		ssd1362_drawRect(56 * 3 + 10, 60, 56 * 3 + 10 + 10, 54, 0x0F, false);
		buttonState[SW4] = SWITCH_RELEASED;
		button_tick = HAL_GetTick();
		clear_button = 0;
	}
	if (buttonState[SW5] == SWITCH_PRESSED)
	{
		ssd1362_drawRect(56 * 4 + 10, 60, 56 * 4 + 10 + 10, 54, 0x0F, false);
		buttonState[SW5] = SWITCH_RELEASED;
		button_tick = HAL_GetTick();
		clear_button = 0;
	}

	if (HAL_GetTick() > (button_tick + BUTTON_DELAY) && clear_button != 1)
	{
		clear_button = 1;
		ssd1362_drawRect(0, DISPLAY_AERA2_Y1POS, DISPLAY_WIDTH, DISPLAY_AERA2_Y2POS, 0, false);
		ssd1362_drawRect(0 + 10, 60, 10 + 10, 54, 0x05, false);
		ssd1362_drawRect(56 + 10, 60, 56 + 10 + 10, 54, 0x05, false);
		ssd1362_drawRect(56 * 2 + 10, 60, 56 * 2 + 10 + 10, 54, 0x05, false);
		ssd1362_drawRect(56 * 3 + 10, 60, 56 * 3 + 10 + 10, 54, 0x05, false);
		ssd1362_drawRect(56 * 4 + 10, 60, 56 * 4 + 10 + 10, 54, 0x05, false);
	}
}

/**
 * @brief  Display Audio demo hint
 * @param  None
 * @retval None
 */
static void cisynth_eth_SetHint(void)
{
	/* Set Audio header description */
	ssd1362_clearBuffer();
	ssd1362_writeFullBuffer();
	ssd1362_drawRect(0, DISPLAY_HEAD_Y1POS, DISPLAY_WIDTH, DISPLAY_HEAD_Y2POS, 4, false);
	ssd1362_drawString(100, 1, (int8_t *)"CISYNTH 3", 0xF, 8);
	ssd1362_drawString(232, 1, (int8_t *)"ETH", 0xF, 8);

	ssd1362_drawRect(0, DISPLAY_AERA2_Y1POS, DISPLAY_WIDTH, DISPLAY_AERA2_Y2POS, 0, false);
	ssd1362_drawRect(0 + 10, 60, 10 + 10, 54, 0x05, false);
	ssd1362_drawRect(56 + 10, 60, 56 + 10 + 10, 54, 0x05, false);
	ssd1362_drawRect(56 * 2 + 10, 60, 56 * 2 + 10 + 10, 54, 0x05, false);
	ssd1362_drawRect(56 * 3 + 10, 60, 56 * 3 + 10 + 10, 54, 0x05, false);
	ssd1362_drawRect(56 * 4 + 10, 60, 56 * 4 + 10 + 10, 54, 0x05, false);

	ssd1362_writeFullBuffer();
}
/* Private functions ---------------------------------------------------------*/
