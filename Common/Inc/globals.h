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
#include "sp3ctra_link.h"
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
	IMAGE_COLOR_R = 0,
	IMAGE_COLOR_G,
	IMAGE_COLOR_B,
}CIS_Packet_ImageColorTypeDef;

typedef enum
{
	CIS_CAL_REQUESTED = 0,
	CIS_CAL_START,
	CIS_CAL_WHITE,                    // MODIFIÉ (ancien CIS_CAL_PLACE_ON_WHITE)
	CIS_CAL_INTERMEDIATE,             // NOUVEAU (ancien CIS_CAL_PLACE_ON_INTERMEDIATE)
	CIS_CAL_BLACK,                    // MODIFIÉ (ancien CIS_CAL_PLACE_ON_BLACK)
	CIS_CAL_EXTRACT_INNACTIVE_REF,
	CIS_CAL_EXTRACT_EXTREMUMS,
	CIS_CAL_EXTRACT_OFFSETS,
	CIS_CAL_COMPUTE_GAINS,
	CIS_CAL_COMPUTE_TRANSITIONS,      // NOUVEAU
	CIS_CAL_END,
}CIS_Calibration_StateTypeDef;

// Packet header structure defining the common header for all packet types// Structure for packets containing startup information like version info

// One LINE fragment as sent on the wire (SLP v1): negotiated header + planar RGB.
// sizeof == SLP_LINE_BYTES(UDP_LINE_FRAGMENT_SIZE); the whole struct is the datagram.
struct __attribute__((aligned(4))) slp_line_cis
{
	struct slp_line_hdr h;
	uint8_t r[UDP_LINE_FRAGMENT_SIZE];
	uint8_t g[UDP_LINE_FRAGMENT_SIZE];
	uint8_t b[UDP_LINE_FRAGMENT_SIZE];
};

struct __attribute__((aligned(4))) buffers_Scanline
{
	struct slp_line_cis scanline_buff1[UDP_MAX_NB_PACKET_PER_LINE];
	struct slp_line_cis scanline_buff2[UDP_MAX_NB_PACKET_PER_LINE];
};

struct __attribute__((aligned(4))) button_Event
{
	buttonStateTypeDef state;
	uint32_t pressed_time;
	uint32_t sequence_number;
};

// Structure for packets containing button state information

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

// Structure for packets containing sensor data (accelerometer and gyroscope)
// Latest IMU sample published by the CM7 HID task for the CM4 (IMU strip, screensaver).
// Lives in the CACHED shared region: the CM7 cleans the D-cache after each write.
struct __attribute__((aligned(4))) shared_imu
{
	float_t acc[3];           						// g
	float_t gyro[3];          						// dps
	float_t temp_c;
	uint32_t seq;                                   // +1 per sample
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
	// Offset unique (valeur noire) - OPTIMISÉ 16-bit
	int16_t offsetData[CIS_MAX_USEFUL_DATA_SIZE * CIS_ADC_OUT_LANES];

	// Gains pour les deux segments - OPTIMISÉ Q8.8 format
	int16_t gainsData_seg1[CIS_MAX_USEFUL_DATA_SIZE * CIS_ADC_OUT_LANES];   // 0% → 50%
	int16_t gainsData_seg2[CIS_MAX_USEFUL_DATA_SIZE * CIS_ADC_OUT_LANES];   // 50% → 100%

	// Points de transition par pixel (valeur ADC à 50%) - OPTIMISÉ 16-bit
	int16_t transitionPoint[CIS_MAX_USEFUL_DATA_SIZE * CIS_ADC_OUT_LANES];

	// Références pour correction de dérive (INCHANGÉ)
	int32_t blackRefInactiveAvg[COLOR_CHANNELS][CIS_ADC_OUT_LANES];  // Red, Green, Blue for each lane
};

// IMU calibration data structure
struct __attribute__((aligned(4))) imuCals
{
	// Gyroscope biases (dps)
	float gyroBiasX;
	float gyroBiasY;
	float gyroBiasZ;

	// Accelerometer biases (g)
	float accelBiasX;
	float accelBiasY;
	float accelBiasZ;

	// Accelerometer scale factors
	float accelScaleX;
	float accelScaleY;
	float accelScaleZ;
};

struct __attribute__((aligned(4))) shared_var
{
	int32_t cis_process_rdy;
	int32_t cis_process_cnt;
	int32_t cis_freq;
	int32_t cis_cal_request;
	uint32_t cis_cal_progressbar;
	CIS_Calibration_StateTypeDef cis_cal_state;
	struct button_Event button_events[3];
	struct led_State ledState[3];
    uint32_t led_update_requested[3];
};

// Longueur du mot de passe d'administration. Il est tire au sort au premier
// demarrage plutot que fixe par defaut : un mot de passe d'usine identique sur
// toutes les machines ne protege de rien des que la documentation circule.
// L'alphabet ecarte les caracteres ambigus a lire sur une petite dalle OLED.
#define ADMIN_PASSWORD_LEN 12
#define ADMIN_PASSWORD_ALPHABET "ABCDEFGHJKLMNPQRSTUVWXYZ23456789"

struct __attribute__((aligned(4))) shared_config
{
	uint8_t network_ip[4];
	uint8_t network_netmask[4];
	uint8_t network_gw[4];
	uint8_t network_dest_ip[4];     // STREAM target while no host session is bound
	uint16_t network_udp_port;      // STREAM port while no host session is bound
	uint16_t network_link_port;     // SLP CONTROL port (device listens)
	uint8_t stream_when_unbound;    // 1 = keep streaming to network_dest_ip without a session
	uint8_t cis_print_calibration;
	uint16_t cis_dpi;
	uint8_t cis_oversampling;
	uint8_t cis_handedness;
	uint8_t imu_gyro_sensitivity;   // GyroFS enum value (0x00-0x07)
	uint8_t imu_accel_sensitivity;  // AccelFS enum value (0x00-0x03)
	// GUI and screensaver configuration
	uint8_t gui_show_imu;           // 0=hide IMU panel, 1=show IMU panel
	uint8_t gui_invert_cis_image;   // 0=normal, 1=inverted CIS image colors
	uint16_t screensaver_timeout_sec; // Screensaver timeout in seconds (1-1000)
	float motion_threshold_acc;     // Accelerometer motion threshold in g (0.01-1.0)
	float motion_threshold_gyro;    // Gyroscope motion threshold in dps (0.5-10.0)
	// Administration credentials, guarding every request that CHANGES the device
	char admin_password[ADMIN_PASSWORD_LEN + 1]; // generated on first boot, never a fixed default
	uint8_t admin_password_ack;     // 1 once it has been used: the boot screen stops showing it
};

// CM7 boot progress, shown on the CM4 boot screen (shared_feedback.boot_stage).
typedef enum
{
	BOOT_STAGE_UNKNOWN = 0,
	BOOT_STAGE_STARTING,
	BOOT_STAGE_CONFIG,
	BOOT_STAGE_NETWORK,
	BOOT_STAGE_LINK,
	BOOT_STAGE_IMU,
	BOOT_STAGE_CIS,
	BOOT_STAGE_READY
} BootStageTypeDef;

// Host -> device feedback published by the CM7 link server for the CM4.
// NOLOAD shared region: consumers must react to *_seq CHANGES only (boot-time garbage).
struct __attribute__((aligned(4))) shared_feedback
{
	uint32_t overlay_seq;                 // +1 after overlay is written
	struct slp_oled_overlay overlay;      // last OLED_OVERLAY datagram (count == 0 -> clear)
	uint32_t link_seq;                    // +1 after link_state / peer_ip are written
	uint32_t link_state;                  // 0 = no host session, 1 = bound
	uint8_t peer_ip[4];
	uint8_t led_no_local_press;           // bit i = LED i must NOT light while its button is pressed
	uint8_t boot_stage;                   // BootStageTypeDef, written by the CM7 during StartDefaultTask
	uint8_t reserved[2];
	char device_name[16];                 // "Sp3ctra-XXXX", written by the CM7 BEFORE it releases the CM4
	                                      // (the MCU unique-id region 0x1FF1E800 bus-faults when read from the CM4)
	char admin_password[ADMIN_PASSWORD_LEN + 1]; // published for the boot screen ONLY
	uint8_t admin_show_password;          // 1 while the password has never been used: the screen is
	                                      // the only way to learn it, and it must never leave by the network
};

/**************************************************************************************/
/******************                  CM4 and CM7                    *******************/
/**************************************************************************************/

extern volatile struct shared_var shared_var;
extern volatile struct shared_config shared_config;
extern volatile struct slp_line_cis scanline_CM4[UDP_MAX_NB_PACKET_PER_LINE];
extern volatile struct shared_imu shared_imu;
extern volatile struct shared_feedback shared_feedback;
extern int params_size;


/**************************************************************************************/
/******************                      CM7                        *******************/
/**************************************************************************************/

__attribute__((aligned(4)))
typedef struct
{
    int32_t pixels_per_color_per_lane;
    int32_t pixels_nb;
    int32_t pixel_area_stop;
    int32_t start_offset;
    int32_t lane_size;
    int32_t adc_buff_size;

    int32_t red_lane_offset;
    int32_t green_lane_offset;
    int32_t blue_lane_offset;

    int32_t useful_data_size_per_color_per_lane;
    int32_t useful_data_size_per_lane;

    int32_t red_offset;
    int32_t green_offset;
    int32_t blue_offset;

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
extern uint16_t cisData_ADC1[CIS_MAX_ADC_BUFF_SIZE];
extern uint16_t cisData_ADC2[CIS_MAX_ADC_BUFF_SIZE];
extern uint16_t cisData_ADC3[CIS_MAX_ADC_BUFF_SIZE];
extern int32_t cisDataCpy[CIS_MAX_USEFUL_DATA_SIZE * CIS_ADC_OUT_LANES];
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
