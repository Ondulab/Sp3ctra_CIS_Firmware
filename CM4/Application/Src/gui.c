/**
 ******************************************************************************
 * @file           : gui.c
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
#include "stdlib.h"
#include "basetypes.h"

#include "globals.h"
#include "config.h"

#include "ssd1362.h"
#include "tim.h"
#include "leds.h"

#include "gui.h"

/* Private define ------------------------------------------------------------*/

/* Private typedef -----------------------------------------------------------*/
// Structure to store IMU data and moving average
struct IMU_average
{
	float32_t acc[3];  // Moving average of accelerometer
	float32_t gyro[3]; // Moving average of gyroscope
};

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

struct IMU_average IMU_average = {0};

/* Variable containing black and white frame from CIS*/

/* Private function prototypes -----------------------------------------------*/
static void gui_displayPopUp();
static void gui_displayImage(void);
static void gui_displayIMU(void);
static void gui_displayWaiting();
static void gui_interractiveMenu(void);
static void gui_startCalibration(void);
static void gui_changeHand(void);

int gui_mainLoop(void)
{
    int32_t last_refresh_tick = HAL_GetTick(); // Initialization of the last refresh tick
    int32_t last_process_count = shared_var.cis_process_cnt; // Last recorded process counter
    memset(&packet_Image, 0, sizeof(packet_Image));

    gui_displayWaiting();
    gui_changeHand();

    /* Infinite loop */
    while (1)
    {
        // Update the interface
        gui_displayImage();
        leds_check_update_state();

        int32_t current_tick = HAL_GetTick(); // Get the current tick

        if ((current_tick - last_refresh_tick) >= 200)
        {
            int32_t current_process_count = shared_var.cis_process_cnt;
            int32_t process_count_diff = current_process_count - last_process_count;

            if (process_count_diff > 0) // Make sure there have been processes since the last refresh
            {
                int32_t tick_diff = current_tick - last_refresh_tick;

                if (tick_diff > 0 && process_count_diff > 0)
                {
                    shared_var.cis_freq = (process_count_diff * 1000000) / (tick_diff * 1000);
                }
            }

            last_refresh_tick = current_tick; // Update the last refresh tick
            last_process_count = current_process_count; // Update the last process counter
        }

#if 0
        static char str[12];
        static int32_t counter = 0;
        counter++;

        sprintf(str, "%lu", counter);
        ssd1362_drawString(0, 0, (int8_t *)str, 15, 8);
#endif

        gui_interractiveMenu();
        gui_displayIMU();
    	ssd1362_writeUpdates();
    }
}

/* Private functions ---------------------------------------------------------*/

void gui_displayImage(void)
{
	uint8_t cis_rgb[3] = {0};
	int32_t cis_color = 0;
	int32_t i = 0;
	int32_t y = 0;
	float32_t packet, index;
	int32_t line_Ypos = DISPLAY_AERA1_Y2POS - (DISPLAY_AERAS1_HEIGHT / 2);
	int32_t pixel_intensity = 0;
	float64_t angle = 0;

	ssd1362_fillRect(0, DISPLAY_AERA1_Y1POS, DISPLAY_WIDTH, DISPLAY_AERA1_Y2POS, 0, false);

	// CIS DISPLAY
	for (i = 0; i < (DISPLAY_WIDTH); i++)
	{
		if (shared_config.cis_dpi > 200)
		{
			packet = (float32_t)(i * UDP_MAX_NB_PACKET_PER_LINE - 1.0)/(DISPLAY_WIDTH - 1.0);
			index = (packet - (uint32_t)packet) * (CIS_400DPI_PIXELS_NB / UDP_MAX_NB_PACKET_PER_LINE);
		}
		else
		{
			packet = (float32_t)(i * (UDP_MAX_NB_PACKET_PER_LINE / 2) - 1.0)/(DISPLAY_WIDTH - 1.0);
			index = (packet - (uint32_t)packet) * (CIS_200DPI_PIXELS_NB / (UDP_MAX_NB_PACKET_PER_LINE / 2));
		}

		cis_rgb[0] = packet_Image[(uint32_t)packet].imageData_R[(uint32_t)index];
		cis_rgb[1] = packet_Image[(uint32_t)packet].imageData_G[(uint32_t)index];
		cis_rgb[2] = packet_Image[(uint32_t)packet].imageData_B[(uint32_t)index];

		// Convert the RGB values to a single brightness value. The numbers 299, 587, and 114
		// are weights given to the R, G, and B components respectively,
		// according to the ITU-R BT.601 standard for converting color to grayscale.
		// This standard assumes that human eyes are less sensitive to the blue component as compared to red and green.
		// Note that cis_rgb[0], cis_rgb[1] and cis_rgb[2] are assumed to be the R, G, B values respectively.
		// cis_color = (299 * (uint32_t)cis_rgb[0]) + 587 * ((uint32_t)cis_rgb[1]) + (114 * (uint32_t)cis_rgb[2]);
		cis_color = cis_rgb[0] + cis_rgb[1] + cis_rgb[2];

		cis_color = cis_color < 0 ? 0 : cis_color > 765 ? 765 : cis_color;

		angle = cis_color * (PI / 2) / 765.00;

		for (y = 0; y < (DISPLAY_AERAS1_HEIGHT / 2); y++)
		{
			if (angle < (PI / 2))
				pixel_intensity = tan(angle) * (y + 1);

			pixel_intensity = pixel_intensity < 0 ? 0 : pixel_intensity > 15 ? 15 : pixel_intensity;

			pixel_intensity = 15 - pixel_intensity;

			// Draw a pixel above the center of the line for symmetry
			ssd1362_drawPixel(i, line_Ypos + y, pixel_intensity, false);

			// Draw a pixel below the center of the line for symmetry
			ssd1362_drawPixel(i, line_Ypos - y, pixel_intensity, false);
		}
	}
}

// Function to update the moving average
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

void gui_displayIMU(void)
{
	// IMU DISPLAY
	static int32_t x1 = 0;
	static int32_t w2 = 0;

	update_IMU_average();

	ssd1362_fillRect(0, DISPLAY_AERA2_Y1POS, DISPLAY_WIDTH, DISPLAY_AERA2_Y2POS, 1, false);
	ssd1362_fillRect(24, DISPLAY_AERA2_Y1POS + 4, 231, DISPLAY_AERA2_Y2POS - 4, 5, false);
	ssd1362_fillRect(0, DISPLAY_AERA2_Y1POS, 22, DISPLAY_AERA2_Y2POS, 5, false);
	ssd1362_fillRect(DISPLAY_WIDTH - 1, DISPLAY_AERA2_Y1POS, DISPLAY_WIDTH - 1 - 22, DISPLAY_AERA2_Y2POS, 5, false);

	//ACC Y
	int32_t accY = (int32_t)(IMU_average.acc[0] * 50);
	if (accY > (DISPLAY_AERAS2_HEIGHT / 2))
		accY = (DISPLAY_AERAS2_HEIGHT / 2);
	if (accY < ((DISPLAY_AERAS2_HEIGHT / 2) * -1))
		accY = (DISPLAY_AERAS2_HEIGHT / 2) * -1;

	x1 = 2;
	w2 = 18;
	ssd1362_fillRect(x1, DISPLAY_AERA2_Y1POS + (DISPLAY_AERAS2_HEIGHT / 2), x1 + w2, DISPLAY_AERA2_Y1POS + (DISPLAY_AERAS2_HEIGHT / 2) + accY, 0, false);

	//GYRO Y
	int32_t gyroY = (int32_t)(IMU_average.gyro[0]);
	if (gyroY > 103)
		gyroY = 103;
	if (gyroY < -103)
		gyroY = -103;

	x1 = DISPLAY_WIDTH / 2;
	w2 = gyroY;

	ssd1362_fillRect(x1, DISPLAY_AERA2_Y1POS + 4, x1 + w2, DISPLAY_AERA2_Y2POS - 4, 13, false);

	//ACC X and ACC Z
	int32_t accZ = (int32_t)(IMU_average.acc[2] * 200);
	const int32_t accZ_RectWith = 25;

	if (accZ > accZ_RectWith)
		accZ = accZ_RectWith;
	if (accZ < (accZ_RectWith * -1))
		accZ = accZ_RectWith * -1;

	int32_t accX = (int32_t)(IMU_average.acc[1] * 200);
	if (accX > 51)
		accX = 51;
	if (accX < -51)
		accX = -51;

	x1 = DISPLAY_WIDTH / 2 - accX;
	w2 = accZ_RectWith - accZ;

	ssd1362_fillRect(x1, DISPLAY_AERA2_Y1POS + 5, x1 + w2, DISPLAY_AERA2_Y2POS - 5, 0, false);
	ssd1362_fillRect(x1 - w2, DISPLAY_AERA2_Y1POS + 5, x1, DISPLAY_AERA2_Y2POS - 5, 0, false);

	//GYRO X
	int32_t gyroX = (int32_t)(IMU_average.gyro[1] / 5);
	x1 = 24;
	w2 = 207;
	if (gyroX < 0)
	{
		gyroX = abs((int)gyroX);
		if ((gyroX) > 8)
			gyroX = 8;
		ssd1362_fillRect(x1, DISPLAY_AERA2_Y1POS + 2, x1 + w2, DISPLAY_AERA2_Y1POS, 7 + gyroX, false);
		ssd1362_fillRect(x1, DISPLAY_AERA2_Y2POS, x1 + w2, DISPLAY_AERA2_Y2POS - 2, 8 - gyroX, false);
	}
	else if (gyroX > 0)
	{
		if ((gyroX) > 8)
			gyroX = 8;
		ssd1362_fillRect(x1, DISPLAY_AERA2_Y2POS, x1 + w2, DISPLAY_AERA2_Y2POS - 2, 7 + gyroX, false);
		ssd1362_fillRect(x1, DISPLAY_AERA2_Y1POS + 2, x1 + w2, DISPLAY_AERA2_Y1POS, 8 - gyroX, false);
	}
	else
	{
		ssd1362_fillRect(x1, DISPLAY_AERA2_Y1POS + 2, x1 + w2, DISPLAY_AERA2_Y1POS, 7, false);
		ssd1362_fillRect(x1, DISPLAY_AERA2_Y2POS, x1 + w2, DISPLAY_AERA2_Y2POS - 2, 7, false);
	}

	int32_t gyroZ = (int32_t)(IMU_average.gyro[2] / 5);
	if (gyroZ > (DISPLAY_AERAS2_HEIGHT / 2))
		gyroZ = (DISPLAY_AERAS2_HEIGHT / 2);
	if (gyroZ < ((DISPLAY_AERAS2_HEIGHT / 2) * -1))
		gyroZ = (DISPLAY_AERAS2_HEIGHT / 2) * -1;

	w2 = 9;
	x1 = 235 + 9;
	ssd1362_fillRect(x1, DISPLAY_AERA2_Y1POS + (DISPLAY_AERAS2_HEIGHT / 2), x1 + w2, DISPLAY_AERA2_Y1POS + (DISPLAY_AERAS2_HEIGHT / 2) + gyroZ, 15, false);
	x1 = 235;
	ssd1362_fillRect(x1, DISPLAY_AERA2_Y1POS + (DISPLAY_AERAS2_HEIGHT / 2), x1 + w2, DISPLAY_AERA2_Y1POS + (DISPLAY_AERAS2_HEIGHT / 2) - gyroZ, 15, false);
}

void gui_displayPopUp()
{
	uint8_t textData[256] = {0};

	ssd1362_fillRect(10, 5, 61, 35, 15, false);
	ssd1362_drawRect(9, 4, 62, 36, 0, false);

    sprintf((char *)textData, "%dDPI", (int)shared_config.cis_dpi);
	ssd1362_drawString(12, 07, (int8_t *)textData, 0, 8);

	if (shared_config.cis_oversampling < 10)
	{
		sprintf((char *)textData, "OVS  %d", (int)shared_config.cis_oversampling);
	}
	else
	{
		sprintf((char *)textData, "OVS %d", (int)shared_config.cis_oversampling);
	}
	ssd1362_drawString(12, 17, (int8_t *)textData, 0, 8);

	if (shared_var.cis_freq < 100)
		sprintf((char *)textData, "%d  Hz", (int)(shared_var.cis_freq));
	else if (shared_var.cis_freq < 1000)
		sprintf((char *)textData, "%d Hz", (int)(shared_var.cis_freq));
	else
		sprintf((char *)textData, "%dHz", (int)(shared_var.cis_freq));

	ssd1362_drawString(12, 27, (int8_t*)textData, 0, 8);
}

void gui_displayWaiting2(void)
{
	static uint32_t lastUpdateTime = 0; // Time of the last display update
	static const uint32_t updateInterval = 1000; // Update display every 1 second
	static int dotCount = 0;
	static char waitMessage[20];

	while (shared_var.cis_process_rdy != TRUE)
	{
		uint32_t currentTime = HAL_GetTick(); // Get current system tick

		// Update display every second
		if (currentTime - lastUpdateTime >= updateInterval)
		{
			lastUpdateTime = currentTime;
			ssd1362_clearBuffer();
			snprintf(waitMessage, sizeof(waitMessage), "PLEASE WAIT%.*s", dotCount + 1, "...");
			ssd1362_drawString(76, 25, (int8_t *)waitMessage, 8, 10);
			//ssd1362_writeUpdates();
			ssd1362_writeFullBuffer();
			dotCount = (dotCount + 1) % 3; // Cycle through 1, 2, 3 dots
		}
	}
	gui_changeHand();
}

// Function to generate a random float between two values
float randomFloat(float min, float max)
{
    return min + ((float)rand() / RAND_MAX) * (max - min);
}

#define NUM_LINE 5

void gui_displayWaiting(void)
{
    static uint32_t lastUpdateTime = 0; // Time of the last display update
    static const uint32_t updateInterval = 50; // Update every 50 ms
    static int offset = 0; // Horizontal offset for the scrolling pattern
    static int lightOffset = 0; // Offset for the faster moving light wave
    static const uint16_t screenWidth = SSD1362_WIDTH; // Screen width (256 pixels)
    static const uint16_t screenHeight = SSD1362_HEIGHT; // Screen height (64 pixels)
    static const uint8_t waveHeight = 8; // Height of each wave

    // Frequency and increment min/max values
    static const float modFreqMin[] = {0.03, 0.01, 0.03, 0.02, 0.04};
    static const float modFreqMax[] = {0.5, 0.3, 0.5, 0.35, 0.45};
    static const float modIncMin[] = {-0.01, -0.01, -0.03, -0.02, -0.04};
    static const float modIncMax[] = {0.15, 0.03, 0.05, 0.08, 0.12};

    // Dynamic modulation frequencies and increments
    static float modFreqLine[NUM_LINE] = {0};
    static float modIncLine[NUM_LINE] = {0};
    for (int i = 0; i < NUM_LINE; i++) {
        modFreqLine[i] = randomFloat(modFreqMin[i], modFreqMax[i]);
        modIncLine[i] = randomFloat(modIncMin[i], modIncMax[i]);
    }

    static const float contrastFreqMin = 0.01;
    static const float contrastFreqMax = 0.05;
    static float contrastFreq = 0;
    contrastFreq = randomFloat(contrastFreqMin, contrastFreqMax);

    static const float contrastIncMin = -0.003;
    static const float contrastIncMax = 0.003;
    static float contrastInc = 0;
    contrastInc = randomFloat(contrastIncMin, contrastIncMax);

    while (shared_var.cis_process_rdy != TRUE)
    {
        uint32_t currentTime = HAL_GetTick();

        if (currentTime - lastUpdateTime >= updateInterval)
        {
            lastUpdateTime = currentTime;
            ssd1362_clearBuffer();

            for (int i = 0; i < NUM_LINE; i++)
            {
                int y = waveHeight * (i + 1) * 2 - 16;

                for (int x = 0; x < screenWidth; x++)
                {
                    int wave = (int)(waveHeight * sin((x + offset) * 0.1));
                    int yPos = y + wave;

                    int modulation = (int)(5 * sin((x + offset) * modFreqLine[i]));
                    int thickness = 1 + abs(modulation);

                    uint8_t contrast = 5 + (uint8_t)(5 * (1 + sin((x + lightOffset * i) * contrastFreq)));
                    contrast = contrast > 15 ? 15 : contrast;

                    if (yPos < screenHeight)
                    {
                        for (int t = -thickness; t <= thickness; t++)
                        {
                            if (yPos + t >= 0 && yPos + t < screenHeight)
                            {
                                ssd1362_drawPixel(x, yPos + t, contrast, false);
                            }
                        }
                    }
                }
            }

            offset = (offset + 1) % screenWidth;
            lightOffset = (lightOffset - 5) % screenWidth;

            for (int i = 0; i < NUM_LINE; i++)
            {
                modFreqLine[i] += modIncLine[i];
                if (modFreqLine[i] > modFreqMax[i] || modFreqLine[i] < modFreqMin[i]) {
                    modFreqLine[i] = randomFloat(modFreqMin[i], modFreqMax[i]);
                }
            }

            contrastFreq += contrastInc;
            if (contrastFreq > contrastFreqMax) contrastFreq = randomFloat(contrastFreqMin, contrastFreqMax);
            if (contrastFreq < contrastFreqMin) contrastFreq = randomFloat(contrastFreqMin, contrastFreqMax);

            ssd1362_writeFullBuffer();
        }
    }
}

void gui_startCalibration()
{
	//uint8_t textData[256] = {0};
	HAL_Delay(100);
	shared_var.cis_cal_state = CIS_CAL_START;

#ifdef POLYNOMIAL_CALIBRATION
	HAL_Delay(200);
	shared_var.cis_cal_state = CIS_CAL_END;
	return;
#endif

	while (shared_var.cis_cal_state != CIS_CAL_END)
	{
		switch (shared_var.cis_cal_state)
		{
		case CIS_CAL_PLACE_ON_WHITE :
			/*-------- 1 --------*/
			ssd1362_fillRect(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, 0, false);
			ssd1362_fillRect(0, DISPLAY_HEAD_Y1POS, DISPLAY_WIDTH, DISPLAY_HEAD_Y2POS, BANNER_BACKGROUND_COLOR, true);
			ssd1362_drawString(0, DISPLAY_HEAD_Y1POS + 1, (int8_t *)" MOVE CIS ON WHITE SURFACE - HL ", 0xF, 8);
			ssd1362_writeFullBuffer();

			while (shared_var.cis_cal_progressbar < 99)
			{
				ssd1362_progressBar(30, 30, shared_var.cis_cal_progressbar, 0xF);
			}

			ssd1362_progressBar(30, 30, 99, 0xF);
			HAL_Delay(10);

			while (shared_var.cis_cal_state == CIS_CAL_PLACE_ON_WHITE);
			break;
		case CIS_CAL_PLACE_ON_BLACK :
			/*-------- 2 --------*/
			ssd1362_fillRect(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, 0, false);
			ssd1362_fillRect(0, DISPLAY_HEAD_Y1POS, DISPLAY_WIDTH, DISPLAY_HEAD_Y2POS, BANNER_BACKGROUND_COLOR, true);
			ssd1362_drawString(0, DISPLAY_HEAD_Y1POS + 1, (int8_t *)" MOVE CIS ON WHITE SURFACE - LL ", 0xF, 8);
			ssd1362_writeFullBuffer();

			while (shared_var.cis_cal_progressbar < 99)
			{
				ssd1362_progressBar(30, 30, shared_var.cis_cal_progressbar, 0xF);
			}

			ssd1362_progressBar(30, 30, 99, 0xF);
			HAL_Delay(100);
			while (shared_var.cis_cal_state == CIS_CAL_PLACE_ON_BLACK);
			break;
		case CIS_CAL_EXTRACT_EXTREMUMS :
			ssd1362_fillRect(0, DISPLAY_HEAD_Y2POS, DISPLAY_WIDTH, DISPLAY_HEIGHT, 0, false);
			ssd1362_fillRect(0, DISPLAY_HEAD_Y1POS, DISPLAY_WIDTH, DISPLAY_HEAD_Y2POS, BANNER_BACKGROUND_COLOR, true);
			ssd1362_drawString(0, DISPLAY_HEAD_Y1POS + 1, (int8_t *)" EXTRACT EXTREMUMS AND DELTAS ", 0xF, 8);
			ssd1362_writeFullBuffer();

			ssd1362_writeFullBuffer();
			while (shared_var.cis_cal_state == CIS_CAL_EXTRACT_EXTREMUMS);
			break;
		case CIS_CAL_EXTRACT_OFFSETS :
			/*-------- 3 --------*/
			ssd1362_fillRect(0, DISPLAY_HEAD_Y2POS, DISPLAY_WIDTH, DISPLAY_HEIGHT, 0, false);
			ssd1362_fillRect(0, DISPLAY_HEAD_Y1POS, DISPLAY_WIDTH, DISPLAY_HEAD_Y2POS, BANNER_BACKGROUND_COLOR, true);
			ssd1362_drawString(0, DISPLAY_HEAD_Y1POS + 1, (int8_t *)"EXTRACT DIFFERENTIAL OFFSETS", 0xF, 8);
			ssd1362_writeFullBuffer();
			while (shared_var.cis_cal_state == CIS_CAL_EXTRACT_OFFSETS);
			break;
		case CIS_CAL_COMPUTE_GAINS :
			/*-------- 4 --------*/
			ssd1362_fillRect(0, DISPLAY_HEAD_Y1POS, DISPLAY_WIDTH, DISPLAY_HEAD_Y2POS, BANNER_BACKGROUND_COLOR, false);
			ssd1362_drawString(0, DISPLAY_HEAD_Y1POS + 1, (int8_t *)" COMPUTE COMPENSATIION GAINS", 0xF, 8);
			ssd1362_writeFullBuffer();
			while (shared_var.cis_cal_state == CIS_CAL_COMPUTE_GAINS);
			break;
		default:
			break;
		}
	}
	ssd1362_clearBuffer();
	ssd1362_writeFullBuffer();
}

void gui_changeHand()
{
	ssd1362_screenRotation(shared_config.cis_handedness);
}

void gui_interractiveMenu()
{
	static uint32_t button_current_tick[NUMBER_OF_BUTTONS] = {0, 0, 0};
	static uint32_t button_initial_tick[NUMBER_OF_BUTTONS] = {0, 0, 0};
	static uint8_t clear_button[NUMBER_OF_BUTTONS] = {0, 0, 0};

	static uint8_t oldScanDir = 0;
	static uint8_t oldOversampling = 0;
	static uint16_t oldDPI = 0;
	static uint32_t start_tick = 0;

	if (shared_var.cis_cal_state == CIS_CAL_REQUESTED)
	{
		gui_startCalibration();
	}

	if (shared_config.cis_oversampling != oldOversampling)
	{
		start_tick = HAL_GetTick();
		oldOversampling = shared_config.cis_oversampling;
	}
	if (shared_config.cis_dpi != oldDPI)
	{
		start_tick = HAL_GetTick();
		oldDPI = shared_config.cis_dpi;
	}
	if ((HAL_GetTick() - start_tick) < 3000)
	{
		gui_displayPopUp();
	}

	if (shared_config.cis_handedness != oldScanDir)
	{
		gui_changeHand();
		oldScanDir = shared_config.cis_handedness;
	}

	for (int i = 0; i < NUMBER_OF_BUTTONS; i++)
	{
		GPIO_TypeDef* button_port = (i == 0) ? SW1_GPIO_Port : (i == 1) ? SW2_GPIO_Port : SW3_GPIO_Port;
		uint16_t button_pin = (i == 0) ? SW1_Pin : (i == 1) ? SW2_Pin : SW3_Pin;

		if (HAL_GPIO_ReadPin(button_port, button_pin) == GPIO_PIN_RESET)
		{
			button_current_tick[i] = HAL_GetTick();
			if (clear_button[i] == 1)
			{
				clear_button[i] = 0;
				button_initial_tick[i] = button_current_tick[i];
			}

			leds_pressFeedback(i, SWITCH_PRESSED);
			shared_var.buttonState[i].state = SWITCH_PRESSED;
			shared_var.buttonState[i].pressed_time = button_current_tick[i] - button_initial_tick[i];
			shared_var.button_update_requested[i] = TRUE;
		}

		if (HAL_GetTick() > (button_current_tick[i] + shared_config.ui_button_delay) && clear_button[i] != 1)
		{
			clear_button[i] = 1;

			leds_pressFeedback(i, SWITCH_RELEASED);
			shared_var.buttonState[i].state = SWITCH_RELEASED;
			shared_var.button_update_requested[i] = TRUE;
		}
	}
}
