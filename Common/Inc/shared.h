/**
 ******************************************************************************
 * @file           : shared.h
 ******************************************************************************
 * @attention
 *
 * Copyright (C) 2018-present Reso-nance Numerique.
 * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
 *
 ******************************************************************************
 */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __SHARED_H__
#define __SHARED_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

/* Private includes ----------------------------------------------------------*/
#include "stdint.h"
#include "config.h"
#include "arm_math.h"

/* Exported types ------------------------------------------------------------*/

typedef enum
{
 SW1  = 0x00U,
 SW2  = 0x01U,
 SW3  = 0x02U,
 SW4  = 0x03U,
 SW5  = 0x04U
}buttonTypeDef;

typedef enum
{
	SWITCH_RELEASED = 0,
	SWITCH_PRESSED
}buttonStateTypeDef;

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
__attribute__ ((packed))
struct packet_StartupInfo {
    uint8_t type; 						// Identifies the data type
    uint32_t packet_id;               	// Sequence number, useful for ordering packets
    uint8_t version_info[64]; 			// Information about the version, and other startup details
};

// Structure for image data packets, including metadata for image fragmentation
__attribute__ ((packed))
struct packet_Image {
    uint8_t type; 						// Identifies the data type
    uint32_t packet_id;               	// Sequence number, useful for ordering packets
    uint32_t line_id;      				// Line identifier
    uint8_t fragment_id;      			// Fragment position
    uint8_t total_fragments;  			// Total number of fragments for the complete image
    uint16_t fragment_size;   			// Size of this particular fragment
    uint8_t imageData_R[CIS_PIXELS_NB / UDP_NB_PACKET_PER_LINE];   			// Pointer to the fragmented red image data
    uint8_t imageData_G[CIS_PIXELS_NB / UDP_NB_PACKET_PER_LINE];   			// Pointer to the fragmented green image data
    uint8_t imageData_B[CIS_PIXELS_NB / UDP_NB_PACKET_PER_LINE];   			// Pointer to the fragmented blue image data
};

// Structure for packets containing button state information
__attribute__ ((packed))
struct packet_HID {
    uint8_t type; 						// Identifies the data type
    uint32_t packet_id;               	// Sequence number, useful for ordering packets
    uint8_t button_A;     				// State of the buttons (pressed/released, etc.)
    uint8_t button_B;     				// State of the buttons (pressed/released, etc.)
    uint8_t button_C;     				// State of the buttons (pressed/released, etc.)
};

// Structure for packets containing sensor data (accelerometer and gyroscope)
__attribute__ ((packed))
struct packet_IMU {
    uint8_t type; 						// Identifies the data type
    uint32_t packet_id;               	// Sequence number, useful for ordering packets
    float32_t acc[3];           			// Accelerometer data: x, y, and z axis
    float32_t gyro[3];          			// Gyroscope data: x, y, and z axis
    float32_t integrated_acc[3];          // Accelerometer data: x, y, and z axis
    float32_t integrated_gyro[3];         // Gyroscope data: x, y, and z axis
};

__attribute__ ((packed))
struct cisRgbBuffers {
	uint8_t R[CIS_PIXELS_NB];
	uint8_t G[CIS_PIXELS_NB];
	uint8_t B[CIS_PIXELS_NB];
};

typedef enum
{
	CIS_CAL_REQUESTED = 0,
	CIS_CAL_START,
	CIS_CAL_PLACE_ON_WHITE,
	CIS_CAL_PLACE_ON_BLACK,
	CIS_CAL_EXTRACT_INNACTIVE_REF,
	CIS_CAL_EXTRACT_EXTREMUMS,
	CIS_CAL_EXTRACT_OFFSETS,
	CIS_CAL_COMPUTE_GAINS,
	CIS_CAL_END,
}CIS_Calibration_StateTypeDef;

__attribute__ ((packed))
struct cisCals {
	float32_t offsetData[CIS_ADC_BUFF_SIZE * 3];
	float32_t gainsData[CIS_ADC_BUFF_SIZE * 3];
};

__attribute__ ((packed))
struct shared_var {
	int32_t cis_process_rdy;
	int32_t cis_process_cnt;
	int32_t cis_freq;
	int32_t cis_cal_request;
	uint32_t cis_cal_progressbar;
	CIS_Calibration_StateTypeDef cis_cal_state;
	buttonStateTypeDef  buttonState[3];
};

__attribute__ ((packed))
struct shared_config
{
    uint32_t ui_button_delay;
    uint8_t network_ip[4];
    uint8_t network_netmask[4];
    uint8_t network_gw[4];
    uint8_t network_dest_ip[4];
    uint16_t network_udp_port;
    uint8_t cis_print_calibration;
    uint8_t cis_raw;
    uint16_t cis_dpi;
    uint8_t cis_monochrome;
    uint16_t cis_max_line_freq;
    uint32_t cis_clk_freq;
    uint8_t cis_oversampling;
    uint8_t cis_handedness;
};

/**************************************************************************************/
/******************                  CM4 and CM7                    *******************/
/**************************************************************************************/

extern volatile struct shared_var shared_var;
extern volatile struct shared_config shared_config;
extern struct packet_Image packet_Image[UDP_NB_PACKET_PER_LINE];
extern struct packet_IMU packet_IMU;
extern int params_size;


/**************************************************************************************/
/******************                      CM7                        *******************/
/**************************************************************************************/

__attribute__ ((packed))
struct CalibrationCoefficients {
    float32_t a;
    float32_t b;
    float32_t c;
};

__attribute__ ((packed))
struct RGB_Calibration {
	struct CalibrationCoefficients red[CIS_PIXELS_NB];
	struct CalibrationCoefficients green[CIS_PIXELS_NB];
	struct CalibrationCoefficients blue[CIS_PIXELS_NB];
};

__attribute__ ((packed))
struct cisLeds_Calibration {
	int32_t redLed_power2PWM[CIS_LEDS_MAX_PWM + 1];
	int32_t greenLed_power2PWM[CIS_LEDS_MAX_PWM + 1];
	int32_t blueLed_power2PWM[CIS_LEDS_MAX_PWM + 1];
	int32_t redLed_maxPulse;
	int32_t greenLed_maxPulse;
	int32_t blueLed_maxPulse;
	float32_t redMeanAtLedPower;
	float32_t greenMeanAtLedPower;
	float32_t blueMeanAtLedPower;
};

__attribute__((aligned(4)))
struct RAWImage{
     float32_t redLine[CIS_PIXELS_NB];
     float32_t greenLine[CIS_PIXELS_NB];
     float32_t blueLine[CIS_PIXELS_NB];
};

extern int16_t cisData[CIS_ADC_BUFF_SIZE * 3];
extern float32_t cisDataCpy_f32[CIS_ADC_BUFF_SIZE * 3];
extern struct RGB_Calibration rgbCalibration;
extern struct cisCals cisCals;
//extern struct RAWImage RAWImageCalibration[11];
extern struct cisLeds_Calibration cisLeds_Calibration;
extern q31_t cisDataCpy_q31[CIS_ADC_BUFF_SIZE * 3] __attribute__ ((aligned (32)));

/* Exported constants --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/

/* Exported functions prototypes ---------------------------------------------*/

/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /*__SHARED_H__*/
