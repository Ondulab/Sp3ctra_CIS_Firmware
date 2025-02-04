/**
 ******************************************************************************
 * @file           : globals.h
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

/**************************************************************************************/
/******************           COMMON STRUCTURE CIS / MAX            *******************/
/**************************************************************************************/

typedef enum
{
	SW1  = 0,
	SW2,
	SW3,
}buttonIdTypeDef;

typedef enum
{
	SWITCH_RELEASED = 0,
	SWITCH_PRESSED
}buttonStateTypeDef;

typedef enum
{
	LED_1 = 0,
	LED_2,
	LED_3,
}ledIdTypeDef;

typedef enum
{
    STARTUP_INFO_HEADER = 0x11,
    IMAGE_DATA_HEADER = 0x12,
    IMU_DATA_HEADER = 0x13,
    BUTTON_DATA_HEADER= 0x14,
    LED_DATA_HEADER = 0x15,
}CIS_Packet_HeaderTypeDef;

typedef enum
{
	IMAGE_COLOR_R = 0,
	IMAGE_COLOR_G,
	IMAGE_COLOR_B,
}CIS_Packet_ImageColorTypeDef;

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

// Packet header structure defining the common header for all packet types// Structure for packets containing startup information like version info
struct __attribute__((aligned(4))) packet_StartupInfo
{
	CIS_Packet_HeaderTypeDef type; 					// Identifies the data type
	uint32_t packet_id;               				// Sequence number, useful for ordering packets
	uint8_t version_info[32]; 						// Information about the version, and other startup details
};

// Structure for image data packets, including metadata for image fragmentation
struct __attribute__((aligned(4))) packet_Scanline
{
	CIS_Packet_HeaderTypeDef type; 					// Identifies the data type
	uint32_t packet_id;               				// Sequence number, useful for ordering packets
	uint32_t line_id;      							// Line identifier
	uint8_t fragment_id;      						// Fragment position
	uint8_t total_fragments;  						// Total number of fragments for the complete image
	uint16_t fragment_size;   						// Size of this particular fragment
	uint8_t imageData_R[UDP_LINE_FRAGMENT_SIZE];   	// Pointer to the fragmented red image data
	uint8_t imageData_G[UDP_LINE_FRAGMENT_SIZE];  	// Pointer to the fragmented green image data
	uint8_t imageData_B[UDP_LINE_FRAGMENT_SIZE];	// Pointer to the fragmented blue image data
};

struct __attribute__((aligned(4))) buffers_Scanline
{
	struct packet_Scanline scanline_buff1[UDP_MAX_NB_PACKET_PER_LINE];
	struct packet_Scanline scanline_buff2[UDP_MAX_NB_PACKET_PER_LINE];
};

struct __attribute__((aligned(4))) button_State
{
	buttonStateTypeDef state;
	uint32_t pressed_time;
};

// Structure for packets containing button state information
struct __attribute__((aligned(4))) packet_Button
{
	CIS_Packet_HeaderTypeDef type; 					// Identifies the data type
	uint32_t packet_id;               				// Sequence number, useful for ordering packets
	buttonIdTypeDef button_id;     					// Id of the button
	struct button_State button_state;     			// State of the led A
};

struct __attribute__((aligned(4))) led_State
{
    uint16_t brightness_1;
    uint16_t time_1;
    uint16_t glide_1;
    uint16_t brightness_2;
    uint16_t time_2;
    uint16_t glide_2;
    uint32_t blink_count;
};

// Structure for packets containing leds state
struct __attribute__((aligned(4))) packet_Leds
{
	CIS_Packet_HeaderTypeDef type; 					// Identifies the data type
	uint32_t packet_id;               				// Sequence number, useful for ordering packets
	ledIdTypeDef led_id;     						// Id of the led
	struct led_State led_state;     				// State of the selected led
};

// Structure for packets containing sensor data (accelerometer and gyroscope)
struct __attribute__((aligned(4))) packet_IMU
{
	CIS_Packet_HeaderTypeDef type; 					// Identifies the data type
	uint32_t packet_id;               				// Sequence number, useful for ordering packets
	float_t acc[3];           						// Accelerometer data: x, y, and z axis
	float_t gyro[3];          						// Gyroscope data: x, y, and z axis
	float_t integrated_acc[3];        				// Accelerometer data: x, y, and z axis
	float_t integrated_gyro[3];       				// Gyroscope data: x, y, and z axis
};

struct __attribute__((aligned(4))) cisRgbBuffers
{
	uint8_t R[CIS_MAX_PIXELS_NB];
	uint8_t G[CIS_MAX_PIXELS_NB];
	uint8_t B[CIS_MAX_PIXELS_NB];
};

/**************************************************************************************/

struct __attribute__((aligned(4))) cisCals
{
	int32_t offsetData[CIS_MAX_ADC_BUFF_SIZE * 3];
	int32_t gainsData[CIS_MAX_ADC_BUFF_SIZE * 3];
};

struct __attribute__((aligned(4))) shared_var
{
	int32_t cis_process_rdy;
	int32_t cis_process_cnt;
	int32_t cis_freq;
	int32_t cis_cal_request;
	uint32_t cis_cal_progressbar;
	CIS_Calibration_StateTypeDef cis_cal_state;
	struct button_State buttonState[3];
	struct led_State ledState[3];
    uint32_t led_update_requested[3];
    uint32_t button_update_requested[3];
};

struct __attribute__((aligned(4))) shared_config
{
	uint32_t ui_button_delay;
	uint8_t network_ip[4];
	uint8_t network_netmask[4];
	uint8_t network_gw[4];
	uint8_t network_dest_ip[4];
	uint16_t network_udp_port;
	uint16_t network_tcp_port;
	uint8_t cis_print_calibration;
	uint16_t cis_dpi;
	uint32_t cis_clk_freq;
	uint8_t cis_oversampling;
	uint8_t cis_handedness;
};

/**************************************************************************************/
/******************                  CM4 and CM7                    *******************/
/**************************************************************************************/

extern volatile struct shared_var shared_var;
extern volatile struct shared_config shared_config;
extern volatile struct packet_Scanline scanline_CM4[UDP_MAX_NB_PACKET_PER_LINE];
extern struct packet_IMU packet_IMU;
extern int params_size;


/**************************************************************************************/
/******************                      CM7                        *******************/
/**************************************************************************************/

__attribute__((aligned(4)))
typedef struct
{
    int32_t pixels_per_lane;
    int32_t pixels_nb;
    int32_t pixel_area_stop;
    int32_t start_offset;
    int32_t lane_size;
    int32_t adc_buff_size;

    int32_t red_lane_offset;
    int32_t green_lane_offset;
    int32_t blue_lane_offset;

    int32_t leds_off_index;

    uint16_t udp_nb_packet_per_line;
} CIS_Config;

__attribute__((aligned(4)))
struct CalibrationCoefficients
{
	float32_t a;
	float32_t b;
	float32_t c;
};

__attribute__((aligned(4)))
struct cisRGB_Calibration
{
	struct CalibrationCoefficients red[CIS_MAX_PIXELS_NB];
	struct CalibrationCoefficients green[CIS_MAX_PIXELS_NB];
	struct CalibrationCoefficients blue[CIS_MAX_PIXELS_NB];
};

__attribute__((aligned(4)))
struct cisLeds_Calibration
{
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
	float32_t redLine[CIS_MAX_PIXELS_NB];
	float32_t greenLine[CIS_MAX_PIXELS_NB];
	float32_t blueLine[CIS_MAX_PIXELS_NB];
};

extern struct buffers_Scanline buffers_Scanline;
extern CIS_Config cisConfig;
extern uint16_t cisData[CIS_MAX_ADC_BUFF_SIZE * 3];
extern uint32_t cisDataCpy[CIS_MAX_ADC_BUFF_SIZE * 3];
extern struct cisRGB_Calibration cisRGB_Calibration;
extern struct cisCals cisCals;
extern struct cisLeds_Calibration cisLeds_Calibration;

/* Exported constants --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/

/* Exported functions prototypes ---------------------------------------------*/

/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /*__SHARED_H__*/
