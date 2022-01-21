/*
 * sss_Scan.c
 *
 *  Created on: May 31, 2020
 *      Author: zhonx
 */

/* Includes ------------------------------------------------------------------*/
#include <sss_Scan.h>
#include "stdbool.h"
#include "stdio.h"
#include "arm_math.h"

#include "udp_client.h"
#include "lwip.h"

#include "shared.h"
#include "config.h"

#include "cis.h"
#include "ssd1362.h"

#include "sss_Scan.h"


/* Private includes ----------------------------------------------------------*/

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private typedef -----------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
__IO uint32_t eth_process_cnt;

/* Variable containing black and white frame from CIS*/

/* Private function prototypes -----------------------------------------------*/

extern void Ethernet_Link_Periodic_Handle(struct netif *netif);

/* Private user code ---------------------------------------------------------*/
/**
 * @brief  The application entry point.
 * @retval int
 */
int sss_Scan(void)
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

	cis_Init(params.calibrationRequest);

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
	}
}
/* Private functions ---------------------------------------------------------*/
