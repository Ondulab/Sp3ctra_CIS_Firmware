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

__attribute__ ((section(".image")))
volatile int32_t imageData[CIS_PIXELS_NB];

/* Private function prototypes -----------------------------------------------*/

/* Private user code ---------------------------------------------------------*/
