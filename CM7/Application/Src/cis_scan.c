/**
 ******************************************************************************
 * @file           : cis_scan.c
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
#include <http_server.h>
#include "stdbool.h"
#include "stdio.h"

#include "lwip.h"

#include "basetypes.h"
#include "globals.h"
#include "config.h"

#include "tim.h"
#include "cis.h"
#include "cis_linearCal.h"
#include "cis_polyCal.h"

#include "udp_client.h"
#include "lwip.h"
#include "icm42688.h"
#include "FreeRTOS.h"
#include "task.h"

#include "cmsis_os.h"

#include "cis_scan.h"


/* Private includes ----------------------------------------------------------*/

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private typedef -----------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
TaskHandle_t cis_scanThreadHandle = NULL;

/* Variable containing black and white frame from CIS*/

/* Private function prototypes -----------------------------------------------*/

static void cis_scanThread(void *arg);
extern void Ethernet_Link_Periodic_Handle(struct netif *netif);

/* Private user code ---------------------------------------------------------*/
/**
 * @brief  The application entry point.
 * @retval int
 */
void cis_scanInit(void)
{
	printf("----- CIS INITIALIZATIONS -----\n");

	memset((uint32_t *)&packet_Image, 0, sizeof(packet_Image));
	SCB_CleanDCache_by_Addr((uint32_t *)&packet_Image, sizeof(packet_Image));

	udp_clientInit();

	cis_init();

	shared_var.cis_process_cnt = 0;
    shared_var.cis_process_rdy = TRUE;

    if (xTaskCreate(cis_scanThread, "cis_thread", 32000, NULL, osPriorityNormal, &cis_scanThreadHandle) == pdPASS) {
        printf("CIS task created successfully.\n");
    } else {
        printf("Failed to create CIS task.\n");
        Error_Handler();
    }
}

/* Private user code ---------------------------------------------------------*/
/**
 * @brief  The application entry point.
 * @retval int
 */
static void cis_scanThread(void *arg)
{
	printf("------ CIS THREAD SARTED -------\n");

    TickType_t xLastWakeTime;
    const TickType_t xFrequency = pdMS_TO_TICKS(1); // Converts approximately 1428.57 ms to ticks

    // Initialize xLastWakeTime with the current tick count.
    xLastWakeTime = xTaskGetTickCount();

#if 0
    osDelay(1000);
	cis_PrintForcharacterization(cisDataCpy_f32);
	while(1);
#endif

	/* Infinite loop */
	while (1)
	{
        vTaskDelayUntil(&xLastWakeTime, xFrequency);

		if (shared_var.cis_cal_state != CIS_CAL_END)
		{
			printf("CIS calibration state = %d\n", (int)shared_var.cis_cal_state);
			while (shared_var.cis_cal_state != CIS_CAL_START);
#ifdef POLYNOMIAL_CALIBRATION
			cis_StartCalibration(20); //WIP
#else
			cis_startLinearCalibration(500, 255);
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
			osDelay(2000);
#endif
		}

		cis_imageProcess(cisDataCpy_f32, packet_Image);

		SCB_CleanDCache_by_Addr((uint32_t *)&packet_Image, sizeof(packet_Image));

		udp_clientSendPackets(packet_Image);

		shared_var.cis_process_cnt++;

		/*
		if ((shared_var.cis_process_cnt % 100) == 0)
		{
			cis_Stop_capture();
			osDelay(100);
			cis_Start_capture();
		}
		 */
	}
}
