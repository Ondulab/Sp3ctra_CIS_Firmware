/**
 ******************************************************************************
 * @file           : gui_imu.c
 * @brief          : GUI IMU module - IMU data display and motion detection
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
#include "stdint.h"
#include "math.h"
#include "string.h"
#include "stdlib.h"

#include "basetypes.h"
#include "globals.h"
#include "gui_config.h"

#include "ssd1362.h"
#include "gui_core.h"
#include "gui_imu.h"

/* Private variables ---------------------------------------------------------*/
struct IMU_average IMU_average = {0};

/* Private functions ---------------------------------------------------------*/

/**
 * @brief Updates the moving average for the IMU sensor data.
 *
 * This function accumulates new accelerometer and gyroscope data from the current
 * IMU packet, updates the rolling sum for each axis, and computes the average over
 * a predefined window size.
 */
void update_IMU_average(void)
{
    static float32_t acc_values[3][WINDOW_IMU_AVERAGE_SIZE] = {{0}};
    static float32_t gyro_values[3][WINDOW_IMU_AVERAGE_SIZE] = {{0}};
    static int32_t imuIndex[3] = {0}; // One index per axis
    static float32_t acc_sum[3] = {0};
    static float32_t gyro_sum[3] = {0};

    for (int i = 0; i < 3; i++)
    {
        // Update the accelerometer
        acc_sum[i] -= acc_values[i][imuIndex[i]];
        acc_sum[i] += packet_IMU.acc[i];
        acc_values[i][imuIndex[i]] = packet_IMU.acc[i];

        // Update the gyroscope
        gyro_sum[i] -= gyro_values[i][imuIndex[i]];
        gyro_sum[i] += packet_IMU.gyro[i];
        gyro_values[i][imuIndex[i]] = packet_IMU.gyro[i];

        // Calculate the averages
        IMU_average.acc[i] = acc_sum[i] / WINDOW_IMU_AVERAGE_SIZE;
        IMU_average.gyro[i] = gyro_sum[i] / WINDOW_IMU_AVERAGE_SIZE;
    }

    // Update the index for the next cycle
    imuIndex[0] = (imuIndex[0] + 1) % WINDOW_IMU_AVERAGE_SIZE;
    imuIndex[1] = (imuIndex[1] + 1) % WINDOW_IMU_AVERAGE_SIZE;
    imuIndex[2] = (imuIndex[2] + 1) % WINDOW_IMU_AVERAGE_SIZE;
}

/**
 * @brief Displays the IMU data on the screen.
 *
 * Uses the averaged IMU data to render graphs and visual indicators of the sensor
 * values (accelerometer and gyroscope) on the designated area of the display.
 */
void gui_displayIMU(void)
{
    // IMU DISPLAY
    static int32_t x1 = 0;
    static int32_t w2 = 0;

    // Get dynamic area positions
    uint32_t area2_y1pos = GUI_GET_AREA2_Y1POS();
    uint32_t area2_y2pos = GUI_GET_AREA2_Y2POS();
    uint32_t area2_height = GUI_GET_AREA2_HEIGHT();

    // Don't display IMU if area height is 0
    if (area2_height == 0) {
        return;
    }

    update_IMU_average();

    ssd1362_fillRect(0, area2_y1pos, DISPLAY_WIDTH, area2_y2pos, 1, false);
    ssd1362_fillRect(24, area2_y1pos + 4, 231, area2_y2pos - 4, 5, false);
    ssd1362_fillRect(0, area2_y1pos, 22, area2_y2pos, 5, false);
    ssd1362_fillRect(DISPLAY_WIDTH - 1, area2_y1pos, DISPLAY_WIDTH - 1 - 22, area2_y2pos, 5, false);

    //ACC Y (adjusted for ±4g range and calibrated values)
    int32_t accY = (int32_t)(IMU_average.acc[0] * 25);  // Reduced from 50 to 25 for ±4g range
    if (accY > (int32_t)(area2_height / 2))
        accY = (int32_t)(area2_height / 2);
    if (accY < ((int32_t)(area2_height / 2) * -1))
        accY = (int32_t)(area2_height / 2) * -1;

    x1 = 2;
    w2 = 18;
    ssd1362_fillRect(x1, area2_y1pos + (area2_height / 2), x1 + w2, area2_y1pos + (area2_height / 2) + accY, 0, false);

    //GYRO Y
    int32_t gyroY = (int32_t)(IMU_average.gyro[0]);
    if (gyroY > 103)
        gyroY = 103;
    if (gyroY < -103)
        gyroY = -103;

    x1 = DISPLAY_WIDTH / 2;
    w2 = gyroY;

    ssd1362_fillRect(x1, area2_y1pos + 4, x1 + w2, area2_y2pos - 4, 13, false);

    //ACC X and ACC Z (values in m/s² from driver)
    // For Z: expect ~9.81 m/s² at rest, amplify variations around gravity for visible animation
    // Scaling: ±1 m/s² → ±25 pixels (sensitive to 0.04g variations)
    int32_t accZ = (int32_t)((IMU_average.acc[2] + 9.81f) * 25);  // Variations around 9.81 m/s²
    const int32_t accZ_RectWith = 25;

    if (accZ > accZ_RectWith)
        accZ = accZ_RectWith;
    if (accZ < (accZ_RectWith * -1))
        accZ = accZ_RectWith * -1;

    // For X: expect ~±0.002g, so increase scaling for better visibility
    int32_t accX = (int32_t)(IMU_average.acc[1] * 100);  // Reduced from 200 to 100 for ±4g range
    if (accX > 51)
        accX = 51;
    if (accX < -51)
        accX = -51;

    x1 = DISPLAY_WIDTH / 2 - accX;
    w2 = accZ_RectWith - accZ;

    ssd1362_fillRect(x1, area2_y1pos + 5, x1 + w2, area2_y2pos - 5, 0, false);
    ssd1362_fillRect(x1 - w2, area2_y1pos + 5, x1, area2_y2pos - 5, 0, false);

    //GYRO X
    int32_t gyroX = (int32_t)(IMU_average.gyro[1] / 5);
    x1 = 24;
    w2 = 207;
    if (gyroX < 0)
    {
        gyroX = abs((int)gyroX);
        if ((gyroX) > 8)
            gyroX = 8;
        ssd1362_fillRect(x1, area2_y1pos + 2, x1 + w2, area2_y1pos, 7 + gyroX, false);
        ssd1362_fillRect(x1, area2_y2pos, x1 + w2, area2_y2pos - 2, 8 - gyroX, false);
    }
    else if (gyroX > 0)
    {
        if ((gyroX) > 8)
            gyroX = 8;
        ssd1362_fillRect(x1, area2_y2pos, x1 + w2, area2_y2pos - 2, 7 + gyroX, false);
        ssd1362_fillRect(x1, area2_y1pos + 2, x1 + w2, area2_y1pos, 8 - gyroX, false);
    }
    else
    {
        ssd1362_fillRect(x1, area2_y1pos + 2, x1 + w2, area2_y1pos, 7, false);
        ssd1362_fillRect(x1, area2_y2pos, x1 + w2, area2_y2pos - 2, 7, false);
    }

    int32_t gyroZ = (int32_t)(IMU_average.gyro[2] / 5);
    if (gyroZ > (int32_t)(area2_height / 2))
        gyroZ = (int32_t)(area2_height / 2);
    if (gyroZ < ((int32_t)(area2_height / 2) * -1))
        gyroZ = (int32_t)(area2_height / 2) * -1;

    w2 = 9;
    x1 = 235 + 9;
    ssd1362_fillRect(x1, area2_y1pos + (area2_height / 2), x1 + w2, area2_y1pos + (area2_height / 2) + gyroZ, 15, false);
    x1 = 235;
    ssd1362_fillRect(x1, area2_y1pos + (area2_height / 2), x1 + w2, area2_y1pos + (area2_height / 2) - gyroZ, 15, false);
}

/**
 * @brief Detects significant motion from IMU data.
 *
 * Compares current IMU values with previous ones to determine if there's
 * intentional movement above the noise threshold.
 *
 * @return bool True if significant motion is detected, false otherwise.
 */
bool gui_isSignificantMotion(void)
{
    static float last_acc[3] = {0.0f, 0.0f, 0.0f};
    static float last_gyro[3] = {0.0f, 0.0f, 0.0f};
    static bool first_run = true;

    // Skip first run to initialize baseline values
    if (first_run) {
        memcpy(last_acc, (const void*)packet_IMU.acc, sizeof(last_acc));
        memcpy(last_gyro, (const void*)packet_IMU.gyro, sizeof(last_gyro));
        first_run = false;
        return false;
    }

    // Calculate delta for accelerometer
    float acc_delta = fabsf(packet_IMU.acc[0] - last_acc[0]) +
                      fabsf(packet_IMU.acc[1] - last_acc[1]) +
                      fabsf(packet_IMU.acc[2] - last_acc[2]);

    // Calculate delta for gyroscope
    float gyro_delta = fabsf(packet_IMU.gyro[0] - last_gyro[0]) +
                       fabsf(packet_IMU.gyro[1] - last_gyro[1]) +
                       fabsf(packet_IMU.gyro[2] - last_gyro[2]);

    // Update last values for next comparison
    memcpy(last_acc, (const void*)packet_IMU.acc, sizeof(last_acc));
    memcpy(last_gyro, (const void*)packet_IMU.gyro, sizeof(last_gyro));

    // Return true if motion exceeds thresholds (use shared_config values)
    return (acc_delta > shared_config.motion_threshold_acc || gyro_delta > shared_config.motion_threshold_gyro);
}
