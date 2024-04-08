/*************************************************************************************************
 *          Macronix xSPI-NOR Flash BSP driver header file for STM32 QUADSPI IP
 * @file    MXIC.h
 * @brief   Basic Read/Write/Erase functions for Macronix xSPI-NOR Flash.
 * @Author  Peter Chang / Macronix; peterchang01@mxic.com.tw
 *************************************************************************************************
 * History :
 * 2023/12/20 Release V2.0.0
 *            Rename stm32f769i_discovery_qspi_MXIC.h to stm32xxx_qspi_MXIC.h
 *            Rename stm32f769i_discovery_qspi_MXIC.c to stm32xxx_qspi_MXIC.c
 *            Remove USE_AUTO_DC_CONFIGURATION define
 *            Alignment BSP input parameter between QUAD-SPI & OCTO-SPI IP
 *            Move BSP_QSPI_ReadConfigurationRegister2() & BSP_QSPI_WriteConfigurationRegister2()
 *            form stm32xxx_qspi_MXIC_ex.c to stm32xxx_qspi_MXIC.c
 * 2023/01/30 Release V1.2.0
 * 2022/12/12 Release V1.1.0
 * 2022/12/08 Rename to stm32xxx_QUADSPI_MXIC.h from stm32f769i_discovery_qspi_MXIC.h
 *            Add error item "BSP_ERROR_DRIVER_NOT_MATCH_DEVICE"
 * 2022/10/06 Move RPMC functions to stm32f769i_discovery_qspi_MXIC_ex.c
 * 2022/06/17 Release V1.0.1
 *            Remove test functions
 * 2022/06/15 Release V1.0.0
 * 2021/12/09 Initial version.
 *************************************************************************************************/
/* Define to prevent recursive inclusion --------------------------------------------------------*/
#ifndef __STM32XXX_QUADSPI_MXIC_H
#define __STM32XXX_QUADSPI_MXIC_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "MXIC_xSPINOR.h"
#include "stm32h7xx_hal.h"

/* Exported constants --------------------------------------------------------*/
#define QSPI_NOR_INSTANCE_NUMBER    1U

/* QSPI Info */
#define BSP_QSPI_Info_t   MXIC_SNOR_DriverInfo_t
#define BSP_QSPI_Init_t   MXIC_SNOR_Mode_t
#define BSP_QSPI_Erase_t  MXIC_SNOR_EraseTypeDef

/******************************************************************************
 * BSP Error Type
 * @addtogroup STM32XXX_QUADSPI
 ******************************************************************************/
typedef enum
{
  BSP_ERROR_UNLOCK_FAILURE              = -15,
  BSP_ERROR_LOCK_FAILURE                = -14,
  BSP_ERROR_RPMC_FAILURE                = -13,
  BSP_ERROR_INSTANCE_MMP_UNLOCK_FAILURE = -12,
  BSP_ERROR_MSP_FAILURE                 = -11,
  BSP_ERROR_CLOCK_FAILURE               = -10,
  BSP_ERROR_BUS_FAILURE                 = -9,
  BSP_ERROR_DRIVER_NOT_MATCH_DEVICE     = -8,
  BSP_ERROR_UNKNOWN_COMPONENT           = -7,
  BSP_ERROR_UNKNOWN_FAILURE             = -6,
  BSP_ERROR_COMPONENT_FAILURE           = -5,
  BSP_ERROR_PERIPH_FAILURE              = -4,
  BSP_ERROR_BUSY                        = -3,
  BSP_ERROR_WRONG_PARAM                 = -2,
  BSP_ERROR_NO_INIT                     = -1,
  BSP_ERROR_NONE                        =  0,
} QSPI_ErrorTypeDef;

typedef enum
{
  QSPI_ACCESS_NONE = 0,          /*!<  Instance not initialized,             */
  QSPI_ACCESS_INDIRECT,          /*!<  Instance use indirect mode access     */
  QSPI_ACCESS_MMP,               /*!<  Instance use Memeoy Mapped Mode read  */
} QSPI_AccessTypeDef;


/* Exported types ------------------------------------------------------------*/
typedef struct
{
  QSPI_AccessTypeDef   IsInitialized;   /* Instance access Flash method     */
  BSP_QSPI_Init_t      InterfaceMode;   /* Flash Interface mode of Instance */
  uint32_t             IsMspCallbacksValid;
} QSPI_Ctx_t;

#if (USE_HAL_QSPI_REGISTER_CALLBACKS == 1)
typedef void (*pBSP_QSPI_CallbackTypeDef)(void *pHandle);

typedef struct
{
  pBSP_QSPI_CallbackTypeDef  pMspInitCb;
  pBSP_QSPI_CallbackTypeDef  pMspDeInitCb;
} BSP_QSPI_Cb_t;
#endif /* (USE_HAL_QSPI_REGISTER_CALLBACKS == 1) */

/* Exported functions --------------------------------------------------------*/
/*
 * @addtogroup STM32XXX_QUADSPI_Exported_Functions
 * @{
 */
int32_t BSP_QSPI_Init(uint32_t Instance, BSP_QSPI_Init_t Init);
int32_t BSP_QSPI_DeInit(uint32_t Instance);
#if (USE_HAL_QSPI_REGISTER_CALLBACKS == 1)
int32_t BSP_QSPI_Register_QspiMspCallbacks (uint32_t Instance, pQSPI_CallbackTypeDef pMspInitCb, pQSPI_CallbackTypeDef pMspDeInitCb);
int32_t BSP_QSPI_Register_DefaultMspCallbacks (uint32_t Instance);
#endif /* (USE_HAL_QSPI_REGISTER_CALLBACKS == 1) */

int32_t BSP_QSPI_GetInfo(uint32_t Instance, BSP_QSPI_Info_t *pInfo);
int32_t BSP_QSPI_GetStatus(uint32_t Instance);
int32_t BSP_QSPI_SetFlashInterface(uint32_t Instance, BSP_QSPI_Init_t Init);
int32_t BSP_QSPI_GetFlashInterface(uint32_t Instance, BSP_QSPI_Init_t *pInit);
int32_t BSP_QSPI_EnableMemoryMappedMode(uint32_t Instance);
int32_t BSP_QSPI_DisableMemoryMappedMode(uint32_t Instance);

/* Flash Operation Functions *********************************************************/
int32_t BSP_QSPI_ReadID(uint32_t Instance, uint32_t *pID);
int32_t BSP_QSPI_Read(uint32_t Instance, uint8_t *pData, uint32_t Address, uint32_t Size);
int32_t BSP_QSPI_Write(uint32_t Instance, uint8_t *pData, uint32_t Address, uint32_t Size);
int32_t BSP_QSPI_EraseBlock(uint32_t Instance, uint32_t Address, BSP_QSPI_Erase_t Size);
int32_t BSP_QSPI_EraseChip(uint32_t Instance);

int32_t BSP_QSPI_ReadStatusRegister(uint32_t Instance, MXIC_SNOR_StatusRegister_t *pData);
int32_t BSP_QSPI_WriteStatusRegister(uint32_t Instance, MXIC_SNOR_StatusRegister_t Data);

#ifdef MXIC_SNOR_READ_CONFIGURATION_REG_CMD
int32_t BSP_QSPI_ReadConfigurationRegister(uint32_t Instance, uint8_t *pData, uint32_t Size);
int32_t BSP_QSPI_WriteConfigurationRegister(uint32_t Instance, uint8_t *pData, uint32_t Size);
#endif

#ifdef SUPPORT_CONFIGURATION_REGISTER2
int32_t BSP_QSPI_ReadConfigurationRegister2(uint32_t Instance, uint8_t *pData, uint32_t Address);
int32_t BSP_QSPI_WriteConfigurationRegister2(uint32_t Instance, uint8_t Data, uint32_t Address);
#endif  // SUPPORT_CONFIGURATION_REGISTER2

int32_t BSP_QSPI_ResetMemory(int32_t Instance);

/* These functions can be modified in case the current settings
   need to be changed for specific application needs */
__weak HAL_StatusTypeDef MX_QSPI_Init(QSPI_HandleTypeDef *hqspi, uint32_t FlashSize, uint32_t ClockPrescaler);

/******************************************************************************
 *   STM32 MCU MSP define for QUADSPI interface
 * User need modify these pin define for fit system design.
 * MSP initial & de-initial functions defined in stm32xxx_QUADSPI_MXIC.c
 * static void QSPI_MspInit(QSPI_HandleTypeDef *hqspi);
 * static void QSPI_MspDeInit(QSPI_HandleTypeDef *hqspi);
 ******************************************************************************/

#define QSPI_FORCE_RESET()         __HAL_RCC_QSPI_FORCE_RESET()
#define QSPI_RELEASE_RESET()       __HAL_RCC_QSPI_RELEASE_RESET()

#ifdef __cplusplus
}
#endif

#endif /* __STM32XXX_QUADSPI_MXIC_H */
/************************ (C) COPYRIGHT Macronix **************END OF FILE****/
