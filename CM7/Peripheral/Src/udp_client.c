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
#include "shared.h"

#include "lwip/pbuf.h"
#include "lwip/udp.h"
#include "lwip/tcp.h"
#include "lwip.h"

#include "arm_math.h"

/* Private includes ----------------------------------------------------------*/
#include "udp_client.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
struct udp_pcb *upcb;
__IO uint32_t message_count = 0;
int32_t udp_imageData[UDP_PACKET_SIZE] = {0};

static struct packet_StartupInfo packet_StartupInfo = {0};
static struct packet_HID packet_HID = {0};
static struct packet_IMU packet_IMU = {0};

static uint32_t packetsCounter = 0;

/* Private function prototypes -----------------------------------------------*/
void udp_clientReceiveCallback(void *arg, struct udp_pcb *upcb, struct pbuf *p, const ip_addr_t *addr, u16_t port);

/* Private user code ---------------------------------------------------------*/

void udp_clientInit(void)
{
	ip_addr_t DestIPaddr;
	err_t err;

	//	lwiperf_start_tcp_server_default(NULL, NULL); // TCP Perf = iperf -c 192.168.0.1 -i1 -t60 -u -b 1000M UDP Perf = iperf -c 192.168.0.1 -i1 -t60

	/* Create a new UDP control block  */
	upcb = udp_new();
	ip_set_option(upcb, SOF_BROADCAST); // test for broadcast, useless at frist view
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

	for (int32_t packet = UDP_NB_PACKET_PER_LINE; --packet >= 0;)
	{
		rgbBuffers[packet].fragment_size = CIS_PIXELS_NB / UDP_NB_PACKET_PER_LINE;
		rgbBuffers[packet].total_fragments = UDP_NB_PACKET_PER_LINE;
		rgbBuffers[packet].type = IMAGE_DATA_HEADER;
	}

	packet_StartupInfo.type = STARTUP_INFO_HEADER;
	packet_HID.type = HID_DATA_HEADER;
	packet_IMU.type = IMU_DATA_HEADER;

	packet_StartupInfo.packet_id = packetsCounter;
	sprintf((char *)packet_StartupInfo.version_info, "CISYNTH v3.0 RESO-NANCE");

	udp_clientSendStartupInfoPacket();
}

void udp_sendData(void *data, uint16_t length)
{
	struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, length, PBUF_RAM);
	if (p != NULL)
	{
		pbuf_take(p, data, length);
		udp_send(upcb, p);
		pbuf_free(p);
	}
}


void udp_clientSendStartupInfoPacket(void)
{
	udp_sendData(&packet_StartupInfo, sizeof(packet_StartupInfo));
}

#pragma GCC push_options
#pragma GCC optimize ("unroll-loops")
void udp_clientSendPackets(struct packet_Image *rgbBuffers)
{
	static int32_t packet = 0;

	for (packet = UDP_NB_PACKET_PER_LINE; --packet >= 0;)
	{
		rgbBuffers[packet].packet_id = packetsCounter++;
		udp_sendData(&rgbBuffers[packet], sizeof(struct packet_Image));
	}

	packet_IMU.packet_id = packetsCounter++;

	//icm42688_getAGT();
    /*
	packet_IMU.gyro[0] = (uint16_t)(icm42688_gyrX() * 100.0);
	packet_IMU.gyro[1] = (uint16_t)(icm42688_gyrY() * 100.0);
	packet_IMU.gyro[2] = (uint16_t)(icm42688_gyrZ() * 100.0);

	packet_IMU.acc[0] = (uint16_t)(icm42688_accX() * 100.0);
	packet_IMU.acc[1] = (uint16_t)(icm42688_accY() * 100.0);
	packet_IMU.acc[2] = (uint16_t)(icm42688_accZ() * 100.0);
     */

	udp_sendData(&packet_IMU, sizeof(packet_IMU));

	packet_HID.packet_id = packetsCounter++;

	packet_HID.button_A = 0;
	packet_HID.button_B = 0;
	packet_HID.button_C = 0;

	udp_sendData(&packet_HID, sizeof(packet_HID));
}
#pragma GCC pop_options

void udp_clientReceiveCallback(void *arg, struct udp_pcb *upcb, struct pbuf *p, const ip_addr_t *addr, u16_t port)
{
	/*increment message count */
	message_count++;

	/* Free receive pbuf */
	pbuf_free(p);
}
