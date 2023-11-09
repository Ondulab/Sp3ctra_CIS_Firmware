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
static void cisynth_eth_SetHint(void);
static void cisynth_interractiveMenu(void);
void cis_StartCalibration(void);

//#define DISPLAY_SCOLL_IMAGE

/* Private user code ---------------------------------------------------------*/
/**
 * @brief  The application entry point.
 * @retval int
 */
int sss_Display(void)
{
	uint8_t FreqStr[256] = {0};
	uint8_t *cis_rgb = NULL;
	int32_t cis_color = 0;
	int32_t start_tick = 0;
	int32_t old_cis_process_cnt = 0;
	uint32_t cis_line_freq = 0;
	int32_t i = 0;
	int32_t y = 0;
	int32_t line_Ypos = DISPLAY_AERA1_Y2POS - (DISPLAY_AERAS1_HEIGHT / 2);

	printf("----- ETHERNET MODE START -----\n");
	printf("-------------------------------\n");

	cisynth_eth_SetHint();
	old_cis_process_cnt = shared_var.cis_process_cnt;

	start_tick = HAL_GetTick();

	/* Infinite loop */
	while (1)
	{
		cisynth_interractiveMenu();

		if ((shared_var.cis_process_cnt - old_cis_process_cnt) > 100)
		{
			cis_line_freq = 1000000 / (((HAL_GetTick() - start_tick) * 1000) / (shared_var.cis_process_cnt - old_cis_process_cnt));
			start_tick = HAL_GetTick();
			old_cis_process_cnt = shared_var.cis_process_cnt;
		}

		sprintf((char *)FreqStr, "%dHz", (int)(cis_line_freq));

#ifdef DISPLAY_SCOLL_IMAGE
		line_Ypos++;
		if (line_Ypos > (DISPLAY_AERAS1_HEIGHT - 2))
			line_Ypos = 0;

		for (i = 0; i < (DISPLAY_WIDTH); i++)
		{
			cis_rgb = (uint8_t*)&(imageData[(uint32_t)(i * ((float)CIS_PIXELS_NB / (float)DISPLAY_WIDTH)) + UDP_HEADER_SIZE]);
			cis_color = cis_rgb[0] * cis_rgb[1] * cis_rgb[2];
			cis_color >>= 20;

			//			ssd1362_drawVLine(DISPLAY_WIDTH - 1 - i, DISPLAY_AERA1_Y1POS + 1, (DISPLAY_AERAS1_HEIGHT - 2) / 2, cis_color, false);
			ssd1362_drawPixel(DISPLAY_WIDTH - 1 - i, DISPLAY_AERA1_Y1POS + 1 + line_Ypos, cis_color, false);

		}
#else
		static int32_t line_length, line_intensity, pixel_intensity;
		ssd1362_drawRect(0, DISPLAY_AERA1_Y1POS, DISPLAY_WIDTH, DISPLAY_AERA1_Y2POS, 0, false);

		for (i = 0; i < (DISPLAY_WIDTH); i++)
		{
			// cast imageData pointer to a uint8_t pointer, and offset by calculated index value
			// the index is calculated as the product of the current horizontal pixel position 'i'
			// and the ratio of total CIS_PIXELS_NB and DISPLAY_WIDTH. UDP_HEADER_SIZE is then added to this
			// to skip the header bytes in imageData.
			cis_rgb = (uint8_t*)&(imageData[(uint32_t)(i * ((float)CIS_PIXELS_NB / (float)DISPLAY_WIDTH)) + UDP_HEADER_SIZE]);

			// Convert the RGB values to a single brightness value. The numbers 299, 587, and 114
			// are weights given to the R, G, and B components respectively,
			// according to the ITU-R BT.601 standard for converting color to grayscale.
			// This standard assumes that human eyes are less sensitive to the blue component as compared to red and green.
			// Note that cis_rgb[0], cis_rgb[1] and cis_rgb[2] are assumed to be the R, G, B values respectively.
			cis_color = (299*cis_rgb[0]) + 587 * (cis_rgb[1]) + (114*cis_rgb[2]);
			cis_color = 255000 - cis_color;

			// Ensure that cis_color is within the expected range
			cis_color = cis_color < 0 ? 0 : cis_color > 255000 ? 255000 : cis_color;

			// Calculate the length of the line in pixels (0 to 20)
			// Dividing by 1000 is necessary because cis_color is scaled up by a factor of 1000
			line_length = (int)(cis_color / 255.0 * (DISPLAY_AERAS1_HEIGHT / 2)) / 1000;

			// Make sure line_length does not exceed 20
			line_length = line_length > (DISPLAY_AERAS1_HEIGHT / 2) ? (DISPLAY_AERAS1_HEIGHT / 2) : line_length;

			// Calculate the intensity of the line (0 to 15)
			// Again, dividing by 1000 because cis_color is scaled up
			line_intensity = (cis_color / 255.0 * 15) / 1000;

			// Ensure that the line intensity is within the expected range
			line_intensity = line_intensity < 0 ? 0 : line_intensity > 15 ? 15 : line_intensity;

			// Draw each pixel of the line
			for (y = 0; y < line_length; y++)
			{
			    // Decrease intensity for each additional pixel
			    pixel_intensity = (line_intensity + (DISPLAY_AERAS1_HEIGHT / 2) - 15) - y;


			    // Ensure that the pixel intensity is within the expected range
			    pixel_intensity = pixel_intensity < 0 ? 0 : pixel_intensity > 15 ? 15 : pixel_intensity;

			    // Draw a pixel above the center of the line for symmetry
			    ssd1362_drawPixel(DISPLAY_WIDTH - i, line_Ypos + y, pixel_intensity, false);

			    // Draw a pixel below the center of the line for symmetry
			    ssd1362_drawPixel(DISPLAY_WIDTH - i, line_Ypos - y, pixel_intensity, false);
			}

		}
#endif

		ssd1362_drawRect(0, DISPLAY_HEAD_Y1POS, 60, DISPLAY_HEAD_Y2POS, 4, false);
		ssd1362_drawString(0, 1, (int8_t*)FreqStr, 15, 8);

		ssd1362_writeUpdates();
		//		HAL_Delay(1);
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
	HAL_Delay(200);
	shared_var.cis_cal_state = CIS_CAL_END;
	return;

	while (shared_var.cis_cal_state != CIS_CAL_END)
	{
		switch (shared_var.cis_cal_state)
		{
		case CIS_CAL_PLACE_ON_WHITE :
			/*-------- 1 --------*/
			ssd1362_drawRect(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, 0, false);
			ssd1362_drawRect(0, DISPLAY_HEAD_Y1POS, DISPLAY_WIDTH, DISPLAY_HEAD_Y2POS, 4, true);
			ssd1362_drawString(10, DISPLAY_HEAD_Y1POS + 1, (int8_t *)" MOVE CIS ON WHITE SURFACE - HL ", 0xF, 8);
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
			ssd1362_drawRect(0, DISPLAY_HEAD_Y1POS, DISPLAY_WIDTH, DISPLAY_HEAD_Y2POS, 4, true);
			ssd1362_drawString(10, DISPLAY_HEAD_Y1POS + 1, (int8_t *)" MOVE CIS ON WHITE SURFACE - LL ", 0xF, 8);
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
				ssd1362_drawVLine(DISPLAY_WIDTH / 9 * i, 0, DISPLAY_HEIGHT, 4, false);
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
			ssd1362_drawRect(0, DISPLAY_HEAD_Y1POS, DISPLAY_WIDTH, DISPLAY_HEAD_Y2POS, 4, true);
			ssd1362_drawString(10, DISPLAY_HEAD_Y1POS + 1, (int8_t *)" EXTRACT EXTREMUMS AND DELTAS ", 0xF, 8);
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
			ssd1362_drawRect(0, DISPLAY_HEAD_Y1POS, DISPLAY_WIDTH, DISPLAY_HEAD_Y2POS, 4, true);
			ssd1362_drawString(10, DISPLAY_HEAD_Y1POS + 1, (int8_t *)"EXTRACT DIFFERENTIAL OFFSETS", 0xF, 8);
			ssd1362_drawString(0, 18, (int8_t *)"  R1 G1 B1  R2 G2 B2  R3 G3 B3", 10, 8);
			for (uint32_t i = 1; i < 9; i++)
			{
				ssd1362_drawVLine(DISPLAY_WIDTH / 9 * i, 28, DISPLAY_HEIGHT, 4, false);
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
			ssd1362_drawRect(0, DISPLAY_HEAD_Y1POS, DISPLAY_WIDTH, DISPLAY_HEAD_Y2POS, 4, false);
			ssd1362_drawString(10, DISPLAY_HEAD_Y1POS + 1, (int8_t *)" COMPUTE COMPENSATIION GAINS", 0xF, 8);
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
	uint8_t textData[256] = {0};

    n = (n + 1) % 6;
	shared_var.cis_oversampling = pow(2, n);

	ssd1362_drawRect(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, 0, false);
	ssd1362_drawRect(0, DISPLAY_HEAD_Y1POS, DISPLAY_WIDTH, DISPLAY_HEAD_Y2POS, 4, true);

	sprintf((char *)textData, "OVSP %d", (int)shared_var.cis_oversampling);
	ssd1362_drawString(95, DISPLAY_HEAD_Y1POS + 1, (int8_t *)textData, 0xF, 8);
	ssd1362_writeFullBuffer();
}

/**
 * @brief  Change scan direction
 * @param  None
 * @retval None
 */
void cis_ChangeScanDir()
{
	uint8_t textData[256] = {0};

	shared_var.cis_scanDir = !shared_var.cis_scanDir;
	shared_var.cis_scanDir = shared_var.cis_scanDir < 0 ? 0 : shared_var.cis_scanDir > 1 ? 1 : shared_var.cis_scanDir;


	ssd1362_screenRotation(shared_var.cis_scanDir);

	ssd1362_drawRect(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, 0, false);
	ssd1362_drawRect(0, DISPLAY_HEAD_Y1POS, DISPLAY_WIDTH, DISPLAY_HEAD_Y2POS, 4, true);

	sprintf((char *)textData, "DIR %d", (int)shared_var.cis_scanDir);
	ssd1362_drawString(100, DISPLAY_HEAD_Y1POS + 1, (int8_t *)textData, 0xF, 8);
	ssd1362_writeFullBuffer();
}

#define BUTTON_DELAY			500

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
	}

	if (HAL_GetTick() > (button_tick + BUTTON_DELAY) && clear_button != 1)
	{
		clear_button = 1;

		cisynth_eth_SetHint();
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
static void cisynth_eth_SetHint(void)
{
	/* Set Audio header description */
	ssd1362_clearBuffer();
	ssd1362_writeFullBuffer();
	ssd1362_drawRect(0, DISPLAY_HEAD_Y1POS, DISPLAY_WIDTH, DISPLAY_HEAD_Y2POS, 4, false);
	ssd1362_drawString(100, 1, (int8_t *)"CISYNTH 3", 0xF, 8);
	ssd1362_drawString(232, 1, (int8_t *)"ETH", 0xF, 8);

	ssd1362_drawRect(0, DISPLAY_AERA2_Y1POS, DISPLAY_WIDTH, DISPLAY_AERA2_Y2POS, 0, false);
	//ssd1362_drawRect(0 + 10, 60, 10 + 10, 54, 0x05, false);
	//ssd1362_drawRect(56 + 10, 60, 56 + 10 + 10, 54, 0x05, false);
	//ssd1362_drawRect(56 * 2 + 10, 60, 56 * 2 + 10 + 10, 54, 0x05, false);
	//ssd1362_drawRect(56 * 3 + 10, 60, 56 * 3 + 10 + 10, 54, 0x05, false);
	//ssd1362_drawRect(56 * 4 + 10, 60, 56 * 4 + 10 + 10, 54, 0x05, false);

	//ssd1362_writeFullBuffer();
}
/* Private functions ---------------------------------------------------------*/
