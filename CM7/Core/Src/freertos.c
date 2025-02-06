/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * File Name          : freertos.c
 * Description        : Code for freertos applications
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2024 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "lwip.h"
#include "cis_scan.h"
#include "cis_linearCal.h"
#include "icm42688.h"
#include "file_manager.h"
#include "ftpd.h"
#include "http_server.h"
#include "tcp_client.h"
#include "udp_client.h"
#include "tim.h"
#include "stm32_flash.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */
osThreadId defaultTaskHandle;

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void const * argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* GetIdleTaskMemory prototype (linked to static allocation support) */
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize );

/* Hook prototypes */
void configureTimerForRunTimeStats(void);
unsigned long getRunTimeCounterValue(void);
void vApplicationStackOverflowHook(xTaskHandle xTask, signed char *pcTaskName);

/* USER CODE BEGIN 1 */
/* Functions needed when configGENERATE_RUN_TIME_STATS is on */
/* Functions needed when configGENERATE_RUN_TIME_STATS is on */
__weak void configureTimerForRunTimeStats(void)
{
	HAL_TIM_Base_Start_IT(&htim6);
}

__weak unsigned long getRunTimeCounterValue(void)
{
	return ulHighFrequencyTimerTicks;
}
/* USER CODE END 1 */

/* USER CODE BEGIN 4 */
__weak void vApplicationStackOverflowHook(xTaskHandle xTask, signed char *pcTaskName)
{
   /* Run time stack overflow checking is performed if
   configCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2. This hook function is
   called if a stack overflow is detected. */

    taskDISABLE_INTERRUPTS();

    printf("Stack overflow for task : %s\n", pcTaskName);

    Error_Handler();
}
/* USER CODE END 4 */

/* USER CODE BEGIN GET_IDLE_TASK_MEMORY */
static StaticTask_t xIdleTaskTCBBuffer;
static StackType_t xIdleStack[configMINIMAL_STACK_SIZE];

void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize )
{
  *ppxIdleTaskTCBBuffer = &xIdleTaskTCBBuffer;
  *ppxIdleTaskStackBuffer = &xIdleStack[0];
  *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
  /* place for user code */
}
/* USER CODE END GET_IDLE_TASK_MEMORY */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
	/* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
	/* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
	/* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
	/* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* definition and creation of defaultTask */
  osThreadDef(defaultTask, StartDefaultTask, osPriorityHigh, 0, 16384);
  defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
	/* add threads, ... */

  /* USER CODE END RTOS_THREADS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
 * @brief  Function implementing the defaultTask thread.
 * @param  argument: Not used
 * @retval None
 */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void const * argument)
{
  /* USER CODE BEGIN StartDefaultTask */

	if (!HAL_GPIO_ReadPin(SW2_GPIO_Port, SW2_Pin) && !HAL_GPIO_ReadPin(SW3_GPIO_Port, SW3_Pin))
	{
		file_factoryReset();
	}

    printf("- CONFIG FILE INITIALIZATIONS -\n");
	if (file_initConfig(&shared_config) != FILEMANAGER_OK)
	{
		printf("File initialization ERROR\n");
	}

	printf("----- IMU INITIALIZATIONS -----\n");
	if (icm42688_init() != ICM42688_OK)
	{
		printf("IMU initialization ERROR\n");
	}

    printf("---- LWIP INITIALIZATIONS -----\n");
	MX_LWIP_Init();

    printf("----- FTP INITIALIZATIONS -----\n");
	if (ftpd_init() != ERR_OK)
    {
		printf("FTP initialization ERROR\n");
    }

	printf("----- HTTP INITIALIZATIONS ----\n");
	if (http_serverInit() != HTTPSERVER_OK)
	{
		printf("HTTP initialization ERROR\n");
	}

	printf("- READ FIRMWARE UPDATE STATUS -\n");
	FW_UpdateState dataRead;
	if (STM32Flash_readPersistentData(&dataRead) != STM32FLASH_OK)
	{
		printf("Read update status ERROR\n");
	}

	if (dataRead != FW_UPDATE_NONE)
	{
	    STM32Flash_StatusTypeDef status = STM32Flash_writePersistentData(FW_UPDATE_DONE);
	    if (status == STM32FLASH_OK)
	    {
	        printf("Firmware update must be tested now.\n");
	    }
	    else
	    {
	        printf("Failed to write firmware update status in STM32 flash\n");
	    }

		printf("Rebooting in 3\n");
		/* Wait 3 seconds. */
		osDelay(3000);
		NVIC_SystemReset();
	}

	printf("--- TCP LED INITIALIZATIONS ---\n");
	if (tcp_clientInit() != TCPCLIENT_OK)
	{
		printf("TCP initialization ERROR\n");
	}

	printf("---------- UDP INIT -----------\n");
    if (udp_clientInit() != UDPCLIENT_OK)
    {
    	printf("UDP initialization ERROR\n");
    }

	printf("---- CIS LOAD CALIBRATION -----\n");
    if (cis_linearCalibrationInit() != CISCALIBRATION_OK)
    {
    	printf("CIS load calibration ERROR\n");
    }

    printf("----- CIS INITIALIZATIONS -----\n");
	if (cis_scanInit() != CISSCAN_OK)
	{
		printf("CIS initialization ERROR\n");
	}

#ifndef DEBUG_LWIP_STATS
	osDelay(200);
    printf("------ INIT TASK COMPLETE -----\n");
    vTaskDelete(NULL); //delete task
#endif

	/* Infinite loop */
	for(;;)
	{
		stats_display(); //must comment vTaskDelete to use it
		osDelay(1000);
	}
  /* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */
