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
#include "httpserver.h"
#include "lwip.h"
#include "cis_scan.h"

#include "ff.h" // FATFS include
#include "diskio.h" // DiskIO include

#include "ftpd.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define WORKING_BUFFER_SIZE (2 * _MAX_SS)  // Dépend de votre configuration et besoins

#define DTCM_RAM __attribute__((section(".tcmram")))
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
FATFS FatFs; // Variable pour le volume de travail FATFS

/* USER CODE END Variables */
osThreadId defaultTaskHandle;

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void const * argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* GetIdleTaskMemory prototype (linked to static allocation support) */
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize );

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
	osThreadDef(defaultTask, StartDefaultTask, osPriorityNormal, 0, 8 * configMINIMAL_STACK_SIZE);
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
	FRESULT fres; // Variable pour stocker le résultat des opérations FATFS

	// Essayer de monter le système de fichiers
	fres = f_mount(&FatFs, "0:", 1); // 1 pour monter immédiatement
	if (fres != FR_OK) {
		printf("FS mount ERROR\n");

		// Si le montage échoue, essayer de formater la flash
		printf("Attempting to format the QSPI flash...\n");

		BYTE work[WORKING_BUFFER_SIZE];   // Allocation statique pour simplifier

		fres = f_mkfs("0:", FM_ANY, 0, work, WORKING_BUFFER_SIZE);
		if (fres != FR_OK) {
			printf("Failed to format the QSPI flash.\n");
		}

		// Essayer de monter à nouveau le système de fichiers après le formatage
		fres = f_mount(&FatFs, "0:", 1);
		if (fres != FR_OK) {
			printf("Failed to mount the filesystem even after formatting.\n");
		} else {
			printf("FS mount SUCCESS after formatting.\n");
		}
	} else {
		printf("FS mount SUCCESS\n");
	}

	FIL fil; // Variable de fichier
	UINT bw; // Variable pour compter les octets écrits

#if 1
	// Créer un fichier et l'ouvrir
	fres = f_open(&fil, "test.txt", FA_CREATE_ALWAYS | FA_WRITE);
	if (fres == FR_OK) {
		// Écrire quelque chose dans le fichier
		fres = f_write(&fil, "Hello, World!\n", 14, &bw);
		if (fres == FR_OK) {
			// Données écrites avec succès
		} else {
			// Échec de l'écriture
		}
		// Fermer le fichier
		f_close(&fil);
	} else {
		// Échec de l'ouverture du fichier
	}

	//f_mount(NULL, "0:", 0); // Démonter le volume
#endif

	MX_LWIP_Init();

	ftpd_init();

	http_server_init();
	cis_scan();
	/* Infinite loop */
	for(;;)
	{
		osDelay(1);
	}
	/* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */
