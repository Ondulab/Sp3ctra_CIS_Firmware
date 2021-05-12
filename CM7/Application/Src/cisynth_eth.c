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
#include "menu.h"
#include "ssd1362.h"

#include "lwip/udp.h"
#include "lwiperf.h"
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
//static uint16_t imageData[((CIS_END_CAPTURE * CIS_ADC_OUT_LINES) / CIS_IFFT_OVERSAMPLING_RATIO) - 1]; // for debug

uint8_t data[100];
__IO uint32_t message_count = 0;
struct udp_pcb *upcb;

struct netif gnetif;

/* Private function prototypes -----------------------------------------------*/
static void cisynth_eth_SetHint(void);
void udp_receive_callback(void *arg, struct udp_pcb *upcb, struct pbuf *p, const ip_addr_t *addr, u16_t port);
void udp_echoclient_connect(void);
void udp_echoclient_send(void);

extern void Ethernet_Link_Periodic_Handle(struct netif *netif);

/* Private user code ---------------------------------------------------------*/
/**
 * @brief  The application entry point.
 * @retval int
 */
int cisynth_eth(void)
{
	uint8_t FreqStr[256] = {0};
	uint32_t cis_color = 0;

	printf("----- ETHERNET MODE START -----\n");
	printf("-------------------------------\n");

	//allocate the contiguous memory area for storage image data
	imageData = malloc(NUMBER_OF_NOTES * sizeof(int32_t*) + 10 * sizeof(int8_t*));
	if (imageData == NULL)
	{
		Error_Handler();
	}

	memset(imageData, 0, NUMBER_OF_NOTES * sizeof(int32_t*) + 10 * sizeof(int8_t*));

	ssd1362_clearBuffer();

	cis_Init(IFFT_MODE);

	cisynth_eth_SetHint();

	/* UDP client connect */
	udp_echoclient_connect();

//	while(1)
//	{
//		udp_echoclient_send();
//		MX_LWIP_Process();
//		HAL_Delay(1);
//	}

	//	SCB_CleanInvalidateDCache();

	//	lwiperf_start_tcp_server_default(NULL, NULL); // TCP Perf = iperf -c 192.168.0.1 -i1 -t60 -u -b 1000M UDP Perf = iperf -c 192.168.0.1 -i1 -t60

	/* Infinite loop */
	static uint32_t start_tick;
	uint32_t latency;
	int32_t i = 0;
	struct pbuf *p;

	sprintf((char *)imageData, "START");

	while (1)
	{
		start_tick = HAL_GetTick();
		while ((HAL_GetTick() - start_tick) < DISPLAY_REFRESH_FPS)
		{
			cis_ImageProcessBW(&imageData[5 - 1]);

			/* allocate pbuf from pool*/
			p = pbuf_alloc(PBUF_TRANSPORT, (NUMBER_OF_NOTES + 5) * 2, PBUF_RAM);

			if (p != NULL)
			{
				/* copy data to pbuf */
				pbuf_take(p, (char*)imageData, (NUMBER_OF_NOTES + 5) * 2);

				/* send udp data */
				udp_send(upcb, p);

				/* free pbuf */
				pbuf_free(p);
			}

			MX_LWIP_Process();

			eth_process_cnt++;
		}

		latency = HAL_GetTick() - start_tick;
		sprintf((char *)FreqStr, "%dHz", (int)((eth_process_cnt * 1000) / latency));
		eth_process_cnt = 0;

		ssd1362_drawRect(0, DISPLAY_AERA1_Y1POS, DISPLAY_MAX_X_LENGTH / 2 - 1, DISPLAY_AERA1_Y2POS, 3, false);
		ssd1362_drawRect(DISPLAY_MAX_X_LENGTH / 2 + 1, DISPLAY_AERA1_Y1POS, DISPLAY_MAX_X_LENGTH, DISPLAY_AERA1_Y2POS, 4, false);
		ssd1362_drawRect(0, DISPLAY_AERA2_Y1POS, DISPLAY_MAX_X_LENGTH, DISPLAY_AERA2_Y2POS, 3, false);
		ssd1362_drawRect(0, DISPLAY_AERA3_Y1POS, DISPLAY_MAX_X_LENGTH, DISPLAY_AERA3_Y2POS, 8, false);

		for (i = 0; i < (DISPLAY_MAX_X_LENGTH); i++)
		{
			cis_color = imageData[(uint32_t)(i * ((float)cis_GetEffectivePixelNb() / (float)DISPLAY_MAX_X_LENGTH))] >> 12;
			ssd1362_drawPixel(DISPLAY_MAX_X_LENGTH - 1 - i, DISPLAY_AERA2_Y1POS + DISPLAY_AERAS2_HEIGHT - DISPLAY_INTER_AERAS_HEIGHT - (cis_color) - 1, 15, false);

			ssd1362_drawVLine(DISPLAY_MAX_X_LENGTH - 1 - i, DISPLAY_AERA3_Y1POS + 1, DISPLAY_AERAS3_HEIGHT - 2, cis_color, false);
		}
		ssd1362_drawRect(195, DISPLAY_HEAD_Y1POS, DISPLAY_MAX_X_LENGTH, DISPLAY_HEAD_Y2POS, 4, false);
		ssd1362_drawString(195, 1, (int8_t*)FreqStr, 15, 8);
		ssd1362_writeFullBuffer();

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
	ssd1362_drawRect(0, DISPLAY_HEAD_Y1POS, DISPLAY_MAX_X_LENGTH, DISPLAY_HEAD_Y2POS, 4, false);
	ssd1362_drawString(100, 1, (int8_t *)"CISYNTH 3", 0xF, 8);
	ssd1362_drawString(0, 1, (int8_t *)"ETHERNET", 0xF, 8);
	ssd1362_writeFullBuffer();
}
/* Private functions ---------------------------------------------------------*/

/**
 * @brief  Connect to UDP echo server
 * @param  None
 * @retval None
 */
void udp_echoclient_connect(void)
{
	ip_addr_t DestIPaddr;
	err_t err;

	/* Create a new UDP control block  */
	upcb = udp_new();

	if (upcb!=NULL)
	{
		/*assign destination IP address */
		IP4_ADDR( &DestIPaddr, DEST_IP_ADDR0, DEST_IP_ADDR1, DEST_IP_ADDR2, DEST_IP_ADDR3 );

		/* configure destination IP address and port */
		err= udp_connect(upcb, &DestIPaddr, UDP_SERVER_PORT);

		if (err == ERR_OK)
		{
			/* Set a receive callback for the upcb */
			udp_recv(upcb, udp_receive_callback, NULL);
		}
	}
}

/**
 * @brief This function is called when an UDP datagrm has been received on the port UDP_PORT.
 * @param arg user supplied argument (udp_pcb.recv_arg)
 * @param pcb the udp_pcb which received data
 * @param p the packet buffer that was received
 * @param addr the remote IP address from which the packet was received
 * @param port the remote port from which the packet was received
 * @retval None
 */
void udp_echoclient_send(void)
{
	struct pbuf *p;

	sprintf((char*)data, "sending udp client message %d", (int)message_count);

	/* allocate pbuf from pool*/
	p = pbuf_alloc(PBUF_TRANSPORT,strlen((char*)data), PBUF_RAM);

	if (p != NULL)
	{
		/* copy data to pbuf */
		pbuf_take(p, (char*)data, strlen((char*)data));

		/* send udp data */
		udp_send(upcb, p);

		/* free pbuf */
		pbuf_free(p);
	}
}

/**
 * @brief This function is called when an UDP datagrm has been received on the port UDP_PORT.
 * @param arg user supplied argument (udp_pcb.recv_arg)
 * @param pcb the udp_pcb which received data
 * @param p the packet buffer that was received
 * @param addr the remote IP address from which the packet was received
 * @param port the remote port from which the packet was received
 * @retval None
 */
void udp_receive_callback(void *arg, struct udp_pcb *upcb, struct pbuf *p, const ip_addr_t *addr, u16_t port)
{
	/*increment message count */
	message_count++;

	/* Free receive pbuf */
	pbuf_free(p);
}
