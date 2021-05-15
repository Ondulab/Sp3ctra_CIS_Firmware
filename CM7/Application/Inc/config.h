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

/**************************************************************************************/
/********************              debug definitions               ********************/
/**************************************************************************************/
//#define PRINT_IFFT_FREQUENCY
//#define PRINT_IFFT_FREQUENCY_FULL
//#define PRINT_CIS_CALIBRATION

/**************************************************************************************/
/********************             Display definitions              ********************/
/**************************************************************************************/
#define DISPLAY_REFRESH_FPS						(20)

/**************************************************************************************/
/********************              Synth definitions               ********************/
/**************************************************************************************/
#define SAMPLING_FREQUENCY				      	(48000)

#define IFFT_GAP_PER_MS							(5000)

#define STEREO_1

/**************************************************************************************/
/******************              Ethernet definitions               *******************/
/**************************************************************************************/
#define SEND_CIS_LINE_FREQUENCY				    (3000)
#define IMAGE_HEADER							(1397969715) //01010011 01010011 01010011 00110011 SSS3
#define IMAGE_HEADER_SIZE						(1)//uint32


/* UDP local connection port */
#define UDP_SERVER_PORT    						((uint16_t)60U)
/* UDP remote connection port */
#define UDP_CLIENT_PORT   						((uint16_t)60U)

/*Static DEST IP ADDRESS: DEST_IP_ADDR0.DEST_IP_ADDR1.DEST_IP_ADDR2.DEST_IP_ADDR3 */
#define DEST_IP_ADDR0   						((uint8_t)192U)
#define DEST_IP_ADDR1   						((uint8_t)168U)
#define DEST_IP_ADDR2   						((uint8_t)0U)
#define DEST_IP_ADDR3   						((uint8_t)1U)

/*Static IP ADDRESS: IP_ADDR0.IP_ADDR1.IP_ADDR2.IP_ADDR3 */
#define IP_ADDR0   								((uint8_t) 192U)
#define IP_ADDR1   								((uint8_t) 168U)
#define IP_ADDR2   								((uint8_t) 0U)
#define IP_ADDR3   								((uint8_t) 10U)

/*NETMASK*/
#define NETMASK_ADDR0   						((uint8_t) 255U)
#define NETMASK_ADDR1   						((uint8_t) 255U)
#define NETMASK_ADDR2   						((uint8_t) 255U)
#define NETMASK_ADDR3   						((uint8_t) 0U)

/*Gateway Address*/
#define GW_ADDR0   								((uint8_t) 192U)
#define GW_ADDR1   								((uint8_t) 168U)
#define GW_ADDR2   								((uint8_t) 0U)
#define GW_ADDR3   								((uint8_t) 1U)

/**************************************************************************************/
/********************              DAC definitions                 ********************/
/**************************************************************************************/
#define AUDIO_BUFFER_SIZE             			(256)

/**************************************************************************************/
/********************              CIS definitions                 ********************/
/**************************************************************************************/
#define CIS_400DPI
#define CIS_BW
#define CIS_INVERT_COLOR
//#define CIS_INVERT_COLOR_SMOOTH
//#define CIS_NORMAL_COLOR_SMOOTH

#define CIS_CLK_FREQ							(5000000)

#define CIS_READ_OFFSET_CORRECTION				(4)
#define CIS_SP_WIDTH							(2)
#define CIS_SP_ON								(CIS_SP_OFF - CIS_SP_WIDTH)
#define CIS_SP_OFF								(10 - CIS_READ_OFFSET_CORRECTION)
#define CIS_LED_ON								(CIS_SP_OFF + 4)
#define CIS_INACTIVE_WIDTH						(38 + CIS_READ_OFFSET_CORRECTION)
#define CIS_INACTIVE_AERA_STOP					(CIS_INACTIVE_WIDTH + CIS_SP_OFF)

#ifdef CIS_BW
#define CIS_LED_RED_OFF							(1200)//900
#define CIS_LED_GREEN_OFF						(1200)
#define CIS_LED_BLUE_OFF						(1200)
#else
#define CIS_LED_RED_OFF							(3600)
#define CIS_LED_GREEN_OFF						(3100)
#define CIS_LED_BLUE_OFF						(1600)
#endif

#ifdef CIS_400DPI
#define CIS_ACTIVE_PIXELS_PER_LINE				(1152)
#else
#define CIS_ACTIVE_PIXELS_PER_LINE				(576)
#endif
#define CIS_PIXEL_AERA_STOP						((CIS_ACTIVE_PIXELS_PER_LINE) + (CIS_INACTIVE_AERA_STOP))
#define CIS_OVER_SCAN							(64)
#define CIS_END_CAPTURE 						(CIS_PIXEL_AERA_STOP + CIS_OVER_SCAN)

#define CIS_ADC_OUT_LINES						(3)

#ifdef CIS_400DPI
#define CIS_IFFT_OVERSAMPLING_RATIO				(16)
#else
#define CIS_IFFT_OVERSAMPLING_RATIO				(8)
#endif

#define CIS_IMGPLY_OVERSAMPLING_RATIO			(16)

/**************************************************************************************/
/********************         Wave generation definitions          ********************/
/**************************************************************************************/
#define WAVE_AMP_RESOLUTION 					(65535)   	//in decimal
#define START_FREQUENCY     					(30)
#define MAX_OCTAVE_NUMBER   					(10)
#define SEMITONE_PER_OCTAVE 					(12)
#define COMMA_PER_SEMITONE  					(2)

#define NUMBER_OF_NOTES     					(((CIS_ACTIVE_PIXELS_PER_LINE) * (CIS_ADC_OUT_LINES)) / CIS_IFFT_OVERSAMPLING_RATIO)

#endif // __CONFIG_H__

