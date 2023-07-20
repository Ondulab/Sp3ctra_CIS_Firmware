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

#include "udp_client.h"
#include "lwip.h"

#include "shared.h"
#include "config.h"

#include "tim.h"
#include "cis.h"
#include "cis_linearCal.h"
#include "cis_polyCal.h"


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
	printf("-------------------------------\n");

#ifndef ETHERNET_OFF
	MX_LWIP_Init();
	udp_clientInit();
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
			//cis_StartLinearCalibration(500);
			//cis_StartPolynomialCalibration(500);
			//cis_calibrateLeds();
		}

		cis_ImageProcessRGB_2(imageData);
		SCB_CleanDCache_by_Addr((uint32_t *)imageData, (CIS_PIXELS_NB * sizeof(uint32_t)));

#ifndef ETHERNET_OFF
		udp_clientSendImage(imageData);
#endif

		shared_var.cis_process_cnt++;

//		if (shared_var.cis_process_cnt % 100 == 0)
//		{
//			static uint32_t sw = 0;
//			sw++;
//			if (sw % 2)
////				cis_LedsOff();
//				cis_Stop_capture();
//			else
////				cis_LedsOn();
//				cis_Start_capture();
//		}

	}
}
/* Private functions ---------------------------------------------------------*/

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	if (htim->Instance == TIM6)
	{
		main_loop_flg = MAIN_SCAN_LOOP_FLG_SET;
	}
}
