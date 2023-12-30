/**
 ******************************************************************************
 * @file           : shared.c
 * @brief          : shared data structure for both cpu
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "shared.h"

/* Private includes ----------------------------------------------------------*/

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/**************************************************************************************/
/******************                  CM4 and CM7                    *******************/
/**************************************************************************************/

#pragma GCC push_options
#pragma GCC optimize ("O0")

__attribute__ ((section(".shared_var")))
struct shared_var shared_var = {0};

__attribute__ ((section(".cisCals")))
struct cisCals cisCals = {0};

__attribute__ ((section(".imageData")))
int32_t imageData[CIS_PIXELS_NB] = {0};

__attribute__ ((section(".rgbBuffers")))
struct cisRgbBuffers rgbBuffers = {0};

#pragma GCC pop_options

/**************************************************************************************/
/******************                      CM7                        *******************/
/**************************************************************************************/

#pragma GCC push_options
#pragma GCC optimize ("O0")

__attribute__ ((section(".cisDataCpy")))
float32_t cisDataCpy_f32[CIS_ADC_BUFF_SIZE * 3] = {0};

__attribute__ ((section(".cisData")))
int16_t cisData[CIS_ADC_BUFF_SIZE * 3] __attribute__ ((aligned (32))) = {0};

__attribute__ ((section(".cisRAWImage")))
struct RAWImage RAWImageCalibration[11] = {0};

__attribute__ ((section(".rgbCalibration")))
struct RGB_Calibration rgbCalibration = {0};

__attribute__ ((section(".cisLeds_Calibration")))
struct cisLeds_Calibration cisLeds_Calibration = {0};

__attribute__ ((section(".cisDataCpy_q31")))
q31_t cisDataCpy_q31[CIS_ADC_BUFF_SIZE * 3] = {0};

#pragma GCC pop_options

/* Private function prototypes -----------------------------------------------*/

/* Private user code ---------------------------------------------------------*/
