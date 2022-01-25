/**
 ******************************************************************************
 * @file           : udp_client.c
 * @brief          : sss udp client
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"
#include "main.h"
#include "config.h"

#include "lwip/pbuf.h"
#include "lwip/udp.h"
#include "lwip/tcp.h"
#include "lwip.h"

/* Private includes ----------------------------------------------------------*/
#include "udp_client.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
struct udp_pcb *upcb;
__IO uint32_t message_count = 0;

/* Private function prototypes -----------------------------------------------*/
void udp_clientReceiveCallback(void *arg, struct udp_pcb *upcb, struct pbuf *p, const ip_addr_t *addr, u16_t port);

/* Private user code ---------------------------------------------------------*/

/**
 * @brief  Connect to UDP echo server
 * @param  None
 * @retval None
 */
void udp_clientInit(void)
{
	ip_addr_t DestIPaddr;
	err_t err;

	//	lwiperf_start_tcp_server_default(NULL, NULL); // TCP Perf = iperf -c 192.168.0.1 -i1 -t60 -u -b 1000M UDP Perf = iperf -c 192.168.0.1 -i1 -t60

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
			udp_recv(upcb, udp_clientReceiveCallback, NULL);
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
void udp_clientSendImage(int32_t *image_buff)
{
	struct pbuf *p;

	/* allocate pbuf from pool*/
	p = pbuf_alloc(PBUF_TRANSPORT, (CIS_PIXELS_NB + UDP_HEADER_SIZE) * sizeof(uint32_t), PBUF_RAM);

	if (p != NULL)
	{
		/* copy data to pbuf */
		pbuf_take(p, (char*)image_buff, (CIS_PIXELS_NB + UDP_HEADER_SIZE) * sizeof(uint32_t));

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
void udp_clientReceiveCallback(void *arg, struct udp_pcb *upcb, struct pbuf *p, const ip_addr_t *addr, u16_t port)
{
	/*increment message count */
	message_count++;

	/* Free receive pbuf */
	pbuf_free(p);
}
