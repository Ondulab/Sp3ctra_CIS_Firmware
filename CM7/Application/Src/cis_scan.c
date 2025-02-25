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
#include "stdbool.h"
#include "stdio.h"

#include "FreeRTOS.h"
#include "lwip.h"

#include "mdma.h"

#include "basetypes.h"
#include "globals.h"
#include "config.h"

#include "cis.h"
#include "cis_linearCal.h"
#include "udp_client.h"

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
static void cis_start_MDMA_Transfer(uint32_t *src, uint32_t *dst, uint32_t length);
static void cis_userCal(void);
static void cis_scanTask(void *argument);
static void cis_sendTask(void *argument);

/* Private user code ---------------------------------------------------------*/

/**
 * @brief Start an MDMA transfer with interrupt.
 * @param src Pointer to the source address.
 * @param dst Pointer to the destination address.
 * @param length Transfer length in bytes.
 */
static void cis_start_MDMA_Transfer(uint32_t *src, uint32_t *dst, uint32_t length)
{
    /* Start the MDMA transfer */
    HAL_MDMA_Start_IT(&hmdma_mdma_channel0_sw_0, (uint32_t)src, (uint32_t)dst, length, 1);
}

/**
 * @brief Initializes CIS scanning system and related tasks.
 */
CISSCAN_StatusTypeDef cis_scanInit(void)
{
    // Create queues for free and ready buffers
    freeBufferQueue = xQueueCreate(2, sizeof(struct packet_Scanline *));
    readyBufferQueue = xQueueCreate(2, sizeof(struct packet_Scanline *));

    if (freeBufferQueue == NULL || readyBufferQueue == NULL)
    {
    	printf("Failed to createqueue\n");
    	return CISSCAN_ERROR;
    }

    // Allocate buffer pointers
    struct packet_Scanline *pBufA = buffers_Scanline.scanline_buff1;
    struct packet_Scanline *pBufB = buffers_Scanline.scanline_buff2;
    xQueueSend(freeBufferQueue, &pBufA, 0);
    xQueueSend(freeBufferQueue, &pBufB, 0);

    if (cis_init() != CIS_OK)
    {
    	return CISSCAN_ERROR;
    }

    shared_var.cis_process_cnt = 0;
    shared_var.cis_process_rdy = TRUE;

    // Create scanning and sending tasks
    if (xTaskCreate(cis_scanTask, "cis_scanTask", 4096, NULL, osPriorityHigh, NULL) != pdPASS)
    {
        printf("Failed to create cis_scanTask.\n");
        return CISSCAN_ERROR;
    }

    if (xTaskCreate(cis_sendTask, "cis_sendTask", 4096, NULL, osPriorityRealtime, NULL) != pdPASS)
    {
        printf("Failed to create cis_sendTask.\n");
        return CISSCAN_ERROR;
    }

    return CISSCAN_OK;
}

/**
 * @brief Performs CIS user calibration.
 */
static void cis_userCal(void)
{
    if (shared_var.cis_cal_state != CIS_CAL_END)
    {
        while (shared_var.cis_cal_state != CIS_CAL_START)
        {
        	 osDelay(1);
        }
#ifdef POLYNOMIAL_CALIBRATION
        cis_StartCalibration(20); // WIP
#else
        cis_startLinearCalibration(cisDataCpy, 500, 255);
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
    struct packet_Scanline *pCurrentBuffer = NULL;

    while (1)
    {
        cis_userCal();

        // 1) Retrieve a free buffer (blocks if none available)
        xQueueReceive(freeBufferQueue, &pCurrentBuffer, portMAX_DELAY);

        // 2) Process the image data and fill the buffer
        cis_imageProcess(cisDataCpy, pCurrentBuffer);

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
    struct packet_Scanline *pSendBuffer = NULL;

    while (1)
    {
        // 1) Wait for a "ready" buffer
        xQueueReceive(readyBufferQueue, &pSendBuffer, portMAX_DELAY);

        // 2) Clean the cache
        SCB_CleanDCache_by_Addr((uint32_t *)pSendBuffer, UDP_MAX_NB_PACKET_PER_LINE * sizeof(struct packet_Scanline));

        // 3) Start MDMA transfer for CM4 display
        cis_start_MDMA_Transfer((uint32_t *)pSendBuffer, (uint32_t *)scanline_CM4, UDP_MAX_NB_PACKET_PER_LINE * sizeof(struct packet_Scanline));

        // 4) Send the buffer
        udpClient_sendPackets(pSendBuffer);

        // 5) Return the buffer to the "free" queue
        xQueueSend(freeBufferQueue, &pSendBuffer, portMAX_DELAY);

        shared_var.cis_process_cnt++;
    }
}
