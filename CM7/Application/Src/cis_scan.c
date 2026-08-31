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
#include "task.h"
#include "queue.h"
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
/* Web preview (GET /scan.bin, see cis_scan.h). A page that stops polling stops
 * the publishing after CIS_PREVIEW_ARM_MS. The ring is sized in bytes: at 1/4
 * resolution it holds a dozen lines (half a second of viewing at 25 polls/s),
 * at full resolution only three -- resolution is paid for in line rate. */
#define CIS_PREVIEW_ARM_MS      2000
#define CIS_PREVIEW_POOL_BYTES  (32 * 1024)
#define CIS_PREVIEW_MAX_SLOTS   64

/* Private variables ---------------------------------------------------------*/
// To manage the switch easily, we store a pointer
static QueueHandle_t freeBufferQueue;
static QueueHandle_t readyBufferQueue;

/* The ring. Single writer (cis_sendTask), single reader (http_thread), no lock:
 * see the contract in cis_scan.h. */
static uint8_t previewPool[CIS_PREVIEW_POOL_BYTES];
static volatile uint16_t previewPixels = 0;      /* pixels kept per line */
static volatile uint16_t previewLineBytes = 0;   /* 3 * previewPixels */
static volatile uint16_t previewSlots = 0;       /* lines the pool holds */
static volatile uint32_t previewHead = 0;        /* id of the next line written */
static volatile uint32_t previewOldest = 0;      /* oldest id still in the pool */
static volatile uint32_t previewDeadline = 0;    /* armed until this tick */
static volatile uint8_t previewDecim = 4;        /* asked for by the client */
static volatile uint16_t previewRate = 25;       /* lines per second asked for */
static volatile uint16_t previewPubRate = 0;     /* lines per second published */
static uint8_t previewSetDecim = 0;              /* decimation the pool is laid out for */
static int32_t previewSetPixels = 0;             /* cis pixel count it was laid out for */
static uint32_t previewDiv = 1;                  /* publish one line out of this many */
static uint32_t previewSeen = 0;

/* Private function prototypes -----------------------------------------------*/
static void cis_start_MDMA_Transfer(uint32_t *src, uint32_t *dst, uint32_t length);
static void cis_userCal(void);
static void cis_scanTask(void *argument);
static void cis_sendTask(void *argument);
static void cis_publishPreview(const struct slp_line_cis *line);

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
    freeBufferQueue = xQueueCreate(2, sizeof(struct slp_line_cis *));
    readyBufferQueue = xQueueCreate(2, sizeof(struct slp_line_cis *));

    if (freeBufferQueue == NULL || readyBufferQueue == NULL)
    {
    	printf("Failed to createqueue\n");
    	return CISSCAN_ERROR;
    }

    // Allocate buffer pointers
    struct slp_line_cis *pBufA = buffers_Scanline.scanline_buff1;
    struct slp_line_cis *pBufB = buffers_Scanline.scanline_buff2;
    xQueueSend(freeBufferQueue, &pBufA, 0);
    xQueueSend(freeBufferQueue, &pBufB, 0);

    if (cis_init() != CIS_OK)
    {
    	return CISSCAN_ERROR;
    }

    shared_var.cis_process_cnt = 0;
    shared_var.cis_process_rdy = TRUE;

    // Create scanning and sending tasks
    // Priorities are raw FreeRTOS values (CMSIS-RTOS v2 enums: Low=8 < Normal=24 = tcpip_thread).
    // cis_scanTask busy-waits on the ADC DMA in cis_imageProcess(), so it MUST stay below
    // tcpip_thread/http_thread or it starves the network stack (it was priority 2 with CMSIS v1).
    if (xTaskCreate(cis_scanTask, "cis_scanTask", 4096, NULL, osPriorityLow, NULL) != pdPASS)
    {
        printf("Failed to create cis_scanTask.\n");
        return CISSCAN_ERROR;
    }

    if (xTaskCreate(cis_sendTask, "cis_sendTask", 4096, NULL, osPriorityNormal, NULL) != pdPASS)
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
        cis_startLinearCalibration(cisDataCpy, 1000, 255);
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
    struct slp_line_cis *pCurrentBuffer = NULL;

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
    struct slp_line_cis *pSendBuffer = NULL;

    while (1)
    {
        // 1) Wait for a "ready" buffer
        xQueueReceive(readyBufferQueue, &pSendBuffer, portMAX_DELAY);

        // 2) Clean the cache
        SCB_CleanDCache_by_Addr((uint32_t *)pSendBuffer, UDP_MAX_NB_PACKET_PER_LINE * sizeof(struct slp_line_cis));

        // 3) Start MDMA transfer for CM4 display
        cis_start_MDMA_Transfer((uint32_t *)pSendBuffer, (uint32_t *)scanline_CM4, UDP_MAX_NB_PACKET_PER_LINE * sizeof(struct slp_line_cis));

        // 4) Send the buffer
        udpClient_sendPackets(pSendBuffer);

        // 4b) Publish a copy for the embedded web viewer (no-op when unwatched)
        cis_publishPreview(pSendBuffer);

        // 5) Return the buffer to the "free" queue
        xQueueSend(freeBufferQueue, &pSendBuffer, portMAX_DELAY);

        shared_var.cis_process_cnt++;
    }
}

/**
 * @brief Lay the pool out for a decimation: slot size, and how many fit.
 */
static void cis_previewConfigure(uint8_t dec)
{
    const uint16_t pixels = (uint16_t)(cisConfig.pixels_nb / dec);
    const uint16_t bytes = (uint16_t)(3U * pixels);
    uint16_t slots = (uint16_t)(CIS_PREVIEW_POOL_BYTES / bytes);

    if (slots > CIS_PREVIEW_MAX_SLOTS)
    {
        slots = CIS_PREVIEW_MAX_SLOTS;
    }

    previewPixels = pixels;
    previewLineBytes = bytes;
    previewSlots = slots;
    previewSetDecim = dec;
    previewSetPixels = cisConfig.pixels_nb;
    /* What the pool holds no longer matches the new layout. */
    previewOldest = previewHead;
    previewSeen = 0;
}

/**
 * @brief Publish the line just sent, if a page is watching and it is this
 *        line's turn (the client asks for a rate, the sensor runs at its own).
 * @param line Array of cisConfig.udp_nb_packet_per_line fragments.
 */
static void cis_publishPreview(const struct slp_line_cis *line)
{
    if ((int32_t)(HAL_GetTick() - previewDeadline) >= 0)
    {
        return; /* nobody polled recently */
    }

    const uint8_t dec = previewDecim;
    if (dec != previewSetDecim || cisConfig.pixels_nb != previewSetPixels)
    {
        cis_previewConfigure(dec);
    }

    if (++previewSeen < previewDiv)
    {
        return;
    }
    previewSeen = 0;

    uint8_t *dst = previewPool + (previewHead % previewSlots) * previewLineBytes;

    for (int32_t packet = 0; packet < cisConfig.udp_nb_packet_per_line; packet++)
    {
        for (int32_t i = 0; i < UDP_LINE_FRAGMENT_SIZE; i += dec)
        {
            *dst++ = line[packet].r[i];
            *dst++ = line[packet].g[i];
            *dst++ = line[packet].b[i];
        }
    }

    previewHead++;
    if ((previewHead - previewOldest) > previewSlots)
    {
        previewOldest = previewHead - previewSlots;
    }
}

void cisScan_requestPreview(uint8_t decimation, uint16_t rateHz)
{
    if (decimation < 1U)
    {
        decimation = 1U;
    }
    else if (decimation > 8U)
    {
        decimation = 8U;
    }
    if (rateHz < 1U)
    {
        rateHz = 1U;
    }

    /* One line out of `div`: at 1000 lines/s, 250 lines/s means every fourth. */
    uint32_t lps = (uint32_t)shared_var.cis_freq;
    if (lps < 1U)
    {
        lps = 1U;
    }
    uint32_t div = lps / rateHz;
    if (div < 1U)
    {
        div = 1U;
    }

    previewDecim = decimation;
    previewRate = rateHz;
    previewDiv = div;
    previewPubRate = (uint16_t)(lps / div);
    previewDeadline = HAL_GetTick() + CIS_PREVIEW_ARM_MS;
}

uint16_t cisScan_previewBatch(uint32_t since, uint32_t *firstId, uint16_t *pixels,
                              uint16_t *dropped, uint16_t *rateHz)
{
    const uint32_t head = previewHead;
    const uint32_t oldest = previewOldest;
    const uint16_t slots = previewSlots;

    *pixels = previewPixels;
    *rateHz = previewPubRate;
    *dropped = 0;
    *firstId = head;

    if (slots == 0U || head == 0U)
    {
        return 0; /* nothing published yet */
    }

    uint32_t start;
    if (since == 0U || (int32_t)(since - head) > 0)
    {
        start = head - 1U; /* a fresh page starts on the newest line, not on history */
    }
    else if ((int32_t)(since - oldest) < 0)
    {
        const uint32_t lost = oldest - since;   /* the ring lapped this client */
        *dropped = (uint16_t)(lost > 0xFFFFU ? 0xFFFFU : lost);
        start = oldest;
    }
    else
    {
        start = since;
    }

    uint32_t count = head - start;
    if (count > slots)
    {
        count = slots;
        start = head - count;
    }

    *firstId = start;
    return (uint16_t)count;
}

const uint8_t *cisScan_previewLine(uint32_t lineId)
{
    if (previewSlots == 0U)
    {
        return NULL;
    }
    return previewPool + (lineId % previewSlots) * previewLineBytes;
}
