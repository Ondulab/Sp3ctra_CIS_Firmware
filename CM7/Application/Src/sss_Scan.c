/*
 * sss_Scan.c
 *
 *  Created on: May 31, 2020
 *      Author: zhonx
 */

/* Includes ------------------------------------------------------------------*/
#include <sss_Scan.h>
#include "stdbool.h"
#include "stdio.h"

#include "lwip.h"

#include "shared.h"
#include "config.h"

#include "tim.h"
#include "cis.h"
#include "cis_linearCal.h"
#include "cis_polyCal.h"

#include "stm32_flash.h"

#include "udp_client.h"
#include "http_cgi_ssi.h"

#include "lwip.h"
#include "icm42688.h"

#include "sss_Scan.h"


/* Private includes ----------------------------------------------------------*/

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private typedef -----------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
volatile Scan_StateTypeDef main_loop_flg = MAIN_SCAN_LOOP_FLG_RESET;

/* Variable containing black and white frame from CIS*/

/* Private function prototypes -----------------------------------------------*/

extern void Ethernet_Link_Periodic_Handle(struct netif *netif);

/* Private user code ---------------------------------------------------------*/
/**
 * @brief  The application entry point.
 * @retval int
 */
int sss_Scan(void)
{
	printf("----- ETHERNET MODE START -----\n");

#ifndef ETHERNET_OFF
	MX_LWIP_Init();
	udp_clientInit();
	http_server_init();
#endif

	HAL_TIM_Base_Start_IT(&htim6);

	cis_Init();

	shared_var.cis_cal_state = CIS_CAL_END;

	/* Infinite loop */
	while (1)
	{
		MX_LWIP_Process();

		while (main_loop_flg != MAIN_SCAN_LOOP_FLG_SET);
		main_loop_flg = MAIN_SCAN_LOOP_FLG_RESET;

		if (shared_var.cis_cal_state != CIS_CAL_END)
		{
#ifdef POLYNOMIAL_CALIBRATION
			cis_StartCalibration(20); //WIP
#else
			cis_StartLinearCalibration(500);
#endif

#ifdef PRINT_CIS_CALIBRATION
			for (int32_t power_idx = 0; power_idx < 11; power_idx++)
			{
				for (int i = 0; i < 50; i++)
				{
					cis_ConvertRAWImageToRGBImage(&RAWImageCalibration[power_idx], imageData);

					SCB_CleanDCache_by_Addr((uint32_t *)imageData, (CIS_PIXELS_NB * sizeof(uint32_t)));
					udp_clientSendImage(imageData);
				}
			}
			HAL_Delay(2000);
#endif
		}

		cis_ImageProcess(cisDataCpy_f32, packet_Image);
		SCB_CleanDCache_by_Addr((uint32_t *)&packet_Image, sizeof(packet_Image));
		udp_clientSendPackets(packet_Image);

		shared_var.cis_process_cnt++;

		/*
		if ((shared_var.cis_process_cnt % 100) == 0)
		{
			cis_Stop_capture();
			HAL_Delay(100);
			cis_Start_capture();
		}
		*/
	}
}
/* Private functions ---------------------------------------------------------*/

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	if (htim->Instance == TIM6)
	{
		main_loop_flg = MAIN_SCAN_LOOP_FLG_SET;
		icm42688_TIM_Callback();
	}
}
