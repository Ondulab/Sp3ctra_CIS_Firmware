/**
 ******************************************************************************
 * @file           : hid_task.h
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
 */
#ifndef __HID_TASK_H__
#define __HID_TASK_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    HIDTASK_OK = 0,
    HIDTASK_ERROR = 1
} HIDTASK_StatusTypeDef;

/** Create the HID task (call after the IMU is initialised). */
HIDTASK_StatusTypeDef hid_taskInit(void);

#ifdef __cplusplus
}
#endif

#endif /* __HID_TASK_H__ */
