/*
 * cisynth_ifft.c
 *
 *  Created on: May 31, 2020
 *      Author: zhonx
 */

#include "cisynth_ifft.h"
#include "synth.h"
#include "times_base.h"
#include "cis.h"
#include "wave_generation.h"
#include "wave_sommation.h"
#include "config.h"
#include "stdio.h"
#include "arm_math.h"

extern __IO uint32_t rfft_cnt;
static void cisynth_ifft_SetHint(void);
//static int16_t *cisBuff = NULL;

/**
 * @brief  The application entry point.
 * @retval int
 */
int cisynth_ifft(void)
{
	uint8_t FreqStr[256] = {0};
	uint32_t cis_color = 0;

	cis_Init();
	synthInit();

	cisynth_ifft_SetHint();

//	cis_Test();

	/* Infinite loop */
	static uint32_t start_tick;
	static uint32_t old_tick;
	uint32_t latency;
	old_tick = HAL_GetTick();
	uint16_t *cisBuff = NULL;
	uint16_t rfftDataBuff[NUMBER_OF_NOTES] = {0};
	uint32_t i = 0;
	while (1)
	{
		start_tick = HAL_GetTick();
		while (rfft_cnt < (48000 / DISPLAY_REFRESH_FPS))
		{
			synthAudioProcess();
		}
		rfft_cnt = 0;
		latency = HAL_GetTick() - start_tick;
#ifdef DEBUG_SYNTH
		sprintf((char *)FreqStr, "%d KHz", (int)(48000/ (latency * DISPLAY_REFRESH_FPS)));
		GUI_DisplayStringAt(0, 5, (uint8_t*)FreqStr, RIGHT_MODE);

		GUI_FillRect(0, DISPLAY_AERA1_YPOS, FT5336_MAX_X_LENGTH, DISPLAY_AERAS_HEIGHT, GUI_COLOR_BLACK);
		GUI_FillRect(0, DISPLAY_AERA2_YPOS, FT5336_MAX_X_LENGTH, DISPLAY_AERAS_HEIGHT, GUI_COLOR_BLACK);
		for (uint32_t i = 0; i < FT5336_MAX_X_LENGTH; i++)
		{
			GUI_SetPixel(i, DISPLAY_AERA1_YPOS + (DISPLAY_AERAS_HEIGHT / 2) + ((synthGetRfftBuffData(i) >> 16) / 2047) , GUI_COLOR_LIGHTGREEN);
			GUI_SetPixel(i, DISPLAY_AERA2_YPOS + (DISPLAY_AERAS_HEIGHT / 2) + ((synthGetRfftBuffData(i) << 16 >> 16) / 2047) , GUI_COLOR_LIGHTGREEN);
		}
#endif
#ifdef DEBUG_CIS
		GUI_FillRect(0, DISPLAY_AERA3_YPOS, FT5336_MAX_X_LENGTH, DISPLAY_AERAS_HEIGHT, GUI_COLOR_ST_GREEN_DARK);
		for (uint32_t i = 0; i < FT5336_MAX_X_LENGTH; i++)
		{
			cis_color = cis_GetBuffData((i * ((float)CIS_PIXELS_NB / (float)FT5336_MAX_X_LENGTH)));
			cis_color = cis_color >> 8;
			GUI_SetPixel(i, DISPLAY_AERA3_YPOS + DISPLAY_AERAS_HEIGHT - DISPLAY_INTER_AERAS_HEIGHT - (cis_color >> 3) , GUI_COLOR_LIGHTMAGENTA);
			cis_color |= 0xFF000000;
			cis_color |= cis_color << 8;
			cis_color |= cis_color << 16;
			GUI_DrawLine(i, DISPLAY_AERA4_YPOS + DISPLAY_INTER_AERAS_HEIGHT, i, DISPLAY_AERA4_YPOS + DISPLAY_AERAS_HEIGHT - DISPLAY_INTER_AERAS_HEIGHT, cis_color);
		}
#endif
		old_tick = HAL_GetTick();
	}
}

/**
 * @brief  Display Audio demo hint
 * @param  None
 * @retval None
 */
static void cisynth_ifft_SetHint(void)
{
	/* Set Audio Demo description */
	GUI_FillRect(0, 0, FT5336_MAX_X_LENGTH, DISPLAY_HEAD_HEIGHT, GUI_COLOR_DARKRED);
	GUI_SetTextColor(GUI_COLOR_LIGHTGRAY);
	GUI_SetBackColor(GUI_COLOR_DARKRED);
	GUI_SetFont(&Font20);
	GUI_DisplayStringAt(0, 2, (uint8_t *)"CISYNTH 3", CENTER_MODE);
	GUI_SetFont(&Font16);
	GUI_DisplayStringAt(0, 5, (uint8_t *)"BW ifft", LEFT_MODE);
	GUI_FillRect(0, DISPLAY_HEAD_HEIGHT, FT5336_MAX_X_LENGTH, DISPLAY_HEAD_HEIGHT + DISPLAY_INTER_AERAS_HEIGHT, GUI_COLOR_DARKGRAY);
}
