/**
 ******************************************************************************
 * @file           : udp_client.h
 * @brief          : 5 key keybord driver
 ******************************************************************************
 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __UDP_CLIENT_H__
#define __UDP_CLIENT_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "arm_math.h"
#include "config.h"
#include "shared.h"

/* Private includes ----------------------------------------------------------*/

/* Exported types ------------------------------------------------------------*/
typedef enum
{
	STARTUP_INFO_HEADER = 0,
	IMAGE_DATA_HEADER,
	IMU_DATA_HEADER,
	HID_DATA_HEADER,
}CIS_Packet_HeaderTypeDef;

typedef enum
{
	IMAGE_COLOR_R = 0,
	IMAGE_COLOR_G,
	IMAGE_COLOR_B,
}CIS_Packet_ImageColorTypeDef;

// Packet header structure defining the common header for all packet types// Structure for packets containing startup information like version info
struct packet_StartupInfo{
    uint8_t type; 						// Identifies the data type
    uint32_t packet_id;               	// Sequence number, useful for ordering packets
    uint8_t version_info[64]; 			// Information about the version, and other startup details
};

// Structure for image data packets, including metadata for image fragmentation
struct packet_Image{
    uint8_t type; 						// Identifies the data type
    uint32_t packet_id;               	// Sequence number, useful for ordering packets
    uint32_t line_id;      				// Identifier for the fragment of the image
    uint8_t fragment_id;      				// Identifier for the fragment of the image
    uint8_t total_fragments;  			// Total number of fragments for the complete image
    uint16_t fragment_size;   			// Size of this particular fragment
    uint8_t imageColor;   				// Pointer to the fragmented image data
    uint8_t imageData[CIS_PIXELS_NB / UDP_NB_PACKET_PER_LINE];   			// Pointer to the fragmented image data
};

// Structure for packets containing button state information
struct packet_HID {
    uint8_t type; 						// Identifies the data type
    uint32_t packet_id;               	// Sequence number, useful for ordering packets
    uint8_t button_A;     				// State of the buttons (pressed/released, etc.)
    uint8_t button_B;     				// State of the buttons (pressed/released, etc.)
    uint8_t button_C;     				// State of the buttons (pressed/released, etc.)
};

// Structure for packets containing sensor data (accelerometer and gyroscope)
struct packet_IMU {
    uint8_t type; 						// Identifies the data type
    uint32_t packet_id;               	// Sequence number, useful for ordering packets
    int16_t acc[3];           			// Accelerometer data: x, y, and z axis
    int16_t gyro[3];          			// Gyroscope data: x, y, and z axis
    int16_t integrated_acc[3];          // Accelerometer data: x, y, and z axis
    int16_t integrated_gyro[3];         // Gyroscope data: x, y, and z axis
};

/* Exported constants --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/

/* Exported functions prototypes ---------------------------------------------*/
void udp_clientInit(void);
void udp_clientSendStartupInfoPacket(void);
void udp_clientSendPackets(struct cisRgbBuffers *rgbBuffers);
void udp_clientSendImage(int32_t *image_buff);

/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /*__UDP_CLIENT_H__*/
