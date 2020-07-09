/*
 * synth_v3.c
 *
 *  Created on: May 31, 2020
 *      Author: zhonx
 */

#include "synth.h"
#include "times_base.h"
#include "cis.h"
#include "wave_generation.h"
#include "wave_sommation.h"
#include "config.h"
#include "stdio.h"

extern __IO uint32_t rfft_cnt;
static void cisynth_v3_SetHint(uint32_t Index);

/**
 * @brief  The application entry point.
 * @retval int
 */
int synth_v3(void)
{
	cisynth_v3_SetHint(0);
	uint8_t FreqStr[256] = {0};

	printf("Start CIS Demo\n");

	cisInit();
	synthInit();
	synthTest();

	/* Infinite loop */
	static uint32_t start_tick;
	static uint32_t old_tick;
	uint32_t latency;
	old_tick = HAL_GetTick();

	while (1)
	{
		start_tick = HAL_GetTick();
		while (rfft_cnt < 44100)
		{
			AUDIO_Process();
		}
		rfft_cnt = 0;
		latency = HAL_GetTick() - start_tick;

#ifdef DEBUG_SYNTH
		sprintf((char *)FreqStr, "rfft Frequency = : %d", (int) (44100000 / latency));
		GUI_DisplayStringAt(0, LINE(15), (uint8_t*)FreqStr, CENTER_MODE);
		//			printf("-----------------------------------------\n");
		//			printf("rfft  cnt : %d\n", (int)rfft_cnt);
		rfft_cnt = 0;
		GUI_FillRect(0, 25, LCD_DEFAULT_WIDTH, 150, GUI_COLOR_DARKGRAY);
		for (uint32_t i = 0; i < LCD_DEFAULT_WIDTH; i++)
		{
			GUI_SetPixel(i, 25 + (getBuffData(i) >> 9) , GUI_COLOR_YELLOW);
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
static void cisynth_v3_SetHint(uint32_t Index)
{
	uint32_t x_size, y_size;

	BSP_LCD_GetXSize(0, &x_size);
	BSP_LCD_GetYSize(0, &y_size);

	BSP_LCD_SetBrightness(0, 40);

	/* Clear the LCD */
	GUI_Clear(GUI_COLOR_DARKGRAY);

	/* Set Audio Demo description */
	GUI_FillRect(0, 0, x_size, 24, GUI_COLOR_DARKRED);
	GUI_SetTextColor(GUI_COLOR_LIGHTGRAY);
	GUI_SetBackColor(GUI_COLOR_DARKRED);
	GUI_SetFont(&Font24);
	if(Index == 0)
	{
		GUI_DisplayStringAt(0, 0, (uint8_t *)"CISYNTH DEMO", CENTER_MODE);
		GUI_SetFont(&Font12);
	}
	GUI_SetBackColor(GUI_COLOR_DARKGRAY);
}
