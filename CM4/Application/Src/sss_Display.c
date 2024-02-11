/*
 * sss_Display.c
 *
 *  Created on: May 31, 2020
 *      Author: zhonx
 */

/* Includes ------------------------------------------------------------------*/
#include "stdbool.h"
#include "stdio.h"
#include "stdlib.h"

#include "shared.h"
#include "config.h"

#include "ssd1362.h"
#include "buttons.h"

#include "sss_Display.h"

/* Private includes ----------------------------------------------------------*/

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private typedef -----------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Variable containing black and white frame from CIS*/

/* Private function prototypes -----------------------------------------------*/
static void cis_DisplayOversampling();
static void cisynth_interractiveMenu(void);
static void cis_StartCalibration(void);
static void cis_ChangeOversampling();
static void cis_ChangeScanDir();
static void cisynth_interractiveMenu();

//#define DISPLAY_SCOLL_IMAGE

/* Private user code ---------------------------------------------------------*/

// Structure to store IMU data and moving average
struct IMU_average
{
	float32_t acc[3];  // Moving average of accelerometer
	float32_t gyro[3]; // Moving average of gyroscope
};

struct IMU_average IMU_average = {0};

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

void cis_DisplayOversampling()
{
	uint8_t textData[256] = {0};
	int32_t tick = 0;
	int32_t start_tick = 0;
	int32_t curr_tick = 0;

	int32_t old_process_cnt = shared_var.cis_process_cnt;
	start_tick = HAL_GetTick();
	curr_tick = start_tick;

	while ((HAL_GPIO_ReadPin(SW_3_GPIO_Port, SW_2_Pin) != GPIO_PIN_SET) || (HAL_GetTick() < (start_tick + 200)))
	{
		ssd1362_fillRect(10, 10, 51, 30, 15, false);
		ssd1362_drawRect(9, 9, 52, 31, 0, false);

		if ((shared_var.cis_process_cnt - old_process_cnt) > 50)
		{
			tick = HAL_GetTick();

			shared_var.cis_freq = 1000000 / (((tick - curr_tick) * 1000) / (shared_var.cis_process_cnt - old_process_cnt));
			curr_tick = tick;
			old_process_cnt = shared_var.cis_process_cnt;
		}

		if (shared_var.cis_oversampling < 10)
		{
			sprintf((char *)textData, "OVS %d", (int)shared_var.cis_oversampling);
		}
		else
		{
			sprintf((char *)textData, "OVS%d", (int)shared_var.cis_oversampling);
		}
		ssd1362_drawString(12, 12, (int8_t *)textData, 0, 8);

		if (HAL_GetTick() < (start_tick + 1000))
		{
			sprintf((char *)textData, "---Hz");
		}
		else
		{
			if (shared_var.cis_freq < 100)
				sprintf((char *)textData, "%d Hz", (int)(shared_var.cis_freq));
			else
				sprintf((char *)textData, "%dHz", (int)(shared_var.cis_freq));
		}
		ssd1362_drawString(12, 22, (int8_t*)textData, 0, 8);
		ssd1362_writeUpdates();
	}
}

int sss_Display(void)
{
	uint8_t cis_rgb[3] = {0};
	int32_t cis_color = 0;
	int32_t i = 0;
	int32_t y = 0;
	float32_t packet, index;
	int32_t line_Ypos = DISPLAY_AERA1_Y2POS - (DISPLAY_AERAS1_HEIGHT / 2);
	int32_t pixel_intensity = 0;
	float64_t angle = 0;

	//printf("----- ETHERNET MODE START -----\n");
	//printf("-------------------------------\n");

	shared_var.cis_process_cnt = 0;

	ssd1362_clearBuffer();
	ssd1362_writeFullBuffer();

	if (shared_var.cis_process_cnt == 0)
	{
		while (shared_var.cis_process_cnt == 0)
		{
			ssd1362_drawString(76, 25, (int8_t *)"PLEASE WAIT.", 8, 10);
			ssd1362_writeUpdates();
			HAL_Delay(200);
			if (shared_var.cis_process_cnt != 0)
				break;
			ssd1362_drawString(76, 25, (int8_t *)"PLEASE WAIT..", 8, 10);
			ssd1362_writeUpdates();
			HAL_Delay(200);
			if (shared_var.cis_process_cnt != 0)
				break;
			ssd1362_drawString(76, 25, (int8_t *)"PLEASE WAIT...", 8, 10);
			ssd1362_writeUpdates();
			HAL_Delay(200);
		}
	}

	/* Infinite loop */
	while (1)
	{
		cisynth_interractiveMenu();

		ssd1362_fillRect(0, DISPLAY_AERA1_Y1POS, DISPLAY_WIDTH, DISPLAY_AERA1_Y2POS, 0, false);

		// CIS DISPLAY
		for (i = 0; i < (DISPLAY_WIDTH); i++)
		{
			packet = (float32_t)(i * UDP_NB_PACKET_PER_LINE - 1.0)/(DISPLAY_WIDTH - 1.0);

			index = (packet - (uint32_t)packet) * (CIS_PIXELS_NB / UDP_NB_PACKET_PER_LINE);

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

		ssd1362_writeUpdates();
	}
}

void cis_StartCalibration()
{
	uint8_t textData[256] = {0};
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

			// Print curves
			ssd1362_fillRect(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, 0, false);
			ssd1362_drawString(0, 28, (int8_t *)"  R1 G1 B1  R2 G2 B2  R3 G3 B3", 5, 8);
			for (uint32_t i = 1; i < 9; i++)
			{
				ssd1362_drawVLine(DISPLAY_WIDTH / 9 * i, 0, DISPLAY_HEIGHT, BANNER_BACKGROUND_COLOR, false);
			}
			for (uint32_t i = 0; i < (DISPLAY_WIDTH); i++)
			{
				int32_t cis_color = (int32_t)(cisCals.whiteCal.data[(uint32_t)(i * ((float)(CIS_ADC_BUFF_SIZE * 3) / (float)DISPLAY_WIDTH))]) >> 6;
				ssd1362_drawPixel(DISPLAY_WIDTH - 1 - i, DISPLAY_HEIGHT - cis_color - 1, 14, false);

				cis_color = (int32_t)(cisCals.blackCal.data[(uint32_t)(i * ((float)(CIS_ADC_BUFF_SIZE * 3) / (float)DISPLAY_WIDTH))]) >> 6;
				ssd1362_drawPixel(DISPLAY_WIDTH - 1 - i, DISPLAY_HEIGHT - cis_color -1, 8, false);
			}
			ssd1362_writeFullBuffer();
			while (shared_var.cis_cal_state == CIS_CAL_PLACE_ON_BLACK);
			break;
		case CIS_CAL_EXTRACT_EXTREMUMS :
			ssd1362_fillRect(0, DISPLAY_HEAD_Y2POS, DISPLAY_WIDTH, DISPLAY_HEIGHT, 0, false);
			ssd1362_fillRect(0, DISPLAY_HEAD_Y1POS, DISPLAY_WIDTH, DISPLAY_HEAD_Y2POS, BANNER_BACKGROUND_COLOR, true);
			ssd1362_drawString(0, DISPLAY_HEAD_Y1POS + 1, (int8_t *)" EXTRACT EXTREMUMS AND DELTAS ", 0xF, 8);
			ssd1362_writeFullBuffer();

			//			uint8_t textData[256] = {0};
			sprintf((char *)textData, "WH MIN R%d G%d B%d", (int)cisCals.whiteCal.red.minPix, (int)cisCals.whiteCal.green.minPix, (int)cisCals.whiteCal.blue.minPix);
			ssd1362_drawString(0, 10, (int8_t*)textData, 15, 8);
			sprintf((char *)textData, "WH MAX R%d G%d B%d", (int)cisCals.whiteCal.red.maxPix, (int)cisCals.whiteCal.green.maxPix, (int)cisCals.whiteCal.blue.maxPix);
			ssd1362_drawString(0, 19, (int8_t*)textData, 15, 8);
			sprintf((char *)textData, "WH DLT R%d G%d B%d", (int)cisCals.whiteCal.red.deltaPix, (int)cisCals.whiteCal.green.deltaPix, (int)cisCals.whiteCal.blue.deltaPix);
			ssd1362_drawString(0, 28, (int8_t*)textData, 15, 8);

			sprintf((char *)textData, "BL MIN R%d G%d B%d", (int)cisCals.blackCal.red.minPix, (int)cisCals.blackCal.green.minPix, (int)cisCals.blackCal.blue.minPix);
			ssd1362_drawString(0, 37, (int8_t*)textData, 15, 8);
			sprintf((char *)textData, "BL MAX R%d G%d B%d", (int)cisCals.blackCal.red.maxPix, (int)cisCals.blackCal.green.maxPix, (int)cisCals.blackCal.blue.maxPix);
			ssd1362_drawString(0, 46, (int8_t*)textData, 15, 8);
			sprintf((char *)textData, "BL DLT R%d G%d B%d", (int)cisCals.blackCal.red.deltaPix, (int)cisCals.blackCal.green.deltaPix, (int)cisCals.blackCal.blue.deltaPix);
			ssd1362_drawString(0, 55, (int8_t*)textData, 15, 8);

			ssd1362_writeFullBuffer();
			while (shared_var.cis_cal_state == CIS_CAL_EXTRACT_EXTREMUMS);
			break;
		case CIS_CAL_EXTRACT_OFFSETS :
			/*-------- 3 --------*/
			ssd1362_fillRect(0, DISPLAY_HEAD_Y2POS, DISPLAY_WIDTH, DISPLAY_HEIGHT, 0, false);
			ssd1362_fillRect(0, DISPLAY_HEAD_Y1POS, DISPLAY_WIDTH, DISPLAY_HEAD_Y2POS, BANNER_BACKGROUND_COLOR, true);
			ssd1362_drawString(0, DISPLAY_HEAD_Y1POS + 1, (int8_t *)"EXTRACT DIFFERENTIAL OFFSETS", 0xF, 8);
			ssd1362_drawString(0, 18, (int8_t *)"  R1 G1 B1  R2 G2 B2  R3 G3 B3", 10, 8);
			for (uint32_t i = 1; i < 9; i++)
			{
				ssd1362_drawVLine(DISPLAY_WIDTH / 9 * i, 28, DISPLAY_HEIGHT, BANNER_BACKGROUND_COLOR, false);
			}
			ssd1362_writeFullBuffer();

			// Print curve
			for (uint32_t i = 0; i < (DISPLAY_WIDTH); i++)
			{
				uint32_t index = i * ((float)(CIS_ADC_BUFF_SIZE * 3) / (float)DISPLAY_WIDTH);
				int32_t cis_color = (int32_t)(cisCals.offsetData[index]) >> 6;
				ssd1362_drawPixel(DISPLAY_WIDTH - 1 - i, DISPLAY_HEIGHT - cis_color, 15, false);
			}
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
}

void cis_ChangeOversampling()
{
	static int32_t n = 0;

	n = (n + 1) % 6;
	shared_var.cis_oversampling = 1 << n;

	cis_DisplayOversampling();
}

void cis_ChangeScanDir()
{
	shared_var.cis_scanDir = !shared_var.cis_scanDir;
	shared_var.cis_scanDir = shared_var.cis_scanDir < 0 ? 0 : shared_var.cis_scanDir > 1 ? 1 : shared_var.cis_scanDir;

	ssd1362_screenRotation(shared_var.cis_scanDir);
}

void cisynth_interractiveMenu()
{
	static uint32_t button_tick = 0;
	static uint8_t clear_button = 0;

	if (buttonState[SW1] == SWITCH_PRESSED)
	{
		//ssd1362_fillRect(0 + 10, 60, 10 + 10, 54, 0x0F, false);
		HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_RESET);
		buttonState[SW1] = SWITCH_RELEASED;
		button_tick = HAL_GetTick();
		clear_button = 0;
		cis_StartCalibration();
	}
	if (buttonState[SW2] == SWITCH_PRESSED)
	{
		//ssd1362_fillRect(56 + 10, 60, 56 + 10 + 10, 54, 0x0F, false);
		HAL_GPIO_WritePin(LED3_GPIO_Port, LED3_Pin, GPIO_PIN_RESET);
		buttonState[SW2] = SWITCH_RELEASED;
		button_tick = HAL_GetTick();
		clear_button = 0;
		cis_ChangeOversampling();
	}
	if (buttonState[SW3] == SWITCH_PRESSED)
	{
		//ssd1362_fillRect(56 * 2 + 10, 60, 56 * 2 + 10 + 10, 54, 0x0F, false);
		HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET);
		buttonState[SW3] = SWITCH_RELEASED;
		button_tick = HAL_GetTick();
		clear_button = 0;
		cis_ChangeScanDir();
	}

	if (HAL_GetTick() > (button_tick + BUTTON_DELAY) && clear_button != 1)
	{
		clear_button = 1;

		HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_SET);
		HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_SET);
		HAL_GPIO_WritePin(LED3_GPIO_Port, LED3_Pin, GPIO_PIN_SET);
	}
}

/* Private functions ---------------------------------------------------------*/
