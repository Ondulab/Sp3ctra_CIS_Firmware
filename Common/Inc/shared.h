/**
 ******************************************************************************
 * @file           : shared.h
 * @brief          : shared data structure for both cpu
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
	IMAGE_DATA_HEADER = 0,
	SENSORS_DATA_HEADER,
	STARTUP_INFO_HEADER,
}CIS_Packet_HeaderTypeDef;

// Packet header structure defining the common header for all packet types
struct packet_Header{
    CIS_Packet_HeaderTypeDef header; // Identifies the data type
    uint32_t sequence;               // Sequence number, useful for ordering packets
};

// Structure for image data packets, including metadata for image fragmentation
struct packet_Image{
    uint8_t fragment_id;      // Identifier for the fragment of the image
    uint8_t total_fragments;  // Total number of fragments for the complete image
    uint16_t fragment_size;   // Size of this particular fragment
    uint8_t *imageData_ptr;   // Pointer to the fragmented image data
};

// Structure for packets containing button state information
struct packet_Buttons {
    uint8_t buttonA_state;     // State of the buttons (pressed/released, etc.)
    uint8_t buttonB_state;     // State of the buttons (pressed/released, etc.)
    uint8_t buttonC_state;     // State of the buttons (pressed/released, etc.)
};

// Structure for packets containing sensor data (accelerometer and gyroscope)
struct packet_Sensors {
    int16_t acc[3];           // Accelerometer data: x, y, and z axis
    int16_t gyro[3];          // Gyroscope data: x, y, and z axis
    int16_t integrated_acc[3];           // Accelerometer data: x, y, and z axis
    int16_t integrated_gyro[3];          // Gyroscope data: x, y, and z axis
};

// Structure for packets containing startup information like version info
struct packet_StartupInfo {
    uint8_t version_info[100]; // Information about the version, and other startup details
};

struct cisRgbBuffers {
	uint8_t R[CIS_PIXELS_NB];
	uint8_t G[CIS_PIXELS_NB];
	uint8_t B[CIS_PIXELS_NB];
};

typedef enum
{
	CIS_CAL_START = 0,
	CIS_CAL_PLACE_ON_WHITE,
	CIS_CAL_PLACE_ON_BLACK,
	CIS_CAL_EXTRACT_INNACTIVE_REF,
	CIS_CAL_EXTRACT_EXTREMUMS,
	CIS_CAL_EXTRACT_OFFSETS,
	CIS_CAL_COMPUTE_GAINS,
	CIS_CAL_END,
}CIS_Calibration_StateTypeDef;

__attribute__ ((packed))
struct cisColorsParams {
	float32_t maxPix;
	float32_t minPix;
	float32_t deltaPix;
	float32_t inactiveAvrgPix[3];
};

__attribute__ ((packed))
struct cisCalsTypes {
	float32_t data[CIS_ADC_BUFF_SIZE * 3];
	struct cisColorsParams red;
	struct cisColorsParams green;
	struct cisColorsParams blue;
};

__attribute__ ((packed))
struct cisCals {
	struct cisCalsTypes blackCal;
	struct cisCalsTypes whiteCal;
	float32_t offsetData[CIS_ADC_BUFF_SIZE * 3];
	float32_t gainsData[CIS_ADC_BUFF_SIZE * 3];
};

struct shared_var {
	int32_t cis_process_cnt;
	int32_t cis_cal_request;
	uint32_t cis_cal_progressbar;
	uint32_t cis_oversampling;
	uint32_t cis_scanDir;
	CIS_Calibration_StateTypeDef cis_cal_state;
};

/**************************************************************************************/
/******************                  CM4 and CM7                    *******************/
/**************************************************************************************/

extern struct shared_var shared_var;
extern struct params params;
extern struct cisCals cisCals;
extern int32_t imageData[CIS_PIXELS_NB];
extern struct cisRgbBuffers rgbBuffers;
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
extern struct RAWImage RAWImageCalibration[11];
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
