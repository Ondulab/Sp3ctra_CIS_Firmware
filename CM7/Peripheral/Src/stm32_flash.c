/**
 ******************************************************************************
 * @file           : cis_BW_.c
 * @brief          :
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "config.h"
#include "basetypes.h"
#include "shared.h"

#include "stdlib.h"
#include "stdio.h"
#include "stdbool.h"

#include "arm_math.h"

#include "stm32_flash.h"

/* Private includes ----------------------------------------------------------*/

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Variable containing ADC conversions data */

/* Private function prototypes -----------------------------------------------*/
static uint32_t stm32_flashGetSector(uint32_t Address);

/* Private user code ---------------------------------------------------------*/


/**
 * @brief  Gets the sector of a given address
 * @param  Address Address of the FLASH Memory
 * @retval The sector of a given address
 */
uint32_t stm32_flashGetSector(uint32_t Address)
{
	uint32_t sector = 0;

	if(((Address < ADDR_FLASH_SECTOR_1_BANK1) && (Address >= ADDR_FLASH_SECTOR_0_BANK1)) || \
			((Address < ADDR_FLASH_SECTOR_1_BANK2) && (Address >= ADDR_FLASH_SECTOR_0_BANK2)))
	{
		sector = FLASH_SECTOR_0;
	}
	else if(((Address < ADDR_FLASH_SECTOR_2_BANK1) && (Address >= ADDR_FLASH_SECTOR_1_BANK1)) || \
			((Address < ADDR_FLASH_SECTOR_2_BANK2) && (Address >= ADDR_FLASH_SECTOR_1_BANK2)))
	{
		sector = FLASH_SECTOR_1;
	}
	else if(((Address < ADDR_FLASH_SECTOR_3_BANK1) && (Address >= ADDR_FLASH_SECTOR_2_BANK1)) || \
			((Address < ADDR_FLASH_SECTOR_3_BANK2) && (Address >= ADDR_FLASH_SECTOR_2_BANK2)))
	{
		sector = FLASH_SECTOR_2;
	}
	else if(((Address < ADDR_FLASH_SECTOR_4_BANK1) && (Address >= ADDR_FLASH_SECTOR_3_BANK1)) || \
			((Address < ADDR_FLASH_SECTOR_4_BANK2) && (Address >= ADDR_FLASH_SECTOR_3_BANK2)))
	{
		sector = FLASH_SECTOR_3;
	}
	else if(((Address < ADDR_FLASH_SECTOR_5_BANK1) && (Address >= ADDR_FLASH_SECTOR_4_BANK1)) || \
			((Address < ADDR_FLASH_SECTOR_5_BANK2) && (Address >= ADDR_FLASH_SECTOR_4_BANK2)))
	{
		sector = FLASH_SECTOR_4;
	}
	else if(((Address < ADDR_FLASH_SECTOR_6_BANK1) && (Address >= ADDR_FLASH_SECTOR_5_BANK1)) || \
			((Address < ADDR_FLASH_SECTOR_6_BANK2) && (Address >= ADDR_FLASH_SECTOR_5_BANK2)))
	{
		sector = FLASH_SECTOR_5;
	}
	else if(((Address < ADDR_FLASH_SECTOR_7_BANK1) && (Address >= ADDR_FLASH_SECTOR_6_BANK1)) || \
			((Address < ADDR_FLASH_SECTOR_7_BANK2) && (Address >= ADDR_FLASH_SECTOR_6_BANK2)))
	{
		sector = FLASH_SECTOR_6;
	}
	else if(((Address < ADDR_FLASH_SECTOR_0_BANK2) && (Address >= ADDR_FLASH_SECTOR_7_BANK1)) || \
			((Address < FLASH_END_ADDR) && (Address >= ADDR_FLASH_SECTOR_7_BANK2)))
	{
		sector = FLASH_SECTOR_7;
	}
	else
	{
		sector = FLASH_SECTOR_7;
	}

	return sector;
}

/**
 * @brief  CIS calibration
 * @param  cisData ptr
 * @param  calibration iteration
 * @retval None
 */
void stm32_flashCalibrationRW(CIS_FlashRW_TypeDef RW)
{
	static FLASH_EraseInitTypeDef eraseInitStruct = {0};
	uint32_t firstSector = 0, nbOfSectors = 0, sectorError = 0, address = 0, idx = 0;
	__IO uint32_t memoryProgramStatus = 0;
	__IO uint32_t data32 = 0;

	switch (RW)
	{
	case CIS_READ_CAL :
		address = ADDR_CIS_FLASH_CALIBRATION;
		for(idx = 0; idx < sizeof(cisCals); idx+=4)
		{
			*(uint32_t *)((uint32_t)&cisCals + idx) = *(uint32_t *)address;
			__DSB();

			address+=4;
		}
		break;
	case CIS_WRITE_CAL :
		/* -1- Unlock the Flash to enable the flash control register access ***************/
		HAL_FLASH_Unlock();
		/* -2- Erase the user Flash area
			    (area defined by FLASH_USER_START_ADDR and FLASH_USER_END_ADDR) ***********/

		/* Get the 1st sector to erase */
		firstSector = stm32_flashGetSector(ADDR_CIS_FLASH_CALIBRATION);
		nbOfSectors = stm32_flashGetSector(FLASH_END_ADDR) - firstSector + 1;

		/* Fill EraseInit structure*/
		eraseInitStruct.TypeErase     = FLASH_TYPEERASE_SECTORS;
		eraseInitStruct.VoltageRange  = FLASH_VOLTAGE_RANGE_4;
		eraseInitStruct.Banks         = FLASH_BANK_1;
		eraseInitStruct.Sector        = firstSector;
		eraseInitStruct.NbSectors     = nbOfSectors;

		if (HAL_FLASHEx_Erase(&eraseInitStruct, &sectorError) != HAL_OK)
		{
			printf("Flash write fail\n");
			Error_Handler();
		}
		/* -3- Program the user Flash area word by word
			    (area defined by FLASH_USER_START_ADDR and FLASH_USER_END_ADDR) ***********/
		address = ADDR_CIS_FLASH_CALIBRATION;

		while (address < (ADDR_CIS_FLASH_CALIBRATION + sizeof(cisCals)))
		{
			if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD, address, (uint32_t)&cisCals + idx) == HAL_OK)
			{
				address += 32; //increment for the next Flash word
				idx+=32;
			}
			else
			{
				printf("Flash write fail\n");
				Error_Handler();
			}
		}
		/* -4- Lock the Flash to disable the flash control register access (recommended
			     to protect the FLASH memory against possible unwanted operation) *********/
		HAL_FLASH_Lock();
		/* -5- Check if the programmed data is OK
		      MemoryProgramStatus = 0: data programmed correctly
		      MemoryProgramStatus != 0: number of words not programmed correctly **********/
		address = ADDR_CIS_FLASH_CALIBRATION;

		for(idx = 0; idx < sizeof(cisCals); idx+=4)
		{
			data32 = *(uint32_t *)address;
			__DSB();
			if(data32 != *(uint32_t *)((uint32_t)&cisCals + idx))
			{
				memoryProgramStatus++;
			}
			address+=4;
		}

		/* -6- Check if there is an issue to program data*/
		if (memoryProgramStatus != 0)
		{
			printf("Flash write fail\n");
			Error_Handler();
		}
		break;
	default :
		Error_Handler();
	}
}
