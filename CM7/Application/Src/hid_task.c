/**
 ******************************************************************************
 * @file           : hid_task.c
 * @brief          : Buttons + IMU sampling and SLP HID stream (device -> host)
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
 *
 * 1 ms task, decoupled from the CIS line rate:
 *  - kicks one IMU sample per tick (1 kHz = sensor ODR) and publishes the
 *    latest values to the CM4 (screensaver wake-up, IMU strip);
 *  - emits one slp_hid datagram every 1000/hid_rate_hz ms (rate negotiated in
 *    BIND, SLP_DEFAULT_HID_RATE_HZ otherwise) and IMMEDIATELY on any button
 *    edge. Button edge counters come straight from the CM4 debouncer, so the
 *    host can detect edges even when datagrams are lost.
 *
 * RT rules: no logging, no allocation in the loop.
 */
/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include <stdbool.h>

#include "main.h"
#include "cmsis_os2.h"

#include "globals.h"
#include "config.h"
#include "sp3ctra_link.h"
#include "icm42688.h"
#include "udp_client.h"
#include "link_server.h"
#include "hid_task.h"

/* Private define ------------------------------------------------------------*/
#define HID_TASK_STACK_BYTES    (4096)

/* Private variables ---------------------------------------------------------*/
static struct slp_hid hid_msg __attribute__((aligned(4)));
static uint32_t hid_seq = 0;

static const osThreadAttr_t hidTask_attributes = {
    .name = "hidTask",
    .stack_size = HID_TASK_STACK_BYTES,
    .priority = (osPriority_t) osPriorityNormal,
};

/* Private function prototypes -----------------------------------------------*/
static void hidTask(void *argument);

/* Private user code ---------------------------------------------------------*/

static void hidTask(void *argument)
{
    (void)argument;

    uint32_t last_send_tick = HAL_GetTick();
    uint32_t last_seq[NUMBER_OF_BUTTONS] = {0};
    bool     first = true;

    memset(&hid_msg, 0, sizeof(hid_msg));
    hid_msg.hdr.magic    = SLP_MAGIC;
    hid_msg.hdr.version  = SLP_VERSION;
    hid_msg.hdr.type     = SLP_HID;
    hid_msg.hdr.length   = sizeof(hid_msg);
    hid_msg.valid_mask   = SLP_HID_BUTTONS | SLP_HID_ACC | SLP_HID_GYRO | SLP_HID_TEMP;
    hid_msg.button_count = NUMBER_OF_BUTTONS;

    for (;;)
    {
        /* One SPI/DMA sample per tick (the driver converts in the DMA callback). */
        icm42688_TIM_Callback();

        float acc[3], gyr[3];
        icm42688_getAccG(acc);
        icm42688_getGyroDps(gyr);
        const float temp = icm42688_temp();

        /* Publish to the CM4 (cached shared region: clean after writing). */
        shared_imu.acc[0]  = acc[0];
        shared_imu.acc[1]  = acc[1];
        shared_imu.acc[2]  = acc[2];
        shared_imu.gyro[0] = gyr[0];
        shared_imu.gyro[1] = gyr[1];
        shared_imu.gyro[2] = gyr[2];
        shared_imu.temp_c  = temp;
        shared_imu.seq++;
        SCB_CleanDCache_by_Addr((uint32_t *)&shared_imu, sizeof(shared_imu));

        /* Buttons: state + per-button edge counters from the CM4 debouncer. */
        uint8_t state = 0;
        bool    edge  = false;
        for (uint32_t i = 0; i < NUMBER_OF_BUTTONS; i++)
        {
            const uint32_t s = shared_var.button_events[i].sequence_number;
            if (s != last_seq[i])
            {
                last_seq[i] = s;
                if (!first)
                {
                    edge = true;
                }
            }
            if (shared_var.button_events[i].state == SWITCH_PRESSED)
            {
                state |= (uint8_t)(1U << i);
            }
            hid_msg.button_seq[i] = s;
        }
        first = false;

        const uint32_t now  = HAL_GetTick();
        const uint16_t rate = link_getHidRateHz();
        uint32_t period_ms  = (rate != 0U) ? (1000U / rate) : (1000U / SLP_DEFAULT_HID_RATE_HZ);
        if (period_ms == 0U)
        {
            period_ms = 1U;
        }

        if (edge || (now - last_send_tick) >= period_ms)
        {
            last_send_tick = now;

            if (udpClient_isStreaming())
            {
                hid_msg.hdr.seq      = hid_seq++;
                hid_msg.timestamp_us = now * 1000U;
                hid_msg.button_state = state;
                hid_msg.acc[0]  = acc[0];
                hid_msg.acc[1]  = acc[1];
                hid_msg.acc[2]  = acc[2];
                hid_msg.gyro[0] = gyr[0];
                hid_msg.gyro[1] = gyr[1];
                hid_msg.gyro[2] = gyr[2];
                hid_msg.temp_c  = temp;
                (void)udpClient_sendData(&hid_msg, sizeof(hid_msg));
            }
        }

        osDelay(1);
    }
}

HIDTASK_StatusTypeDef hid_taskInit(void)
{
    if (osThreadNew(hidTask, NULL, &hidTask_attributes) == NULL)
    {
        return HIDTASK_ERROR;
    }
    return HIDTASK_OK;
}
