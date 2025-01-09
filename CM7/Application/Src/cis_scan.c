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
// To manage the switch easily, we store a pointer
static QueueHandle_t freeBufferQueue;
static QueueHandle_t readyBufferQueue;

/* Variable containing black and white frame from CIS */

/* Private function prototypes -----------------------------------------------*/
static void cis_userCal(void);
static void cis_scanTask(void *argument);
static void cis_sendTask(void *argument);

/* Private user code ---------------------------------------------------------*/

/**
 * @brief Initializes CIS scanning system and related tasks.
 */
void cis_scanInit(void)
{
    // Create queues for free and ready buffers
    freeBufferQueue = xQueueCreate(2, sizeof(struct packet_Image *));
    readyBufferQueue = xQueueCreate(2, sizeof(struct packet_Image *));

    if (freeBufferQueue == NULL || readyBufferQueue == NULL)
    {
        Error_Handler();
    }

    // Allocate buffer pointers
    struct packet_Image *pBufA = &packet_Image[0];
    struct packet_Image *pBufB = &packet_Image[UDP_MAX_NB_PACKET_PER_LINE];
    xQueueSend(freeBufferQueue, &pBufA, 0);
    xQueueSend(freeBufferQueue, &pBufB, 0);

    // Initialize CIS
    printf("----- CIS INITIALIZATIONS -----\n");

    memset((uint32_t *)&packet_Image, 0, sizeof(packet_Image));
    SCB_CleanDCache_by_Addr((uint32_t *)&packet_Image, sizeof(packet_Image));

    udp_clientInit();
    cis_init();

    shared_var.cis_process_cnt = 0;
    shared_var.cis_process_rdy = TRUE;

    // Create scanning and sending tasks
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

    printf("CIS initialization SUCCESS\n");
}

/**
 * @brief Performs CIS user calibration.
 */
static void cis_userCal(void)
{
    if (shared_var.cis_cal_state != CIS_CAL_END)
    {
        printf("----- CALIBRATION STARTED ------\n");

        printf("CIS calibration state = %d\n", (int)shared_var.cis_cal_state);
        while (shared_var.cis_cal_state != CIS_CAL_START);
#ifdef POLYNOMIAL_CALIBRATION
        cis_StartCalibration(20); // WIP
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

/**
 * @brief CIS scanning task.
 * @param argument Task arguments (not used).
 */
static void cis_scanTask(void *argument)
{
    struct packet_Image *pCurrentBuffer = NULL;

    printf("------ CIS THREAD STARTED ------\n");

    while (1)
    {
        cis_userCal();

        // 1) Retrieve a free buffer (blocks if none available)
        xQueueReceive(freeBufferQueue, &pCurrentBuffer, portMAX_DELAY);

        // 2) Process the image data and fill the buffer
        cis_imageProcess(cisDataCpy_f32, pCurrentBuffer);

        // 3) Notify the send task that the buffer is ready
        xQueueSend(readyBufferQueue, &pCurrentBuffer, portMAX_DELAY);
    }
}

/**
 * @brief CIS sending task.
 * @param argument Task arguments (not used).
 */
static void cis_sendTask(void *argument)
{
    struct packet_Image *pSendBuffer = NULL;

    printf("------ UDP THREAD STARTED ------\n");

    while (1)
    {
        // 1) Wait for a "ready" buffer
        xQueueReceive(readyBufferQueue, &pSendBuffer, portMAX_DELAY);

        // 2) Clean the cache
        SCB_CleanDCache_by_Addr((uint32_t *)pSendBuffer, UDP_MAX_NB_PACKET_PER_LINE * sizeof(struct packet_Image));

        // 3) Send the buffer
        udp_clientSendPackets(pSendBuffer);

        // 4) Return the buffer to the "free" queue
        xQueueSend(freeBufferQueue, &pSendBuffer, portMAX_DELAY);

        shared_var.cis_process_cnt++;
    }
}
