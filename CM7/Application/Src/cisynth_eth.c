/*
 * cisynth_eth.c
 *
 *  Created on: May 31, 2020
 *      Author: zhonx
 */

/* Includes ------------------------------------------------------------------*/
#include "stdbool.h"
#include "stdio.h"
#include "arm_math.h"

#include "times_base.h"
#include "udp_client.h"
#include "lwip.h"

#include "config.h"

#include "cis.h"
#include "ssd1362.h"
#include "buttons.h"

#include "cisynth_eth.h"

/* Private includes ----------------------------------------------------------*/

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private typedef -----------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
__IO uint32_t eth_process_cnt;

/* Variable containing black and white frame from CIS*/
static int32_t imageData[CIS_PIXELS_NB + UDP_HEADER_SIZE] = {0};

/* Private function prototypes -----------------------------------------------*/
static void cisynth_eth_SetHint(void);
static void cisynth_interractiveMenu();

extern void Ethernet_Link_Periodic_Handle(struct netif *netif);

/* Private user code ---------------------------------------------------------*/
/**
 * @brief  The application entry point.
 * @retval int
 */
int cisynth_eth(void)
{
	uint8_t FreqStr[256] = {0};
	uint8_t *cis_rgb = NULL;
	uint32_t cis_color = 0;
	uint32_t start_tick;
	uint32_t latency;
	int32_t i = 0;

	printf("----- ETHERNET MODE START -----\n");
	printf("-------------------------------\n");

#ifndef ETHERNET_OFF
	MX_LWIP_Init();
	udp_clientInit();
#endif

	cisynth_eth_SetHint();

	cis_Init(buttonState[SW1]);

	//Add "SSS3" header for synchronization
	imageData[0] = UDP_HEADER;

	/* Infinite loop */
	while (1)
	{
		start_tick = HAL_GetTick();
		while ((HAL_GetTick() - start_tick) < DISPLAY_REFRESH_FPS)//todo add TIM us to compute loop latency
		{
			cis_ImageProcessRGB(&imageData[UDP_HEADER_SIZE]);

#ifndef ETHERNET_OFF
			MX_LWIP_Process();
			udp_clientSendImage(imageData);
#endif

			eth_process_cnt++;
		}

		//		static uint32_t sw = 0;
		//		sw++;
		//		if (sw % 2)
		//			cis_LedsOff();
		//		else
		//			cis_LedsOn();

		latency = HAL_GetTick() - start_tick;
		sprintf((char *)FreqStr, "%dHz", (int)((eth_process_cnt * 1000) / latency));
		eth_process_cnt = 0;

		ssd1362_drawRect(0, DISPLAY_AERA1_Y1POS, DISPLAY_WIDTH, DISPLAY_AERA1_Y2POS, 3, false);

		for (i = 0; i < (DISPLAY_WIDTH); i++)
		{
			cis_rgb = (uint8_t*)&(imageData[(uint32_t)(i * ((float)CIS_PIXELS_NB / (float)DISPLAY_WIDTH)) + UDP_HEADER_SIZE]);
			cis_color = cis_rgb[0] * cis_rgb[1] * cis_rgb[2];
			cis_color >>= 20;

			ssd1362_drawVLine(DISPLAY_WIDTH - 1 - i, DISPLAY_AERA1_Y1POS + 1, DISPLAY_AERAS1_HEIGHT - 2, cis_color, false);
		}
		ssd1362_drawRect(0, DISPLAY_HEAD_Y1POS, 100, DISPLAY_HEAD_Y2POS, 4, false);
		ssd1362_drawString(0, 1, (int8_t*)FreqStr, 15, 8);

		HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
		cisynth_interractiveMenu();
		ssd1362_writeUpdates();
	}
}

#define BUTTON_DELAY			2000

/**
 * @brief
 * @param
 * @retval None
 */
void cisynth_interractiveMenu()
{
	static uint32_t button_tick = 0;
	static uint8_t clear_button = 0;

	if (buttonState[SW1] == SWITCH_PRESSED)
	{
		ssd1362_drawRect(0 + 10, 60, 10 + 10, 54, 0x0F, false);
		buttonState[SW1] = SWITCH_RELEASED;
		button_tick = HAL_GetTick();
		clear_button = 0;
	}
	if (buttonState[SW2] == SWITCH_PRESSED)
	{
		ssd1362_drawRect(56 + 10, 60, 56 + 10 + 10, 54, 0x0F, false);
		buttonState[SW2] = SWITCH_RELEASED;
		button_tick = HAL_GetTick();
		clear_button = 0;
	}
	if (buttonState[SW3] == SWITCH_PRESSED)
	{
		ssd1362_drawRect(56 * 2 + 10, 60, 56 * 2 + 10 + 10, 54, 0x0F, false);
		buttonState[SW3] = SWITCH_RELEASED;
		button_tick = HAL_GetTick();
		clear_button = 0;
	}
	if (buttonState[SW4] == SWITCH_PRESSED)
	{
		ssd1362_drawRect(56 * 3 + 10, 60, 56 * 3 + 10 + 10, 54, 0x0F, false);
		buttonState[SW4] = SWITCH_RELEASED;
		button_tick = HAL_GetTick();
		clear_button = 0;
	}
	if (buttonState[SW5] == SWITCH_PRESSED)
	{
		ssd1362_drawRect(56 * 4 + 10, 60, 56 * 4 + 10 + 10, 54, 0x0F, false);
		buttonState[SW5] = SWITCH_RELEASED;
		button_tick = HAL_GetTick();
		clear_button = 0;
	}

	if (HAL_GetTick() > (button_tick + BUTTON_DELAY) && clear_button != 1)
	{
		clear_button = 1;
		ssd1362_drawRect(0, DISPLAY_AERA2_Y1POS, DISPLAY_WIDTH, DISPLAY_AERA2_Y2POS, 0, false);
		ssd1362_drawRect(0 + 10, 60, 10 + 10, 54, 0x05, false);
		ssd1362_drawRect(56 + 10, 60, 56 + 10 + 10, 54, 0x05, false);
		ssd1362_drawRect(56 * 2 + 10, 60, 56 * 2 + 10 + 10, 54, 0x05, false);
		ssd1362_drawRect(56 * 3 + 10, 60, 56 * 3 + 10 + 10, 54, 0x05, false);
		ssd1362_drawRect(56 * 4 + 10, 60, 56 * 4 + 10 + 10, 54, 0x05, false);
	}
}

/**
 * @brief  Display Audio demo hint
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
	ssd1362_drawRect(0 + 10, 60, 10 + 10, 54, 0x05, false);
	ssd1362_drawRect(56 + 10, 60, 56 + 10 + 10, 54, 0x05, false);
	ssd1362_drawRect(56 * 2 + 10, 60, 56 * 2 + 10 + 10, 54, 0x05, false);
	ssd1362_drawRect(56 * 3 + 10, 60, 56 * 3 + 10 + 10, 54, 0x05, false);
	ssd1362_drawRect(56 * 4 + 10, 60, 56 * 4 + 10 + 10, 54, 0x05, false);

	ssd1362_writeFullBuffer();
}
/* Private functions ---------------------------------------------------------*/
