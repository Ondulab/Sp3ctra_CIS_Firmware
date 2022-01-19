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

#include "config.h"
#include "times_base.h"
#include "cis.h"
#include "ssd1362.h"
#include "udp_client.h"

#include "lwip.h"

#include "cisynth_eth.h"

/* Private includes ----------------------------------------------------------*/

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private typedef -----------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
__IO uint32_t eth_process_cnt;

/* Variable containing black and white frame from CIS*/
static int32_t *imageData = NULL;

/* Private function prototypes -----------------------------------------------*/
static void cisynth_eth_SetHint(void);

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

	//allocate the contiguous memory area for storage image data
	imageData = malloc(CIS_PIXELS_NB * sizeof(int32_t) + UDP_HEADER_SIZE * sizeof(int32_t));
	if (imageData == NULL)
	{
		Error_Handler();
	}

	memset(imageData, 0, CIS_PIXELS_NB * sizeof(int32_t) + UDP_HEADER_SIZE * sizeof(int32_t));

	ssd1362_clearBuffer();

	cis_Init();

	udp_clientInit();

	cisynth_eth_SetHint();

	//Add "SSS3" header for synchronization
	imageData[0] = UDP_HEADER;

	/* Infinite loop */
	while (1)
	{
		start_tick = HAL_GetTick();
		while ((HAL_GetTick() - start_tick) < DISPLAY_REFRESH_FPS)//todo add TIM us to compute loop latency
		{
			//Delete image transfert
			MX_LWIP_Process();

#ifdef CIS_BW
			cis_ImageProcessBW(&imageData[UDP_HEADER_SIZE]);
#else
			cis_ImageProcessRGB(&imageData[UDP_HEADER_SIZE]);
#endif
			udp_clientSendImage(imageData);

			eth_process_cnt++;
		}

		latency = HAL_GetTick() - start_tick;
		sprintf((char *)FreqStr, "%dHz", (int)((eth_process_cnt * 1000) / latency));
		eth_process_cnt = 0;

		ssd1362_drawRect(0, DISPLAY_AERA1_Y1POS, DISPLAY_WIDTH, DISPLAY_AERA1_Y2POS, 3, false);
		ssd1362_drawRect(0, DISPLAY_AERA2_Y1POS, DISPLAY_WIDTH, DISPLAY_AERA2_Y2POS, 8, false);

		for (i = 0; i < (DISPLAY_WIDTH); i++)
		{
			static int32_t y;
			cis_rgb = (uint8_t*)&(imageData[(uint32_t)(i * ((float)CIS_PIXELS_NB / (float)DISPLAY_WIDTH)) + UDP_HEADER_SIZE]);
			cis_color = cis_rgb[0] * cis_rgb[1] * cis_rgb[2];
			cis_color >>= 19;

			y = (DISPLAY_AERA1_Y2POS - cis_color - 1);

			if (y < DISPLAY_AERA1_Y1POS)
			{
				y = DISPLAY_AERA1_Y1POS;
			}

			if (y > DISPLAY_AERA1_Y1POS + DISPLAY_AERAS1_HEIGHT - 1)
			{
				y = DISPLAY_AERA1_Y1POS + DISPLAY_AERAS1_HEIGHT - 1;
			}

			ssd1362_drawPixel(DISPLAY_WIDTH - 1 - i, y, 0xF, false);

			ssd1362_drawVLine(DISPLAY_WIDTH - 1 - i, DISPLAY_AERA2_Y1POS + 1, DISPLAY_AERAS2_HEIGHT - 2, cis_color / 2, false);
		}
		ssd1362_drawRect(0, DISPLAY_HEAD_Y1POS, 100, DISPLAY_HEAD_Y2POS, 4, false);
		ssd1362_drawString(0, 1, (int8_t*)FreqStr, 15, 8);
		ssd1362_writeUpdates();

		HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
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
	ssd1362_writeFullBuffer();
}
/* Private functions ---------------------------------------------------------*/
