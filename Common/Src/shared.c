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

__attribute__((aligned(4)))
__attribute__ ((section(".shared_var")))
struct shared_var shared_var = {0};

__attribute__((aligned(4)))
__attribute__ ((section(".cisCals")))
struct cisCals cisCals  = {0};

__attribute__((aligned(4)))
__attribute__ ((section(".cisCals")))
struct RAWImage RAWImageCalibration[11] = {0};

__attribute__ ((section(".rgbCals")))
__attribute__((aligned(4)))
struct RGB_Calibration rgbCalibration[CIS_PIXELS_NB] = {0};

__attribute__((aligned(4)))
__attribute__ ((section(".imageData")))
int32_t imageData[CIS_PIXELS_NB] = {0};

__attribute__((aligned(4)))
__attribute__ ((section(".cisData")))
int16_t cisData[CIS_ADC_BUFF_SIZE * 3] = {0};

//__attribute__((aligned(4)))
//__attribute__ ((section(".cisDataCpy")))
//q31_t cisDataCpy_q31[CIS_ADC_BUFF_SIZE * 3] = {0};
//
//__attribute__((aligned(4)))
//__attribute__ ((section(".cisDataCpy")))
//float32_t cisDataCpy_f32[CIS_ADC_BUFF_SIZE * 3] = {0};

__attribute__((aligned(4)))
__attribute__ ((section(".cisDataCpy")))
struct cisLeds_Calibration cisLeds_Calibration = {0};


/* Private function prototypes -----------------------------------------------*/

/* Private user code ---------------------------------------------------------*/
