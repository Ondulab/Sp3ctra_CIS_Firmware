/*************************************************************************************************
 *          Macronix xSPI-NOR Flash BSP driver
 * @file    stm32xxx_QUADSPI_MXIC.c
 * @brief   Macronix xSPI-NOR Flash BSP driver for STM32 QUASDPI IP.
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
 * 2023/01/10 Change software reset procedure in BSP_QSPI_ResetMemory()
 * 2022/12/27 Add function to check if device support required mode in BSP_QSPI_SetFlashInterface()
 * 2022/12/12 Release V1.1.0
 * 2022/12/08 Rename to stm32xxx_QUADSPI_MXIC.c from stm32f769i_discovery_qspi_MXIC.c
 *            Add procedure to check if device is Macronix parts but driver not match.
 * 2022/10/06 Move RPMC functions to stm32f769i_discovery_qspi_MXIC_ex.c
 * 2022/06/17 Release V1.0.1
 *            Remove test functions
 * 2022/06/15 Release V1.0.0
 * 2021/12/09 Initial version.
 *************************************************************************************************/
/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"
#include "quadspi.h"
//#include "cmsis_os.h"

#include "MXIC.h"


/*******************************************************************************
 * QUADSPI IP support Dual-Quad Flash access
 * This driver use Instance 0 to access single Quad Flash via BK1 only.
 ******************************************************************************/
QSPI_HandleTypeDef QSPIHandle[QSPI_NOR_INSTANCE_NUMBER];
QSPI_Ctx_t            QSPICtx[QSPI_NOR_INSTANCE_NUMBER] = {0};

/* Private functions ---------------------------------------------------------*/
/******************************************************************************
 *   STM32 MCU MSP define for QUADSPI interface
 * User need modify these pin define for fit system design.
 ******************************************************************************/

#ifdef MXIC_SNOR_CR_ODS
static int32_t QSPI_SetODS(int32_t Instance, uint8_t ODS);
#endif  // MXIC_SNOR_CR_ODS

#if defined(MXIC_SNOR_CR_DC) || defined(MXIC_SNOR_CR2_DC)
static int32_t QSPI_DummyCyclesCfg(int32_t Instance, uint8_t DCIndex);
#endif

/*******************************************************************************
 * Export Functions
 ******************************************************************************/
/**
  * @brief  Initializes the QSPI interface.
  * @param  Instance   QSPI Instance
  *         Init       QSPI Init structure
  * @retval BSP status
  */
int32_t BSP_QSPI_Init(uint32_t Instance, BSP_QSPI_Init_t Init)
{
  BSP_QSPI_Info_t Info;

  /* Check if the instance is supported */
  if(Instance >= QSPI_NOR_INSTANCE_NUMBER)
  {
    return BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    QSPIHandle[Instance].Instance = QUADSPI;

    /* Call the DeInit function to reset the driver */
    if (HAL_QSPI_DeInit(&QSPIHandle[Instance]) != HAL_OK)
    {
      return BSP_ERROR_BUS_FAILURE;
    }

#if (USE_HAL_QSPI_REGISTER_CALLBACKS == 1)
    /* Register the QSPI MSP Callbacks */
    if(QSPICtx[Instance].IsMspCallbacksValid == 0)
    {
      if(BSP_QSPI_Register_DefaultMspCallbacks(Instance) != BSP_ERROR_NONE)
      {
        return BSP_ERROR_PERIPH_FAILURE;
      }
    }
#else
    /* Msp SD initialization */
    HAL_QSPI_MspInit(&QSPIHandle[Instance]);
#endif /* USE_HAL_QSPI_REGISTER_CALLBACKS */

    /* STM32 QSPI interface initialization */
    (void)MXIC_SNOR_GetDriverInfo(&Info);

    if(MX_QSPI_Init(&QSPIHandle[Instance], Info.DeviceSize, 1) != HAL_OK)   // ClockPrescaler, Adjust SCLK frequency
    {
      return BSP_ERROR_PERIPH_FAILURE;
    }/* Reset QSPI memory; After reset Mode = MXIC_SNOR_FREAD_111 + STR always */
    else if(BSP_QSPI_ResetMemory(Instance) != BSP_ERROR_NONE)
    {
      return BSP_ERROR_COMPONENT_FAILURE;
    }
    else
    {
#if 0   /* Check if device match driver */
      MXIC_SNOR_CommandHandle_t IDCommandHandle;

      IDCommandHandle.Mode             = QSPICtx[Instance].InterfaceMode;
      IDCommandHandle.Parameter.Type32 = 0;
      IDCommandHandle.pBuffer          = IDCommandHandle.Parameter.Type8;
      if(MXIC_SNOR_ReadID(&QSPIHandle[Instance], &IDCommandHandle) != MXIC_SNOR_ERROR_NONE)
      {
        return BSP_ERROR_COMPONENT_FAILURE;
      }
      else if(IDCommandHandle.Parameter.Type32 != Info.ID)
      {
        // Check if this is a Macronix parts
        if(IDCommandHandle.Parameter.Type8[0] == MXIC_SNOR_MEMORY_MANUFACTURER_ID) return BSP_ERROR_DRIVER_NOT_MATCH_DEVICE;
        else                                                                       return BSP_ERROR_UNKNOWN_COMPONENT;
      }
#endif

#ifdef MXIC_SNOR_SR_QE
      // Always turn on QE bit to change pin function in I/O mode.
      // If pin H/W function used then user may remove this routine.
      MXIC_SNOR_CommandHandle_t CommandHandle;
      MXIC_SNOR_StatusRegister_t SReg;

      /* Read Status Register, Fore Flash QE bit = 1; Disable WP# pin function */
      CommandHandle.Mode    = QSPICtx[Instance].InterfaceMode;
      CommandHandle.pBuffer = (uint8_t *)&SReg;
      if(MXIC_SNOR_ReadStatusRegister(&QSPIHandle[Instance], &CommandHandle) != MXIC_SNOR_ERROR_NONE)
      {
        return BSP_ERROR_COMPONENT_FAILURE;
      }
      else
      {
        /* Write Status Register, Fore Flash QE bit = 1; Disable WP# pin function */
        SReg.QE = 1;

        if(BSP_QSPI_WriteStatusRegister(Instance, SReg) != BSP_ERROR_NONE)
        {
          return BSP_ERROR_COMPONENT_FAILURE;
        }
      }
#endif  // MXIC_SNOR_SR_QE

#if defined(MXIC_SNOR_CR_DC) || defined(MXIC_SNOR_CR2_DC)
      if(QSPI_DummyCyclesCfg(Instance, DC_INDEX) != BSP_ERROR_NONE)
      {
        return BSP_ERROR_COMPONENT_FAILURE;
      }
#endif  // defined(MXIC_SNOR_CR_DC) || defined(MXIC_SNOR_CR2_DC)

#ifdef MXIC_SNOR_CR_ODS
      /* Configuration of the Output driver strength on memory side. Modify ODS_INDEX in include file. */
      if(QSPI_SetODS(Instance, ODS_INDEX) != BSP_ERROR_NONE)
      {
        return BSP_ERROR_COMPONENT_FAILURE;
      }
#endif  // MXIC_SNOR_CR_ODS

#ifdef SUPPORT_34BYTE_ADDRESS_SWITCH
      /* If support 4 Byte address switch command then force command use 4 Byte address */
      if(MXIC_SNOR_Enter4BytesAddressMode(&QSPIHandle[Instance], QSPICtx[Instance].InterfaceMode) != MXIC_SNOR_ERROR_NONE)
      {
        return BSP_ERROR_COMPONENT_FAILURE;
      }
#endif  // SUPPORT_34BYTE_ADDRESS_SWITCH

      /* Configure Flash to desired Interface */
      if(BSP_QSPI_SetFlashInterface(Instance, Init) != BSP_ERROR_NONE)
      {
        return BSP_ERROR_COMPONENT_FAILURE;
      }
    }
  }
  /* Return BSP status */
  return BSP_ERROR_NONE;
}

/**
  * @brief  De-Initializes the QSPI interface.
  * @param  Instance   QSPI Instance
  * @retval BSP status
  */
int32_t BSP_QSPI_DeInit(uint32_t Instance)
{
  int32_t ret = BSP_ERROR_NONE;

  /* Check if the instance is supported */
  if(Instance >= QSPI_NOR_INSTANCE_NUMBER)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    /* Disable Memory mapped mode */
    if(QSPICtx[Instance].IsInitialized == QSPI_ACCESS_MMP)
    {
      if(BSP_QSPI_DisableMemoryMappedMode(Instance) != BSP_ERROR_NONE)
      {
        ret = BSP_ERROR_COMPONENT_FAILURE;
      }
    }

    /* Set default QSPICtx values even previous process failed ! */
    QSPICtx[Instance].IsInitialized      = QSPI_ACCESS_NONE;
    QSPICtx[Instance].InterfaceMode.IO   = MXIC_SNOR_FREAD_111;
    QSPICtx[Instance].InterfaceMode.Rate = MXIC_SNOR_STR;

#if (USE_HAL_QSPI_REGISTER_CALLBACKS == 0)
    HAL_QSPI_MspDeInit(&QSPIHandle[Instance]);
#endif /* (USE_HAL_QSPI_REGISTER_CALLBACKS == 0) */

    /* Call the DeInit function to reset the driver */
    if (HAL_QSPI_DeInit(&QSPIHandle[Instance]) != HAL_OK)
    {
      ret = BSP_ERROR_BUS_FAILURE;
    }
  }
  /* Return BSP status */
  return ret;
}

#if (USE_HAL_QSPI_REGISTER_CALLBACKS == 1)
/**
  * @brief Default BSP QSPI Msp Callbacks
  * @param Instance      QSPI Instance
  * @retval BSP status
  */
int32_t BSP_QSPI_RegisterDefaultMspCallbacks (uint32_t Instance)
{
  int32_t ret = BSP_ERROR_NONE;

  /* Check if the instance is supported */
  if(Instance >= QSPI_NOR_INSTANCE_NUMBER)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    /* Register MspInit/MspDeInit Callbacks */
    if((HAL_QSPI_RegisterCallback(&QSPIHandle[Instance], HAL_QSPI_MSPINIT_CB_ID, QSPI_MspInit) != HAL_OK) ||
       (HAL_QSPI_RegisterCallback(&QSPIHandle[Instance], HAL_QSPI_MSPDEINIT_CB_ID, QSPI_MspDeInit) != HAL_OK))
    {
      ret = BSP_ERROR_PERIPH_FAILURE;
    }
    else
    {
      QSPICtx[Instance].IsMspCallbacksValid = 1;
    }
  }
  /* Return BSP status */
  return ret;
}

/**
  * @brief BSP QSPI Msp Callback registering
  * @param Instance     QSPI Instance
  * @param CallBacks    pointer to MspInit/MspDeInit callbacks functions
  * @retval BSP status
  */
int32_t BSP_QSPI_RegisterMspCallbacks (uint32_t Instance, BSP_QSPI_Cb_t *CallBacks)
{
  int32_t ret = BSP_ERROR_NONE;

  /* Check if the instance is supported */
  if(Instance >= QSPI_NOR_INSTANCE_NUMBER)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    /* Register MspInit/MspDeInit Callbacks */
    if((HAL_QSPI_RegisterCallback(&QSPIHandle[Instance], HAL_QSPI_MSPINIT_CB_ID, (pQSPI_CallbackTypeDef)CallBacks->pMspInitCb) != HAL_OK) ||
       (HAL_QSPI_RegisterCallback(&QSPIHandle[Instance], HAL_QSPI_MSPDEINIT_CB_ID, (pQSPI_CallbackTypeDef)CallBacks->pMspDeInitCb) != HAL_OK))
    {
      ret = BSP_ERROR_PERIPH_FAILURE;
    }
    else
    {
      QSPICtx[Instance].IsMspCallbacksValid = 1;
    }
  }
  /* Return BSP status */
  return ret;
}
#endif /* (USE_HAL_QSPI_REGISTER_CALLBACKS == 1) */

/**
  * @brief  Return the driver information
  * @param  Instance  QSPI instance
  *         pInfo     pointer on the driver information structure
  * @retval BSP status
  */
int32_t BSP_QSPI_GetInfo(uint32_t Instance, BSP_QSPI_Info_t *pInfo)
{
  int32_t ret = BSP_ERROR_NONE;

  /* Check if the instance is supported */
  if(Instance >= QSPI_NOR_INSTANCE_NUMBER)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    (void)MXIC_SNOR_GetDriverInfo(pInfo);
  }
  /* Return BSP status */
  return ret;
}

/**
  * @brief  Reads current status of the QSPI memory.
  *         If WIP != 0 then return busy.
  * @param  Instance  QSPI instance
  * @retval QSPI memory status: whether busy or not
  */
int32_t BSP_QSPI_GetStatus(uint32_t Instance)
{
  int32_t ret;
  MXIC_SNOR_StatusRegister_t SReg;

  ret = BSP_QSPI_ReadStatusRegister(Instance, &SReg);
  if(ret == BSP_ERROR_NONE)
  {
    if(SReg.WIP)
    {
      ret = BSP_ERROR_BUSY;
    }
  }
  /* Return BSP status */
  return ret;
}

/**
  * @brief  Set Flash to desired Interface mode. And this instance becomes current instance.
  *         If current instance running at MMP mode then exit to indirect mode.
  *         MCU is running at Indirect mode always, after call this function.
  *         SPI/QPI change mode     : Issue 1-x-x EQIO <-> 4-x-x RSTQIO
  *         Octal Flash change mode : 8-8-8/STR <-> 1-1-1-STR <-> 8-8-8/DTR
  * @param  Instance  OSPI_NOR instance
  *         Init      Flash I/O & transfer rate configuration
  * @retval BSP status
  */
int32_t BSP_QSPI_SetFlashInterface(uint32_t Instance, BSP_QSPI_Init_t Init)
{
  int32_t ret = BSP_ERROR_NONE;

  /* Check if the instance is supported */
  if(Instance >= QSPI_NOR_INSTANCE_NUMBER)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    /* Check if device support required mode *************************************/
    switch(Init.IO)
    {
    default :                       // Quad-SPI IP not support 1-1-8/8-8-8 I/O
    //case MXIC_SNOR_FREAD_118 :    // 1-1-8 fast read command or Parallel mode command;                 0x03, 0x7C
    //case MXIC_SNOR_FREAD_888 :    // 8-8-8 fast read Octal command, MCU QSPI IP not support this mode; 0x7C, 0xEC, 0xEE
      ret = BSP_ERROR_WRONG_PARAM;
      break;

    case MXIC_SNOR_NREAD_111 :      // 1-1-1 normal read command with 0 dummy cycle;          0x03, 0x13
      if(Init.Rate == MXIC_SNOR_STR)
      {
#if !defined(MXIC_SNOR_NORMAL_READ_111_CMD) && !defined(MXIC_SNOR_4BYTE_ADDRESS_NORMAL_READ_111_CMD)
        ret = BSP_ERROR_WRONG_PARAM;
#endif
      }
      else ret = BSP_ERROR_WRONG_PARAM;
      break;

    case MXIC_SNOR_FREAD_111 :      // 1-1-1 fast read command, Power on H/W default setting; 0x0B, 0x0C, 0x0D, 0x0E
      if(Init.Rate == MXIC_SNOR_STR)
      {
#if !defined(MXIC_SNOR_FAST_READ_111_CMD) && !defined(MXIC_SNOR_4BYTE_ADDRESS_FAST_READ_111_CMD)
        ret = BSP_ERROR_WRONG_PARAM;
#endif
      }
      else
      {
#if !defined(MXIC_SNOR_FAST_READ_111_DTR_CMD) && !defined(MXIC_SNOR_4BYTE_ADDRESS_FAST_READ_111_DTR_CMD)
        ret = BSP_ERROR_WRONG_PARAM;
#endif
      }
      break;

    case MXIC_SNOR_FREAD_112 :      // 1-1-2 fast read command;                               0x3B, 0x3C
      if(Init.Rate == MXIC_SNOR_STR)
      {
#if !defined(MXIC_SNOR_FAST_READ_112_CMD) && !defined(MXIC_SNOR_4BYTE_ADDRESS_FAST_READ_112_CMD)
        ret = BSP_ERROR_WRONG_PARAM;
#endif
      }
      else ret = BSP_ERROR_WRONG_PARAM;
      break;

    case MXIC_SNOR_FREAD_114 :      // 1-1-4 fast read command;                               0x6B, 0x6C
      if(Init.Rate == MXIC_SNOR_STR)
      {
#if !defined(MXIC_SNOR_FAST_READ_114_CMD) && !defined(MXIC_SNOR_4BYTE_ADDRESS_FAST_READ_114_CMD)
      ret = BSP_ERROR_WRONG_PARAM;
#endif
      }
      else ret = BSP_ERROR_WRONG_PARAM;
      break;

    case MXIC_SNOR_FREAD_122 :      // 1-2-2 fast read command;                               0xBB, 0xBC, 0xBD, 0xBE
      if(Init.Rate == MXIC_SNOR_STR)
      {
#if !defined(MXIC_SNOR_FAST_READ_122_CMD) && !defined(MXIC_SNOR_4BYTE_ADDRESS_FAST_READ_122_CMD)
        ret = BSP_ERROR_WRONG_PARAM;
#endif
      }
      else
      {
#if !defined(MXIC_SNOR_FAST_READ_122_DTR_CMD) && !defined(MXIC_SNOR_4BYTE_ADDRESS_FAST_READ_122_DTR_CMD)
        ret = BSP_ERROR_WRONG_PARAM;
#endif
      }
      break;

    case MXIC_SNOR_FREAD_144 :      // 1-4-4 fast read command;                               0xEA, 0xEB, 0xEC, 0xED, 0xEE
      if(Init.Rate == MXIC_SNOR_STR)
      {
#if !defined(MXIC_SNOR_FAST_READ_144_CMD) && !defined(MXIC_SNOR_4BYTE_ADDRESS_FAST_READ_144_CMD)
        ret = BSP_ERROR_WRONG_PARAM;
#endif
      }
      else
      {
#if !defined(MXIC_SNOR_FAST_READ_144_DTR_CMD) && !defined(MXIC_SNOR_4BYTE_ADDRESS_FAST_READ_144_DTR_CMD)
        ret = BSP_ERROR_WRONG_PARAM;
#endif
      }
      break;

    case MXIC_SNOR_FREAD_144_4DC :  // 1-4-4 fast read command with 4 dummy clock;            0xE7
      if(Init.Rate == MXIC_SNOR_STR)
      {
#if !defined(MXIC_SNOR_FAST_READ_144_4DC_CMD)
        ret = BSP_ERROR_WRONG_PARAM;
#endif
      }
      else ret = BSP_ERROR_WRONG_PARAM;
      break;

    case MXIC_SNOR_FREAD_444 :      // 4-4-4 fast read QPI command;                           0xEA, 0xEB, 0xEC, 0xED, 0xEE
      if(Init.Rate == MXIC_SNOR_STR)
      {
#if !defined(MXIC_SNOR_FAST_READ_444_CMD) && !defined(MXIC_SNOR_4BYTE_ADDRESS_FAST_READ_444_CMD)
        ret = BSP_ERROR_WRONG_PARAM;
#endif
      }
      else
      {
#if !defined(MXIC_SNOR_FAST_READ_444_DTR_CMD) && !defined(MXIC_SNOR_4BYTE_ADDRESS_FAST_READ_444_DTR_CMD)
        ret = BSP_ERROR_WRONG_PARAM;
#endif
      }
      break;

    case MXIC_SNOR_FREAD_444_4DC :  // 4-4-4 fast read QPI command with 4 dummy clock;        0x0B, 0xE7
      if(Init.Rate == MXIC_SNOR_STR)
      {
#if !defined(MXIC_SNOR_FAST_READ_444_4DC_CMD)
        ret = BSP_ERROR_WRONG_PARAM;
#endif
      }
      else ret = BSP_ERROR_WRONG_PARAM;
      break;
    }
    if(ret != BSP_ERROR_NONE) return ret;

    /* Check if MCU running MMP mode *********************************************/
    if(QSPICtx[Instance].IsInitialized == QSPI_ACCESS_MMP)
    {
      /* Abort MMP back to indirect mode */
      if(HAL_QSPI_Abort(&QSPIHandle[Instance]) != HAL_OK) return BSP_ERROR_INSTANCE_MMP_UNLOCK_FAILURE;
      else QSPICtx[Instance].IsInitialized = QSPI_ACCESS_INDIRECT;
    }

    /* Setup Flash interface ***************************************************/
    switch(QSPICtx[Instance].InterfaceMode.IO)
    {
    default :                     // Quad-SPI IP not support 1-1-8/8-8-8 I/O
    //case MXIC_SNOR_FREAD_118 :  // 1-1-8 fast read command, MCU QSPI IP not support this mode         0x03, 0x7C
    //case MXIC_SNOR_FREAD_888 :  // 8-8-8 fast read Octal command, MCU QSPI IP not support this mode   0x7C, 0xEC, 0xEE
      ret = BSP_ERROR_WRONG_PARAM;
      break;

    case MXIC_SNOR_NREAD_111 :      // 1-1-1 normal read command with 0 dummy cycle             0x03, 0x13
    case MXIC_SNOR_FREAD_111 :      // 1-1-1 fast read command, Power on H/W default setting    0x0B, 0x0C, 0x0D, 0x0E
    case MXIC_SNOR_FREAD_112 :      // 1-1-2 fast read command                                  0x3B, 0x3C
    case MXIC_SNOR_FREAD_114 :      // 1-1-4 fast read command                                  0x6B, 0x6C
    case MXIC_SNOR_FREAD_122 :      // 1-2-2 fast read command                                  0xBB, 0xBC, 0xBD, 0xBE
    case MXIC_SNOR_FREAD_144 :      // 1-4-4 fast read command                                  0xEA, 0xEB, 0xEC, 0xED, 0xEE
    case MXIC_SNOR_FREAD_144_4DC :  // 1-4-4 fast read command with 4 dummy clock;              0xE7
      if((Init.IO == MXIC_SNOR_FREAD_444) || (Init.IO == MXIC_SNOR_FREAD_444_4DC))
      {
#ifdef SUPPORT_QPI
        if(MXIC_SNOR_EnterQPIMode(&QSPIHandle[Instance]) != MXIC_SNOR_ERROR_NONE)
        {
           ret = BSP_ERROR_COMPONENT_FAILURE;
        }
#else
        ret = BSP_ERROR_WRONG_PARAM;
#endif  // SUPPORT_QPI
      }
      break;

#ifdef SUPPORT_QPI
    case MXIC_SNOR_FREAD_444 :      // 4-4-4 fast read QPI command                              0xEA, 0xEB, 0xEC, 0xED, 0xEE
    case MXIC_SNOR_FREAD_444_4DC :  // 4-4-4 fast read QPI command with 4 dummy clock; 0xE7     0x0B, 0xE7
      if((Init.IO != MXIC_SNOR_FREAD_444) && (Init.IO != MXIC_SNOR_FREAD_444_4DC))
      {
        if(MXIC_SNOR_ExitQPIMode(&QSPIHandle[Instance]) != MXIC_SNOR_ERROR_NONE)
        {
          ret = BSP_ERROR_COMPONENT_FAILURE;
        }
      }
      break;
#endif  // SUPPORT_QPI
    }

    /* Update QSPI context if all operations are well done */
    if(ret == BSP_ERROR_NONE)
    {
      /* Update current status parameter *****************************************/
      QSPICtx[Instance].IsInitialized = QSPI_ACCESS_INDIRECT;
      QSPICtx[Instance].InterfaceMode = Init;
    }
  }
  /* Return BSP status */
  return ret;
}

/**
  * @brief  Get current Flash interface setting
  * @param  Instance  QSPI instance
  * @retval BSP status
  *         pInit     Flash I/O & transfer rate configuration
  */
int32_t BSP_QSPI_GetFlashInterface(uint32_t Instance, BSP_QSPI_Init_t *pInit)
{
  int32_t ret = BSP_ERROR_NONE;

  /* Check if the instance is supported */
  if(Instance >= QSPI_NOR_INSTANCE_NUMBER)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    *pInit = QSPICtx[Instance].InterfaceMode;
  }

  return ret;
}

/**
  * @brief  Configure the QSPI in memory-mapped mode
  *         Only 1 Instance can running MMP mode. And it will lock system at this mode.
  * @param  Instance  QSPI instance
  * @retval BSP status
  */
int32_t BSP_QSPI_EnableMemoryMappedMode(uint32_t Instance)
{
  int32_t ret = BSP_ERROR_NONE;

  /* Check if the instance is supported */
  if(Instance >= QSPI_NOR_INSTANCE_NUMBER)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
#ifdef SUPPORT_DTR
    if(QSPICtx[Instance].InterfaceMode.Rate == MXIC_SNOR_DTR)
    {
      // DTR read, Set sampling shift none
      QSPIHandle[Instance].Init.SampleShifting = QSPI_SAMPLE_SHIFTING_NONE;

      if(HAL_QSPI_Init(&QSPIHandle[Instance])!= HAL_OK)
      {
        ret = BSP_ERROR_PERIPH_FAILURE;
      }
      else if(MXIC_SNOR_EnableMemoryMappedModeDTR(&QSPIHandle[Instance], QSPICtx[Instance].InterfaceMode) != MXIC_SNOR_ERROR_NONE)
      {
        // MMP DTR read initial fail, Set sampling shift back to half cycle
        QSPIHandle[Instance].Init.SampleShifting = QSPI_SAMPLE_SHIFTING_HALFCYCLE;
        HAL_QSPI_Init(&QSPIHandle[Instance]);

        ret = BSP_ERROR_COMPONENT_FAILURE;
      }
    }
    else
#endif  // SUPPORT_DTR
    {
      if(MXIC_SNOR_EnableMemoryMappedModeSTR(&QSPIHandle[Instance], QSPICtx[Instance].InterfaceMode) != MXIC_SNOR_ERROR_NONE)
      {
        ret = BSP_ERROR_COMPONENT_FAILURE;
      }
    }

    /* Update QSPI context if all operations are well done */
    if(ret == BSP_ERROR_NONE)
    {
      QSPICtx[Instance].IsInitialized = QSPI_ACCESS_MMP;
    }
  }
  /* Return BSP status */
  return ret;
}

/**
  * @brief  Exit form memory-mapped mode
  *         Only 1 Instance can running MMP mode. And it will lock system at this mode.
  * @param  Instance  QSPI instance
  * @retval BSP status
  */
int32_t BSP_QSPI_DisableMemoryMappedMode(uint32_t Instance)
{
  int32_t ret = BSP_ERROR_NONE;

  /* Check if the instance is supported */
  if(Instance >= QSPI_NOR_INSTANCE_NUMBER)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    if(QSPICtx[Instance].IsInitialized != QSPI_ACCESS_MMP)
    {
      ret = BSP_ERROR_INSTANCE_MMP_UNLOCK_FAILURE;
    }/* Abort MMP back to indirect mode */
    else if(HAL_QSPI_Abort(&QSPIHandle[Instance]) != HAL_OK)
    {
      ret = BSP_ERROR_PERIPH_FAILURE;
    }
    else
    {
      QSPICtx[Instance].IsInitialized = QSPI_ACCESS_INDIRECT;

#ifdef SUPPORT_PERFORMANCE_ENHANCE_READ
      if(MXIC_SNOR_PerformanceEnhanceModeReset(&QSPIHandle[Instance]) != MXIC_SNOR_ERROR_NONE)
      {
        ret = BSP_ERROR_COMPONENT_FAILURE;
      }
#endif  // SUPPORT_PERFORMANCE_ENHANCE_READ

      // Force sampling shift back to half cycle, MMP DTR read finish needed
      QSPIHandle[Instance].Init.SampleShifting = QSPI_SAMPLE_SHIFTING_HALFCYCLE;

      if(HAL_QSPI_Init(&QSPIHandle[Instance])!= HAL_OK)
      {
        ret = BSP_ERROR_PERIPH_FAILURE;
      }
    }
  }
  /* Return BSP status */
  return ret;
}

/**************************************************************************************************
 *  Flash Operation Functions
 *************************************************************************************************/
/**
  * @brief  Get flash ID, 3 Byte
  *         Manufacturer ID, Memory type, Memory density
  * @param  Instance   QSPI instance
  * @retval BSP status
  *         pID        Flash ID;
  *                    pID[31:24] = 0
  *                    pID[23:16] = Memory density
  *                    pID[15:08] = Memory type
  *                    pID[07:00] = Manufacturer ID
  */
int32_t BSP_QSPI_ReadID(uint32_t Instance, uint32_t *pID)
{
  int32_t ret = BSP_ERROR_NONE;

  /* Check if the instance is supported */
  if(Instance >= QSPI_NOR_INSTANCE_NUMBER)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    MXIC_SNOR_CommandHandle_t CommandHandle;

    *pID = 0;               // Clear data
    CommandHandle.Mode    = QSPICtx[Instance].InterfaceMode;
    CommandHandle.pBuffer = (uint8_t *)pID;

#ifdef SUPPORT_QPI
    if((CommandHandle.Mode.IO == MXIC_SNOR_FREAD_444) || (CommandHandle.Mode.IO == MXIC_SNOR_FREAD_444_4DC))
    {
      if(MXIC_SNOR_QPIIDRead(&QSPIHandle[Instance], &CommandHandle) != MXIC_SNOR_ERROR_NONE)
      {
        ret = BSP_ERROR_COMPONENT_FAILURE;
      }
    }
    else
#endif  // SUPPORT_QPI
    {
      if(MXIC_SNOR_ReadID(&QSPIHandle[Instance], &CommandHandle) != MXIC_SNOR_ERROR_NONE)
      {
        ret = BSP_ERROR_COMPONENT_FAILURE;
      }
    }
  }
  /* Return Flash ID or BSP status if an error occurred */
  return ret;
}

/**
  * @brief  Reads an amount of data from the QSPI memory.
  * @param  Instance QSPI instance
  *         pData    Pointer to data to be read
  *         Address  Read start address
  *         Size     Read Size in Byte
  * @retval BSP status
  */
int32_t BSP_QSPI_Read(uint32_t Instance, uint8_t *pData, uint32_t Address, uint32_t Size)
{
  int32_t ret = BSP_ERROR_NONE;

  /* Check if the instance is supported */
  if(Instance >= QSPI_NOR_INSTANCE_NUMBER)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    MXIC_SNOR_CommandHandle_t CommandHandle;

    CommandHandle.Mode    = QSPICtx[Instance].InterfaceMode;
    CommandHandle.Address = Address;
    CommandHandle.Size    = Size;
    CommandHandle.pBuffer = pData;

#ifdef SUPPORT_DTR
    if(CommandHandle.Mode.Rate == MXIC_SNOR_DTR)
    {
      // DTR read, Set sampling shift none
      QSPIHandle[Instance].Init.SampleShifting = QSPI_SAMPLE_SHIFTING_NONE;

      if(HAL_QSPI_Init(&QSPIHandle[Instance])!= HAL_OK)
      {
        ret = BSP_ERROR_PERIPH_FAILURE;
      }
      else if(MXIC_SNOR_ReadDTR(&QSPIHandle[Instance], &CommandHandle) != MXIC_SNOR_ERROR_NONE)
      {
        ret = BSP_ERROR_COMPONENT_FAILURE;
      }

      // DTR read finish, Set sampling shift to half cycle for nonDTR command
      QSPIHandle[Instance].Init.SampleShifting = QSPI_SAMPLE_SHIFTING_HALFCYCLE;

      if(HAL_QSPI_Init(&QSPIHandle[Instance])!= HAL_OK)
      {
        ret = BSP_ERROR_PERIPH_FAILURE;
      }
    }
    else
#endif   // Support STR only
    if(MXIC_SNOR_ReadSTR(&QSPIHandle[Instance], &CommandHandle) != MXIC_SNOR_ERROR_NONE)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
  }
  /* Return BSP status */
  return ret;
}

/**
  * @brief  Writes an amount of data to the QSPI memory.
  * @param  Instance QSPI instance
  *         pData    Pointer to data to be writed
  *         Address  Write start address
  *         Size     Write Size in Byte
  * @retval BSP status
  */
int32_t BSP_QSPI_Write(uint32_t Instance, uint8_t *pData, uint32_t Address, uint32_t Size)
{
  int32_t ret = BSP_ERROR_NONE;

  /* Check if the instance is supported */
  if(Instance >= QSPI_NOR_INSTANCE_NUMBER)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    MXIC_SNOR_CommandHandle_t CommandHandle;
    uint32_t end_addr, current_size, current_addr;

    CommandHandle.Mode = QSPICtx[Instance].InterfaceMode;

    /* Initialize the address variables */
    current_addr = Address;
    end_addr     = Address + Size;

    /* Calculation of the size between the write address and the end of the page */
    current_size = MXIC_SNOR_PAGE_SIZE - (Address % MXIC_SNOR_PAGE_SIZE);

    /* Check if the size of the data is less than the remaining place in the page */
    if (current_size > Size)
    {
      current_size = Size;
    }

    /* Perform the write page by page */
    do
    {
      /* Enable write operations */
      if(MXIC_SNOR_WriteEnable(&QSPIHandle[Instance], CommandHandle.Mode) != MXIC_SNOR_ERROR_NONE)
      {
        ret = BSP_ERROR_COMPONENT_FAILURE;
      }
      else /* Issue page program command */
      {
        CommandHandle.Address = current_addr;
        CommandHandle.Size    = current_size;
        CommandHandle.pBuffer = pData;

        if(MXIC_SNOR_PageProgramSTR(&QSPIHandle[Instance], &CommandHandle) != MXIC_SNOR_ERROR_NONE)
        {
          ret = BSP_ERROR_COMPONENT_FAILURE;
        } /* Configure automatic polling mode to wait for end of program */
        else if(MXIC_SNOR_AutoPollingMemReady(&QSPIHandle[Instance], CommandHandle.Mode) != MXIC_SNOR_ERROR_NONE)
        {
          ret = BSP_ERROR_COMPONENT_FAILURE;
        }
      }

      /* Update the address and size variables for next page programming */
      current_addr += current_size;
      pData        += current_size;
      current_size  = ((current_addr + MXIC_SNOR_PAGE_SIZE) > end_addr) ? (end_addr - current_addr) : MXIC_SNOR_PAGE_SIZE;
    } while((current_addr < end_addr) && (ret == BSP_ERROR_NONE));
  }
  /* Return BSP status */
  return ret;
}

/**
  * @brief  Erases the specified address of block
  * @param  Instance  QSPI instance
  *         Address   Block address to erase
  *         Size      Erase Block size
  * @retval BSP status
  */
int32_t BSP_QSPI_EraseBlock(uint32_t Instance, uint32_t Address, BSP_QSPI_Erase_t Size)
{
  int32_t ret = BSP_ERROR_NONE;

  /* Check if the instance is supported */
  if(Instance >= QSPI_NOR_INSTANCE_NUMBER)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    MXIC_SNOR_CommandHandle_t CommandHandle;

    CommandHandle.Mode = QSPICtx[Instance].InterfaceMode;

    /* Enable write operations */
    if(MXIC_SNOR_WriteEnable(&QSPIHandle[Instance], CommandHandle.Mode) != MXIC_SNOR_ERROR_NONE)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
    else
    {
      /* Issue Block Erase command */
      CommandHandle.Address = Address;
      CommandHandle.Size    = Size;
      if(MXIC_SNOR_BlockErase(&QSPIHandle[Instance], &CommandHandle) != MXIC_SNOR_ERROR_NONE)
      {
        ret = BSP_ERROR_COMPONENT_FAILURE;
      }
      else
      {
        /* Wait Flash ready */
        uint8_t MatchMask[2] = {0, MXIC_SNOR_SR_WIP};

        CommandHandle.pBuffer          = MatchMask;
#ifdef MXIC_SNOR_ERASE_CHIP_CMD
        CommandHandle.Parameter.Type32 = (CommandHandle.Size == MXIC_SNOR_ERASE_CHIP) ? MXIC_SNOR_MAX_TIME_ERASE_CHIP : HAL_QPSI_TIMEOUT_DEFAULT_VALUE;
#else
        CommandHandle.Parameter.Type32 = HAL_QPSI_TIMEOUT_DEFAULT_VALUE;
#endif
        if(MXIC_SNOR_PollingStatusRegister(&QSPIHandle[Instance], &CommandHandle) != MXIC_SNOR_ERROR_NONE)
        {
          ret = BSP_ERROR_COMPONENT_FAILURE;
        }
      }
    }
  }
  /* Return BSP status */
  return ret;
}

#ifdef MXIC_SNOR_ERASE_CHIP_CMD
/**
  * @brief  Erases the entire QSPI memory.
  * @param  Instance  QSPI instance
  * @retval BSP status
  */
int32_t BSP_QSPI_EraseChip(uint32_t Instance)
{
  int32_t ret = BSP_ERROR_NONE;

  /* Check if the instance is supported */
  if(Instance >= QSPI_NOR_INSTANCE_NUMBER)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    MXIC_SNOR_CommandHandle_t CommandHandle;

    CommandHandle.Mode = QSPICtx[Instance].InterfaceMode;

    /* Enable write operations */
    if(MXIC_SNOR_WriteEnable(&QSPIHandle[Instance], CommandHandle.Mode) != MXIC_SNOR_ERROR_NONE)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    } /* Issue Chip Erase command */
    else if(MXIC_SNOR_ChipErase(&QSPIHandle[Instance], CommandHandle.Mode) != MXIC_SNOR_ERROR_NONE)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
    else
    {
      /* Wait Flash ready */
      uint8_t MatchMask[2] = {0, MXIC_SNOR_SR_WIP};

      CommandHandle.pBuffer          = MatchMask;
      CommandHandle.Parameter.Type32 = MXIC_SNOR_MAX_TIME_ERASE_CHIP;
      if(MXIC_SNOR_PollingStatusRegister(&QSPIHandle[Instance], &CommandHandle) != MXIC_SNOR_ERROR_NONE)
      {
        ret = BSP_ERROR_COMPONENT_FAILURE;
      }
    }
  }
  /* Return BSP status */
  return ret;
}
#endif  // MXIC_SNOR_ERASE_CHIP_CMD

/**
  * @brief  Read flash status register value
  * @param  Instance  QSPI instance
  * @param  pData     Pointer for retrieve status register value
  * @retval BSP status
  */
int32_t BSP_QSPI_ReadStatusRegister(uint32_t Instance, MXIC_SNOR_StatusRegister_t *pData)
{
  int32_t ret = BSP_ERROR_NONE;


  /* Check if the instance is supported */
  if(Instance >= QSPI_NOR_INSTANCE_NUMBER)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    MXIC_SNOR_CommandHandle_t CommandHandle;

    CommandHandle.Mode    = QSPICtx[Instance].InterfaceMode;
    CommandHandle.pBuffer = (uint8_t *)pData;
    if(MXIC_SNOR_ReadStatusRegister(&QSPIHandle[Instance], &CommandHandle) != MXIC_SNOR_ERROR_NONE)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
  }
  /* Return BSP status */
  return ret;
}

#ifdef MXIC_SNOR_WRITE_STATUS_CONFIGURATION_REG_CMD
/**
  * @brief  Write flash status register.
  * @param  Instance QSPI instance
  * @param  Data     Value for write to status register.
  * @retval BSP status
  */
int32_t BSP_QSPI_WriteStatusRegister(uint32_t Instance, MXIC_SNOR_StatusRegister_t Data)
{
  int32_t ret = BSP_ERROR_NONE;

  /* Check if the instance is supported */
  if(Instance >= QSPI_NOR_INSTANCE_NUMBER)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    MXIC_SNOR_CommandHandle_t CommandHandle;

    CommandHandle.Mode = QSPICtx[Instance].InterfaceMode;

    /* Enable write operations */
    if(MXIC_SNOR_WriteEnable(&QSPIHandle[Instance], CommandHandle.Mode) != MXIC_SNOR_ERROR_NONE)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
    else
    {
      /* Write Status Register */
      CommandHandle.Size    = sizeof(MXIC_SNOR_StatusRegister_t);
      CommandHandle.pBuffer = (uint8_t *)&Data;
      if(MXIC_SNOR_WriteStatusConfigurationRegister(&QSPIHandle[Instance], &CommandHandle) != MXIC_SNOR_ERROR_NONE)
      {
        ret = BSP_ERROR_COMPONENT_FAILURE;
      } /* Wait Flash ready */
      else if(MXIC_SNOR_AutoPollingMemReady(&QSPIHandle[Instance], CommandHandle.Mode) != MXIC_SNOR_ERROR_NONE)
      {
        ret = BSP_ERROR_COMPONENT_FAILURE;
      }
    }
  }
  /* Return BSP status */
  return ret;
}
#endif  // MXIC_SNOR_WRITE_STATUS_CONFIGURATION_REG_CMD

#ifdef MXIC_SNOR_READ_CONFIGURATION_REG_CMD
/**
  * @brief  Read flash configuration register value
  * @param  Instance  QSPI instance
  *         *pData    Pointer for retrieve Configuration register value
  *         Size      Data size in Byte (Some device support CR & CR2)
  * @retval BSP status
  */
int32_t BSP_QSPI_ReadConfigurationRegister(uint32_t Instance, uint8_t *pData, uint32_t Size)
{
  int32_t ret = BSP_ERROR_NONE;


  /* Check if the instance is supported */
  if(Instance >= QSPI_NOR_INSTANCE_NUMBER)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    MXIC_SNOR_CommandHandle_t CommandHandle;

    CommandHandle.Mode    = QSPICtx[Instance].InterfaceMode;
    CommandHandle.Size    = Size;
      CommandHandle.pBuffer = pData;
    if(MXIC_SNOR_ReadConfigurationRegister(&QSPIHandle[Instance], &CommandHandle) != MXIC_SNOR_ERROR_NONE)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
  }

  /* Return BSP status */
  return ret;
}

/**
  * @brief  Write configuration register.
  * @param  Instance  QSPI instance
  * @param  pData     CR, CR2 value for write to configuration register.
  *                   Some device support CR & CR2
  * @retval BSP status
  */
int32_t BSP_QSPI_WriteConfigurationRegister(uint32_t Instance, uint8_t *pData, uint32_t Size)
{
  int32_t ret = BSP_ERROR_NONE;

  /* Check if the instance is supported */
  if(Instance >= QSPI_NOR_INSTANCE_NUMBER)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    MXIC_SNOR_CommandHandle_t CommandHandle;

    CommandHandle.Mode    = QSPICtx[Instance].InterfaceMode;
    CommandHandle.Size    = Size;
    CommandHandle.pBuffer = pData;
    if(MXIC_SNOR_WriteConfigurationRegister(&QSPIHandle[Instance], &CommandHandle) != MXIC_SNOR_ERROR_NONE)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
  }
  /* Return BSP status */
  return ret;
}
#endif  // MXIC_SNOR_READ_CONFIGURATION_REG_CMD

#ifdef MXIC_SNOR_READ_CONFIGURATION_REG2_CMD
/**
  * @brief  RRead Configuration Register 2
  * @param  Instance  QSPI instance
  *         pData     Buffer pointer
  *         Address   CR2 address
  * @retval BSP status
  *         *pData = CR2 value
  */
int32_t BSP_QSPI_ReadConfigurationRegister2(uint32_t Instance, uint8_t *pData, uint32_t Address)
{
  int32_t ret = BSP_ERROR_NONE;

  /* Check if the instance is supported */
  if(Instance >= QSPI_NOR_INSTANCE_NUMBER)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    MXIC_SNOR_CommandHandle_t CommandHandle;

    CommandHandle.Mode    = QSPICtx[Instance].InterfaceMode;
    CommandHandle.Address = Address;
    CommandHandle.pBuffer = pData;
    if(MXIC_SNOR_ReadConfigurationRegister2(&QSPIHandle, &CommandHandle) != MXIC_SNOR_ERROR_NONE)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
  }
  /* Return BSP status */
  return ret;
}
#endif  // MXIC_SNOR_READ_CONFIGURATION_REG2_CMD

#ifdef  MXIC_SNOR_WRITE_CONFIGURATION_REG2_CMD
/**
  * @brief  Write Configuration Register 2
  * @param  Instance QSPI instance
  *         Data     Value for write to CR2
  *         Address  CR2 address
  * @retval BSP status
  */
int32_t BSP_QSPI_WriteConfigurationRegister2(uint32_t Instance, uint8_t Data, uint32_t Address)
{
  int32_t ret = BSP_ERROR_NONE;

  /* Check if the instance is supported */
  if(Instance >= QSPI_NOR_INSTANCE_NUMBER)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    MXIC_SNOR_CommandHandle_t CommandHandle;

    CommandHandle.Mode = QSPICtx[Instance].InterfaceMode;

    /* Enable write operations */
    if(MXIC_SNOR_WriteEnable(&QSPIHandle, CommandHandle.Mode) != MXIC_SNOR_ERROR_NONE)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
    else
    {
      /* Write Configuration Register 2 */
      CommandHandle.Address = Address;
      CommandHandle.pBuffer = &Data;
      if(MXIC_SNOR_WriteConfigurationRegister2(&QSPIHandle, &CommandHandle) != MXIC_SNOR_ERROR_NONE)
      {
        ret = BSP_ERROR_COMPONENT_FAILURE;
      } /* Wait Flash ready */
      else if(MXIC_SNOR_AutoPollingMemReady(&QSPIHandle, CommandHandle.Mode) != MXIC_SNOR_ERROR_NONE)
      {
        ret = BSP_ERROR_COMPONENT_FAILURE;
      }
    }
  }

  /* Return BSP status */
  return ret;
}
#endif  // MXIC_SNOR_WRITE_CONFIGURATION_REG2_CMD

/**
  * @brief  This function reset the QSPI Flash memory.
  *         Fore QPI+SPI reset to avoid system come from unknown status.
  *         Flash accept 1-1-1, 1-1-2, 1-2-2 commands after reset.
  * @param  Instance  QSPI instance
  * @retval BSP status
  */
int32_t BSP_QSPI_ResetMemory(int32_t Instance)
{
  int32_t ret = BSP_ERROR_NONE;

  /* Check if the instance is supported */
  if(Instance >= QSPI_NOR_INSTANCE_NUMBER)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
#ifdef MXIC_SNOR_SOFTWARE_RESET_CMD              // Software reset command
    if(MXIC_SNOR_SoftwareReset(&QSPIHandle[Instance]) != MXIC_SNOR_ERROR_NONE)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }

#elif defined(MXIC_SNOR_RESET_ENABLE_CMD)
    MXIC_SNOR_Mode_t Mode;

#ifdef SUPPORT_QPI      /* Send RESET ENABLE command in QPI mode (QUAD I/Os, 4-0-0) */
    Mode.IO   = MXIC_SNOR_FREAD_444;
    Mode.Rate = MXIC_SNOR_STR;

    if(MXIC_SNOR_ResetEnable(&QSPIHandle[Instance], Mode) != MXIC_SNOR_ERROR_NONE)    // Send Reset Enable by 4-0-0 command
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
    else if(MXIC_SNOR_ResetMemory(&QSPIHandle[Instance], Mode) != MXIC_SNOR_ERROR_NONE)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
#endif  // SUPPORT_QPI

    // 1-1-1 software reset
    if(ret == BSP_ERROR_NONE)
    {
      Mode.IO   = MXIC_SNOR_FREAD_111;
      Mode.Rate = MXIC_SNOR_STR;

      if(MXIC_SNOR_ResetEnable(&QSPIHandle[Instance], Mode) != MXIC_SNOR_ERROR_NONE)    // Send Reset Enable by 1-0-0 command
      {
        ret = BSP_ERROR_COMPONENT_FAILURE;
      }
      else if(MXIC_SNOR_ResetMemory(&QSPIHandle[Instance], Mode) != MXIC_SNOR_ERROR_NONE)
      {
        ret = BSP_ERROR_COMPONENT_FAILURE;
      }
    }

#else   // Hardware Reset
  /* Hardware Reset function place here */
#endif

    if(ret == BSP_ERROR_NONE)
    {
      QSPICtx[Instance].IsInitialized      = QSPI_ACCESS_INDIRECT;  // After reset S/W setting MCU to indirect access
      QSPICtx[Instance].InterfaceMode.IO   = MXIC_SNOR_FREAD_111;   // After reset H/W Flash back to SPI 1-1-1 mode by default
      QSPICtx[Instance].InterfaceMode.Rate = MXIC_SNOR_STR;         // After reset S/W Flash back to STR mode

      /* After S/W reset CMD, wait Reset Recovery time for 1ms. */
      /* The Reset Recovery time may be over 1ms, if reset issued during Flash operating. */
      HAL_Delay(1);
    }
  }
  /* Return BSP status */
  return ret;
}

/******************************************************************************
  * @addtogroup STM32F769I_DISCOVERY_QSPI_Private_Functions
  ****************************************************************************/
/**
  * @brief  Initializes the QSPI interface.
  * @param  hqspi          QSPI handle
  * @param  FlashSize      QSPI flash size
  * @param  ClockPrescaler Clock prescaler
  * @retval BSP status
  */
HAL_StatusTypeDef MX_QSPI_Init(QSPI_HandleTypeDef *hqspi, uint32_t FlashSize, uint32_t ClockPrescaler)
{
  hqspi->Init.ClockPrescaler     = ClockPrescaler;   /* QSPI SCLK freq = AHB MHz/(ClockPrescaler+1) */
  hqspi->Init.FifoThreshold      = 16;
  hqspi->Init.SampleShifting     = QSPI_SAMPLE_SHIFTING_NONE;
  hqspi->Init.FlashSize          = POSITION_VAL(FlashSize) - 1;
  hqspi->Init.ChipSelectHighTime = QSPI_CS_HIGH_TIME_4_CYCLE;
  hqspi->Init.ClockMode          = QSPI_CLOCK_MODE_0;
  hqspi->Init.FlashID            = QSPI_FLASH_ID_1;
  hqspi->Init.DualFlash          = QSPI_DUALFLASH_DISABLE;

  return HAL_QSPI_Init(hqspi);
}

/* Private functions ---------------------------------------------------------*/
#ifdef MXIC_SNOR_CR_ODS
/**
  * @brief  This function configure the Output Driver Strength on memory side.
  *         ODS bit located in Configuration Register[2:0] in general
  * @param  Instance  QSPI instance
  * @retval BSP status
  */
static int32_t QSPI_SetODS(int32_t Instance, uint8_t ODS)
{
  int32_t ret = BSP_ERROR_NONE;

  /* Check if the instance is supported */
  if(Instance >= QSPI_NOR_INSTANCE_NUMBER)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    MXIC_SNOR_CommandHandle_t CommandHandle;
    MXIC_SNOR_ConfigurationRegister_t CReg;

    CommandHandle.Mode    = QSPICtx[Instance].InterfaceMode;
    CommandHandle.Size    = 1;
    CommandHandle.pBuffer = (uint8_t *)&CReg;
    if(MXIC_SNOR_ReadConfigurationRegister(&QSPIHandle[Instance], &CommandHandle) != MXIC_SNOR_ERROR_NONE)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
    else
    {
      /* Set Output Strength of the QSPI memory as Flash data sheet define */
      CReg.ODS = ODS;
      if(MXIC_SNOR_WriteConfigurationRegister(&QSPIHandle[Instance], &CommandHandle) != MXIC_SNOR_ERROR_NONE)
      {
        ret = BSP_ERROR_COMPONENT_FAILURE;
      }
    }
  }
  /* Return BSP status */
  return ret;
}
#endif  // MXIC_SNOR_CR_ODS

#if defined(MXIC_SNOR_CR_DC) || defined(MXIC_SNOR_CR2_DC)
/**
  * @brief  This function configure the dummy cycles on memory side.
  *         MXIC_SNOR_CR_DC                 : Dummy cycle bit located in Configuration Register[7:6]
  *         SUPPORT_CONFIGURATION_REGISTER2 : Dummy cycle bit located in Configuration Register2 Address 0x00000300[2:0]
  * @param  Instance  OSPI_NOR instance
  *         DCIndex   Dummy Clock index
  * @retval BSP status
  */
static int32_t QSPI_DummyCyclesCfg(int32_t Instance, uint8_t DCIndex)
{
  int32_t ret = BSP_ERROR_NONE;

  /* Check if the instance is supported */
  if(Instance >= QSPI_NOR_INSTANCE_NUMBER)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
#ifdef MXIC_SNOR_CR_DC
    MXIC_SNOR_CommandHandle_t CommandHandle;
    MXIC_SNOR_ConfigurationRegister_t CReg;

    CommandHandle.Mode    = QSPICtx[Instance].InterfaceMode;
    CommandHandle.Size    = 1;
    CommandHandle.pBuffer = (uint8_t *)&CReg;
    if(MXIC_SNOR_ReadConfigurationRegister(&QSPIHandle[Instance], &CommandHandle) != MXIC_SNOR_ERROR_NONE)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
    else
    {
      /* Set Dummy Cycle Index of the QSPI memory as Flash data sheet define */
      CReg.DC = DCIndex;
      if(MXIC_SNOR_WriteConfigurationRegister(&QSPIHandle[Instance], &CommandHandle) != MXIC_SNOR_ERROR_NONE)
      {
        ret = BSP_ERROR_COMPONENT_FAILURE;
      }
    }
#else   // CR2
    if(BSP_QSPI_WriteConfigurationRegister2(Instance, DCIndex, CR2_00000300h) != BSP_ERROR_NONE)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
#endif  // MXIC_SNOR_CR_DC
  }
   /* Return BSP status */
  return ret;
}
#endif   // defined(MXIC_SNOR_CR_DC) || defined(MXIC_SNOR_CR2_DC)
/************************ (C) COPYRIGHT Macronix **************END OF FILE****/
