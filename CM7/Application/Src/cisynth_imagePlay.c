/*
 * cisynth_imagePlay.c
 *
 *  Created on: May 31, 2020
 *      Author: zhonx
 */

#include "cisynth_imagePlay.h"
#include "synth.h"
#include "times_base.h"
#include "cis.h"
#include "wave_generation.h"
#include "wave_sommation.h"
#include "config.h"
#include "stdio.h"
#include "arm_math.h"
#include "menu.h"

extern __IO uint32_t synth_process_cnt;
static void cisynth_imagePlay_SetHint(void);

/**
 * @brief  The application entry point.
 * @retval int
 */
int cisynth_imagePlay(void)
{
//	uint8_t FreqStr[256] = {0};
//	uint32_t cis_color = 0;

	printf("Start ImagePlay mode \n");

	cisynth_imagePlay_SetHint();
	cis_Init(PLAY_MODE);

//	GUI_SetBackColor(GUI_COLOR_DARKRED);

//	/* Infinite loop */
//	static uint32_t start_tick;
//	static uint32_t old_tick;
//	uint32_t latency;
//	old_tick = HAL_GetTick();
//	uint16_t *cisBuff = NULL;
//	uint32_t i = 0;
//	while (1)
//	{
//		start_tick = HAL_GetTick();
//		while (synth_process_cnt < (SAMPLING_FREQUENCY / DISPLAY_REFRESH_FPS))
//		{
//			synth_AudioProcess(PLAY_MODE);
//		}
//		synth_process_cnt = 0;
//		latency = HAL_GetTick() - start_tick;
//		sprintf((char *)FreqStr, "%d Hz", (int)(SAMPLING_FREQUENCY * 1000/ (latency * DISPLAY_REFRESH_FPS)));
//		GUI_DisplayStringAt(0, 5, (uint8_t*)FreqStr, RIGHT_MODE);
//
//		GUI_FillRect(0, DISPLAY_AERA1_YPOS, FT5336_MAX_X_LENGTH, DISPLAY_AERAS_HEIGHT * 2, GUI_COLOR_BLACK);
//		GUI_DrawRect(0, DISPLAY_AERA1_YPOS, FT5336_MAX_X_LENGTH / 2, DISPLAY_AERAS_HEIGHT * 2, GUI_COLOR_LIGHTRED);
//		GUI_DrawRect(FT5336_MAX_X_LENGTH / 2, DISPLAY_AERA1_YPOS, FT5336_MAX_X_LENGTH / 2, DISPLAY_AERAS_HEIGHT * 2, GUI_COLOR_LIGHTBLUE);
//		GUI_FillRect(0, DISPLAY_AERA3_YPOS, FT5336_MAX_X_LENGTH, DISPLAY_AERAS_HEIGHT, GUI_COLOR_ST_GREEN_DARK);
//		GUI_FillRect(0, DISPLAY_AERA5_YPOS, FT5336_MAX_X_LENGTH, DISPLAY_AERAS_HEIGHT, GUI_COLOR_ST_GREEN_DARK);
//
//		for (uint32_t i = 0; i < (FT5336_MAX_X_LENGTH); i++)
//		{
//			GUI_SetPixel(i / 2, (DISPLAY_AERA1_YPOS * 2) + (DISPLAY_AERAS_HEIGHT / 2) + ((synth_GetAudioData(i / 2) << 16 >> 16) / 1024) , GUI_COLOR_LIGHTYELLOW);
//			GUI_SetPixel(i / 2 + (FT5336_MAX_X_LENGTH / 2), (DISPLAY_AERA1_YPOS * 2) + (DISPLAY_AERAS_HEIGHT / 2) + ((synth_GetAudioData(i / 2) >> 16) / 1024) , GUI_COLOR_LIGHTYELLOW);
//			cis_color = cis_GetBuffData((i * ((float)cis_GetEffectivePixelNb() / (float)FT5336_MAX_X_LENGTH)));
//			cis_color = cis_color >> 8;
//			GUI_SetPixel(i, DISPLAY_AERA3_YPOS + DISPLAY_AERAS_HEIGHT - DISPLAY_INTER_AERAS_HEIGHT - (cis_color >> 3) , GUI_COLOR_LIGHTMAGENTA);
//			cis_color |= 0xFF000000;
//			cis_color |= cis_color << 8;
//			cis_color |= cis_color << 16;
//			GUI_DrawLine(i, DISPLAY_AERA4_YPOS + DISPLAY_INTER_AERAS_HEIGHT, i, DISPLAY_AERA4_YPOS + DISPLAY_AERAS_HEIGHT - DISPLAY_INTER_AERAS_HEIGHT, cis_color);
//			cis_color = synth_GetImageData((i * ((float)cis_GetEffectivePixelNb() / (float)FT5336_MAX_X_LENGTH)));
//			cis_color = cis_color >> 11;
//			GUI_SetPixel(i, DISPLAY_AERA5_YPOS + DISPLAY_AERAS_HEIGHT - DISPLAY_INTER_AERAS_HEIGHT - cis_color , GUI_COLOR_LIGHTYELLOW);
//		}
//
//		old_tick = HAL_GetTick();
//	}
	return 0;
}
/**
 * @brief  Display Audio demo hint
 * @param  None
 * @retval None
 */
static void cisynth_imagePlay_SetHint(void)
{
	/* Set Audio header description */
//	GUI_Clear(GUI_COLOR_DARKGRAY);
//	GUI_FillRect(0, 0, FT5336_MAX_X_LENGTH, DISPLAY_HEAD_HEIGHT, GUI_COLOR_DARKRED);
//	GUI_SetTextColor(GUI_COLOR_LIGHTGRAY);
//	GUI_SetBackColor(GUI_COLOR_DARKRED);
//	GUI_SetFont(&Font20);
//	GUI_DisplayStringAt(0, 2, (uint8_t *)"CISYNTH 3", CENTER_MODE);
//	GUI_SetFont(&Font12);
//	GUI_DisplayStringAt(0, 5, (uint8_t *)"ImagePlay", LEFT_MODE);
//	GUI_FillRect(0, DISPLAY_HEAD_HEIGHT, FT5336_MAX_X_LENGTH, DISPLAY_HEAD_HEIGHT + DISPLAY_INTER_AERAS_HEIGHT, GUI_COLOR_DARKGRAY);
}
