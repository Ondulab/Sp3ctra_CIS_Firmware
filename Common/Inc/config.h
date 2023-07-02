/* config.h */

#ifndef __CONFIG_H__
#define __CONFIG_H__

#include "stm32h7xx_hal.h"

/**************************************************************************************/
/********************              STM32 definitions               ********************/
/**************************************************************************************/
#define FLASH_BASE_ADDR      					(uint32_t)(FLASH_BASE)
#define FLASH_END_ADDR       					(uint32_t)(0x081FFFFF)

/* Base address of the Flash sectors Bank 1 */
#define ADDR_FLASH_SECTOR_0_BANK1    			((uint32_t)0x08000000) /* Base @ of Sector 0, 128 Kbytes */
#define ADDR_FLASH_SECTOR_1_BANK1     			((uint32_t)0x08020000) /* Base @ of Sector 1, 128 Kbytes */
#define ADDR_FLASH_SECTOR_2_BANK1     			((uint32_t)0x08040000) /* Base @ of Sector 2, 128 Kbytes */
#define ADDR_FLASH_SECTOR_3_BANK1     			((uint32_t)0x08060000) /* Base @ of Sector 3, 128 Kbytes */
#define ADDR_FLASH_SECTOR_4_BANK1     			((uint32_t)0x08080000) /* Base @ of Sector 4, 128 Kbytes */
#define ADDR_FLASH_SECTOR_5_BANK1     			((uint32_t)0x080A0000) /* Base @ of Sector 5, 128 Kbytes */
#define ADDR_FLASH_SECTOR_6_BANK1     			((uint32_t)0x080C0000) /* Base @ of Sector 6, 128 Kbytes */
#define ADDR_FLASH_SECTOR_7_BANK1     			((uint32_t)0x080E0000) /* Base @ of Sector 7, 128 Kbytes */

/* Base address of the Flash sectors Bank 2 */
#define ADDR_FLASH_SECTOR_0_BANK2     			((uint32_t)0x08100000) /* Base @ of Sector 0, 128 Kbytes */
#define ADDR_FLASH_SECTOR_1_BANK2     			((uint32_t)0x08120000) /* Base @ of Sector 1, 128 Kbytes */
#define ADDR_FLASH_SECTOR_2_BANK2     			((uint32_t)0x08140000) /* Base @ of Sector 2, 128 Kbytes */
#define ADDR_FLASH_SECTOR_3_BANK2     			((uint32_t)0x08160000) /* Base @ of Sector 3, 128 Kbytes */
#define ADDR_FLASH_SECTOR_4_BANK2     			((uint32_t)0x08180000) /* Base @ of Sector 4, 128 Kbytes */
#define ADDR_FLASH_SECTOR_5_BANK2     			((uint32_t)0x081A0000) /* Base @ of Sector 5, 128 Kbytes */
#define ADDR_FLASH_SECTOR_6_BANK2     			((uint32_t)0x081C0000) /* Base @ of Sector 6, 128 Kbytes */
#define ADDR_FLASH_SECTOR_7_BANK2     			((uint32_t)0x081E0000) /* Base @ of Sector 7, 128 Kbytes */

#define ADDR_CIS_FLASH_CALIBRATION				(ADDR_FLASH_SECTOR_7_BANK1)
#define ADDR_CIS_FLASH_PARAMETERS				(ADDR_FLASH_SECTOR_7_BANK2)

/**************************************************************************************/
/********************              debug definitions               ********************/
/**************************************************************************************/
//#define PRINT_CIS_CALIBRATION
//#define SKIP_SPLASH
//#define ETHERNET_OFF
//#define CIS_DESACTIVATE_CALIBRATION

/**************************************************************************************/
/********************             Display definitions              ********************/
/**************************************************************************************/
#define DISPLAY_WIDTH				SSD1362_WIDTH
#define DISPLAY_HEIGHT				SSD1362_HEIGHT

#define DISPLAY_HEAD_HEIGHT			(9)
#define DISPLAY_AERAS1_HEIGHT		(40)
#define DISPLAY_AERAS2_HEIGHT		(13)
//#define DISPLAY_AERAS3_HEIGHT		(9)
#define DISPLAY_INTER_AERAS_HEIGHT	(1)

#define DISPLAY_HEAD_Y1POS			(0)
#define DISPLAY_HEAD_Y2POS			(DISPLAY_HEAD_HEIGHT)

#define DISPLAY_AERA1_Y1POS			(DISPLAY_HEAD_HEIGHT + DISPLAY_INTER_AERAS_HEIGHT)
#define DISPLAY_AERA1_Y2POS			(DISPLAY_AERA1_Y1POS + DISPLAY_AERAS1_HEIGHT)

#define DISPLAY_AERA2_Y1POS			(DISPLAY_AERA1_Y2POS + DISPLAY_INTER_AERAS_HEIGHT)
#define DISPLAY_AERA2_Y2POS			(DISPLAY_AERA2_Y1POS + DISPLAY_AERAS2_HEIGHT)

//#define DISPLAY_AERA3_Y1POS			(DISPLAY_AERA2_Y2POS + DISPLAY_INTER_AERAS_HEIGHT)
//#define DISPLAY_AERA3_Y2POS			(DISPLAY_AERA3_Y1POS + DISPLAY_AERAS3_HEIGHT)

/**************************************************************************************/
/******************              Ethernet definitions               *******************/
/**************************************************************************************/
#define LWIP_CLK_FREQ							(1000)			//in Hz

#define UDP_HEADER_SIZE							(1)//uint32
#define UDP_NB_PACKET_PER_LINE					(6)
#define UDP_PACKET_SIZE							(((CIS_PIXELS_NB) / (UDP_NB_PACKET_PER_LINE)) + (UDP_HEADER_SIZE))

/* UDP local connection port */
#define UDP_SERVER_PORT    						((uint16_t)55151U)
/* UDP remote connection port */
#define UDP_CLIENT_PORT   						((uint16_t)55151U)

/*Static IP ADDRESS: */
#define IP_ADDR0   								((uint8_t) 192U)
#define IP_ADDR1   								((uint8_t) 168U)
#define IP_ADDR2   								((uint8_t) 0U)
#define IP_ADDR3   								((uint8_t) 10U)

/*Static DESTINATION IP ADDRESS: */
#define DEST_IP_ADDR0   						((uint8_t) 192U)
#define DEST_IP_ADDR1   						((uint8_t) 168U)
#define DEST_IP_ADDR2   						((uint8_t) 0U)
#define DEST_IP_ADDR3   						((uint8_t) 255U)

/*NETMASK*/
#define NETMASK_ADDR0   						((uint8_t) 255U)
#define NETMASK_ADDR1   						((uint8_t) 255U)
#define NETMASK_ADDR2   						((uint8_t) 255U)
#define NETMASK_ADDR3   						((uint8_t) 0U)

/*Gateway Address*/
#define GW_ADDR0   								((uint8_t) 0U)
#define GW_ADDR1   								((uint8_t) 0U)
#define GW_ADDR2   								((uint8_t) 0U)
#define GW_ADDR3   								((uint8_t) 0U)

/**************************************************************************************/
/********************              CIS definitions                 ********************/
/**************************************************************************************/
//#define CIS_400DPI
//#define CIS_MONOCHROME

#define CIS_CLK_FREQ							(2500000)
#define CLK_DIVIDER 							((400000000) / (CIS_CLK_FREQ)) //120 = 4MHz 96 = 5Mhz

#define CIS_ADC_OUT_LINES						(3)

#define CIS_SP_WIDTH							(2)

#define CIS_INACTIVE_WIDTH						(38 + CIS_SP_WIDTH)

#ifdef CIS_400DPI
#define CIS_PIXELS_PER_LINE						(1152)
#else
#define CIS_PIXELS_PER_LINE						(576)
#endif
#define CIS_PIXELS_NB 		 					((CIS_PIXELS_PER_LINE) * (CIS_ADC_OUT_LINES))

#define CIS_PIXEL_AERA_STOP						((CIS_INACTIVE_WIDTH) + (CIS_PIXELS_PER_LINE))
#define CIS_OVER_SCAN							(12)

#define CIS_LED_RED_ON							(CIS_INACTIVE_WIDTH + 30)
#define CIS_LED_GREEN_ON						(CIS_INACTIVE_WIDTH + 30)
#define CIS_LED_BLUE_ON							(CIS_INACTIVE_WIDTH + 30)
#define CIS_LED_RED_OFF							((202.0 * 2.5))
#define CIS_LED_GREEN_OFF						((244.0 * 2.5))
#define CIS_LED_BLUE_OFF						((243.0 * 2.5))

#define CIS_START_OFFSET	 	 				(CIS_INACTIVE_WIDTH - CIS_SP_WIDTH + 2)
#define CIS_LINE_SIZE 							(CIS_PIXEL_AERA_STOP + CIS_OVER_SCAN)
#define CIS_END_CAPTURE							(CIS_LINE_SIZE)

#define CIS_ADC_BUFF_SIZE 	 	 		 		((CIS_LINE_SIZE) * (CIS_ADC_OUT_LINES))

#endif // __CONFIG_H__

