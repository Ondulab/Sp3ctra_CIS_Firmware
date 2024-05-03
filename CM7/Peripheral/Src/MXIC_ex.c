/*************************************************************************************************
 *          Macronix xSPI-NOR Flash BSP driver extension for STM32 QUADSPI IP
 * @file    stm32xxx_QUADSPI_MXIC_ex.c
 * @brief   This file contains command flow of the Flash specific command.
 * @Author  Peter Chang / Macronix; peterchang01@mxic.com.tw
 *************************************************************************************************
 * History :
 * 2023/12/20 Release V2.0.0
 *            Rename stm32f769i_discovery_qspi_MXIC_ex.h to stm32xxx_qspi_MXIC_ex.h
 *            Rename stm32f769i_discovery_qspi_MXIC_ex.c to stm32xxx_qspi_MXIC_ex.c
 *            Remove USE_AUTO_DC_CONFIGURATION define
 *            Alignment BSP input parameter between QUAD-SPI & OCTO-SPI IP
 *            Move BSP_QSPI_ReadConfigurationRegister2() & BSP_QSPI_WriteConfigurationRegister2()
 *            form stm32xxx_qspi_MXIC_ex.c to stm32xxx_qspi_MXIC.c
 * 2023/01/30 Release V1.2.0
 * 2022/12/21 Add BSP_QSPI_WritePacket() & BSP_QSPI_ReadPacket() commands
 * 2022/12/12 Release V1.1.0
 * 2022/12/08 Rename to stm32xxx_QUADSPI_MXIC_ex.c from stm32f769i_discovery_qspi_MXIC_ex.c
 * 2022/10/06 Move RPMC functions to stm32f769i_discovery_qspi_MXIC_ex.c
 * 2022/06/15 Release V1.0.0
 * 2022/05/04 Initial version.
 *************************************************************************************************/
/* Includes -------------------------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"
#include "MXIC.h"
#include "MXIC_ex.h"

#ifdef SUPPORT_RPMC
#include <string.h>
#include "hmac_sha2.h"
#endif


extern QSPI_HandleTypeDef QSPIHandle[];
extern QSPI_Ctx_t         QSPICtx[];

/* Private variables ---------------------------------------------------------*/
/* Private subroutine---------------------------------------------------------*/

/*******************************************************************************
 * Export Functions
 ******************************************************************************/
/* Secured OTP Memory Array Operations */
#ifdef MXIC_SNOR_ENTER_SECURED_OTP_CMD
/**
  * @brief  Enter Secured OTP mode
  * @param  Instance  QSPI instance
  * @retval BSP status
  */
int32_t BSP_QSPI_EnterSecuredOTP(uint32_t Instance)
{
  int32_t ret = BSP_ERROR_NONE;

  /* Check if the instance is supported */
  if(Instance >= QSPI_NOR_INSTANCE_NUMBER)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else if(MXIC_SNOR_EnterSecuredOTP(&QSPIHandle[Instance], QSPICtx[Instance].InterfaceMode) != MXIC_SNOR_ERROR_NONE)
  {
    ret = BSP_ERROR_COMPONENT_FAILURE;
  }
  /* Return BSP status */
  return ret;
}
#endif  // MXIC_SNOR_ENTER_SECURED_OTP_CMD

#ifdef MXIC_SNOR_EXIT_SECURED_OTP_CMD
/**
  * @brief  Exit Secured OTP mode
  * @param  Instance  QSPI instance
  * @retval BSP status
  */
int32_t BSP_QSPI_ExitSecuredOTP(uint32_t Instance)
{
  int32_t ret = BSP_ERROR_NONE;

  /* Check if the instance is supported */
  if(Instance >= QSPI_NOR_INSTANCE_NUMBER)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else if(MXIC_SNOR_ExitSecuredOTP(&QSPIHandle[Instance], QSPICtx[Instance].InterfaceMode) != MXIC_SNOR_ERROR_NONE)
  {
    ret = BSP_ERROR_COMPONENT_FAILURE;
  }
  /* Return BSP status */
  return ret;
}
#endif  // MXIC_SNOR_EXIT_SECURED_OTP_CMD

#ifdef MXIC_SNOR_WRITE_SECURITY_REGISTER_CMD
/**
  * @brief  Write Security Register. To set the "Lock-Down" bit as 1.
  * @param  Instance  QSPI instance
  * @retval BSP status
  */
int32_t BSP_QSPI_WriteSecurityRegister(uint32_t Instance)
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
    }/* Issue Write Security Register command */
    else if(MXIC_SNOR_WriteSecurityRegister(&QSPIHandle[Instance], CommandHandle.Mode) != MXIC_SNOR_ERROR_NONE)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }/* Configure automatic polling mode to wait for end of write */
    else if(MXIC_SNOR_AutoPollingMemReady(&QSPIHandle[Instance], CommandHandle.Mode) != MXIC_SNOR_ERROR_NONE)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
  }
  /* Return BSP status */
  return ret;
}
#endif  // MXIC_SNOR_WRITE_SECURITY_REGISTER_CMD

#ifdef MXIC_SNOR_ENTER_4K_BIT_MODE_CMD
/**
  * @brief  Enter 4K bit Mode (Additional memory)
  * @param  Instance  QSPI instance
  * @retval BSP status
  */
int32_t BSP_QSPI_Enter4KbitMode(uint32_t Instance)
{
  int32_t ret = BSP_ERROR_NONE;

  /* Check if the instance is supported */
  if(Instance >= QSPI_NOR_INSTANCE_NUMBER)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else if(MXIC_SNOR_Enter4KbitMode(&QSPIHandle[Instance]) != MXIC_SNOR_ERROR_NONE)
  {
    ret = BSP_ERROR_COMPONENT_FAILURE;
  }
  /* Return BSP status */
  return ret;
}
#endif  // MXIC_SNOR_ENTER_4K_BIT_MODE_CMD

#ifdef MXIC_SNOR_EXIT_4K_BIT_MODE_CMD
/**
  * @brief  Exit 4K bit Mode (Additional memory)
  * @param  Instance  QSPI instance
  * @retval BSP status
  */
int32_t BSP_QSPI_Exit4KbitMode(uint32_t Instance)
{
  int32_t ret = BSP_ERROR_NONE;

  /* Check if the instance is supported */
  if(Instance >= QSPI_NOR_INSTANCE_NUMBER)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else if(MXIC_SNOR_Exit4KbitMode(&QSPIHandle[Instance]) != MXIC_SNOR_ERROR_NONE)
  {
    ret = BSP_ERROR_COMPONENT_FAILURE;
  }
  /* Return BSP status */
  return ret;
}
#endif  // MXIC_SNOR_EXIT_4K_BIT_MODE_CMD

#ifdef MXIC_SNOR_ENTER_PARALLEL_MODE_CMD
/**
  * @brief  Enter Parallel Mode, Flash accept 1-1-8 command
  * @param  Instance  QSPI instance
  * @retval BSP status
  */
int32_t BSP_QSPI_EnterParallelMode(uint32_t Instance)
{
  int32_t ret = BSP_ERROR_NONE;

  /* Check if the instance is supported */
  if(Instance >= QSPI_NOR_INSTANCE_NUMBER)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else if(MXIC_SNOR_EnterParallelMode(&QSPIHandle[Instance]) != MXIC_SNOR_ERROR_NONE)
  {
    ret = BSP_ERROR_COMPONENT_FAILURE;
  }
  /* Return BSP status */
  return ret;
}
#endif  // MXIC_SNOR_ENTER_PARALLEL_MODE_CMD

#ifdef MXIC_SNOR_EXIT_PARALLEL_MODE_CMD
/**
  * @brief  Exit Parallel Mode
  * @param  Instance  QSPI instance
  * @retval BSP status
  */
int32_t BSP_QSPI_ExitParallelMode(uint32_t Instance)
{
  int32_t ret = BSP_ERROR_NONE;

  /* Check if the instance is supported */
  if(Instance >= QSPI_NOR_INSTANCE_NUMBER)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else if(MXIC_SNOR_ExitParallelMode(&QSPIHandle[Instance]) != MXIC_SNOR_ERROR_NONE)
  {
    ret = BSP_ERROR_COMPONENT_FAILURE;
  }
  /* Return BSP status */
  return ret;
}
#endif  // MXIC_SNOR_EXIT_PARALLEL_MODE_CMD

#ifdef MXIC_SNOR_ENTER_SECURITY_FIELD_CMD
/**
  * @brief  Enter Security Field
  * @param  Instance  QSPI instance
  * @retval BSP status
  */
int32_t BSP_QSPI_EnterSecurityField(uint32_t Instance)
{
  int32_t ret = BSP_ERROR_NONE;

  /* Check if the instance is supported */
  if(Instance >= QSPI_NOR_INSTANCE_NUMBER)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else if(MXIC_SNOR_EnterSecurityField(&QSPIHandle[Instance]) != MXIC_SNOR_ERROR_NONE)
  {
    ret = BSP_ERROR_COMPONENT_FAILURE;
  }
  /* Return BSP status */
  return ret;
}
#endif  // MXIC_SNOR_ENTER_SECURITY_FIELD_CMD

#ifdef MXIC_SNOR_EXIT_SECURITY_FIELD_CMD
/**
  * @brief  Enter Security Field
  * @param  Instance  QSPI instance
  * @retval BSP status
  */
int32_t BSP_QSPI_ExitSecurityField(uint32_t Instance)
{
  int32_t ret = BSP_ERROR_NONE;

  /* Check if the instance is supported */
  if(Instance >= QSPI_NOR_INSTANCE_NUMBER)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else if(MXIC_SNOR_ExitSecurityField(&QSPIHandle[Instance]) != MXIC_SNOR_ERROR_NONE)
  {
    ret = BSP_ERROR_COMPONENT_FAILURE;
  }
  /* Return BSP status */
  return ret;
}
#endif  // MXIC_SNOR_EXIT_SECURITY_FIELD_CMD

#ifdef SUPPORT_SECURITY_FIELD_COMMAND
/**
  * @brief  Reset Packet buffer address pointer
  * @param  Instance  QSPI instance
  * @retval BSP status
  */
static int32_t QSPI_PacketBufferAddressReset(uint32_t Instance)
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
    if(MXIC_SNOR_WriteEnable(&QSPIHandle[Instance], &CommandHandle) != MXIC_SNOR_ERROR_NONE)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
    else
    {
      CommandHandle.Address = MXIC_SNOR_PACKET_BUFFER_ADDRESS_RESET;
      CommandHandle.Size    = 0;
      if(MXIC_SNOR_WritePacket(&QSPIHandle[Instance], &CommandHandle) != MXIC_SNOR_ERROR_NONE)
      {
        ret = BSP_ERROR_COMPONENT_FAILURE;
      }
    }
  }
  /* Return BSP status */
  return ret;
}

/**
  * @brief  Write a Packet Command to Security Flash
  * @param  Instance  QSPI instance
  * @retval BSP status
  */
int32_t BSP_QSPI_WritePacket(uint32_t Instance, uint8_t *pData, uint32_t Size)
{
  int32_t ret = BSP_ERROR_NONE;

  /* Check if the instance is supported */
  if(Instance >= QSPI_NOR_INSTANCE_NUMBER)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    if(QSPI_PacketBufferAddressReset(Instance) != BSP_ERROR_NONE)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
    else
    {
      MXIC_SNOR_CommandHandle_t CommandHandle;

      CommandHandle.Mode = QSPICtx[Instance].InterfaceMode;

      /* Enable write operations */
      if(MXIC_SNOR_WriteEnable(&QSPIHandle[Instance], &CommandHandle) != MXIC_SNOR_ERROR_NONE)
      {
        ret = BSP_ERROR_COMPONENT_FAILURE;
      }
      else
      {
        CommandHandle.Address = MXIC_SNOR_PACKET_BUFFER_ADDRESS;
        CommandHandle.Size    = Size;
        CommandHandle.pBuffer = pData;
        if(MXIC_SNOR_WritePacket(&QSPIHandle[Instance], &CommandHandle) != MXIC_SNOR_ERROR_NONE)
        {
          ret = BSP_ERROR_COMPONENT_FAILURE;
        } /* Configure automatic polling mode to wait for end of process */
        else if(MXIC_SNOR_AutoPollingMemReady(&QSPIHandle[Instance], CommandHandle.Mode) != MXIC_SNOR_ERROR_NONE)
        {
          ret = BSP_ERROR_COMPONENT_FAILURE;
        }
      }
    }
  }
  /* Return BSP status */
  return ret;
}

/**
  * @brief  Read response data from Security Flash
  * @param  Instance  QSPI instance
  * @retval BSP status
  */
int32_t BSP_QSPI_ReadPacket(uint32_t Instance, uint8_t *pData, uint32_t Size)
{
  int32_t ret = BSP_ERROR_NONE;


  /* Check if the instance is supported */
  if(Instance >= QSPI_NOR_INSTANCE_NUMBER)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    if(QSPI_PacketBufferAddressReset(Instance) != BSP_ERROR_NONE)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
    else
    {
      MXIC_SNOR_CommandHandle_t CommandHandle;

      CommandHandle.Mode    = QSPICtx[Instance].InterfaceMode;
      //CommandHandle.Address = MXIC_SNOR_PACKET_BUFFER_ADDRESS;
      CommandHandle.Size    = Size;
      CommandHandle.pBuffer = pData;
      if(MXIC_SNOR_ReadPacket(&QSPIHandle[Instance], &CommandHandle) != MXIC_SNOR_ERROR_NONE)
      {
        ret = BSP_ERROR_COMPONENT_FAILURE;
      }
    }
  }
  /* Return BSP status */
  return ret;
}
#endif  // SUPPORT_SECURITY_FIELD_COMMAND

#ifdef MXIC_SNOR_READ_SFDP_CMD
/**
  * @brief  Reads an amount of SFDP data from the QSPI memory.
  * @param  pData:   Buffer pointer
  *         Address: Read start address
  *         Size:    Size of data to read in Byte
  * @retval BSP status
  */
int32_t BSP_QSPI_ReadSFDP(uint32_t Instance, uint8_t *pData, uint32_t Address, uint32_t Size)
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
    CommandHandle.Address = Address;
    CommandHandle.Size    = Size;
    CommandHandle.pBuffer = pData;
    if(MXIC_SNOR_ReadSFDP(&QSPIHandle[Instance], &CommandHandle) != MXIC_SNOR_ERROR_NONE)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
  }
  /* Return BSP status */
  return ret;
}
#endif  // MXIC_SNOR_READ_SFDP_CMD

/* SPB/DPB/Permanent Lock Operations; Advanced Sector Protection */
#ifdef MXIC_SNOR_WRITE_PROTECT_SELECTION_CMD
/**
  * @brief  Enter and enable individual block protect mode
  * @param  Instance  QSPI instance
  * @retval BSP status
  */
int32_t BSP_QSPI_EnableIndividualBlockProtectMode(uint32_t Instance)
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
    MXIC_SNOR_SecurityRegister_t SCUReg;

    /* Read WPSEL flag, check if Flash in individual block protect mode already */
    CommandHandle.Mode    = QSPICtx[Instance].InterfaceMode;
    CommandHandle.pBuffer = (uint8_t *)&SCUReg;
    if(MXIC_SNOR_ReadSecurityRegister(&QSPIHandle[Instance], &CommandHandle) != MXIC_SNOR_ERROR_NONE)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
    else if(SCUReg.WPSEL == 0)
    {
      if(MXIC_SNOR_WriteEnable(&QSPIHandle[Instance], CommandHandle.Mode) != MXIC_SNOR_ERROR_NONE)
      {
        ret = BSP_ERROR_COMPONENT_FAILURE;
      }
      else if(MXIC_SNOR_WriteProtectSelection(&QSPIHandle[Instance], CommandHandle.Mode) != MXIC_SNOR_ERROR_NONE)
      {
        ret = BSP_ERROR_COMPONENT_FAILURE;
      } /* Configure automatic polling mode to wait for end of program */
      else if(MXIC_SNOR_AutoPollingMemReady(&QSPIHandle[Instance], CommandHandle.Mode) != MXIC_SNOR_ERROR_NONE)
      {
        ret = BSP_ERROR_COMPONENT_FAILURE;
      } /* Check WPSEL flag, Read again  */
      else
      {
        CommandHandle.pBuffer = (uint8_t *)&SCUReg;
        if(MXIC_SNOR_ReadSecurityRegister(&QSPIHandle[Instance], &CommandHandle) != MXIC_SNOR_ERROR_NONE)
        {
          ret = BSP_ERROR_COMPONENT_FAILURE;
        } /* If set WPSEL flag success ? */
        else if(SCUReg.WPSEL == 0)
        {
          ret = BSP_ERROR_COMPONENT_FAILURE;
        }
      }
    }
  }
  /* Return BSP status */
  return ret;
}
#endif  // MXIC_SNOR_WRITE_PROTECT_SELECTION_CMD

#ifdef MXIC_SNOR_GANG_BLOCK_LOCK_CMD
/**
  * @brief  Gang block lock. Whole chip write protect.
  * @param  Instance  QSPI instance
  * @retval BSP status
  */
int32_t BSP_QSPI_GangBlockLock(uint32_t Instance)
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
    } /* Issue Gang Block Lock command */
    else if(MXIC_SNOR_GangBlockLock(&QSPIHandle[Instance], CommandHandle.Mode) != MXIC_SNOR_ERROR_NONE)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    } /* Configure automatic polling mode to wait for end of program */
    else if(MXIC_SNOR_AutoPollingMemReady(&QSPIHandle[Instance], CommandHandle.Mode) != MXIC_SNOR_ERROR_NONE)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
  }
  /* Return BSP status */
  return ret;
}
#endif  // MXIC_SNOR_GANG_BLOCK_LOCK_CMD

#ifdef MXIC_SNOR_GANG_BLOCK_UNLOCK_CMD
/**
  * @brief  Gang block unlock. Whole chip unprotect.
  * @param  Instance  QSPI instance
  * @retval BSP status
  */
int32_t BSP_QSPI_GangBlockUnlock(uint32_t Instance)
{
  int32_t ret = BSP_ERROR_NONE;

  /* Check if the instance is supported */
  if(Instance >= QSPI_NOR_INSTANCE_NUMBER)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  {
    MXIC_SNOR_CommandHandle_t CommandHandle;

    CommandHandle.Mode = QSPICtx[Instance].InterfaceMode;

    /* Enable write operations */
    if(MXIC_SNOR_WriteEnable(&QSPIHandle[Instance], CommandHandle.Mode) != MXIC_SNOR_ERROR_NONE)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    } /* Issue Gang Block Lock command */
    else if(MXIC_SNOR_GangBlockUnlock(&QSPIHandle[Instance], CommandHandle.Mode) != MXIC_SNOR_ERROR_NONE)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    } /* Configure automatic polling mode to wait for end of program */
    else if(MXIC_SNOR_AutoPollingMemReady(&QSPIHandle[Instance], CommandHandle.Mode) != MXIC_SNOR_ERROR_NONE)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
  }
  /* Return BSP status */
  return ret;
}
#endif  // MXIC_SNOR_GANG_BLOCK_UNLOCK_CMD

#ifdef MXIC_SNOR_SINGLE_BLOCK_LOCK_CMD
/**
  * @brief  Single Block Lock
  * @param  Instance  QSPI instance
  *         Address : Block address
  * @retval BSP status
  */
int32_t BSP_QSPI_SingleBlockLock(uint32_t Instance, uint32_t Address)
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
    MXIC_SNOR_SecurityRegister_t SCUReg;

    /* Read WPSEL flag, check if Flash in individual block protect mode already */
    CommandHandle.Mode    = QSPICtx[Instance].InterfaceMode;
    CommandHandle.pBuffer = (uint8_t *)&SCUReg;
    if(MXIC_SNOR_ReadSecurityRegister(&QSPIHandle[Instance], &CommandHandle) != MXIC_SNOR_ERROR_NONE)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }/* Check if device in individual block protect mode */
    else if(SCUReg.WPSEL == 0)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
    else
    {
      if(MXIC_SNOR_WriteEnable(&QSPIHandle[Instance], CommandHandle.Mode) != MXIC_SNOR_ERROR_NONE)
      {
        ret = BSP_ERROR_COMPONENT_FAILURE;
      }
      else
      {
        CommandHandle.Address = Address;
        if(MXIC_SNOR_SingleBlockLock(&QSPIHandle[Instance], &CommandHandle) != MXIC_SNOR_ERROR_NONE)
        {
          ret = BSP_ERROR_COMPONENT_FAILURE;
        } /* Configure automatic polling mode to wait for end of program */
        else if(MXIC_SNOR_AutoPollingMemReady(&QSPIHandle[Instance], CommandHandle.Mode) != MXIC_SNOR_ERROR_NONE)
        {
          ret = BSP_ERROR_COMPONENT_FAILURE;
        }
        else
        {
          CommandHandle.Address = Address;
          CommandHandle.pBuffer = CommandHandle.Parameter.Type8;
          if(MXIC_SNOR_ReadBlockLockStatus(&QSPIHandle[Instance], &CommandHandle) != MXIC_SNOR_ERROR_NONE)
          {
            ret = BSP_ERROR_COMPONENT_FAILURE;
          }
          else if(CommandHandle.Parameter.Type8[0] != MXIC_SNOR_BLOCK_LOCKED)
          {
            ret = BSP_ERROR_LOCK_FAILURE;
          }
        }
      }
    }
  }
  /* Return BSP status */
  return ret;
}
#endif  // MXIC_SNOR_SINGLE_BLOCK_LOCK_CMD

#ifdef MXIC_SNOR_SINGLE_BLOCK_UNLOCK_CMD
/**
  * @brief  Single Block Un-Lock
  * @param  Instance  QSPI instance
  *         Address : Block address
  * @retval BSP status
  */
int32_t BSP_QSPI_SingleBlockUnLock(uint32_t Instance, uint32_t Address)
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
    MXIC_SNOR_SecurityRegister_t SCUReg;

    /* Read WPSEL flag, check if Flash in individual block protect mode already */
    CommandHandle.Mode    = QSPICtx[Instance].InterfaceMode;
    CommandHandle.pBuffer = (uint8_t *)&SCUReg;
    if(MXIC_SNOR_ReadSecurityRegister(&QSPIHandle[Instance], &CommandHandle) != MXIC_SNOR_ERROR_NONE)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }/* Check if device in individual block protect mode */
    else if(SCUReg.WPSEL == 0)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
    else
    {
      if(MXIC_SNOR_WriteEnable(&QSPIHandle[Instance], CommandHandle.Mode) != MXIC_SNOR_ERROR_NONE)
      {
        ret = BSP_ERROR_COMPONENT_FAILURE;
      }
      else
      {
        CommandHandle.Address = Address;
        if(MXIC_SNOR_SingleBlockUnLock(&QSPIHandle[Instance], &CommandHandle) != MXIC_SNOR_ERROR_NONE)
        {
          return BSP_ERROR_COMPONENT_FAILURE;
        } /* Configure automatic polling mode to wait for end of program */
        else if(MXIC_SNOR_AutoPollingMemReady(&QSPIHandle[Instance], CommandHandle.Mode) != MXIC_SNOR_ERROR_NONE)
        {
          return BSP_ERROR_COMPONENT_FAILURE;
        }

#if defined(MXIC_SNOR_READ_PERMANENT_LOCK_STATUS_CMD) || defined(MXIC_SNOR_READ_PERMANENT_LOCK_STATUS_8DC_CMD)
        /* Check permanent lock status*/
        CommandHandle.Address = Address;
        CommandHandle.pBuffer = CommandHandle.Parameter.Type8;
        if(MXIC_SNOR_ReadBlockPermanentLockStatus(&QSPIHandle[Instance], &CommandHandle) != MXIC_SNOR_ERROR_NONE)
        {
          return BSP_ERROR_COMPONENT_FAILURE;
        }
        else if(CommandHandle.Parameter.Type8[0] == MXIC_SNOR_BLOCK_LOCKED)
        {
          return BSP_ERROR_UNLOCK_FAILURE;
        }
#endif  // defined(MXIC_SNOR_READ_PERMANENT_LOCK_STATUS_CMD) || defined(MXIC_SNOR_READ_PERMANENT_LOCK_STATUS_8DC_CMD)

        /* Check block lock status*/
        CommandHandle.Address = Address;
        CommandHandle.pBuffer = CommandHandle.Parameter.Type8;
        if(MXIC_SNOR_ReadBlockLockStatus(&QSPIHandle[Instance], &CommandHandle) != MXIC_SNOR_ERROR_NONE)
        {
          ret = BSP_ERROR_COMPONENT_FAILURE;
        }
        else if(CommandHandle.Parameter.Type8[0] == MXIC_SNOR_BLOCK_LOCKED)
        {
          ret = BSP_ERROR_UNLOCK_FAILURE;
        }
      }
    }
  }
  /* Return BSP status */
  return ret;
}
#endif  // MXIC_SNOR_SINGLE_BLOCK_UNLOCK_CMD

#ifdef MXIC_SNOR_READ_BLOCK_LOCK_STATUS_CMD
/**
  * @brief  Read Block Lock Status
  * @param  Instance  QSPI instance
  *         Address : Block address
  * @retval BSP status
  */
int32_t BSP_QSPI_ReadBlockLockStatus(uint32_t Instance, MXIC_SNOR_BlockLockTypeDef *pData, uint32_t Address)
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
    if(MXIC_SNOR_ReadBlockLockStatus(&QSPIHandle[Instance], &CommandHandle) != MXIC_SNOR_ERROR_NONE)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
  }
  /* Return BSP status */
  return ret;
}
#endif  // MXIC_SNOR_READ_BLOCK_LOCK_STATUS_CMD

#ifdef MXIC_SNOR_READ_DPB_REGISTER_CMD
/**
  * @brief  Read DPB register, 1 Byte. SPI only
  * @param  Instance  QSPI instance
  * @retval BSP status
  */
int32_t BSP_QSPI_ReadDPBRegister(uint32_t Instance, MXIC_SNOR_DPBRegisterTypeDef *pData, uint32_t Address)
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

    /* Issue Read DPB Register command */
    CommandHandle.Mode    = QSPICtx[Instance].InterfaceMode;
    CommandHandle.Address = Address;
    CommandHandle.pBuffer = pData;
    if(MXIC_SNOR_ReadDPBRegister(&QSPIHandle[Instance], &CommandHandle) != MXIC_SNOR_ERROR_NONE)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
  }
  /* Return BSP status */
  return ret;
}
#endif  // MXIC_SNOR_READ_DPB_REGISTER_CMD

#ifdef MXIC_SNOR_WRITE_DPB_REGISTER_CMD
/**
  * @brief  Write DPB register. SPI only
  * @param  Instance  QSPI instance
  * @retval BSP status
  */
int32_t BSP_QSPI_WriteDPBRegister(uint32_t Instance, MXIC_SNOR_DPBRegisterTypeDef Data, uint32_t Address)
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

    /* Enable write operations */
    CommandHandle.Mode = QSPICtx[Instance].InterfaceMode;
    if(MXIC_SNOR_WriteEnable(&QSPIHandle[Instance], CommandHandle.Mode) != MXIC_SNOR_ERROR_NONE)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
    else /* Issue Write DPB Register command */
    {
      CommandHandle.Address = Address;
      CommandHandle.pBuffer = &Data;
      if(MXIC_SNOR_WriteDPBRegister(&QSPIHandle[Instance], &CommandHandle) != MXIC_SNOR_ERROR_NONE)
      {
        ret = BSP_ERROR_COMPONENT_FAILURE;
      }/* Configure automatic polling mode to wait for command end */
      else if(MXIC_SNOR_AutoPollingMemReady(&QSPIHandle[Instance], CommandHandle.Mode) != MXIC_SNOR_ERROR_NONE)
      {
        ret = BSP_ERROR_COMPONENT_FAILURE;
      }
    }
  }
  /* Return BSP status */
  return ret;
}
#endif  // MXIC_SNOR_WRITE_DPB_REGISTER_CMD

#ifdef MXIC_SNOR_READ_SPB_STATUS_CMD
/**
  * @brief  Read SPB status. SPI only
  * @param  Instance  QSPI instance
  * @retval BSP status
  */
int32_t BSP_QSPI_ReadSPBStatus(uint32_t Instance, MXIC_SNOR_SPBRegisterTypeDef *pData, uint32_t Address)
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

    /* Issue Read SPB Status Register command */
    CommandHandle.Mode    = QSPICtx[Instance].InterfaceMode;
    CommandHandle.Address = Address;
    CommandHandle.pBuffer = pData;
    if(MXIC_SNOR_ReadSPBStatus(&QSPIHandle[Instance], &CommandHandle) != MXIC_SNOR_ERROR_NONE)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
  }
  /* Return BSP status */
  return ret;
}
#endif  // MXIC_SNOR_READ_SPB_STATUS_CMD

#ifdef MXIC_SNOR_WRITE_SPB_CMD
/**
  * @brief  Write SPB. SPB bit program. SPI only
  * @param  Instance  QSPI instance
  * @retval BSP status
  */
int32_t BSP_QSPI_WriteSPB(uint32_t Instance, uint32_t Address)
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
      /* Issue Write SPB command */
      CommandHandle.Address = Address;
      if(MXIC_SNOR_WriteSPB(&QSPIHandle[Instance], &CommandHandle) != MXIC_SNOR_ERROR_NONE)
      {
        ret = BSP_ERROR_COMPONENT_FAILURE;
      }/* Configure automatic polling mode to wait for command end */
      else if(MXIC_SNOR_AutoPollingMemReady(&QSPIHandle[Instance], CommandHandle.Mode) != MXIC_SNOR_ERROR_NONE)
      {
        ret = BSP_ERROR_COMPONENT_FAILURE;
      }
    }
  }
  /* Return BSP status */
  return ret;
}
#endif  // MXIC_SNOR_WRITE_SPB_CMD

#ifdef MXIC_SNOR_ERASE_SPB_CMD
/**
  * @brief  Erase SPB (ESSPB), All SPB bit erase. SPI only
  * @param  Instance  QSPI instance
  * @retval BSP status
  */
int32_t BSP_QSPI_EraseSPB(uint32_t Instance)
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
    else if(MXIC_SNOR_EraseSPB(&QSPIHandle[Instance], CommandHandle.Mode) != MXIC_SNOR_ERROR_NONE)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }/* Configure automatic polling mode to wait for command end */
    else if(MXIC_SNOR_AutoPollingMemReady(&QSPIHandle[Instance], CommandHandle.Mode) != MXIC_SNOR_ERROR_NONE)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
  }
  /* Return BSP status */
  return ret;
}
#endif  // MXIC_SNOR_ERASE_SPB_CMD

/* Lock Register Operations */
#ifdef MXIC_SNOR_READ_LOCK_REGISTER_CMD
/**
  * @brief  Read Lock Register value. 1/2 Byte
  * @param  Instance  QSPI instance
  * @retval BSP status
  */
int32_t BSP_QSPI_ReadLockRegister(uint32_t Instance, MXIC_SNOR_LockRegister_t *pData)
{
  int32_t ret = BSP_ERROR_NONE;

  /* Check if the instance is supported */
  if(Instance >= QSPI_NOR_INSTANCE_NUMBER)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }/* Issue Read Lock Register command */
  else
  {
    MXIC_SNOR_CommandHandle_t CommandHandle;

    CommandHandle.Mode    = QSPICtx[Instance].InterfaceMode;
    CommandHandle.pBuffer = (uint8_t *)pData;
    if(MXIC_SNOR_ReadLockRegister(&QSPIHandle[Instance], &CommandHandle) != MXIC_SNOR_ERROR_NONE)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
  }
  /* Return BSP status */
  return ret;
}
#endif  // MXIC_SNOR_READ_LOCK_REGISTER_CMD

#ifdef MXIC_SNOR_WRITE_LOCK_REGISTER_CMD
/**
  * @brief  Write lock register. 1/2 Byte
  * @param  Instance  QSPI instance
  * @retval BSP status
  */
int32_t BSP_QSPI_WriteLockRegister(uint32_t Instance, MXIC_SNOR_LockRegister_t Data)
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
      CommandHandle.pBuffer = (uint8_t *)&Data;
      if(MXIC_SNOR_WriteLockRegister(&QSPIHandle[Instance], &CommandHandle) != MXIC_SNOR_ERROR_NONE)
      {
        ret = BSP_ERROR_COMPONENT_FAILURE;
      }/* Configure automatic polling mode to wait for command end */
      else if(MXIC_SNOR_AutoPollingMemReady(&QSPIHandle[Instance], CommandHandle.Mode) != MXIC_SNOR_ERROR_NONE)
      {
        ret = BSP_ERROR_COMPONENT_FAILURE;
      }
    }
  }
  /* Return BSP status */
  return ret;
}
#endif  // MXIC_SNOR_WRITE_LOCK_REGISTER_CMD

#ifdef MXIC_SNOR_SPB_LOCK_SET_CMD
/**
  * @brief  SPB lock set. SPI only
  * @param  Instance  QSPI instance
  * @retval BSP status
  */
int32_t BSP_QSPI_SPBLockSet(uint32_t Instance)
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
    }/* Issue SPB Lock Set command */
    else if(MXIC_SNOR_SPBLockSet(&QSPIHandle[Instance]) != MXIC_SNOR_ERROR_NONE)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }/* Configure automatic polling mode to wait for command end */
    else if(MXIC_SNOR_AutoPollingMemReady(&QSPIHandle[Instance], CommandHandle.Mode) != MXIC_SNOR_ERROR_NONE)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
  }
  /* Return BSP status */
  return ret;
}
#endif  // MXIC_SNOR_SPB_LOCK_SET_CMD

#ifdef MXIC_SNOR_READ_SPB_LOCK_REGISTER_CMD
/**
  * @brief  Read SPB lock register value. SPI only
  * @param  Instance  QSPI instance
  * @retval BSP status
  */
int32_t BSP_QSPI_ReadSPBLockRegister(uint32_t Instance, MXIC_SNOR_SPBLockRegister_t *pData)
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
    if(MXIC_SNOR_ReadSPBLockRegister(&QSPIHandle[Instance], &CommandHandle) != MXIC_SNOR_ERROR_NONE)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
  }
  /* Return BSP status */
  return ret;
}
#endif  // MXIC_SNOR_READ_SPB_LOCK_REGISTER_CMD

#ifdef MXIC_SNOR_WRITE_READ_LOCK_CONFIGURATION_CMD
/**
  * @brief  Write Read-Lock Area Configuration Register
  * @param  Instance  QSPI instance
  *         Data      MXIC_SNOR_ReadLockRegister_t data
  * @retval BSP status
  */
int32_t BSP_QSPI_WriteReadLockAreaConfigurationRegister(uint32_t Instance, MXIC_SNOR_ReadLockRegister_t Data)
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
    if(MXIC_SNOR_WriteEnable(&QSPIHandle[Instance], &CommandHandle) != MXIC_SNOR_ERROR_NONE)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
    else /* Issue Write DPB Register command */
    {
      CommandHandle.pBuffer = (uint8_t *)&Data;
      if(MXIC_SNOR_WriteReadLockAreaConfigurationRegister(&QSPIHandle[Instance], &CommandHandle) != MXIC_SNOR_ERROR_NONE)
      {
        ret = BSP_ERROR_COMPONENT_FAILURE;
      }/* Configure automatic polling mode to wait for command end */
      else if(MXIC_SNOR_AutoPollingMemReady(&QSPIHandle[Instance], CommandHandle.Mode) != MXIC_SNOR_ERROR_NONE)
      {
        ret = BSP_ERROR_COMPONENT_FAILURE;
      }
    }
  }
  /* Return BSP status */
  return ret;
}
#endif  // MXIC_SNOR_WRITE_READ_LOCK_CONFIGURATION_CMD

#ifdef MXIC_SNOR_READ_READ_LOCK_CONFIGURATION_CMD
/**
  * @brief  Read Read-Lock Area Configuration Register
  * @param  Instance  QSPI instance
  * @retval BSP status
  */
int32_t BSP_QSPI_ReadReadLockAreaConfigurationRegister(uint32_t Instance, MXIC_SNOR_ReadLockRegister_t *pData)
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
    if(MXIC_SNOR_ReadReadLockAreaConfigurationRegister(&QSPIHandle[Instance], &CommandHandle) != MXIC_SNOR_ERROR_NONE)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
  }
  /* Return BSP status */
  return ret;
}
#endif  // MXIC_SNOR_READ_READ_LOCK_CONFIGURATION_CMD

#ifdef MXIC_SNOR_PERMANENT_LOCK_BIT_LOCK_DOWN_CMD
/**
  * @brief  Permanent Lock bit Lock Down
  * @param  Instance  QSPI instance
  * @retval BSP status
  */
int32_t BSP_QSPI_PermanentLockbitLockDown(uint32_t Instance)
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
    if(MXIC_SNOR_WriteEnable(&QSPIHandle[Instance], &CommandHandle) != MXIC_SNOR_ERROR_NONE)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
    else if(MXIC_SNOR_PermanentLockbitLockDown(&QSPIHandle[Instance]) != MXIC_SNOR_ERROR_NONE)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }/* Configure automatic polling mode to wait for command end */
    else if(MXIC_SNOR_AutoPollingMemReady(&QSPIHandle[Instance], CommandHandle.Mode) != MXIC_SNOR_ERROR_NONE)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
  }
  /* Return BSP status */
  return ret;
}
#endif  // MXIC_SNOR_PERMANENT_LOCK_BIT_LOCK_DOWN_CMD

#ifdef MXIC_SNOR_READ_PERMANENT_LOCK_BIT_LOCK_DOWN_CMD
/**
  * @brief  Read Permanent Lock bit Lock Down
  * @param  Instance  QSPI instance
  * @retval BSP status
  */
int32_t BSP_QSPI_ReadPermanentLockbitLockDown(uint32_t Instance, MXIC_SNOR_PermanentLockDownRegister_t *pData)
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
    if(MXIC_SNOR_ReadPermanentLockbitLockDown(&QSPIHandle[Instance], &CommandHandle) != MXIC_SNOR_ERROR_NONE)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
  }
  /* Return BSP status */
  return ret;
}
#endif  // MXIC_SNOR_READ_PERMANENT_LOCK_BIT_LOCK_DOWN_CMD

#ifdef MXIC_SNOR_WRITE_READ_LOCK_BIT_CMD
/**
  * @brief  Write Read-Lock Bit; Setting read-lock on the specified 128K Byte area.
  * @param  Instance  QSPI instance
  *         Data      Change bit 2 (RLB) of Security Register only.
  * @retval BSP status
  */
int32_t BSP_QSPI_WriteReadLockBit(uint32_t Instance, MXIC_SNOR_SecurityRegister_t Data)
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
    if(MXIC_SNOR_WriteEnable(&QSPIHandle[Instance], &CommandHandle) != MXIC_SNOR_ERROR_NONE)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
    else /* Issue Write DPB Register command */
    {
      CommandHandle.pBuffer = (uint8_t *)&Data;
      if(MXIC_SNOR_WriteReadLockBit(&QSPIHandle[Instance], &CommandHandle) != MXIC_SNOR_ERROR_NONE)
      {
        ret = BSP_ERROR_COMPONENT_FAILURE;
      }/* Configure automatic polling mode to wait for command end */
      else if(MXIC_SNOR_AutoPollingMemReady(&QSPIHandle[Instance], CommandHandle.Mode) != MXIC_SNOR_ERROR_NONE)
      {
        ret = BSP_ERROR_COMPONENT_FAILURE;
      }
    }
  }
  /* Return BSP status */
  return ret;
}
#endif  // MXIC_SNOR_WRITE_READ_LOCK_BIT_CMD

#ifdef MXIC_SNOR_BLOCK_WRITE_LOCK_PROTECTION_CMD
/**
  * @brief  Block Write Lock Protection; Write protection a specified block (64K)
  * @param  Instance  QSPI instance
  *         Address   Protected 64K block address
  * @retval BSP status
  */
int32_t BSP_QSPI_BlockWriteLockProtection(uint32_t Instance, uint32_t Address)
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

    /* Enable write operations */
    CommandHandle.Mode = QSPICtx[Instance].InterfaceMode;
    if(MXIC_SNOR_WriteEnable(&QSPIHandle[Instance], &CommandHandle) != MXIC_SNOR_ERROR_NONE)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
    else /* Issue Write DPB Register command */
    {
      CommandHandle.Address = Address;
      if(MXIC_SNOR_BlockWriteLockProtection(&QSPIHandle[Instance], &CommandHandle) != MXIC_SNOR_ERROR_NONE)
      {
        ret = BSP_ERROR_COMPONENT_FAILURE;
      }/* Configure automatic polling mode to wait for command end */
      else if(MXIC_SNOR_AutoPollingMemReady(&QSPIHandle[Instance], CommandHandle.Mode) != MXIC_SNOR_ERROR_NONE)
      {
        ret = BSP_ERROR_COMPONENT_FAILURE;
      }
    }
  }
  /* Return BSP status */
  return ret;
}
#endif  // MXIC_SNOR_BLOCK_WRITE_LOCK_PROTECTION_CMD

#ifdef MXIC_SNOR_PERMANENT_LOCK_CMD
/**
  * @brief  Permanent Lock; Permanent Lock a specified block (64K, Read only forever)
  * @param  Instance  QSPI instance
  *         Address   Protected 64K block address
  * @retval BSP status
  */
int32_t BSP_QSPI_PermanentLock(uint32_t Instance, uint32_t Address)
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
    if(MXIC_SNOR_WriteEnable(&QSPIHandle[Instance], &CommandHandle) != MXIC_SNOR_ERROR_NONE)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
    else /* Issue Write DPB Register command */
    {
      CommandHandle.Address = Address;
      if(MXIC_SNOR_PermanentLock(&QSPIHandle[Instance], &CommandHandle) != MXIC_SNOR_ERROR_NONE)
      {
        ret = BSP_ERROR_COMPONENT_FAILURE;
      }/* Configure automatic polling mode to wait for command end */
      else if(MXIC_SNOR_AutoPollingMemReady(&QSPIHandle[Instance], CommandHandle.Mode) != MXIC_SNOR_ERROR_NONE)
      {
        ret = BSP_ERROR_COMPONENT_FAILURE;
      }

#if defined(MXIC_SNOR_READ_PERMANENT_LOCK_STATUS_CMD) || defined(MXIC_SNOR_READ_PERMANENT_LOCK_STATUS_8DC_CMD)
      else
      {
        /* Check permanent lock status*/
        CommandHandle.Address = Address;
        CommandHandle.pBuffer = CommandHandle.Parameter.Type8;
        if(MXIC_SNOR_ReadBlockPermanentLockStatus(&QSPIHandle[Instance], &CommandHandle) != MXIC_SNOR_ERROR_NONE)
        {
          ret = BSP_ERROR_COMPONENT_FAILURE;
        }
        else if(CommandHandle.Parameter.Type8[0] != MXIC_SNOR_BLOCK_LOCKED)
        {
          ret = BSP_ERROR_LOCK_FAILURE;
        }
      }
#endif  // defined(MXIC_SNOR_READ_PERMANENT_LOCK_STATUS_CMD) || defined(MXIC_SNOR_READ_PERMANENT_LOCK_STATUS_8DC_CMD)
    }
  }
  /* Return BSP status */
  return ret;
}
#endif  // MXIC_SNOR_PERMANENT_LOCK_CMD

#ifdef MXIC_SNOR_READ_BLOCK_WRITE_LOCK_STATUS_CMD
/**
  * @brief  Read Block Write Lock status; Read the status of a specified block (64K)
  * @param  Instance  QSPI instance
  *         Address   Block address
  *         pData     Buffer pointer
  * @retval BSP status
  *         *pData = Lock status; DQ0 = 1 = Locked
  */
int32_t BSP_QSPI_ReadBlockWriteLockStatus(uint32_t Instance, uint8_t *pData, uint32_t Address)
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
    if(MXIC_SNOR_ReadBlockWriteLockStatus(&QSPIHandle[Instance], &CommandHandle) != MXIC_SNOR_ERROR_NONE)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
  }
  /* Return BSP status */
  return ret;
}
#endif  // MXIC_SNOR_READ_BLOCK_WRITE_LOCK_STATUS_CMD

#if defined(MXIC_SNOR_READ_PERMANENT_LOCK_STATUS_CMD) || defined(MXIC_SNOR_READ_PERMANENT_LOCK_STATUS_8DC_CMD)
/**
  * @brief  Read Block Permanent Lock Status; Read the status of Permanent Lock of a specified block (64K)
  * @param  Instance  QSPI instance
  *         Address   Block address
  *         pData     Buffer pointer
  * @retval BSP status
  *         *pData = Lock status; DQ0 = 1 = Locked
  */
int32_t BSP_QSPI_ReadBlockPermanentLockStatus(uint32_t Instance, MXIC_SNOR_BlockLockTypeDef *pData, uint32_t Address)
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
    if(MXIC_SNOR_ReadBlockPermanentLockStatus(&QSPIHandle[Instance], &CommandHandle) != MXIC_SNOR_ERROR_NONE)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
  }
  /* Return BSP status */
  return ret;
}
#endif  // defined(MXIC_SNOR_READ_PERMANENT_LOCK_STATUS_CMD) || defined(MXIC_SNOR_READ_PERMANENT_LOCK_STATUS_8DC_CMD)

#ifdef MXIC_SNOR_CHIP_UNPROCECT_CMD
/**
  * @brief  Chip unprotect; Disable the lock protection block of the whole chip
  * @param  Instance  QSPI instance
  * @retval BSP status
  */
int32_t BSP_QSPI_ChipUnprotect(uint32_t Instance)
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
    if(MXIC_SNOR_WriteEnable(&QSPIHandle[Instance], &CommandHandle) != MXIC_SNOR_ERROR_NONE)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
    else if(MXIC_SNOR_ChipUnprotect(&QSPIHandle[Instance]) != MXIC_SNOR_ERROR_NONE)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }/* Configure automatic polling mode to wait for command end */
    else if(MXIC_SNOR_AutoPollingMemReady(&QSPIHandle[Instance], CommandHandle.Mode) != MXIC_SNOR_ERROR_NONE)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
  }
  /* Return BSP status */
  return ret;
}
#endif  // MXIC_SNOR_CHIP_UNPROCECT_CMD

#ifdef MXIC_SNOR_BP4_KEY1_CMD
/**
  * @brief  Key command to set BP4
  * @param  Instance  QSPI instance
  * @retval BSP status
  */
int32_t BSP_QSPI_SetBP4Key1(uint32_t Instance)
{
  int32_t ret = BSP_ERROR_NONE;

  /* Check if the instance is supported */
  if(Instance >= QSPI_NOR_INSTANCE_NUMBER)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else if(MXIC_SNOR_BP4Key1(&QSPIHandle[Instance]) != MXIC_SNOR_ERROR_NONE)
  {
    ret = BSP_ERROR_COMPONENT_FAILURE;
  }
  /* Return BSP status */
  return ret;
}
#endif  // MXIC_SNOR_BP4_KEY1_CMD

#ifdef MXIC_SNOR_BP4_KEY2_CMD
/**
  * @brief  Key command to set BP4
  * @param  Instance  QSPI instance
  * @retval BSP status
  */
int32_t BSP_QSPI_SetBP4Key2(uint32_t Instance)
{
  int32_t ret = BSP_ERROR_NONE;

  /* Check if the instance is supported */
  if(Instance >= QSPI_NOR_INSTANCE_NUMBER)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else if(MXIC_SNOR_BP4Key2(&QSPIHandle[Instance]) != MXIC_SNOR_ERROR_NONE)
  {
    ret = BSP_ERROR_COMPONENT_FAILURE;
  }
  /* Return BSP status */
  return ret;
}
#endif  // MXIC_SNOR_BP4_KEY2_CMD

#ifdef MXIC_SNOR_READ_PASSWORD_REGISTER_CMD
/**
  * @brief  Read password register value. 8 Byte, SPI only
  * @param  Instance  QSPI instance
  * @retval BSP status
  */
int32_t BSP_QSPI_ReadPasswordRegister(uint32_t Instance, uint8_t *pData)
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

    /* Issue Read Password Register command */
    CommandHandle.Mode    = QSPICtx[Instance].InterfaceMode;
    CommandHandle.pBuffer = pData;
    if(MXIC_SNOR_ReadPasswordRegister(&QSPIHandle[Instance], &CommandHandle) != MXIC_SNOR_ERROR_NONE)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
  }
  /* Return BSP status */
  return ret;
}
#endif  // MXIC_SNOR_READ_PASSWORD_REGISTER_CMD

#ifdef MXIC_SNOR_WRITE_PASSWORD_REGISTER_CMD
/**
  * @brief  Write password register. Always write 8 byte data, SPI only
  * @param  Instance  QSPI instance
  * @retval BSP status
  */
int32_t BSP_QSPI_WritePasswordRegister(uint32_t Instance, uint8_t *pData)
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
      CommandHandle.pBuffer = pData;
      if(MXIC_SNOR_WritePasswordRegister(&QSPIHandle[Instance], &CommandHandle) != MXIC_SNOR_ERROR_NONE)
      {
        ret = BSP_ERROR_COMPONENT_FAILURE;
      }/* Configure automatic polling mode to wait for command end */
      else if(MXIC_SNOR_AutoPollingMemReady(&QSPIHandle[Instance], CommandHandle.Mode) != MXIC_SNOR_ERROR_NONE)
      {
        ret = BSP_ERROR_COMPONENT_FAILURE;
      }
    }
  }
  /* Return BSP status */
  return ret;
}
#endif  // MXIC_SNOR_WRITE_PASSWORD_REGISTER_CMD

#ifdef MXIC_SNOR_PASSWORD_UNLOCK_CMD
/**
  * @brief  Password unlock. Always write 8 byte data, SPI only
  * @param  Instance  QSPI instance
  *         Password  8 Byte password pointer for compare to Password register
  * @retval BSP status
  */
int32_t BSP_QSPI_PasswordUnlock(uint32_t Instance, uint8_t *pData)
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
      CommandHandle.pBuffer = pData;
      if(MXIC_SNOR_PasswordUnlock(&QSPIHandle[Instance], &CommandHandle) != MXIC_SNOR_ERROR_NONE)
      {
        ret = BSP_ERROR_COMPONENT_FAILURE;
      }/* Configure automatic polling mode to wait for command end */
      else if(MXIC_SNOR_AutoPollingMemReady(&QSPIHandle[Instance], CommandHandle.Mode) != MXIC_SNOR_ERROR_NONE)
      {
        ret = BSP_ERROR_COMPONENT_FAILURE;
      }
    }
  }
  /* Return BSP status */
  return ret;
}
#endif  // MXIC_SNOR_PASSWORD_UNLOCK_CMD

#ifdef MXIC_SNOR_READ_SECURITY_REGISTER_CMD
/**
  * @brief  Read security register value
  * @param  Instance  QSPI instance
  * @param  Security  Content of QSPI security register
  * @retval BSP status
  */
int32_t BSP_QSPI_ReadSecurityRegister(uint32_t Instance, MXIC_SNOR_SecurityRegister_t *pData)
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
    if(MXIC_SNOR_ReadSecurityRegister(&QSPIHandle[Instance],  &CommandHandle) != MXIC_SNOR_ERROR_NONE)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
  }
  /* Return BSP status */
  return ret;
}
#endif  // MXIC_SNOR_READ_SECURITY_REGISTER_CMD

#ifdef MXIC_SNOR_CLEAR_SECURITY_REGISTER_FAIL_FLAG_CMD
/**
  * @brief  Clear Security Register bit 6 & bit 5
  * @param  Instance  QSPI instance
  * @retval BSP status
  */
int32_t BSP_QSPI_ClearSecurityRegisterFailFlags(uint32_t Instance)
{
  int32_t ret = BSP_ERROR_NONE;

  /* Check if the instance is supported */
  if(Instance >= QSPI_NOR_INSTANCE_NUMBER)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else if(MXIC_SNOR_ClearSecurityRegisterFailFlags(&QSPIHandle[Instance]) != MXIC_SNOR_ERROR_NONE)
  {
    ret = BSP_ERROR_COMPONENT_FAILURE;
  }
  /* Return BSP status */
  return ret;
}
#endif  // MXIC_SNOR_CLEAR_SECURITY_REGISTER_FAIL_FLAG_CMD

#ifdef MXIC_SNOR_READ_FAST_BOOT_REGISTER_CMD
/**
  * @brief  Read Fast Boot Register
  * @param  Instance  QSPI instance
  * @param  Security  Content of QSPI security register
  * @retval BSP status
  */
int32_t BSP_QSPI_ReadFastBootRegister(uint32_t Instance, MXIC_SNOR_FastBootRegister_t *pData)
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
    if(MXIC_SNOR_ReadFastBootRegister(&QSPIHandle[Instance],  &CommandHandle) != MXIC_SNOR_ERROR_NONE)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
  }
  /* Return BSP status */
  return ret;
}
#endif  // MXIC_SNOR_READ_FAST_BOOT_REGISTER_CMD

#ifdef MXIC_SNOR_WRITE_FAST_BOOT_REGISTER_CMD
/**
  * @brief  Write Fast Boot Register
  * @param  Instance  QSPI instance
  * @retval BSP status
  */
int32_t BSP_QSPI_WriteFastBootRegister(uint32_t Instance, MXIC_SNOR_FastBootRegister_t Data)
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
      CommandHandle.pBuffer = (uint8_t *)&Data;
      if(MXIC_SNOR_WriteFastBootRegister(&QSPIHandle[Instance], &CommandHandle) != MXIC_SNOR_ERROR_NONE)
      {
        ret = BSP_ERROR_COMPONENT_FAILURE;
      }/* Configure automatic polling mode to wait for command end */
      else if(MXIC_SNOR_AutoPollingMemReady(&QSPIHandle[Instance], CommandHandle.Mode) != MXIC_SNOR_ERROR_NONE)
      {
        ret = BSP_ERROR_COMPONENT_FAILURE;
      }
    }
  }
  /* Return BSP status */
  return ret;
}
#endif  // MXIC_SNOR_WRITE_FAST_BOOT_REGISTER_CMD

#ifdef MXIC_SNOR_ERASE_FAST_BOOT_REGISTER_CMD
/**
  * @brief  Erase Fast Boot Register
  * @param  Instance  QSPI instance
  * @retval BSP status
  */
int32_t BSP_QSPI_EraseFastBootRegister(uint32_t Instance)
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
    else if(MXIC_SNOR_EraseFastBootRegister(&QSPIHandle[Instance], CommandHandle.Mode) != MXIC_SNOR_ERROR_NONE)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }/* Configure automatic polling mode to wait for command end */
    else if(MXIC_SNOR_AutoPollingMemReady(&QSPIHandle[Instance], CommandHandle.Mode) != MXIC_SNOR_ERROR_NONE)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
  }
  /* Return BSP status */
  return ret;
}
#endif  // MXIC_SNOR_ERASE_FAST_BOOT_REGISTER_CMD

#ifdef MXIC_SNOR_PROGRAM_ERASE_SUSPEND_CMD
/**
  * @brief  Suspends Program/Erase
  *         Pending Program/Erase process, Host can issue read command.
  * @param  Instance  QSPI instance
  * @retval BSP status
  */
int32_t BSP_QSPI_ProgramEraseSuspend(uint32_t Instance)
{
  int32_t ret = BSP_ERROR_NONE;

  /* Check if the instance is supported */
  if(Instance >= QSPI_NOR_INSTANCE_NUMBER)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else if(MXIC_SNOR_ProgramEraseSuspend(&QSPIHandle[Instance], QSPICtx[Instance].InterfaceMode) != MXIC_SNOR_ERROR_NONE)
  {
    ret = BSP_ERROR_COMPONENT_FAILURE;
  }
  /* Return BSP status */
  return ret;
}
#endif  // MXIC_SNOR_PROGRAM_ERASE_SUSPEND_CMD

#ifdef MXIC_SNOR_PROGRAM_ERASE_RESUME_CMD
/**
  * @brief  Resumes Program/Erase
  *         Resume Program/Erase process
  * @param  Instance  QSPI instance
  * @retval BSP status
  */
int32_t BSP_QSPI_ProgramEraseResume(uint32_t Instance)
{
  int32_t ret = BSP_ERROR_NONE;

  /* Check if the instance is supported */
  if(Instance >= QSPI_NOR_INSTANCE_NUMBER)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else if(MXIC_SNOR_ProgramEraseResume(&QSPIHandle[Instance], QSPICtx[Instance].InterfaceMode) != MXIC_SNOR_ERROR_NONE)
  {
    ret = BSP_ERROR_COMPONENT_FAILURE;
  }
  /* Return BSP status */
  return ret;
}
#endif  // MXIC_SNOR_PROGRAM_ERASE_RESUME_CMD

#ifdef MXIC_SNOR_DEEP_POWER_DOWN_CMD
/**
  * @brief  Deep power down.
  *         The device is not active and all Write/Program/Erase instruction are ignored.
  * @param  Instance  QSPI instance
  * @retval BSP status
  */
int32_t BSP_QSPI_EnterDeepPowerDown(uint32_t Instance)
{
  int32_t ret = BSP_ERROR_NONE;

  /* Check if the instance is supported */
  if(Instance >= QSPI_NOR_INSTANCE_NUMBER)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else if(MXIC_SNOR_EnterDeepPowerDown(&QSPIHandle[Instance], QSPICtx[Instance].InterfaceMode) != MXIC_SNOR_ERROR_NONE)
  {
    ret = BSP_ERROR_COMPONENT_FAILURE;
  }
  /* Return BSP status */
  return ret;
}
#endif  // MXIC_SNOR_DEEP_POWER_DOWN_CMD

#ifdef MXIC_SNOR_RELEASE_FROM_DEEP_POWER_DOWN_CMD
/**
  * @brief  Release from deep power down.
  *         After CS# go high, system need wait tRES1 time for device ready.
  * @param  Instance  QSPI instance
  * @retval BSP status
  */
int32_t BSP_QSPI_ReleaseFromDeepPowerDown(uint32_t Instance)
{
  int32_t ret = BSP_ERROR_NONE;

  /* Check if the instance is supported */
  if(Instance >= QSPI_NOR_INSTANCE_NUMBER)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else if(MXIC_SNOR_ReleaseFromDeepPowerDown(&QSPIHandle[Instance], QSPICtx[Instance].InterfaceMode) != MXIC_SNOR_ERROR_NONE)
  {
    ret = BSP_ERROR_COMPONENT_FAILURE;
  }

  //osDelay(1);         // For tRES1 time

  /* Return BSP status */
  return ret;
}
#endif  // MXIC_SNOR_RELEASE_FROM_DEEP_POWER_DOWN_CMD

#ifdef MXIC_SNOR_FACTORY_MODE_ENABLE_CMD
/**
  * @brief  Factory Mode Enable
  * @param  Instance  QSPI instance
  * @retval BSP status
  */
int32_t BSP_QSPI_FactoryModeEnable(uint32_t Instance)
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
    else if(MXIC_SNOR_FactoryModeEnable(&QSPIHandle[Instance], CommandHandle.Mode) != MXIC_SNOR_ERROR_NONE)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
#ifdef MXIC_SNOR_READ_FACTORY_MODE_STATUS_REGISTER_CMD
    else
    {
      CommandHandle.pBuffer = CommandHandle.Parameter.Type8;
      if(MXIC_SNOR_ReadFactoryModeStatusRegister(&QSPIHandle[Instance], &CommandHandle) != MXIC_SNOR_ERROR_NONE)
      {
        ret = BSP_ERROR_COMPONENT_FAILURE;
      }
      else if(CommandHandle.Parameter.Type8[0] != 0xFF)
      {
        ret = BSP_ERROR_COMPONENT_FAILURE;
      }
    }
#endif  // MXIC_SNOR_READ_FACTORY_MODE_STATUS_REGISTER_CMD
  }
  /* Return BSP status */
  return ret;
}
#endif  // MXIC_SNOR_FACTORY_MODE_ENABLE_CMD

#ifdef MXIC_SNOR_SET_BURST_LENGTH_CMD
/**
  * @brief  Set burst length
  * @param  Instance  QSPI instance
  * @retval BSP status
  */
int32_t BSP_QSPI_SetBurstLength(uint32_t Instance, MXIC_SNOR_WrapAroundTypeDef Data)
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
    CommandHandle.pBuffer = &Data;
    if(MXIC_SNOR_SetBurstLength(&QSPIHandle[Instance], &CommandHandle) != MXIC_SNOR_ERROR_NONE)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
  }
  /* Return BSP status */
  return ret;
}
#endif  // MXIC_SNOR_SET_BURST_LENGTH_CMD

#ifdef MXIC_SNOR_READ_ELECTRONIC_ID_CMD
/**
  * @brief  Read Electronic Signature (1 Byte)
  * @param  Instance  QSPI instance
  *         pData     Buffer pointer
  * @retval BSP status
  *         *pData = Signature (1 Byte)
  */
int32_t BSP_QSPI_ReadElectronicSignature(uint32_t Instance, uint8_t *pData)
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
    CommandHandle.pBuffer = pData;
    if(MXIC_SNOR_ReadElectronicSignature(&QSPIHandle[Instance],  &CommandHandle) != MXIC_SNOR_ERROR_NONE)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
  }
  /* Return BSP status */
  return ret;
}
#endif  // MXIC_SNOR_READ_ELECTRONIC_ID_CMD

#if defined(MXIC_SNOR_READ_ELECTRONIC_MANFACTURER_ID_CMD)  || defined(MXIC_SNOR_READ_ELECTRONIC_MANFACTURER_ID_CMD2) || \
    defined(MXIC_SNOR_READ_ELECTRONIC_MANFACTURER_ID_CMD4) || defined(MXIC_SNOR_READ_ELECTRONIC_MANFACTURER_ID_CMD4D)
/**
  * @brief  Read Electronic Manufacturer ID & Device ID (2 Byte)
  * @param  Instance  QSPI instance
  *         pData     Buffer pointer
  *         Address
  * @retval BSP status
  */
int32_t BSP_QSPI_ReadElectronicManufacturerDeviceID(uint32_t Instance, uint8_t *pData, uint32_t Address)
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
    if(MXIC_SNOR_ReadElectronicManufacturerDeviceID(&QSPIHandle[Instance],  &CommandHandle) != MXIC_SNOR_ERROR_NONE)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
  }
  /* Return BSP status */
  return ret;
}
#endif  // defined(MXIC_SNOR_READ_ELECTRONIC_MANFACTURER_ID_CMD) ...

#ifdef SUPPORT_WRITE_BUFFER_ACCESS
/**
  * @brief  Interruptible Write to Buffer (Max. 256 Byte)
  * @param  Instance QSPI instance
  *         pData    Buffer pointer
  *         Address  Write address
  *         Size     Data size in Byte
  * @retval BSP status
  */
int32_t BSP_QSPI_InterruptibleWrite(uint32_t Instance, uint8_t *pData, uint32_t Address, uint32_t Size)
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
      CommandHandle.Address = Address;
      CommandHandle.Size    = Size;
      CommandHandle.pBuffer = pData;
      if(MXIC_SNOR_WriteBufferInitial(&QSPIHandle[Instance], &CommandHandle) != MXIC_SNOR_ERROR_NONE)
      {
        ret = BSP_ERROR_COMPONENT_FAILURE;
      }
      else if(MXIC_SNOR_WriteBufferConfirm(&QSPIHandle[Instance], QSPICtx[Instance].InterfaceMode) != MXIC_SNOR_ERROR_NONE)
      {
        ret = BSP_ERROR_COMPONENT_FAILURE;
      }
    }
  }
  /* Return BSP status */
  return ret;
}

/**
  * @brief  Read data from page buffer (Write Buffer Read)
  * @param  Instance QSPI instance
  *         pData    Buffer pointer
  *         Address  Write address
  *         Size     Data size in Byte
  * @retval BSP status
  */
int32_t BSP_QSPI_PageBufferRead(uint32_t Instance, uint8_t *pData, uint32_t Address, uint32_t Size)
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
    if(MXIC_SNOR_WriteBufferRead(&QSPIHandle[Instance], &CommandHandle) != MXIC_SNOR_ERROR_NONE)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
  }
  /* Return BSP status */
  return ret;
}

/**
  * @brief  Write Buffer Continue
  * @param  Instance QSPI instance
  *         pData    Buffer pointer
  *         Address  Write address
  *         Size     Data size in Byte
  * @retval BSP status
  */
int32_t BSP_QSPI_PageBufferContinueWrite(uint32_t Instance, uint8_t *pData, uint32_t Address, uint32_t Size)
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
    if(MXIC_SNOR_WriteBufferContinue(&QSPIHandle[Instance], &CommandHandle) != MXIC_SNOR_ERROR_NONE)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
  }
  /* Return BSP status */
  return ret;
}

/**
  * @brief  Write Buffer Confirm
  * @param  Instance QSPI instance
  * @retval BSP status
  */
int32_t BSP_QSPI_PageBufferConfirm(uint32_t Instance)
{
  int32_t ret = BSP_ERROR_NONE;

  /* Check if the instance is supported */
  if(Instance >= QSPI_NOR_INSTANCE_NUMBER)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else if(MXIC_SNOR_WriteBufferConfirm(&QSPIHandle[Instance], QSPICtx[Instance].InterfaceMode) != MXIC_SNOR_ERROR_NONE)
  {
    ret = BSP_ERROR_COMPONENT_FAILURE;
  }
  /* Return BSP status */
  return ret;
}
#endif  // SUPPORT_WRITE_BUFFER_ACCESS

#ifdef MXIC_SNOR_ENABLE_SO_OUTPUT_RYBY_CMD
/**
  * @brief  Enable SO to output RY/BY# during CP mode
  * @param  Instance  QSPI instance
  * @retval BSP status
  */
int32_t BSP_QSPI_EnableSOOutputRYBY(uint32_t Instance)
{
  int32_t ret = BSP_ERROR_NONE;

  /* Check if the instance is supported */
  if(Instance >= QSPI_NOR_INSTANCE_NUMBER)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else if(MXIC_SNOR_EnableSOOutputRYBY(&QSPIHandle[Instance]) != MXIC_SNOR_ERROR_NONE)
  {
    ret = BSP_ERROR_COMPONENT_FAILURE;
  }
  /* Return BSP status */
  return ret;
}
#endif  // MXIC_SNOR_ENABLE_SO_OUTPUT_RYBY_CMD

#ifdef MXIC_SNOR_DISABLE_SO_OUTPUT_RYBY_CMD
/**
  * @brief  Disable SO to output RY/BY# during CP mode
  * @param  Instance  QSPI instance
  * @retval BSP status
  */
int32_t BSP_QSPI_DisableSOOutputRYBY(uint32_t Instance)
{
  int32_t ret = BSP_ERROR_NONE;

  /* Check if the instance is supported */
  if(Instance >= QSPI_NOR_INSTANCE_NUMBER)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else if(MXIC_SNOR_DisableSOOutputRYBY(&QSPIHandle[Instance]) != MXIC_SNOR_ERROR_NONE)
  {
    ret = BSP_ERROR_COMPONENT_FAILURE;
  }
  /* Return BSP status */
  return ret;
}
#endif  // MXIC_SNOR_DISABLE_SO_OUTPUT_RYBY_CMD

#ifdef MXIC_SNOR_HOLD_ENABLE_CMD
/**
  * @brief  Hold Enable, Enable HOLD# pin function
  * @param  Instance  QSPI instance
  * @retval BSP status
  */
int32_t BSP_QSPI_EnableHoldPinFunction(uint32_t Instance)
{
  int32_t ret = BSP_ERROR_NONE;

  /* Check if the instance is supported */
  if(Instance >= QSPI_NOR_INSTANCE_NUMBER)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else if(MXIC_SNOR_HoldEnable(&QSPIHandle[Instance]) != MXIC_SNOR_ERROR_NONE)
  {
    ret = BSP_ERROR_COMPONENT_FAILURE;
  }
  /* Return BSP status */
  return ret;
}
#endif  // MXIC_SNOR_HOLD_ENABLE_CMD

#ifdef MXIC_SNOR_IDLE_ERASE_CMD
/**
  * @brief  Idle Erase
  * @param  Instance  QSPI instance
  * @retval BSP status
  */
int32_t BSP_QSPI_EnterIdleErase(uint32_t Instance)
{
  int32_t ret = BSP_ERROR_NONE;

  /* Check if the instance is supported */
  if(Instance >= QSPI_NOR_INSTANCE_NUMBER)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else if(MXIC_SNOR_IdleErase(&QSPIHandle[Instance]) != MXIC_SNOR_ERROR_NONE)
  {
    ret = BSP_ERROR_COMPONENT_FAILURE;
  }
  /* Return BSP status */
  return ret;
}
#endif  // MXIC_SNOR_IDLE_ERASE_CMD

#ifdef MXIC_SNOR_RELEASE_FROM_IDLE_ERASE_CMD
/**
  * @brief  Release from Idle Erase
  * @param  Instance  QSPI instance
  * @retval BSP status
  */
int32_t BSP_QSPI_ReleaseFromIdeleErase(uint32_t Instance)
{
  int32_t ret = BSP_ERROR_NONE;

  /* Check if the instance is supported */
  if(Instance >= QSPI_NOR_INSTANCE_NUMBER)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else if(MXIC_SNOR_ReleaseFromIdeleErase(&QSPIHandle[Instance]) != MXIC_SNOR_ERROR_NONE)
  {
    ret = BSP_ERROR_COMPONENT_FAILURE;
  }/* Configure automatic polling mode to wait for WIP = 0 */
  else if(MXIC_SNOR_AutoPollingMemReady(&QSPIHandle[Instance], QSPICtx[Instance].InterfaceMode) != MXIC_SNOR_ERROR_NONE)
  {
    ret = BSP_ERROR_COMPONENT_FAILURE;
  }
  /* Return BSP status */
  return ret;
}
#endif  // MXIC_SNOR_RELEASE_FROM_IDLE_ERASE_CMD

#ifdef MXIC_SNOR_READ_IDLE_ERASE_STATUS_CMD
/**
  * @brief  Release from Idle Erase
  * @param  Instance  QSPI instance
  * @retval BSP status
  *         *pData = Idle Erase Status Value
  */
int32_t BSP_QSPI_ReadIdeleEraseStatus(uint32_t Instance, MXIC_SNOR_IdleEraseStatus_t *pData)
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
    if(MXIC_SNOR_ReadIdeleEraseStatus(&QSPIHandle[Instance], &CommandHandle) != MXIC_SNOR_ERROR_NONE)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
  }
  /* Return BSP status */
  return ret;
}
#endif  // MXIC_SNOR_READ_IDLE_ERASE_STATUS_CMD

/*******************************************************************************
 * RPMC Functions
 ******************************************************************************/
#ifdef SUPPORT_RPMC
/**
  * WRITE_ROOT_KEY_REGISTER :
  *      Byte      |   0    |    1    |       2       |    3   |  4-35  |     36-63
  * ---------------+--------+---------+---------------+--------+--------+-------------------
  * Write Root Key |OP1 0x9B|CmdType 0|Counter Address|Reserved|Root Key|Truncated Signature
  *                |Command |                  Data
  * @brief  RPMC　Flash Write Root Key Register
  *         Message[4] = OP1 + CmdType + CounterAddr + Reserved
  * @param  Instance   QSPI Instance
  *         CounterAddress  Counter Address of the root key
  *         pRootKey        Must be 32 Byte root key buffer pointer
  * @retval BSP status
  */
int32_t BSP_QSPI_RPMCWriteRootKeyRegister(uint32_t Instance, uint8_t CounterAddress, uint8_t *pRootKey)
{
  int32_t ret = BSP_ERROR_NONE;
  MXIC_SNOR_CommandHandle_t CommandHandle;
  uint8_t Buffer[64];
  uint8_t *Message            = Buffer;         // 4 Byte = OP1 + CmdType + CounterAddr + Reserved
  uint8_t *Key                = &Buffer[4];     // 32 Byte
  uint8_t *TruncatedSignature = &Buffer[32];    // 32 Byte, Truncate first 4 Byte of Signature

  CommandHandle.Mode = QSPICtx[Instance].InterfaceMode;

  /* Check if the instance is supported */
  if(Instance >= QSPI_NOR_INSTANCE_NUMBER)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    Message[0] = MXIC_SNOR_RPMC_OP1_CMD;                  // OP1
    Message[1] = MXIC_SNOR_RPMC_WRITE_ROOT_KEY_REGISTER;  // CmdType
    Message[2] = CounterAddress;                          // CounterAddr
    Message[3] = 0;                                       // Reserved, force to 0
    hmac_sha256(pRootKey, MXIC_SNOR_RPMC_KEY_SIZE, Message, 4, TruncatedSignature, SHA256_DIGEST_SIZE);
    memcpy(Key, pRootKey, MXIC_SNOR_RPMC_KEY_SIZE);       // 32 Byte, Truncate Signature first 4 Byte

    CommandHandle.Size    = sizeof(Buffer) - 1;
    CommandHandle.pBuffer = &Buffer[1];
    if(MXIC_SNOR_RPMCOP1(&QSPIHandle[Instance], &CommandHandle) != MXIC_SNOR_ERROR_NONE)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
    else if(MXIC_SNOR_RPMCPollingReady(&QSPIHandle[Instance]) != MXIC_SNOR_ERROR_NONE)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
    else
    {
      // Read Extended Status Register
      CommandHandle.Size    = 1;
      CommandHandle.pBuffer = CommandHandle.Parameter.Type8;
      if(MXIC_SNOR_RPMCOP2(&QSPIHandle[Instance], &CommandHandle) != MXIC_SNOR_ERROR_NONE)
      {
        ret = BSP_ERROR_COMPONENT_FAILURE;
      }
      else if(CommandHandle.Parameter.Type8[0] != MXIC_SNOR_RPMC_ESR_SUCCESSFUL)
      {
        ret = BSP_ERROR_RPMC_FAILURE;
      }
    }
  }
  /* Return BSP status */
  return ret;
}

/**
  * @brief  RPMC　Flash Update HMAC Key Register
  *         Message[8] = OP1 + CmdType + CounterAddr + Reserved + Key Data
  * @param  Instance   QSPI Instance
  *         CounterAddress  Counter Address of the root key
  *         pRootKey        Must be 32 Byte root key buffer pointer
  *         pKeyData        Must be 4 Byte key data buffer pointer
  *         pHMACKey        Must be 32 Byte HMAC Key buffer pointer
  * @retval BSP status
  *         *pHMACKey       Retrieve 32 Byte HMAC Key
  */
int32_t BSP_QSPI_RPMCUpdateHMACKeyRegister(uint32_t Instance, uint8_t CounterAddress, uint8_t *pRootKey, uint8_t *pKeyData, uint8_t *pHMACKey)
{
  int32_t ret = BSP_ERROR_NONE;
  MXIC_SNOR_CommandHandle_t CommandHandle;
  uint8_t Buffer[40];
  uint8_t *Message   = Buffer;         // 8 Byte = OP1 + CmdType + CounterAddr + Reserved + Key Data
  uint8_t *KeyData   = &Buffer[4];     // 4 Byte
  uint8_t *Signature = &Buffer[8];     // 32 Byte

  CommandHandle.Mode = QSPICtx[Instance].InterfaceMode;

  /* Check if the instance is supported */
  if(Instance >= QSPI_NOR_INSTANCE_NUMBER)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    Message[0] = MXIC_SNOR_RPMC_OP1_CMD;                  // OP1
    Message[1] = MXIC_SNOR_RPMC_UPDATE_HMAC_KEY_REGISTER; // CmdType
    Message[2] = CounterAddress;                          // CounterAddr
    Message[3] = 0;                                       // Reserved, force to 0
    memcpy(KeyData, pKeyData, 4);

    hmac_sha256(pRootKey, MXIC_SNOR_RPMC_KEY_SIZE, KeyData, 4, pHMACKey,  SHA256_DIGEST_SIZE);
    hmac_sha256(pHMACKey, MXIC_SNOR_RPMC_KEY_SIZE, Message, 8, Signature, SHA256_DIGEST_SIZE);

    CommandHandle.Size    = sizeof(Buffer) - 1;
    CommandHandle.pBuffer = &Buffer[1];
    if(MXIC_SNOR_RPMCOP1(&QSPIHandle[Instance], &CommandHandle) != MXIC_SNOR_ERROR_NONE)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
    else if(MXIC_SNOR_RPMCPollingReady(&QSPIHandle[Instance]) != MXIC_SNOR_ERROR_NONE)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
    else
    {
      // Read Extended Status Register
      CommandHandle.Size    = 1;
      CommandHandle.pBuffer = CommandHandle.Parameter.Type8;
      if(MXIC_SNOR_RPMCOP2(&QSPIHandle[Instance], &CommandHandle) != MXIC_SNOR_ERROR_NONE)
      {
        ret = BSP_ERROR_COMPONENT_FAILURE;
      }
      else if(CommandHandle.Parameter.Type8[0] != MXIC_SNOR_RPMC_ESR_SUCCESSFUL)
      {
        ret = BSP_ERROR_RPMC_FAILURE;
      }
    }
  }
  /* Return BSP status */
  return ret;
}

/**
  * @brief  RPMC　Flash Increment Monotonic Counter
  *         Message[8] = OP1 + CmdType + CounterAddr + Reserved + Counter Data
  * @param  Instance   QSPI Instance
  *         CounterAddress  Counter Address of the root key
  *         pHMACKey        Must be 32 Byte HMAC key buffer pointer
  *         Counter         32 bit counter value (little endian)
  * @retval BSP status
  */
int32_t BSP_QSPI_RPMCIncrementMonotonicCounter(uint32_t Instance, uint8_t CounterAddress, uint8_t *pHMACKey, uint32_t Counter)
{
  int32_t ret = BSP_ERROR_NONE;
  MXIC_SNOR_CommandHandle_t CommandHandle;
  uint8_t Buffer[40];
  uint8_t *Message     = Buffer;         // 8 Byte = OP1 + CmdType + CounterAddr + Reserved + Counter Data
  uint8_t *CounterData = &Buffer[4];     // 4 Byte
  uint8_t *Signature   = &Buffer[8];     // 32 Byte

  CommandHandle.Mode = QSPICtx[Instance].InterfaceMode;

  /* Check if the instance is supported */
  if(Instance >= QSPI_NOR_INSTANCE_NUMBER)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    Message[0] = MXIC_SNOR_RPMC_OP1_CMD;                     // OP1
    Message[1] = MXIC_SNOR_RPMC_INCREMENT_MONOTONIC_COUNTER; // CmdType
    Message[2] = CounterAddress;                             // CounterAddr
    Message[3] = 0;                                          // Reserved, force to 0
    // Little endian convert to bit endian
    CounterData[0] = Counter >> 24;
    CounterData[1] = Counter >> 16;
    CounterData[2] = Counter >> 8;
    CounterData[3] = Counter;

    hmac_sha256(pHMACKey, MXIC_SNOR_RPMC_KEY_SIZE, Message, 8, Signature, SHA256_DIGEST_SIZE);

    CommandHandle.Size    = sizeof(Buffer) - 1;
    CommandHandle.pBuffer = &Buffer[1];
    if(MXIC_SNOR_RPMCOP1(&QSPIHandle[Instance], &CommandHandle) != MXIC_SNOR_ERROR_NONE)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
    else if(MXIC_SNOR_RPMCPollingReady(&QSPIHandle[Instance]) != MXIC_SNOR_ERROR_NONE)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
    else
    {
      // Read Extended Status Register
      CommandHandle.Size    = 1;
      CommandHandle.pBuffer = CommandHandle.Parameter.Type8;
      if(MXIC_SNOR_RPMCOP2(&QSPIHandle[Instance], &CommandHandle) != MXIC_SNOR_ERROR_NONE)
      {
        ret = BSP_ERROR_COMPONENT_FAILURE;
      }
      else if(CommandHandle.Parameter.Type8[0] != MXIC_SNOR_RPMC_ESR_SUCCESSFUL)
      {
        ret = BSP_ERROR_RPMC_FAILURE;
      }
    }
  }
  /* Return BSP status */
  return ret;
}

/**
  * @brief  RPMC　Flash Request Monotonic Counter
  *         Message[16] = OP1 + CmdType + CounterAddr + Reserved + Tag
  * @param  Instance   QSPI Instance
  *         CounterAddress  Counter Address of the root key
  *         pHMACKey        Must be 32 Byte HMAC key buffer pointer
  *         pTag            Must be 12 Byte tag buffer pointer
  *         Counter         32 bit counter buffer pointer
  * @retval BSP status
  *         *Counter        Retrieve 32 bit counter value
  */
int32_t BSP_QSPI_RPMCRequestMonotonicCounter(uint32_t Instance, uint8_t CounterAddress, uint8_t *pHMACKey, uint8_t *pTag, uint32_t *Counter)
{
  int32_t ret = BSP_ERROR_NONE;
  MXIC_SNOR_CommandHandle_t CommandHandle;
  uint8_t Buffer[48];
  uint8_t *Message   = Buffer;         // 16 Byte = OP1 + CmdType + CounterAddr + Reserved + Tag
  uint8_t *Tag       = &Buffer[4];     // 12 Byte
  uint8_t *Signature = &Buffer[16];    // 32 Byte

  CommandHandle.Mode = QSPICtx[Instance].InterfaceMode;

  /* Check if the instance is supported */
  if(Instance >= QSPI_NOR_INSTANCE_NUMBER)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    Message[0] = MXIC_SNOR_RPMC_OP1_CMD;                     // OP1
    Message[1] = MXIC_SNOR_RPMC_REQUEST_MONOTONIC_COUNTER;   // CmdType
    Message[2] = CounterAddress;                             // CounterAddr
    Message[3] = 0;                                          // Reserved, force to 0
    memcpy(Tag, pTag, 12);

    hmac_sha256(pHMACKey, MXIC_SNOR_RPMC_KEY_SIZE, Message, 16, Signature, SHA256_DIGEST_SIZE);

    CommandHandle.Size    = sizeof(Buffer) - 1;
    CommandHandle.pBuffer = &Buffer[1];
    if(MXIC_SNOR_RPMCOP1(&QSPIHandle[Instance], &CommandHandle) != MXIC_SNOR_ERROR_NONE)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
    else if(MXIC_SNOR_RPMCPollingReady(&QSPIHandle[Instance]) != MXIC_SNOR_ERROR_NONE)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
    else
    {
      RPMC_OP2ReadData_t OP2ReadData;

      CommandHandle.Size    = sizeof(RPMC_OP2ReadData_t);
      CommandHandle.pBuffer = (uint8_t *)&OP2ReadData;
      if(MXIC_SNOR_RPMCOP2(&QSPIHandle[Instance], &CommandHandle) != MXIC_SNOR_ERROR_NONE)
      {
        ret = BSP_ERROR_COMPONENT_FAILURE;
      }
      else if(OP2ReadData.ESR != MXIC_SNOR_RPMC_ESR_SUCCESSFUL)
      {
        ret = BSP_ERROR_RPMC_FAILURE;
      }
      else
      {
        /* Verify Signature **************************************************/
        hmac_sha256(pHMACKey, MXIC_SNOR_RPMC_KEY_SIZE, &OP2ReadData.Tag[0], 16, Signature, SHA256_DIGEST_SIZE);

        if(memcmp(OP2ReadData.Signature, Signature, SHA256_DIGEST_SIZE) != 0)
        {
          ret = BSP_ERROR_RPMC_FAILURE;
        }
        else
        {
          // Big endian convert to little endian
          *Counter = (OP2ReadData.CRD[0] << 24) |(OP2ReadData.CRD[1] << 16) | (OP2ReadData.CRD[2] << 8) | OP2ReadData.CRD[3];
        }
      }
    }
  }
  /* Return BSP status */
  return ret;
}

/**
  * @brief  RPMC　Flash Read data
  * @param  Instance   QSPI Instance
  *         pBuffer    Buffer for store read data
  *         Size       Read data size in Byte
  * @retval BSP status
  *         *pBuffer   Retrieve read data
  */
int32_t BSP_QSPI_RPMCReadData(uint32_t Instance, uint8_t *pBuffer, uint8_t Size)
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
    CommandHandle.pBuffer = pBuffer;
    if(MXIC_SNOR_RPMCOP2(&QSPIHandle[Instance], &CommandHandle) != MXIC_SNOR_ERROR_NONE)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
  }
  /* Return BSP status */
  return ret;
}
#endif  // SUPPORT_RPMC
/************************ (C) COPYRIGHT Macronix *****END OF FILE****/
