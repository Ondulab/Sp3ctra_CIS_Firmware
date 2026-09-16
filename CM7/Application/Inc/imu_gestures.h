/**
 ******************************************************************************
 * @file           : imu_gestures.h
 * @brief          : Device-side gesture detectors (HIT per face / resting FACE)
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

#ifndef __IMU_GESTURES_H__
#define __IMU_GESTURES_H__

#include "stdint.h"
#include "stdbool.h"

/* Wire-facing gesture state: field semantics match slp_hid (u8 wrapping
 * counter, enum slp_face). */
typedef struct
{
    uint8_t face;          /* enum slp_face: the face the device RESTS on */
    uint8_t hit_seq;       /* +1 per HIT, wraps */
    uint8_t hit_velocity;  /* 1..127, strength of the last HIT */
    uint8_t hit_face;      /* enum slp_face: which face was STRUCK
                            * (SLP_FACE_MOVING = shock along the bar) */
} ImuGestureState;

/** Feed one IMU sample. MUST be called at 1 kHz (the HID task tick): every
 *  internal timing constant counts calls as milliseconds. */
void imuGestures_update(const float acc[3], const float gyr[3]);

/** Latest gesture state (single writer: the HID task). */
const ImuGestureState *imuGestures_state(void);

/** True once per new event (hit / face change) - lets the HID task send the
 *  datagram immediately, like a button edge. */
bool imuGestures_takeEvent(void);

#endif /* __IMU_GESTURES_H__ */
