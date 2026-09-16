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
	// Marqueur de format : la lecture ne validait que la taille, et un fichier de
	// l'ancienne disposition est plus GRAND que celle-ci — il aurait donc été relu
	// sans erreur, comme des données valides.
	uint32_t magic;
	uint32_t version;

	// --- Ce qui varie d'un pixel à l'autre : dispersion de fabrication -------------
	int16_t offsetData[CIS_MAX_USEFUL_DATA_SIZE * CIS_ADC_OUT_LANES];  // niveau noir, comptages ADC
	int16_t gainData[CIS_MAX_USEFUL_DATA_SIZE * CIS_ADC_OUT_LANES];    // sensibilité, Q4.12
	// Écarts par pixel aux DEUX ancres basses (niveaux CIS_CAL_LOW_ANCHOR_IDX_A/_B,
	// 4 % et 8 %), en unités d'index de courbe, saturés int8. Retranchés de n au
	// runtime, pondérés par les bases triangulaires confinées sous 15 %
	// (voir config.h, "ancres basses PAR PIXEL").
	int8_t  lowDeltaA[CIS_MAX_USEFUL_DATA_SIZE * CIS_ADC_OUT_LANES];
	int8_t  lowDeltaB[CIS_MAX_USEFUL_DATA_SIZE * CIS_ADC_OUT_LANES];

	// Carte de VOILE par pixel : excès du plancher d'ombre au-dessus du plancher de
	// la voie, en unités d'index de courbe (saturé int8). Le voile est de la lumière
	// LED parasite additive, de géométrie fixe COMMUNE aux 3 couleurs (mesuré : stries
	// verticales corrélées 0,85-0,93, ~0,7-1,2 % linéaire RMS, spread 15-25 unités
	// d'index). Un point noir SCALAIRE ne peut pas l'aplatir → banding vertical dans
	// les ombres. Retranché de n au runtime (uniforme : le voile est constant en index,
	// indépendant de la scène). Rempli par cis_calibrateBlackPoint ; zéro = pas de
	// correction (défaut tant que la calibration du point noir n'a pas été faite).
	int8_t  veilDelta[CIS_MAX_USEFUL_DATA_SIZE * CIS_ADC_OUT_LANES];

	// --- Ce qui est commun : la forme de la réponse de la chaîne -------------------
	// Points de cassure de la linéarisation, PLEINE PRÉCISION : la courbe tabulée
	// uint8 qu'ils remplacent quantifiait le linéaire à 8 bits avant l'étirement
	// point noir + sRGB (banding de quantification près du noir). curveX = positions
	// normalisées mesurées (0..CIS_CAL_CURVE_MAX, monotonie forcée, ancres 0 et max
	// exactes) ; curveY = sorties visées partagées, proportionnelles au rapport
	// cyclique, en Q16 (0..CIS_CAL_CURVE_Y_MAX). La LUT de rendu est interpolée
	// depuis ces points par cis_composeOutputLut().
	int16_t  curveX[COLOR_CHANNELS][CIS_ADC_OUT_LANES][CIS_CAL_LEVEL_COUNT];
	uint16_t curveY[CIS_CAL_LEVEL_COUNT];

	// Références pour correction de dérive (INCHANGÉ)
	int32_t blackRefInactiveAvg[COLOR_CHANNELS][CIS_ADC_OUT_LANES];  // Red, Green, Blue for each lane

	// Température (IMU, °C) au moment de la calibration : référence de
	// l'asservissement thermique des LED.
	float cal_temp_c;
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

// Requests the CM4 device menu can post to the CM7 (see shared_var menu_req_*).
typedef enum
{
	MENU_REQ_NONE = 0,
	MENU_REQ_CFG_SET,        // menu_req_id/_value -> cfg apply + persist (link_server cfg_write path)
	MENU_REQ_IMU_CAL,        // icm42688_performCalibration (blocking ~1.2 s on the CM7 link task)
	MENU_REQ_BLACKPOINT_CAL, // async: progress mirrored in menu_bp_cal_state
	MENU_REQ_FACTORY_RESET,  // file_factoryReset (QSPI format, blocking several s) + reboot
} menuReqKindTypeDef;

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
	/* CM4 device menu -> CM7 request mailbox. NOLOAD region: the CM7 zeroes
	   these words BEFORE it releases the CM4 (main.c), so both sides start
	   from seq == done_seq == 0. The CM4 is the only writer of the request
	   words and posts seq LAST; the CM7 link task executes, writes result,
	   then echoes done_seq. One request in flight at a time. */
	uint32_t menu_req_seq;       // CM4: +1 per request (posted last)
	uint32_t menu_req_kind;      // menuReqKindTypeDef
	uint32_t menu_req_id;        // slp_cfg_id for MENU_REQ_CFG_SET
	uint32_t menu_req_value;     // raw value (IEEE-754 bits for f32 ids, packed a|b<<8|c<<16|d<<24 for ip4)
	uint32_t menu_req_done_seq;  // CM7: last executed request
	uint32_t menu_req_result;    // SLP_CFG_F_* flags of the last executed request
	uint32_t menu_bp_cal_state;  // CM7 mirror of cisBlackPointCalState: 0 idle, 1 running, 2 done, 3 failed
};

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
	uint8_t cis_black_point;        // Point noir de sortie, x1000 lineaire (0..200) : 0=physique, 55=dessin, ~25=photo
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
	uint8_t net_link_up;                  // Ethernet carrier (CM7 isConnected mirror) for the CM4 menu
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

    /* Domaine HORLOGES (periodes de CP), distinct du domaine ECHANTILLONS : en 400 dpi
       le capteur sort 2 px/horloge, donc lane_clocks = lane_size/2. Les timers comptent
       des horloges, les tampons des echantillons -- les confondre etait le bug de la
       duplication d'image. */
    int32_t cis_clk_freq;
    int32_t lane_clocks;
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
