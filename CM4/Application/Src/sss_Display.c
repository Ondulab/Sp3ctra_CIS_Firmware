/*
 * sss_Display.c
 *
 *  Created on: May 31, 2020
 *      Author: zhonx
 */

/* Includes ------------------------------------------------------------------*/
#include "stdbool.h"
#include "stdio.h"

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
static void cisynth_SetHint(void);
static void cisynth_interractiveMenu(void);
void cis_StartCalibration(void);

//#define DISPLAY_SCOLL_IMAGE

/* Private user code ---------------------------------------------------------*/
void cis_DisplayOversampling()
{
	uint8_t textData[256] = {0};

	ssd1362_drawRect(208, DISPLAY_HEAD_Y1POS, DISPLAY_WIDTH, DISPLAY_HEAD_Y2POS, BANNER_BACKGROUND_COLOR, false);
	sprintf((char *)textData, "OVS %d", (int)shared_var.cis_oversampling);

	if (shared_var.cis_oversampling < 10)
	{
		ssd1362_drawString(216, 1, (int8_t *)textData, 0xF, 8);
	}
	else
	{
		ssd1362_drawString(208, 1, (int8_t *)textData, 0xF, 8);
	}
}

void cis_DisplayFrequency(uint32_t frequency)
{
	uint8_t FreqStr[256] = {0};

	sprintf((char *)FreqStr, "%dHz", (int)(frequency));
	ssd1362_drawRect(0, DISPLAY_HEAD_Y1POS, 60, DISPLAY_HEAD_Y2POS, BANNER_BACKGROUND_COLOR, false);
	ssd1362_drawString(0, 1, (int8_t*)FreqStr, 15, 8);
}

/**
 * @brief  The application entry point.
 * @retval int
 */
int sss_Display(void)
{
	uint8_t cis_rgb[3] = {0};
	int32_t cis_color = 0;
	int32_t start_tick = 0;
	int32_t old_process_cnt = 0;
	uint32_t freq = 0;
	int32_t i = 0;
	int32_t y = 0;
	float32_t packet, index;
	int32_t line_Ypos = DISPLAY_AERA1_Y2POS - (DISPLAY_AERAS1_HEIGHT / 2);
	int32_t pixel_intensity = 0;
	float64_t angle = 0;

	//printf("----- ETHERNET MODE START -----\n");
	//printf("-------------------------------\n");

	shared_var.cis_process_cnt = 0;
	shared_var.udp_process_cnt = 0;

	cisynth_SetHint();

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

	start_tick = HAL_GetTick();

	/* Infinite loop */
	while (1)
	{
		cisynth_interractiveMenu();

		if ((shared_var.cis_process_cnt - old_process_cnt) > 100)
		{
			int32_t tick = HAL_GetTick();

			freq = 1000000 / (((tick - start_tick) * 1000) / (shared_var.cis_process_cnt - old_process_cnt));
			start_tick = tick;
			old_process_cnt = shared_var.cis_process_cnt;

			cis_DisplayFrequency(freq);
		}

		ssd1362_drawRect(0, DISPLAY_AERA1_Y1POS, DISPLAY_WIDTH, DISPLAY_AERA1_Y2POS, 0, false);

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




		ssd1362_drawRect(0, DISPLAY_FOOT_Y1POS, DISPLAY_WIDTH, DISPLAY_FOOT_Y2POS, BANNER_BACKGROUND_COLOR, false);

		uint8_t textData[1024] = {0};

		sprintf((char *)textData, "Ax%03dAy%03dAz%03d  Gx%03dGy%03dGz%03d", (int)packet_IMU.acc[0],
				(int)packet_IMU.acc[1],
				(int)packet_IMU.acc[2],
				(int)packet_IMU.gyro[0],
				(int)packet_IMU.gyro[1],
				(int)packet_IMU.gyro[2]);

		ssd1362_drawString(0, DISPLAY_FOOT_Y1POS + 1, (int8_t *)textData, 0xF, 8);




		ssd1362_writeUpdates();
	}
}

/**
 * @brief  CIS start calibration
 * @param  calibration iteration
 * @retval None
 */
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
			ssd1362_drawRect(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, 0, false);
			ssd1362_drawRect(0, DISPLAY_HEAD_Y1POS, DISPLAY_WIDTH, DISPLAY_HEAD_Y2POS, BANNER_BACKGROUND_COLOR, true);
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
			ssd1362_drawRect(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, 0, false);
			ssd1362_drawRect(0, DISPLAY_HEAD_Y1POS, DISPLAY_WIDTH, DISPLAY_HEAD_Y2POS, BANNER_BACKGROUND_COLOR, true);
			ssd1362_drawString(0, DISPLAY_HEAD_Y1POS + 1, (int8_t *)" MOVE CIS ON WHITE SURFACE - LL ", 0xF, 8);
			ssd1362_writeFullBuffer();

			while (shared_var.cis_cal_progressbar < 99)
			{
				ssd1362_progressBar(30, 30, shared_var.cis_cal_progressbar, 0xF);
			}

			ssd1362_progressBar(30, 30, 99, 0xF);
			HAL_Delay(100);

			// Print curves
			ssd1362_drawRect(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, 0, false);
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
			ssd1362_drawRect(0, DISPLAY_HEAD_Y2POS, DISPLAY_WIDTH, DISPLAY_HEIGHT, 0, false);
			ssd1362_drawRect(0, DISPLAY_HEAD_Y1POS, DISPLAY_WIDTH, DISPLAY_HEAD_Y2POS, BANNER_BACKGROUND_COLOR, true);
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
			ssd1362_drawRect(0, DISPLAY_HEAD_Y2POS, DISPLAY_WIDTH, DISPLAY_HEIGHT, 0, false);
			ssd1362_drawRect(0, DISPLAY_HEAD_Y1POS, DISPLAY_WIDTH, DISPLAY_HEAD_Y2POS, BANNER_BACKGROUND_COLOR, true);
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
			ssd1362_drawRect(0, DISPLAY_HEAD_Y1POS, DISPLAY_WIDTH, DISPLAY_HEAD_Y2POS, BANNER_BACKGROUND_COLOR, false);
			ssd1362_drawString(0, DISPLAY_HEAD_Y1POS + 1, (int8_t *)" COMPUTE COMPENSATIION GAINS", 0xF, 8);
			ssd1362_writeFullBuffer();
			while (shared_var.cis_cal_state == CIS_CAL_COMPUTE_GAINS);
			break;
		default:
			break;
		}
	}
}

/**
 * @brief  Change CIS oversampling
 * @param  None
 * @retval None
 */
void cis_ChangeOversampling()
{
	static int32_t n = 0;

	n = (n + 1) % 6;
	shared_var.cis_oversampling = 1 << n;

	cis_DisplayOversampling();
}

/**
 * @brief  Change scan direction
 * @param  None
 * @retval None
 */
void cis_ChangeScanDir()
{
	shared_var.cis_scanDir = !shared_var.cis_scanDir;
	shared_var.cis_scanDir = shared_var.cis_scanDir < 0 ? 0 : shared_var.cis_scanDir > 1 ? 1 : shared_var.cis_scanDir;

	ssd1362_screenRotation(shared_var.cis_scanDir);
}

/**
 * @brief  Get pressed button, call the function and display message
 * @param  None
 * @retval None
 */
void cisynth_interractiveMenu()
{
	static uint32_t button_tick = 0;
	static uint8_t clear_button = 0;

	if (buttonState[SW1] == SWITCH_PRESSED)
	{
		//ssd1362_drawRect(0 + 10, 60, 10 + 10, 54, 0x0F, false);
		HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_RESET);
		buttonState[SW1] = SWITCH_RELEASED;
		button_tick = HAL_GetTick();
		clear_button = 0;
		cis_StartCalibration();
		cisynth_SetHint();
	}
	if (buttonState[SW2] == SWITCH_PRESSED)
	{
		//ssd1362_drawRect(56 + 10, 60, 56 + 10 + 10, 54, 0x0F, false);
		HAL_GPIO_WritePin(LED3_GPIO_Port, LED3_Pin, GPIO_PIN_RESET);
		buttonState[SW2] = SWITCH_RELEASED;
		button_tick = HAL_GetTick();
		clear_button = 0;
		cis_ChangeOversampling();
	}
	if (buttonState[SW3] == SWITCH_PRESSED)
	{
		//ssd1362_drawRect(56 * 2 + 10, 60, 56 * 2 + 10 + 10, 54, 0x0F, false);
		HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET);
		buttonState[SW3] = SWITCH_RELEASED;
		button_tick = HAL_GetTick();
		clear_button = 0;
		cis_ChangeScanDir();
		cisynth_SetHint();
	}

	if (HAL_GetTick() > (button_tick + BUTTON_DELAY) && clear_button != 1)
	{
		clear_button = 1;

		/*
		ssd1362_drawRect(0, DISPLAY_AERA2_Y1POS, DISPLAY_WIDTH, DISPLAY_AERA2_Y2POS, 0, false);
		ssd1362_drawRect(0 + 10, 60, 10 + 10, 54, 0x05, false);
		ssd1362_drawRect(56 + 10, 60, 56 + 10 + 10, 54, 0x05, false);
		ssd1362_drawRect(56 * 2 + 10, 60, 56 * 2 + 10 + 10, 54, 0x05, false);
		ssd1362_drawRect(56 * 3 + 10, 60, 56 * 3 + 10 + 10, 54, 0x05, false);
		ssd1362_drawRect(56 * 4 + 10, 60, 56 * 4 + 10 + 10, 54, 0x05, false);
		 */
		HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_SET);
		HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_SET);
		HAL_GPIO_WritePin(LED3_GPIO_Port, LED3_Pin, GPIO_PIN_SET);
	}
}

/**
 * @brief  Display hint
 * @param  None
 * @retval None
 */
static void cisynth_SetHint(void)
{
	/* Set Audio header description */
	ssd1362_clearBuffer();
	ssd1362_writeFullBuffer();
	ssd1362_drawRect(0, DISPLAY_HEAD_Y1POS, DISPLAY_WIDTH, DISPLAY_HEAD_Y2POS, BANNER_BACKGROUND_COLOR, false);
	ssd1362_drawString(100, 1, (int8_t *)"CISYNTH 3", 0xF, 8);

	cis_DisplayOversampling();

	ssd1362_drawRect(0, DISPLAY_AERA2_Y1POS, DISPLAY_WIDTH, DISPLAY_AERA2_Y2POS, 0, false);
}
/* Private functions ---------------------------------------------------------*/
