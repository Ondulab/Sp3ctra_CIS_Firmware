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

/**
  * @brief  The application entry point.
  * @retval int
  */
int synth_v2(void)
{
	HAL_EnableCompensationCell();
	printf("Start CIS Demo\n");
	synth_init();
	cisInit();

  /* Infinite loop */
	static int old_tick;
	old_tick = HAL_GetTick();
	//	for (uint32_t idx = 0; idx < CIS_PIXELS_NB; idx++)
	//	{
	//		cis_adc_data[idx] = 65534;
	//	}
	while (1)
	{
		if (((HAL_GetTick() - old_tick)) < 1000)
		{
#ifdef DEBUG_SAMPLE_RATE
			HAL_TIM_PeriodElapsedCallback(&htim15);
#endif

		}
		else
		{
#ifdef DEBUG_SYNTH
			printf("-----------------------------------------\n");
			printf("rfft  cnt : %d\n", (int)rfft_cnt);
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

			old_tick = HAL_GetTick();
		}
	}
}
