/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
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
#include "main.h"
#include "crc.h"
#include "fatfs.h"
#include "quadspi.h"
#include "rng.h"
#include "usart.h"
#include "gpio.h"
#include "fmc.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "boot_config.h"
#include "stm32_flash.h"
#include "file_manager.h"

#include "update.h"
#include "update_gui.h"

#include "basetypes.h"
#include "stdio.h"
#include "stdbool.h"

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

/* USER CODE BEGIN PV */

typedef void (*pFunction)(void);

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MPU_Config(void);
/* USER CODE BEGIN PFP */

/* Function prototypes */
static void configureBootConfiguration(void);
static void reboot(void);
static void gotoFirmware(uint32_t fwFlashStartAdd);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

#include "stm32h7xx_hal.h"

/**
 * @brief  Configure boot settings for CM4.
 *         This function checks and updates the option bytes to ensure the correct
 *         boot address and permissions for the Cortex-M4 core.
 */
void configureBootConfiguration(void)
{
    bool needUpdate = false;
    FLASH_OBProgramInitTypeDef currentOB = {0};
    FLASH_OBProgramInitTypeDef newOB     = {0};

    // 1) Unlock the FLASH and the option bytes
    HAL_FLASH_Unlock();
    HAL_FLASH_OB_Unlock();

    // 2) Read the current option bytes configuration
    HAL_FLASHEx_OBGetConfig(&currentOB);

    // 3) Check if the current boot addresses match the desired ones
    uint32_t currentCm4Boot = currentOB.CM4BootAddr0;

    if (currentCm4Boot != FW_CM4_START_ADDR)
    {
        newOB.OptionType |= OPTIONBYTE_CM4_BOOTADD;
        newOB.CM4BootConfig = OB_BOOT_ADD0;
        newOB.CM4BootAddr0 = FW_CM4_START_ADDR;

        needUpdate = true;
    }

    // 4) Check if the CM4 is currently allowed to boot
    if (READ_BIT(SYSCFG->UR1, SYSCFG_UR1_BCM4) != 0U)
    {
        newOB.OptionType |= OPTIONBYTE_USER;
        newOB.USERType   |= OB_USER_BCM4;
        newOB.USERConfig = (currentOB.USERConfig & ~OB_BCM4_ENABLE) | OB_BCM4_DISABLE;

        needUpdate = true;
    }

    // 5) Apply the new configuration if needed
    if (needUpdate)
    {
        // Program the new option bytes
        if (HAL_FLASHEx_OBProgram(&newOB) != HAL_OK)
        {
            Error_Handler();
        }

        // Trigger the update of the option bytes
        if (HAL_FLASH_OB_Launch() != HAL_OK)
        {
            Error_Handler();
        }

        // Restart the system to apply the changes
        HAL_NVIC_SystemReset();
    }

    // 8) Relock
    HAL_FLASH_OB_Lock();
    HAL_FLASH_Lock();
}

/**
 * @brief  Reboot the system after a delay.
 */
static void reboot(void)
{
	printf("Rebooting in 2\n");
	/* Wait 2 seconds. */
	HAL_Delay(2000);
	NVIC_SystemReset();
}

/**
 * @brief  Jump to the firmware stored in flash memory.
 * @param  fwFlashStartAdd  Address where the firmware starts in flash memory.
 */
static void gotoFirmware(uint32_t fwFlashStartAdd)
{
	HAL_MPU_Disable();
	HAL_SuspendTick();

	__disable_irq(); // Disable interrupt

	SysTick->CTRL = 0;  // Disable SysTick
	SysTick->LOAD = 0;
	SysTick->VAL  = 0;

	HAL_RCC_DeInit();

	for (uint8_t i = 0; i < 8; i++) // Clear all NVIC Enable and Pending registers
	{
		NVIC->ICER[i] = 0xFFFFFFFF;
		NVIC->ICPR[i] = 0xFFFFFFFF;
	}

	// Validate firmware address
	uint32_t appStack = *((volatile uint32_t*) fwFlashStartAdd);
	uint32_t appEntry = *((volatile uint32_t*) (fwFlashStartAdd + 4));

	SCB_DisableICache();
	SCB_DisableDCache();

	__enable_irq();

	__DMB();
	SCB->VTOR = fwFlashStartAdd;
	__DSB();

	HAL_DeInit();
	__set_MSP(appStack);

	pFunction jumpToApplication = (pFunction)appEntry;
	jumpToApplication();
}
/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void)
{

	/* USER CODE BEGIN 1 */

	/* USER CODE END 1 */
	/* USER CODE BEGIN Boot_Mode_Sequence_0 */

	/* USER CODE END Boot_Mode_Sequence_0 */

	/* MPU Configuration--------------------------------------------------------*/
	MPU_Config();

	/* Enable the CPU Cache */

	/* Enable I-Cache---------------------------------------------------------*/
	SCB_EnableICache();

	/* Enable D-Cache---------------------------------------------------------*/
	SCB_EnableDCache();

	/* USER CODE BEGIN Boot_Mode_Sequence_1 */

	/* USER CODE END Boot_Mode_Sequence_1 */
	/* MCU Configuration--------------------------------------------------------*/

	/* Reset of all peripherals, Initializes the Flash interface and the Systick. */
	HAL_Init();

	/* USER CODE BEGIN Init */
	configureBootConfiguration();
	/* USER CODE END Init */

	/* Configure the system clock */
	SystemClock_Config();
	/* USER CODE BEGIN Boot_Mode_Sequence_2 */
	//STM32Flash_writePersistentData(FW_UPDATE_RECEIVED);	//uncomment for testing
	//STM32Flash_writePersistentData(FW_UPDATE_NONE);

	/* USER CODE END Boot_Mode_Sequence_2 */

	/* USER CODE BEGIN SysInit */

	/* Initialize the Flash Update State */
	FW_UpdateState dataRead;
	STM32Flash_readPersistentData(&dataRead);

	if (dataRead == FW_UPDATE_NONE)
	{
		gotoFirmware(FW_CM7_START_ADDR);
	}

	if (dataRead == FW_UPDATE_TO_TEST)
	{
		STM32Flash_writePersistentData(FW_UPDATE_TESTING);
		gotoFirmware(FW_CM7_START_ADDR);
	}
	/* USER CODE END SysInit */

	/* Initialize all configured peripherals */
	MX_GPIO_Init();
	MX_FMC_Init();
	MX_USART1_UART_Init();
	MX_RNG_Init();
	MX_CRC_Init();
	MX_QUADSPI_Init();
	MX_FATFS_Init();
	/* USER CODE BEGIN 2 */

	printf("\n------- START BOOTLOADER -------\n");

	gui_init();
	printf("Bootloader version: %s\n", BL_VERSION);

	printf("----- FILE INITIALIZATION ------\n");

	FRESULT fres; // Variable to store the result of FATFS operations

	// Attempt to mount the file system on SD card or USB (where the package is stored)
	fres = f_mount(&fs, "0:", 1);
	if (fres != FR_OK)
	{
		printf("FS mount ERROR\n");
		gui_displayUpdateFailed();
	}
	printf("FS mount SUCCESS\n");

	if (dataRead == FW_UPDATE_TESTING)
	{
		printf("--- RESTORE PREVIOUS FIRMWARE --\n");
		if (update_restoreBackupFirmwares() != FWUPDATE_OK)
		{
			gui_displayUpdateFailed();

			reboot();
		}

		/* Reboot after we close the connection. */
		if (STM32Flash_writePersistentData(FW_UPDATE_TO_TEST) != STM32FLASH_OK)
		{
			printf("Failed to write firmware update status in STM32 flash\n");
		}
		else
		{
			printf("Firmware update done, reset firmware update flag\n");
		}

		gui_displayUpdateSuccess();

		reboot();
	}

	if (dataRead == FW_UPDATE_DONE)
	{
		/* Reboot after we close the connection. */
		if (STM32Flash_writePersistentData(FW_UPDATE_NONE) != STM32FLASH_OK)
		{
			printf("Failed to write firmware update status in STM32 flash\n");
		}
		else
		{
			printf("Firmware update done, reset firmware update flag\n");
		}

		gui_displayUpdateSuccess();

		reboot();
	}

	if (dataRead == FW_UPDATE_RECEIVED)
	{
		char packageFilePath[64];

		// Find the package file
		if (update_findPackageFile(packageFilePath, sizeof(packageFilePath)) != FWUPDATE_OK)
		{
			printf("No firmware found in /firmware/\n");

			if (STM32Flash_writePersistentData(FW_UPDATE_NONE) != STM32FLASH_OK)
			{
				printf("Failed to write firmware update status in STM32 flash\n");
			}
			else
			{
				printf("Firmware update abort, reset firmware update flag\n");
			}

			gui_displayUpdateFailed();
			reboot();
		}
		else
		{
			printf("Found package file: %s\n", packageFilePath);

			printf("--------- START UPDATE ---------\n");

			fwupdate_StatusTypeDef status = update_processPackageFile(packageFilePath);
			if (status != FWUPDATE_OK)
			{
				if (status != FWUPDATE_CRCMISMATCH)
				{
					printf("Firmware update failed\n");
					gui_displayUpdateFailed();
				}
				else
				{
					printf("Preparing to reset all cores \n");

					if (STM32Flash_writePersistentData(FW_UPDATE_NONE) != STM32FLASH_OK)
					{
						printf("Failed to write firmware update status in STM32 flash\n");
					}
					else
					{
						printf("Firmware update aborted \n");
						reboot();
					}
				}
			}
			else
			{
				printf("Firmware update completed successfully\n");
				gui_displayUpdateTesting();
			}
		}

		printf("Preparing to reset all cores \n");

		/* Reboot after we close the connection. */
		if (STM32Flash_writePersistentData(FW_UPDATE_TO_TEST) != STM32FLASH_OK)
		{
			printf("Failed to write firmware update status in STM32 flash\n");
		}
		else
		{
			printf("Firmware update must be tested now \n");
			reboot();
		}
	}

	/* USER CODE END 2 */

	/* Infinite loop */
	/* USER CODE BEGIN WHILE */
	while (1)
	{
		/* USER CODE END WHILE */

		/* USER CODE BEGIN 3 */
	}
	/* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void)
{
	RCC_OscInitTypeDef RCC_OscInitStruct = {0};
	RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

	/** Supply configuration update enable
	 */
	HAL_PWREx_ConfigSupply(PWR_DIRECT_SMPS_SUPPLY);

	/** Configure the main internal regulator output voltage
	 */
	__HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

	while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

	/** Initializes the RCC Oscillators according to the specified parameters
	 * in the RCC_OscInitTypeDef structure.
	 */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI48|RCC_OSCILLATORTYPE_HSE;
	RCC_OscInitStruct.HSEState = RCC_HSE_ON;
	RCC_OscInitStruct.HSI48State = RCC_HSI48_ON;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
	RCC_OscInitStruct.PLL.PLLM = 5;
	RCC_OscInitStruct.PLL.PLLN = 160;
	RCC_OscInitStruct.PLL.PLLP = 2;
	RCC_OscInitStruct.PLL.PLLQ = 8;
	RCC_OscInitStruct.PLL.PLLR = 4;
	RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_2;
	RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
	RCC_OscInitStruct.PLL.PLLFRACN = 0;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
	{
		Error_Handler();
	}

	/** Initializes the CPU, AHB and APB buses clocks
	 */
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
			|RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
			|RCC_CLOCKTYPE_D3PCLK1|RCC_CLOCKTYPE_D1PCLK1;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;
	RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
	RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
	{
		Error_Handler();
	}
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/* MPU Configuration */

void MPU_Config(void)
{

	/* Disables the MPU */
	HAL_MPU_Disable();
	/* Enables the MPU */
	HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);

}

/**
 * @brief  Period elapsed callback in non blocking mode
 * @note   This function is called  when TIM2 interrupt took place, inside
 * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
 * a global variable "uwTick" used as application time base.
 * @param  htim : TIM handle
 * @retval None
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	/* USER CODE BEGIN Callback 0 */

	/* USER CODE END Callback 0 */
	if (htim->Instance == TIM2) {
		HAL_IncTick();
	}
	/* USER CODE BEGIN Callback 1 */

	/* USER CODE END Callback 1 */
}

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void)
{
	/* USER CODE BEGIN Error_Handler_Debug */
	/* User can add his own implementation to report the HAL error return state */
	__disable_irq();
	while (1)
	{
	}
	/* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t *file, uint32_t line)
{
	/* USER CODE BEGIN 6 */
	/* User can add his own implementation to report the file name and line number,
         ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
	/* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
