/* USER CODE BEGIN Header */
/**
 ******************************************************************************
  * File Name          : LWIP.c
  * Description        : This file provides initialization code for LWIP
  *                      middleWare.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "lwip.h"
#include "lwip/init.h"
#include "lwip/netif.h"
#if (defined ( __CC_ARM ) || defined (__ARMCC_VERSION))  /* MDK ARM Compiler */
#include "lwip/sio.h"
#endif /* MDK ARM Compiler */
#include "ethernetif.h"
#include <string.h>

/* USER CODE BEGIN 0 */
#include "globals.h"
#include "stdio.h"
#include "udp_client.h"
#include "http_server.h"
#include "lwip/netifapi.h"
#include "lwip/apps/mdns.h"
#include "rtpmidi.h"

/* Global variable to track system initialization state */
volatile uint8_t systemFullyInitialized = 0;

/* USER CODE END 0 */
/* Private function prototypes -----------------------------------------------*/
static void ethernet_link_status_updated(struct netif *netif);
/* ETH Variables initialization ----------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN 1 */

/**
 * @brief Perform automatic system reset after network disconnection
 */
static void performAutomaticReset(void)
{
    printf("=== AUTOMATIC SYSTEM RESET ===\n");
    printf("Network disconnection detected after full initialization\n");
    printf("Performing system reset in 2 seconds...\n");

    // Give time for the message to be transmitted
    osDelay(2000);

    System_SafeReset();
}

/**
 * @brief mDNS TXT callback for Apple MIDI service
 * @note Adds required TXT records for Apple MIDI (RTP-MIDI) discovery
 */
static void mdns_apple_midi_txt_callback(struct mdns_service *service, void *txt_userdata)
{
    err_t res;

    // Add txtvers=1 (required by Apple MIDI spec)
    res = mdns_resp_add_service_txtitem(service, "txtvers=1", 9);
    if (res != ERR_OK) {
        printf("mDNS: Failed to add txtvers TXT record\n");
        return;
    }

    // Add protovers=2 (required by Apple MIDI spec - RTP-MIDI protocol version)
    res = mdns_resp_add_service_txtitem(service, "protovers=2", 11);
    if (res != ERR_OK) {
        printf("mDNS: Failed to add protovers TXT record\n");
        return;
    }

    printf("mDNS: Apple MIDI TXT records added (txtvers=1, protovers=2)\n");
}

/* USER CODE END 1 */

/* Variables Initialization */
struct netif gnetif;
ip4_addr_t ipaddr;
ip4_addr_t netmask;
ip4_addr_t gw;
uint8_t IP_ADDRESS[4];
uint8_t NETMASK_ADDRESS[4];
uint8_t GATEWAY_ADDRESS[4];
/* USER CODE BEGIN OS_THREAD_ATTR_CMSIS_RTOS_V2 */
#define INTERFACE_THREAD_STACK_SIZE ( 1024 )
osThreadAttr_t attributes;
/* USER CODE END OS_THREAD_ATTR_CMSIS_RTOS_V2 */

/* USER CODE BEGIN 2 */

/* USER CODE END 2 */

/**
  * LwIP initialization function
  */
void MX_LWIP_Init(void)
{
  /* IP addresses initialization */
  IP_ADDRESS[0] = 192;
  IP_ADDRESS[1] = 168;
  IP_ADDRESS[2] = 0;
  IP_ADDRESS[3] = 10;
  NETMASK_ADDRESS[0] = 255;
  NETMASK_ADDRESS[1] = 255;
  NETMASK_ADDRESS[2] = 255;
  NETMASK_ADDRESS[3] = 0;
  GATEWAY_ADDRESS[0] = 0;
  GATEWAY_ADDRESS[1] = 0;
  GATEWAY_ADDRESS[2] = 0;
  GATEWAY_ADDRESS[3] = 0;

/* USER CODE BEGIN IP_ADDRESSES */
    IP_ADDRESS[0] = shared_config.network_ip[0];
    IP_ADDRESS[1] = shared_config.network_ip[1];
    IP_ADDRESS[2] = shared_config.network_ip[2];
    IP_ADDRESS[3] = shared_config.network_ip[3];
    NETMASK_ADDRESS[0] = shared_config.network_netmask[0];
    NETMASK_ADDRESS[1] = shared_config.network_netmask[1];
    NETMASK_ADDRESS[2] = shared_config.network_netmask[2];
    NETMASK_ADDRESS[3] = shared_config.network_netmask[3];
    GATEWAY_ADDRESS[0] = shared_config.network_gw[0];
    GATEWAY_ADDRESS[1] = shared_config.network_gw[1];
    GATEWAY_ADDRESS[2] = shared_config.network_gw[2];
    GATEWAY_ADDRESS[3] = shared_config.network_gw[3];

/* USER CODE END IP_ADDRESSES */

  /* Initialize the LwIP stack with RTOS */
  tcpip_init( NULL, NULL );

  /* IP addresses initialization without DHCP (IPv4) */
  IP4_ADDR(&ipaddr, IP_ADDRESS[0], IP_ADDRESS[1], IP_ADDRESS[2], IP_ADDRESS[3]);
  IP4_ADDR(&netmask, NETMASK_ADDRESS[0], NETMASK_ADDRESS[1] , NETMASK_ADDRESS[2], NETMASK_ADDRESS[3]);
  IP4_ADDR(&gw, GATEWAY_ADDRESS[0], GATEWAY_ADDRESS[1], GATEWAY_ADDRESS[2], GATEWAY_ADDRESS[3]);

  /* add the network interface (IPv4/IPv6) with RTOS */
  netif_add(&gnetif, &ipaddr, &netmask, &gw, NULL, &ethernetif_init, &tcpip_input);

  /* Registers the default network interface */
  netif_set_default(&gnetif);

  /* We must always bring the network interface up connection or not... */
  netif_set_up(&gnetif);

  /* Set the link callback function, this function is called on change of link status*/
  netif_set_link_callback(&gnetif, ethernet_link_status_updated);

  /* Create the Ethernet link handler thread */
/* USER CODE BEGIN H7_OS_THREAD_NEW_CMSIS_RTOS_V2 */
  memset(&attributes, 0x0, sizeof(osThreadAttr_t));
  attributes.name = "EthLink";
  attributes.stack_size = INTERFACE_THREAD_STACK_SIZE;
  attributes.priority = osPriorityBelowNormal;
  osThreadNew(ethernet_link_thread, &gnetif, &attributes);
/* USER CODE END H7_OS_THREAD_NEW_CMSIS_RTOS_V2 */

/* USER CODE BEGIN 3 */
  // Initialize mDNS responder (official LwIP 2.2.1 implementation)
  // mDNS is enabled purely based on configuration, independent of RTP-MIDI mode.
  // In RTP-MIDI CLIENT mode, advertising the _apple-midi service may be undesired,
  // but the mDNS hostname remains useful for device discovery.
  if (shared_config.mdns_enabled) {
    printf("--- mDNS INITIALIZATIONS ---\n");
    mdns_resp_init();

    // Add network interface to mDNS (publishes hostname)
    // LwIP 2.2: the TTL argument was removed (MDNS_TTL_* defaults are used)
    err_t err = mdns_resp_add_netif(&gnetif, "sp3ctra");
    if (err == ERR_OK) {
      printf("mDNS: Network interface added successfully\n");

      // Advertise RTP-MIDI service regardless of RTP-MIDI mode.
      // This matches the configuration intent: if mDNS is enabled, always publish the service.
      // Note: mdns_resp_add_service returns s8_t (slot ID), not err_t
      s8_t slot = mdns_resp_add_service(&gnetif, "sp3ctra", "_apple-midi",
                                        DNSSD_PROTO_UDP, shared_config.rtpmidi_control_port,
                                        mdns_apple_midi_txt_callback, NULL);
      if (slot >= 0) {
        printf("mDNS: RTP-MIDI service registered successfully (slot=%d)\n", slot);
      } else {
        printf("mDNS: Failed to register RTP-MIDI service (slot=%d)\n", slot);
      }
    } else {
      printf("mDNS: Failed to add network interface (err=%d)\n", err);
    }
  } else {
    printf("mDNS: Service disabled in configuration\n");
  }
/* USER CODE END 3 */
}

#ifdef USE_OBSOLETE_USER_CODE_SECTION_4
/* Kept to help code migration. (See new 4_1, 4_2... sections) */
/* Avoid to use this user section which will become obsolete. */
/* USER CODE BEGIN 4 */
/* USER CODE END 4 */
#endif

/**
  * @brief  Notify the User about the network interface config status
  * @param  netif: the network interface
  * @retval None
  */
static void ethernet_link_status_updated(struct netif *netif)
{
  if (netif_is_up(netif))
  {
/* USER CODE BEGIN 5 */
    /* Lien Ethernet actif */
    printf("Ethernet link is UP\n");
    isConnected = 1;

    osSemaphoreRelease(udpReadySemaphoreHandle);  // Signal that the stack is ready
/* USER CODE END 5 */
  }
  else /* netif is down */
  {
/* USER CODE BEGIN 6 */
      /* Lien Ethernet inactif */
      printf("Ethernet link is DOWN\n");
      isConnected = 0;
      startupPacketSent = 0;

      /* Check if system was fully initialized before disconnection */
      if (systemFullyInitialized == 1)
      {
          printf("System was fully initialized - triggering automatic reset\n");
          performAutomaticReset();
      }
      else
      {
          printf("System not fully initialized yet - normal disconnection handling\n");
      }
/* USER CODE END 6 */
  }
}

#if (defined ( __CC_ARM ) || defined (__ARMCC_VERSION))  /* MDK ARM Compiler */
/**
 * Opens a serial device for communication.
 *
 * @param devnum device number
 * @return handle to serial device if successful, NULL otherwise
 */
sio_fd_t sio_open(u8_t devnum)
{
  sio_fd_t sd;

/* USER CODE BEGIN 7 */
  sd = 0; // dummy code
/* USER CODE END 7 */

  return sd;
}

/**
 * Sends a single character to the serial device.
 *
 * @param c character to send
 * @param fd serial device handle
 *
 * @note This function will block until the character can be sent.
 */
void sio_send(u8_t c, sio_fd_t fd)
{
/* USER CODE BEGIN 8 */
/* USER CODE END 8 */
}

/**
 * Reads from the serial device.
 *
 * @param fd serial device handle
 * @param data pointer to data buffer for receiving
 * @param len maximum length (in bytes) of data to receive
 * @return number of bytes actually received - may be 0 if aborted by sio_read_abort
 *
 * @note This function will block until data can be received. The blocking
 * can be cancelled by calling sio_read_abort().
 */
u32_t sio_read(sio_fd_t fd, u8_t *data, u32_t len)
{
  u32_t recved_bytes;

/* USER CODE BEGIN 9 */
  recved_bytes = 0; // dummy code
/* USER CODE END 9 */
  return recved_bytes;
}

/**
 * Tries to read from the serial device. Same as sio_read but returns
 * immediately if no data is available and never blocks.
 *
 * @param fd serial device handle
 * @param data pointer to data buffer for receiving
 * @param len maximum length (in bytes) of data to receive
 * @return number of bytes actually received
 */
u32_t sio_tryread(sio_fd_t fd, u8_t *data, u32_t len)
{
  u32_t recved_bytes;

/* USER CODE BEGIN 10 */
  recved_bytes = 0; // dummy code
/* USER CODE END 10 */
  return recved_bytes;
}
#endif /* MDK ARM Compiler */

