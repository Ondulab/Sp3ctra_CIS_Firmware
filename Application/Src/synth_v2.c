/*
 * synth_v2.c
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
static void cisynth_v2_SetHint(uint32_t Index);

/**
 * @brief  The application entry point.
 * @retval int
 */
int synth_v2(void)
{
	cisynth_v2_SetHint(0);
	uint8_t FreqStr[256] = {0};

	printf("Start CIS Demo\n");

	cis_adc_data[30] = 32000;
	cis_adc_data[10] = 32000;
	cis_adc_data[5] = 32000;
	cis_adc_data[40] = 32000;

	synth_init();
	//	cisInit();

	/* Infinite loop */
	static int old_tick;
	old_tick = HAL_GetTick();

	while (1)
	{
		if (((HAL_GetTick() - old_tick)) < 100)
		{

		}
		else
		{
#ifdef DEBUG_SYNTH
			sprintf((char *)FreqStr, "rfft cnt : %d", (int)rfft_cnt * 10);
			GUI_DisplayStringAt(0, LINE(15), (uint8_t*)FreqStr, CENTER_MODE);
			//			printf("-----------------------------------------\n");
			//			printf("rfft  cnt : %d\n", (int)rfft_cnt);
			rfft_cnt = 0;
#endif
#ifdef DEBUG_CIS
			printf("-----------------------------------------\n");
			printf("CIS cnt   : %d\n", (int)cis_dbg_cnt);
			printf("CIS cal   : %d\n", (int)cis_dbg_data_cal);
			printf("CIS dat   : %d\n", (int)cis_dbg_data);
			printf("ADC [240] : %d\n", (int)cis_adc_data[240]);
			cis_dbg_cnt = 0;
#endif

			GUI_FillRect(0, 25, LCD_DEFAULT_WIDTH, 150, GUI_COLOR_DARKGRAY);
			for (uint32_t i = 0; i < LCD_DEFAULT_WIDTH; i++)
			{
				GUI_SetPixel(i, 25 + (getBuffData(i) >> 9) , GUI_COLOR_YELLOW);
			}

//			static uint32_t idx = 0;
//			++idx;
//
//			cis_adc_data[idx - 1] = 0;
//			cis_adc_data[idx] = 16000;
//
//			if (idx >= 30)
//			{
//				cis_adc_data[idx] = 0;
//				idx = 0;
//			}

			old_tick = HAL_GetTick();
		}
	}
}

/**
 * @brief  Display Audio demo hint
 * @param  None
 * @retval None
 */
static void cisynth_v2_SetHint(uint32_t Index)
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
