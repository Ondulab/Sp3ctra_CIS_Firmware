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

__attribute__ ((section(".shared_var")))
struct shared_var shared_var;

__attribute__ ((section(".params")))
struct params params;

__attribute__ ((section(".cisCals")))
struct cisCals cisCals;

__attribute__ ((section(".imageData")))
int32_t imageData[CIS_PIXELS_NB + UDP_HEADER_SIZE];

__attribute__ ((section(".cisData")))
int32_t cisData[CIS_ADC_BUFF_SIZE * 3];

__attribute__ ((section(".cisDataCpy")))
int32_t cisDataCpy[CIS_ADC_BUFF_SIZE * 3];

/* Private function prototypes -----------------------------------------------*/

/* Private user code ---------------------------------------------------------*/
