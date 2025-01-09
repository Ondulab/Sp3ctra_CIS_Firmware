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
#include "FreeRTOS.h"

#include "cmsis_os.h"

#include "cis_scan.h"


/* Private includes ----------------------------------------------------------*/

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private typedef -----------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
TaskHandle_t cis_scanThreadHandle = NULL;


// Pour gérer facilement la bascule, on stocke un pointeur
static QueueHandle_t freeBufferQueue;
static QueueHandle_t readyBufferQueue;

/* Variable containing black and white frame from CIS*/

/* Private function prototypes -----------------------------------------------*/

extern void Ethernet_Link_Periodic_Handle(struct netif *netif);

static void cis_userCal(void);

static void cis_scanTask(void *argument);
static void cis_sendTask(void *argument);

/* Private user code ---------------------------------------------------------*/
void cis_scanInit(void)
{
	freeBufferQueue = xQueueCreate(2, sizeof(struct packet_Image *));
	readyBufferQueue = xQueueCreate(2, sizeof(struct packet_Image *));

	if (freeBufferQueue == NULL || readyBufferQueue == NULL)
	{
		Error_Handler();
	}

	struct packet_Image *pBufA = &packet_Image[0];
	struct packet_Image *pBufB = &packet_Image[UDP_MAX_NB_PACKET_PER_LINE];
	xQueueSend(freeBufferQueue, &pBufA, 0);
	xQueueSend(freeBufferQueue, &pBufB, 0);

	// Initialisation du CIS
	printf("----- CIS INITIALIZATIONS -----\n");

	memset((uint32_t *)&packet_Image, 0, sizeof(packet_Image));
	SCB_CleanDCache_by_Addr((uint32_t *)&packet_Image, sizeof(packet_Image));

	udp_clientInit();
	cis_init();

	shared_var.cis_process_cnt = 0;
	shared_var.cis_process_rdy = TRUE;

	// Création des tâches
	if (xTaskCreate(cis_scanTask, "cis_scanTask", 32768, NULL, osPriorityHigh, NULL) != pdPASS)
	{
		printf("Failed to create cis_scanTask.\n");
		Error_Handler();
	}

	if (xTaskCreate(cis_sendTask, "cis_sendTask", 4096, NULL, osPriorityRealtime, NULL) != pdPASS)
	{
		printf("Failed to create cis_sendTask.\n");
		Error_Handler();
	}

#if 0
	if (xTaskCreate(cis_scanThread, "cis_thread", 32768, NULL, osPriorityNormal, &cis_scanThreadHandle) == pdPASS) {
		printf("CIS initialization SUCCESS\n");
	} else {
		printf("Failed to create CIS task.\n");
		Error_Handler();
	}
#endif

	printf("CIS initialization SUCCESS\n");
}

/* Private user code ---------------------------------------------------------*/
/**
 * @brief  The application entry point.
 * @retval int
 */
//static void cis_scanThread(void *arg)
static void cis_userCal(void)
{
	if (shared_var.cis_cal_state != CIS_CAL_END)
	{
		printf("----- CALIBRATION SARTED ------\n");

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
}

static void cis_scanTask(void *argument)
{
	struct packet_Image *pCurrentBuffer = NULL;

	printf("------ CIS THREAD SARTED ------\n");

	while (1)
	{
		cis_userCal();

		// 1) Récupérer un buffer libre (bloque si aucun disponible)
		xQueueReceive(freeBufferQueue, &pCurrentBuffer, portMAX_DELAY);

		// 2) Remplir les UDP_MAX_NB_PACKET_PER_LINE lignes
		//    On imagine que cis_imageProcess() peut remplir TOUT le bloc
		//    ou alors on fait un for(...) pour remplir chaque ligne.
		cis_imageProcess(cisDataCpy_f32, pCurrentBuffer);

		// 4) Annoncer à la tâche d'envoi que le buffer est prêt
		xQueueSend(readyBufferQueue, &pCurrentBuffer, portMAX_DELAY);

		// 5) Incrémenter un compteur pour info
		//shared_var.cis_process_cnt++;
	}
}


static void cis_sendTask(void *argument)
{
	struct packet_Image *pSendBuffer = NULL;

	printf("------ UDP THREAD SARTED ------\n");

	while (1)
	{
		// 1) Attendre un buffer "prêt"
		xQueueReceive(readyBufferQueue, &pSendBuffer, portMAX_DELAY);

		// 3) Nettoyer le cache
		SCB_CleanDCache_by_Addr((uint32_t *)pSendBuffer, UDP_MAX_NB_PACKET_PER_LINE * sizeof(struct packet_Image));

		// 2) Envoyer ce buffer
		udp_clientSendPackets(pSendBuffer);

		// 3) Remettre ce buffer dans la file "libres"
		xQueueSend(freeBufferQueue, &pSendBuffer, portMAX_DELAY);

		shared_var.cis_process_cnt++;
	}
}
