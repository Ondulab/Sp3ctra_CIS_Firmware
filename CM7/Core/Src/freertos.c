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
#include "FreeRTOS.h"
#include "cmsis_os2.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "lwip.h"
#include "cis_scan.h"
#include "cis.h"
#include "icm42688.h"
#include "file_manager.h"
#include "ftpd.h"
#include "http_server.h"
#include "udp_client.h"
#include "tim.h"
#include "stm32_flash.h"
#include "lwip.h"
#include "link_server.h"
#include "hid_task.h"
#include "sys_identity.h"

/* Enable to log button->MIDI events (verbose, for debugging only). */
/* #define DEBUG_MIDI_BUTTONS */

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
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 4096 * 4,
  .priority = (osPriority_t) osPriorityHigh,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* Hook prototypes */
void configureTimerForRunTimeStats(void);
unsigned long getRunTimeCounterValue(void);
void vApplicationStackOverflowHook(xTaskHandle xTask, char *pcTaskName);

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
__weak void vApplicationStackOverflowHook(xTaskHandle xTask, char *pcTaskName)
{
   /* Run time stack overflow checking is performed if
   configCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2. This hook function is
   called if a stack overflow is detected. */

    taskDISABLE_INTERRUPTS();

    printf("Stack overflow for task : %s\n", pcTaskName);

    Error_Handler();
}
/* USER CODE END 4 */

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
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
	/* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
 * @brief  Function implementing the defaultTask thread.
 * @param  argument: Not used
 * @retval None
 */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN StartDefaultTask */

	if (!HAL_GPIO_ReadPin(SW2_GPIO_Port, SW2_Pin) && !HAL_GPIO_ReadPin(SW3_GPIO_Port, SW3_Pin))
	{
		file_factoryReset();
	}

    shared_feedback.boot_stage = BOOT_STAGE_CONFIG;
    printf("- CONFIG FILE INITIALIZATIONS -\n");
	if (file_initConfig(&shared_config) != FILEMANAGER_OK)
	{
		printf("File initialization ERROR\n");
	}

	printf("-------- POWER ON CIS ---------\n");
	cis_Power(ON);

    shared_feedback.boot_stage = BOOT_STAGE_NETWORK;
    printf("---- LWIP INITIALIZATIONS -----\n");
	MX_LWIP_Init();

    printf("----- FTP INITIALIZATIONS -----\n");
	if (ftpd_init() != ERR_OK)
    {
		printf("FTP initialization ERROR\n");
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

		printf("Rebooting\n");
		System_SafeReset();
	}

	printf("----- HTTP INITIALIZATIONS ----\n");
	if (http_serverInit() != HTTPSERVER_OK)
	{
		printf("HTTP initialization ERROR\n");
	}

	printf("---------- UDP INIT -----------\n");
    if (udpClient_init() != UDPCLIENT_OK)
    {
    	printf("UDP initialization ERROR\n");
    }

    printf("--- WAITING FOR NETWORK CONNECTION ---\n");
    uint32_t network_wait_count = 0;
    while(isConnected == 0)
    {
        osDelay(500);
        network_wait_count++;
        if (network_wait_count % 10 == 0) // Print every 5 seconds
        {
            printf("Still waiting for network connection... (%lu seconds)\n", network_wait_count / 2);
        }

        // Safety timeout after 60 seconds
        if (network_wait_count > 120)
        {
            printf("Network connection timeout - proceeding anyway\n");
            break;
        }
    }

    if (isConnected == 1)
    {
        printf("Network connection established - proceeding with initialization\n");
    }

    // Wait a bit more for LwIP to be fully ready
    printf("Waiting for network interface to be fully ready...\n");
    osDelay(200);  // 200ms delay to ensure LwIP is stable

    {
        char name[SYS_IDENTITY_NAME_LEN], serial[SYS_IDENTITY_SERIAL_LEN];
        uint8_t mac[6];
        sys_identity_name(name);
        sys_identity_serial(serial);
        sys_identity_mac(mac);
        printf("Device %s, serial %s, MAC %02X:%02X:%02X:%02X:%02X:%02X\n",
               name, serial, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    }

    shared_feedback.boot_stage = BOOT_STAGE_LINK;
    printf("---- LINK INITIALIZATIONS -----\n");
    if (link_serverInit() != LINKSERVER_OK)
    {
        printf("Link server initialization ERROR\n");
    }

    shared_feedback.boot_stage = BOOT_STAGE_IMU;
    printf("----- IMU INITIALIZATIONS -----\n");
	if (icm42688_init() != ICM42688_OK)
	{
		printf("IMU initialization ERROR\n");
	}

    printf("----- HID INITIALIZATIONS -----\n");
    if (hid_taskInit() != HIDTASK_OK)
    {
        printf("HID task initialization ERROR\n");
    }

    shared_feedback.boot_stage = BOOT_STAGE_CIS;
    printf("----- CIS INITIALIZATIONS -----\n");
	if (cis_scanInit() != CISSCAN_OK)
	{
		printf("CIS initialization ERROR\n");
	}

    /* Mark system as fully initialized - enables automatic reset on network disconnection */
    systemFullyInitialized = 1;
    shared_feedback.boot_stage = BOOT_STAGE_READY;
    printf("System marked as fully initialized - automatic reset enabled\n");

#if !defined(DEBUG_LWIP_STATS) && !defined(DEBUG_ICM42688)
	osDelay(200);
    printf("------ INIT TASK COMPLETE -----\n");
    vTaskDelete(NULL); //delete task
#endif

	/* Infinite loop */
	for(;;)
	{
#ifdef DEBUG_ICM42688
    icm42688_handle_debug_print();
#else
		stats_display(); //must comment vTaskDelete to use it
#endif
		osDelay(1);
	}
  /* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */


/* USER CODE END Application */

