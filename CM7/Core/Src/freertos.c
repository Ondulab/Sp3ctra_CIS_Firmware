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
#include "cis.h"
#include "icm42688.h"
#include "file_manager.h"
#include "ftpd.h"
#include "http_server.h"
#include "tcp_client.h"
#include "udp_client.h"
#include "tim.h"
#include "stm32_flash.h"
#include "lwip.h"
#include "rtpmidi.h"
#include "midi_led_mapper.h"
#include "midi_button_mapper.h"

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
osThreadId defaultTaskHandle;

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
void StartMidiTask(void const * argument);
/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void const * argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* GetIdleTaskMemory prototype (linked to static allocation support) */
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize );

/* GetTimerTaskMemory prototype (linked to static allocation support) */
void vApplicationGetTimerTaskMemory( StaticTask_t **ppxTimerTaskTCBBuffer, StackType_t **ppxTimerTaskStackBuffer, uint32_t *pulTimerTaskStackSize );

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

/* USER CODE BEGIN GET_TIMER_TASK_MEMORY */
static StaticTask_t xTimerTaskTCBBuffer;
static StackType_t xTimerStack[configTIMER_TASK_STACK_DEPTH];

void vApplicationGetTimerTaskMemory( StaticTask_t **ppxTimerTaskTCBBuffer, StackType_t **ppxTimerTaskStackBuffer, uint32_t *pulTimerTaskStackSize )
{
  *ppxTimerTaskTCBBuffer = &xTimerTaskTCBBuffer;
  *ppxTimerTaskStackBuffer = &xTimerStack[0];
  *pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
  /* place for user code */
}
/* USER CODE END GET_TIMER_TASK_MEMORY */

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
  osThreadDef(defaultTask, StartDefaultTask, osPriorityHigh, 0, 4096);
  defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
	/* add threads, ... */
	osThreadDef(midiTask, StartMidiTask, osPriorityNormal, 0, 2048);
	osThreadCreate(osThread(midiTask), NULL);
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

	printf("-------- POWER ON CIS ---------\n");
	cis_Power(ON);

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

    // Verify network interface has valid IP
    struct netif *netif = netif_default;
    if (!netif || ip_addr_isany(&netif->ip_addr)) {
        printf("Network interface not ready - RTP-MIDI initialization skipped\n");
    } else {
        printf("--- RTP-MIDI INITIALIZATIONS --\n");

        // Convert config destination IP to LwIP ip_addr_t
        ip_addr_t remote_ip;
        IP4_ADDR(&remote_ip,
                 shared_config.network_dest_ip[0],
                 shared_config.network_dest_ip[1],
                 shared_config.network_dest_ip[2],
                 shared_config.network_dest_ip[3]);

        // Determine RTP-MIDI mode from configuration
        rtpmidi_mode_t mode = (rtpmidi_mode_t)shared_config.rtpmidi_mode;
        const char* mode_str = (mode == RTPMIDI_MODE_SERVER) ? "SERVER" : "CLIENT";

        printf("RTP-MIDI: Mode=%s, Destination IP=%d.%d.%d.%d\n",
               mode_str,
               shared_config.network_dest_ip[0],
               shared_config.network_dest_ip[1],
               shared_config.network_dest_ip[2],
               shared_config.network_dest_ip[3]);

        // Initialize RTP-MIDI with mode from configuration
        // SERVER mode (0): Passive, waits for INVITE from macOS (uses mDNS for discovery)
        // CLIENT mode (1): Active, initiates connection to remote IP from config
        if (rtpmidi_init("Sp3ctra_CIS", &remote_ip, mode) != RTPMIDI_OK)
        {
            printf("RTP-MIDI initialization ERROR\n");
        }
        else
        {
            // Initialize mappers
            midi_button_mapper_init();
            midi_led_mapper_init(LED_MODE_SIMPLE);

            // Register LED callback for incoming MIDI
            rtpmidi_register_rx_callback(midi_led_mapper_handle_cc);

            // In CLIENT mode, initiate connection to remote server
            if (mode == RTPMIDI_MODE_CLIENT) {
                if (rtpmidi_connect() != RTPMIDI_OK)
                {
                    printf("RTP-MIDI: Failed to initiate connection\n");
                }
                else
                {
                    printf("RTP-MIDI: Initialized in CLIENT mode (connecting to remote)\n");
                    printf("RTP-MIDI initialization SUCCESS\n");
                }
            } else {
                printf("RTP-MIDI: Initialized in SERVER mode (waiting for macOS invitation)\n");
                printf("RTP-MIDI initialization SUCCESS\n");
            }
        }
    }

    printf("----- IMU INITIALIZATIONS -----\n");
	if (icm42688_init() != ICM42688_OK)
	{
		printf("IMU initialization ERROR\n");
	}

    printf("----- CIS INITIALIZATIONS -----\n");
	if (cis_scanInit() != CISSCAN_OK)
	{
		printf("CIS initialization ERROR\n");
	}

    /* Mark system as fully initialized - enables automatic reset on network disconnection */
    systemFullyInitialized = 1;
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

/* USER CODE BEGIN Header_StartMidiTask */
/**
 * @brief  Function implementing the MIDI task.
 * @param  argument: Not used
 * @retval None
 */
/* USER CODE END Header_StartMidiTask */
#pragma GCC push_options
#pragma GCC optimize ("O0")
void StartMidiTask(void const * argument)
{
    /* USER CODE BEGIN StartMidiTask */

    // Wait for system to be fully initialized
    printf("--- MIDI TASK: Waiting for system init ---\n");
    while(systemFullyInitialized == 0)
    {
        osDelay(100);
    }

    printf("--- MIDI TASK: Starting RTP-MIDI processing ---\n");

    // mDNS is now initialized in MX_LWIP_Init() in lwip.c
    // RTP-MIDI is initialized in StartDefaultTask where it worked before
    // Track last processed sequence numbers for each button
    static uint32_t last_processed_sequence[NUMBER_OF_BUTTONS] = {0};

    // Main loop
    for(;;)
    {
        // Process RTP-MIDI (handles session, RX, timeouts)
        rtpmidi_process();

        // mDNS is now handled automatically by LwIP

        // Check for button events using sequence numbers (edge-triggered)
        for (uint8_t i = 0; i < NUMBER_OF_BUTTONS; i++)
        {
            // Read current sequence number (atomic operation)
            uint32_t current_sequence = shared_var.button_events[i].sequence_number;

            // Check if sequence has changed (new event available)
            if (current_sequence != last_processed_sequence[i])
            {
                // Get button state from shared memory
                buttonStateTypeDef state = shared_var.button_events[i].state;

                // Send MIDI message
                uint8_t pressed = (state == SWITCH_PRESSED) ? 1 : 0;
                midi_button_mapper_on_change(i, pressed);

#ifdef DEBUG_MIDI_BUTTONS
                /* Keep logs out of critical paths by default.
                 * This task runs at 1ms period, so printing here can spam.
                 */
                uint8_t cc = (i == 0U) ? MIDI_BUTTON1_CC : (i == 1U) ? MIDI_BUTTON2_CC : MIDI_BUTTON3_CC;
                uint8_t value = pressed ? 127U : 0U;
                printf("MIDI: Button %u %s (seq=%lu) -> CC %u = %u\n",
                       (unsigned)i,
                       pressed ? "PRESSED" : "RELEASED",
                       (unsigned long)current_sequence,
                       (unsigned)cc,
                       (unsigned)value);
#endif

                // Update last processed sequence
                last_processed_sequence[i] = current_sequence;
            }
        }

        osDelay(1);  // 1ms task period
    }

    /* USER CODE END StartMidiTask */
}
#pragma GCC pop_options

/* USER CODE END Application */
