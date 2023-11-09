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


__attribute__((aligned(4)))
struct cisLeds_Calibration {
	int32_t redLed_power2PWM[CIS_LEDS_MAX_PWM];
	int32_t greenLed_power2PWM[CIS_LEDS_MAX_PWM];
	int32_t blueLed_power2PWM[CIS_LEDS_MAX_PWM];
	int32_t redLed_maxPulse;
	int32_t greenLed_maxPulse;
	int32_t blueLed_maxPulse;
	float32_t redMeanAtLedPower;
	float32_t greenMeanAtLedPower;
	float32_t blueMeanAtLedPower;
};

__attribute__((aligned(4)))
struct shared_var {
	int32_t cis_process_cnt;
	int32_t cis_cal_request;
	uint32_t cis_cal_progressbar;
	uint32_t cis_oversampling;
	uint32_t cis_scanDir;
	CIS_Calibration_StateTypeDef cis_cal_state;
};

__attribute__((aligned(4)))
struct RAWImage{
     float32_t redLine[CIS_PIXELS_NB];
     float32_t greenLine[CIS_PIXELS_NB];
     float32_t blueLine[CIS_PIXELS_NB];
};

extern struct shared_var shared_var;
extern struct params params;
extern struct cisCals cisCals;
extern struct RGB_Calibration rgbCalibration[CIS_PIXELS_NB];
extern struct RAWImage RAWImageCalibration[11];
extern struct cisLeds_Calibration cisLeds_Calibration;
extern int32_t imageData[CIS_PIXELS_NB];

__attribute__((aligned(4)))
extern int16_t cisData[CIS_ADC_BUFF_SIZE * 3];

extern float32_t cisDataCpy_f32[CIS_ADC_BUFF_SIZE * 3];

__attribute__((aligned(4)))
extern q31_t cisDataCpy_q31[CIS_ADC_BUFF_SIZE * 3];

extern int params_size;

/* Exported constants --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/

/* Exported functions prototypes ---------------------------------------------*/

/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /*__SHARED_H__*/
