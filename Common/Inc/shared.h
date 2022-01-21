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
__attribute__ ((packed))
struct cisColorsParams {
	int32_t maxPix;
	int32_t minPix;
	int32_t deltaPix;
};

__attribute__ ((packed))
struct cisCalsTypes {
	int32_t data[CIS_ADC_BUFF_SIZE * 3];
	struct cisColorsParams red;
	struct cisColorsParams green;
	struct cisColorsParams blue;
};

__attribute__ ((packed))
struct cisCals {
	struct cisCalsTypes blackCal;
	struct cisCalsTypes whiteCal;
	int32_t offsetData[CIS_ADC_BUFF_SIZE * 3];
	float32_t gainsData[CIS_ADC_BUFF_SIZE * 3];
};

__attribute__ ((packed))
struct params {
	int32_t cis_dpi; //not use
};

__attribute__ ((packed))
struct shared_var {
	int32_t cis_process_cnt;
	int32_t calibrationRequest;
};

extern struct shared_var shared_var;
extern struct params params;
extern struct cisCals cisCals;
extern int32_t imageData[CIS_PIXELS_NB + UDP_HEADER_SIZE];
extern int32_t cisData[CIS_ADC_BUFF_SIZE * 3];
extern int32_t cisDataCpy[CIS_ADC_BUFF_SIZE * 3];

extern int params_size;

/* Exported constants --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/

/* Exported functions prototypes ---------------------------------------------*/

/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /*__SHARED_H__*/
