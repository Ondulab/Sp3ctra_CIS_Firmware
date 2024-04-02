/*************************************************************************************************
 *          Macronix xSPI-NOR Flash command pool for STM32 QUADSPI IP
 * @file    MXIC_xSPINOR_QUADSPI.c
 * @brief   Macronix xSPI-NOR Flash Command pool driver for STM32 QUADSPI IP.
 * @Author  Peter Chang / Macronix; peterchang01@mxic.com.tw
 *
 *   MXIC_SNOR_IOTypeDef is base on Read Command defined. Other commands will mapping to it.
 *
 * Read Commands :
 *  Rate |    IO     | 3/4 Byte address | 4 Byte address | Enhance Read
 * ------+-----------+------------------+----------------+------------------
 *  STR  | 1-1-1+0DC | 0x03             |  0x13          |
 *       | 1-1-1     | 0x0B             |  0x0C          |
 *       | 1-1-2     | 0x3B             |  0x3C          |
 *       | 1-1-4     | 0x6B             |  0x6C          |
 *       | 1-1-8     | 0x03             |  0x7C          |
 *       | 1-2-2     | 0xBB             |  0xBC          |
 *       | 1-4-4     | 0xEA, 0xEB       |  0xEC          | 0xEA, 0xEB, 0xEC
 *       | 1-4-4+4DC | 0xE7             |        -       |
 *       | 4-4-4     | 0x0B, 0xEA, 0xEB |  0xEC          | 0xEA, 0xEB, 0xEC
 *       | 4-4-4+4DC | 0x0B, 0xE7       |        -       |
 *       | 8-8-8     |         -        |  0x7C, 0xEC    |
 * ------+-----------+------------------+----------------+------------------
 *  DTR  | 1-1-1+0DC |         -        |        -       |
 *       | 1-1-1     | 0x0D             |  0x0E          |
 *       | 1-1-2     |         -        |        -       |
 *       | 1-1-4     |         -        |        -       |
 *       | 1-1-8     |         -        |        -       |
 *       | 1-2-2     | 0xBD             |  0xBE          |
 *       | 1-4-4     | 0xED             |  0xEE          | 0xED, 0xEE
 *       | 1-4-4+4DC |         -        |        -       |
 *       | 4-4-4     | 0xED             |  0xEE          | 0xED, 0xEE
 *       | 4-4-4+4DC |         -        |        -       |
 *       | 8-8-8     |         -        |  0xEE          |
 *
 * Page Program Commands :
 *  Rate |    IO     | 3/4 Byte address | 4 Byte address
 * ------+-----------+------------------+----------------
 *  STR  | 1-1-1+0DC |         -        |        -
 *       | 1-1-1     | 0x02             | 0x12
 *       | 1-1-2     |         -        |        -
 *       | 1-1-4     | 0x32             |        -
 *       | 1-1-8     | 0x02             |        -
 *       | 1-2-2     |         -        |        -
 *       | 1-4-4     | 0x38             | 0x3E
 *       | 1-4-4+4DC |         -        |        -
 *       | 4-4-4     | 0x02             | 0x12
 *       | 4-4-4+4DC |         -        |        -
 *       | 8-8-8     |         -        | 0x12
 * ------+-----------+------------------+----------------
 *  DTR  | 1-1-1+0DC |         -        |        -
 *       | 1-1-1     |         -        |        -
 *       | 1-1-2     |         -        |        -
 *       | 1-1-4     |         -        |        -
 *       | 1-1-8     |         -        |        -
 *       | 1-2-2     |         -        |        -
 *       | 1-4-4     |         -        |        -
 *       | 1-4-4+4DC |         -        |        -
 *       | 4-4-4     |         -        |        -
 *       | 4-4-4+4DC |         -        |        -
 *       | 8-8-8     |         -        | 0x12
 *
 * Erase Commands :
 *  Rate |    IO     | 3/4 Byte address | 4 Byte address
 * ------+-----------+------------------+------------------
 *  STR  | 1-1-1+0DC | 1-0-0  Chip Erase 0x60, 0xC7
 *       | 1-1-1     | 1-1-0        Block Erase
 *       | 1-1-2     | 0x20, 0x52, 0xD8 | 0x21, 0x5C, 0xDC
 *       | 1-1-4     |                  |
 *       | 1-1-8     |                  |
 *       | 1-2-2     |                  |
 *       | 1-4-4     |                  |
 *       | 1-4-4+4DC |                  |
 *       +-----------+------------------+------------------
 *       | 4-4-4     | 4-0-0  Chip Erase 0x60, 0xC7
 *       | 4-4-4+4DC | 4-4-0        Block Erase
 *       |           | 0x20, 0x52, 0xD8 | 0x21, 0x5C, 0xDC
 *       +-----------+------------------+------------------
 *       | 8-8-8     | 8-0-0  Chip Erase 0x60, 0xC7
 *       |           | 8-8-0        Block Erase
 *       |           |                  | 0x21, 0xDC
 * ------+-----------+------------------+------------------
 *  DTR  | 1-1-1+0DC |                  -
 *       | 1-1-1     |                  -
 *       | 1-1-2     |                  -
 *       | 1-1-4     |                  -
 *       | 1-1-8     |                  -
 *       | 1-2-2     |                  -
 *       | 1-4-4     |                  -
 *       | 1-4-4+4DC |                  -
 *       | 4-4-4     |                  -
 *       | 4-4-4+4DC |                  -
 *       +-----------+------------------+------------------
 *       | 8-8-8     | 8-0-0  Chip Erase 0x60, 0xC7
 *       |           | 8-8-0        Block Erase
 *       |           |         -        | 0x21, 0xDC
 *************************************************************************************************
 * History :
 * 2023/12/20 Release V2.0.0 
 *            Rename MXIC_xSPINOR.c to MXIC_xSPINOR_QUADSPI.c
 *            Alignment input parameter between QUAD-SPI & OCTO-SPI IP  
 * 2022/12/21 Add MXIC_SNOR_WritePacket() & MXIC_SNOR_ReadPacket() commands
 * 2022/12/12 Release V1.1.0
 * 2022/12/08 Bug fix, MXIC_SNOR_BlockErase() case 64K Block erase define error.
 *            defined(MXIC_SNOR_4BYTE_ADDRESS_ERASE_32K_CMD) -> defined(MXIC_SNOR_4BYTE_ADDRESS_ERASE_64K_CMD)
 * 2022/06/17 Release V1.0.1
 *            Remove test functions
 * 2022/06/15 Release V1.0.0
 * 2021/12/09 Initial version.
 *************************************************************************************************/
/* Includes -------------------------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"
#include "MXIC_xSPINOR.h"

/* Private typedef ------------------------------------------------------------------------------*/
#define MXIC_SNOR_PERFORMANCE_ENHANCE_INDICATOR (uint32_t)0x5A5A5A5A

/* Private define -------------------------------------------------------------------------------*/
/* Private macro --------------------------------------------------------------------------------*/
/* Private variables ----------------------------------------------------------------------------*/
/* Private functions ----------------------------------------------------------------------------*/
static MXIC_xSPINORErrorTypeDef MXIC_SNOR_SetupReadCommandSTR(QSPI_CommandTypeDef *QSPI_Command, MXIC_SNOR_Mode_t Mode);

#ifdef SUPPORT_DTR
static MXIC_xSPINORErrorTypeDef MXIC_SNOR_SetupReadCommandDTR(QSPI_CommandTypeDef *QSPI_Command, MXIC_SNOR_Mode_t Mode);
#endif  // SUPPORT_DTR

static MXIC_xSPINORErrorTypeDef x00_Command(void *Ctx, uint8_t OPCode, MXIC_SNOR_IOTypeDef IO);

static void SetupRegisterCommandSPIQPIx0x(QSPI_CommandTypeDef *QSPI_Command, uint8_t OPCode, MXIC_SNOR_IOTypeDef IO, uint32_t Size);
static void SetupRegisterCommandSPIQPIxxx(QSPI_CommandTypeDef *QSPI_Command, uint8_t OPCode, MXIC_SNOR_IOTypeDef IO, uint32_t Size);

/**************************************************************************************************
 * BSP Request Functions
 *************************************************************************************************/
/*
 * @brief  Get driver information, Not the real device information
 * @param  Component driver object pointer
 * @retval MXIC_SNOR_DriverInfo_t
 */
MXIC_xSPINORErrorTypeDef MXIC_SNOR_GetDriverInfo(MXIC_SNOR_DriverInfo_t *pInfo)
{
  pInfo->ID         = MXIC_SNOR_DEVICE_ID;     // Driver Supported Flash ID
  pInfo->DeviceSize = MXIC_SNOR_FLASH_SIZE;    // Driver Supported Flash Size in Byte
  pInfo->PageSize   = MXIC_SNOR_PAGE_SIZE;     // Driver Supported Flash Page Size in Byte
  pInfo->EraseType1 = MXIC_SNOR_ERASE_4K;      // Driver Supported Flash Erase Type 1 Size in Byte; 0 = Not Supported
  pInfo->EraseType2 = MXIC_SNOR_ERASE_32K;     // Driver Supported Flash Erase Type 2 Size in Byte; 0 = Not Supported
  pInfo->EraseType3 = MXIC_SNOR_ERASE_64K;     // Driver Supported Flash Erase Type 3 Size in Byte; 0 = Not Supported
  return MXIC_SNOR_ERROR_NONE;
}

/*
 * MXIC_SNOR_READ_STATUS_REG_CMD                   0x05   // RDSR, Read Status Register; 1-0-1/4-0-4/8S-8S-8S/8D-8D-8D
 * @brief  Polling Status Register WIP bit become 0.
 *         Time out = HAL_QPSI_TIMEOUT_DEFAULT_VALUE (5s).
 * @param  *Ctx         : Device handle
 *         Handle->Mode : Command interface
 * @retval MXIC_xSPINORErrorTypeDef
 */
MXIC_xSPINORErrorTypeDef MXIC_SNOR_AutoPollingMemReady(void *Ctx, MXIC_SNOR_Mode_t Mode)
{
  MXIC_SNOR_CommandHandle_t CommandHandle;
  uint8_t MatchMask[2] = {0, MXIC_SNOR_SR_WIP};

  CommandHandle.Mode             = Mode;
  CommandHandle.pBuffer          = MatchMask;
  CommandHandle.Parameter.Type32 = HAL_QPSI_TIMEOUT_DEFAULT_VALUE;

  return MXIC_SNOR_PollingStatusRegister(Ctx, &CommandHandle);
}

/*
 * MXIC_SNOR_READ_STATUS_REG_CMD                   0x05   // RDSR, Read Status Register; 1-0-1/4-0-4/8S-8S-8S/8D-8D-8D
 * @brief  Polling Status Register bit become 0 or 1
 * @param  *Ctx                     : Device handle
 *         Handle->Mode.IO          : I/O Mode
 *         Handle->pBuffer[0]       : Match = Check value after mask
 *         Handle->pBuffer[1]       : Mask  = bit mask for check
 *         Handle->Parameter.Type32 : Timeout in ms
 * @retval MXIC_xSPINORErrorTypeDef
 */
MXIC_xSPINORErrorTypeDef MXIC_SNOR_PollingStatusRegister(void *Ctx, MXIC_SNOR_CommandHandle_t *Handle)
{
  QSPI_CommandTypeDef     s_command;
  QSPI_AutoPollingTypeDef s_config;

  /* Setup command structure */
  SetupRegisterCommandSPIQPIx0x(&s_command, MXIC_SNOR_READ_STATUS_REG_CMD, Handle->Mode.IO, 1);

  /* Setup auto polling mask & match structure */
  s_config.Match           = Handle->pBuffer[0];
  s_config.Mask            = Handle->pBuffer[1];
  s_config.MatchMode       = QSPI_MATCH_MODE_AND;
  s_config.StatusBytesSize = 1;
  s_config.Interval        = 0x10;
  s_config.AutomaticStop   = QSPI_AUTOMATIC_STOP_ENABLE;

  if (HAL_QSPI_AutoPolling(Ctx, &s_command, &s_config, Handle->Parameter.Type32) != HAL_OK)
  {
    return MXIC_SNOR_ERROR_POLLING;
  }

  return MXIC_SNOR_ERROR_NONE;
}

#ifdef SUPPORT_PERFORMANCE_ENHANCE_READ
/* MXIC_SNOR_RELEASE_READ_ENHANCED_CMD   0xFF              // PEMR, STR & DTR; 1-4-4/4-4-4
 * @brief  Issue Performance Enhance Mode Reset Command. Quit from Performance Read mode.
 *         3 Byte address mode  8 clock I/O keep high
 *         4 Byte address mode 10 clock I/O keep high
 *         Program force use 4-4-4 + 4 Byte address mode send out 10 clock 0xFF data
 * @param  *Ctx : Device handle
 * @retval MXIC_xSPINORErrorTypeDef
 */
MXIC_xSPINORErrorTypeDef MXIC_SNOR_PerformanceEnhanceModeReset(void *Ctx)
{
  QSPI_CommandTypeDef s_command;
  uint8_t Data[] = {0xFF, 0xFF, 0xFF, 0xFF};

  /* Setup command structure */
  SetupRegisterCommandSPIQPIx0x(&s_command, 0xFF, MXIC_SNOR_FREAD_444, sizeof(Data));

  /* Configure the write volatile configuration register command */
  if (HAL_QSPI_Command(Ctx, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return MXIC_SNOR_ERROR_COMMAND;
  }

  /* Transmission of the data */
  if (HAL_QSPI_Transmit(Ctx, Data, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return MXIC_SNOR_ERROR_TRANSMIT;
  }

  return MXIC_SNOR_ERROR_NONE;
}
#endif  // SUPPORT_PERFORMANCE_ENHANCE_READ

/**************************************************************************************************
 * Macronix xSPI-NOR Flash Read Command Pool
 *************************************************************************************************/
/*
 * #SUPPORT_3BYTE_ADDRESS_COMMAND
 * MXIC_SNOR_NORMAL_READ_111_CMD                   0x03    // READ, Normal Read 3/4 Byte Address with 0 dummy clock; 1-1-1/1-1-8
 * MXIC_SNOR_FAST_READ_111_CMD                     0x0B    // FAST READ, Fast Read 3/4 Byte Address; 1-1-1/4-4-4
 * MXIC_SNOR_FAST_READ_112_CMD                     0x3B    // DREAD, 1I 2O Read 3/4 Byte Address; 1-1-2
 * MXIC_SNOR_FAST_READ_114_CMD                     0x6B    // QREAD, 1I 4O Read 3/4 Byte Address; 1-1-4
 * MXIC_SNOR_NORMAL_READ_118_CMD                   0x03    // READ, Normal Read 3/4 Byte Address with 0 dummy clock; 1-1-1/1-1-8
 * MXIC_SNOR_FAST_READ_122_CMD                     0xBB    // 2READ, 2 x I/O Read 3/4 Byte Address; 1-2-2
 * MXIC_SNOR_FAST_READ_144_CMD                     0xEB    // 4READ, 4 I/O Read 3/4 Byte Address; 1-4-4/4-4-4 + Enhance
 * MXIC_SNOR_FAST_READ_144_CMD1                    0xEA    // 4READ, 4 I/O Read 3/4 Byte Address Top; 1-4-4/4-4-4 + Enhance
 * MXIC_SNOR_FAST_READ_144_4DC_CMD                 0xE7    // W4READ, 4 I/O Read 3/4 Byte Address with 4 dummy clock; 1-4-4/4-4-4
 * MXIC_SNOR_FAST_READ_444_CMD                     0xEB    // 4READ, 4 I/O Read 3/4 Byte Address; 1-4-4/4-4-4 + Enhance
 * MXIC_SNOR_FAST_READ_444_CMD1                    0xEA    // 4READ, 4 I/O Read 3/4 Byte Address Top; 1-4-4/4-4-4 + Enhance
 * MXIC_SNOR_FAST_READ_444_CMD2                    0x0B    // FAST READ, Fast Read 3/4 Byte Address; 1-1-1/4-4-4 + Enhance
 * MXIC_SNOR_FAST_READ_444_4DC_CMD                 0x0B    // FAST READ, Fast Read 3/4 Byte Address; 1-1-1/4-4-4
 * MXIC_SNOR_FAST_READ_444_4DC_CMD1                0xE7    // W4READ, 4 I/O Read 3/4 Byte Address with 4 dummy clock; 1-4-4/4-4-4
 *
 * #SUPPORT_4BYTE_ADDRESS_COMMAND
 * MXIC_SNOR_4BYTE_ADDRESS_NORMAL_READ_111_CMD     0x13    // READ4B, Normal Read 4 Byte address with 0 dummy clock; 1-1-1
 * MXIC_SNOR_4BYTE_ADDRESS_FAST_READ_111_CMD       0x0C    // FAST READ4B, Fast Read 4 Byte address; 1-1-1
 * MXIC_SNOR_4BYTE_ADDRESS_FAST_READ_112_CMD       0x3C    // DREAD4B, Read by Dual Output 4 Byte address; 1-1-2
 * MXIC_SNOR_4BYTE_ADDRESS_FAST_READ_114_CMD       0x6C    // QREAD4B, Read by Quad Output 4 Byte address; 1-1-4
 * MXIC_SNOR_4BYTE_ADDRESS_FAST_READ_118_CMD       0x7C    // OREAD4B, Read by 8 x I/O 4 Byte address; 1-1-8/8S-8S-8S
 * MXIC_SNOR_4BYTE_ADDRESS_FAST_READ_122_CMD       0xBC    // 2READ4B, Read by 2 x I/O 4 Byte address; 1-2-2
 * MXIC_SNOR_4BYTE_ADDRESS_FAST_READ_144_CMD       0xEC    // 4READ4B, Read by 4 x I/O 4 Byte address; 1-4-4/4-4-4/8S-8S-8S + Enhance
 * MXIC_SNOR_4BYTE_ADDRESS_FAST_READ_444_CMD       0xEC    // 4READ4B, Read by 4 x I/O 4 Byte address; 1-4-4/4-4-4/8S-8S-8S + Enhance
 * MXIC_SNOR_4BYTE_ADDRESS_FAST_READ_888_CMD       0xEC    // 8READ,   Read by 8 x I/O 4 Byte address; 8S-8S-8S
 * MXIC_SNOR_4BYTE_ADDRESS_FAST_READ_888_CMD1      0x7C    // OREAD4B, Read by 8 x I/O 4 Byte address; 1-1-8/8S-8S-8S
 *
 * @brief  Setup STR read command for MMP mode usage.
 *         MMP mode running in performance enhance mode if device supported.
 *         Indirect read will issue command on every read cycle.
 *         Exist 2 dummy clock cycle difference between MMP & Indirect mode.
 * @param  *QSPI_Command : Pointer for setup command to
 *         Mode          : Command interface
 * @retval MXIC_xSPINORErrorTypeDef
 *         OperationType not setup
 */
static MXIC_xSPINORErrorTypeDef MXIC_SNOR_SetupReadCommandSTR(QSPI_CommandTypeDef *QSPI_Command, MXIC_SNOR_Mode_t Mode)
{
  /* Setup command structure */
  //QSPI_Command->InstructionMode    =
  //QSPI_Command->Instruction        =
  //QSPI_Command->AddressMode        =

#if defined(SUPPORT_4BYTE_ADDRESS_COMMAND) || defined(SUPPORT_34BYTE_ADDRESS_SWITCH) || defined(FORCE_USE_4BYTE_ADDRESS)
  QSPI_Command->AddressSize       = QSPI_ADDRESS_32_BITS;
#else
  QSPI_Command->AddressSize       = QSPI_ADDRESS_24_BITS;
#endif  // defined(SUPPORT_4BYTE_ADDRESS_COMMAND) || defined(SUPPORT_34BYTE_ADDRESS_SWITCH)

  //QSPI_Command->Address            =
  QSPI_Command->AlternateByteMode  = QSPI_ALTERNATE_BYTES_NONE;     // Modifyed by SUPPORT_PERFORMANCE_ENHANCE_READ
  //QSPI_Command->AlternateBytesSize =
  //QSPI_Command->AlternateBytes     =
  //QSPI_Command->DummyCycles        =
  //QSPI_Command->DataMode           =
  //QSPI_Command->NbData             =
  QSPI_Command->DdrMode            = QSPI_DDR_MODE_DISABLE;
  QSPI_Command->DdrHoldHalfCycle   = QSPI_DDR_HHC_ANALOG_DELAY;
  QSPI_Command->SIOOMode           = QSPI_SIOO_INST_EVERY_CMD;      // Modifyed by SUPPORT_PERFORMANCE_ENHANCE_READ

  switch(Mode.IO)
  {
  default :                           // 1-1-8/8-8-8, H/W IP not supported
    return MXIC_SNOR_ERROR_NOT_SUPPORTED;

#if defined(MXIC_SNOR_NORMAL_READ_111_CMD) || defined(MXIC_SNOR_4BYTE_ADDRESS_NORMAL_READ_111_CMD)
  case MXIC_SNOR_NREAD_111 :        // 1-1-1 normal read command with 0 dummy cycle
    QSPI_Command->InstructionMode    = QSPI_INSTRUCTION_1_LINE;

#ifdef SUPPORT_4BYTE_ADDRESS_COMMAND
    QSPI_Command->Instruction        = MXIC_SNOR_4BYTE_ADDRESS_NORMAL_READ_111_CMD;
#else
    QSPI_Command->Instruction        = MXIC_SNOR_NORMAL_READ_111_CMD;
#endif  // SUPPORT_4BYTE_ADDRESS_COMMAND

    QSPI_Command->AddressMode        = QSPI_ADDRESS_1_LINE;
    //QSPI_Command->AddressSize        =
    //QSPI_Command->Address            =
    //QSPI_Command->AlternateByteMode  =
    //QSPI_Command->AlternateBytesSize =
    //QSPI_Command->AlternateBytes     =
    QSPI_Command->DummyCycles        = 0;
    QSPI_Command->DataMode           = QSPI_DATA_1_LINE;
    //QSPI_Command->NbData             =
    //QSPI_Command->DdrMode            = QSPI_DDR_MODE_DISABLE;
    //QSPI_Command->DdrHoldHalfCycle   = QSPI_DDR_HHC_ANALOG_DELAY;
    //QSPI_Command->SIOOMode           = QSPI_SIOO_INST_EVERY_CMD;
    break;
#endif  // defined(MXIC_SNOR_NORMAL_READ_111_CMD) || defined(MXIC_SNOR_4BYTE_ADDRESS_NORMAL_READ_111_CMD)

#if defined(MXIC_SNOR_FAST_READ_111_CMD) || defined(MXIC_SNOR_4BYTE_ADDRESS_FAST_READ_111_CMD)
  case MXIC_SNOR_FREAD_111 :        // 1-1-1 fast read command, Power on H/W default setting
    QSPI_Command->InstructionMode    = QSPI_INSTRUCTION_1_LINE;

#ifdef SUPPORT_4BYTE_ADDRESS_COMMAND
    QSPI_Command->Instruction        = MXIC_SNOR_4BYTE_ADDRESS_FAST_READ_111_CMD;
#else
    QSPI_Command->Instruction        = MXIC_SNOR_FAST_READ_111_CMD;
#endif  // SUPPORT_4BYTE_ADDRESS_COMMAND

    QSPI_Command->AddressMode        = QSPI_ADDRESS_1_LINE;
    //QSPI_Command->AddressSize        =
    //QSPI_Command->Address            =
    //QSPI_Command->AlternateByteMode  =
    //QSPI_Command->AlternateBytesSize =
    //QSPI_Command->AlternateBytes     =
    QSPI_Command->DummyCycles        = MXIC_SNOR_DUMMY_CLOCK_111;
    QSPI_Command->DataMode           = QSPI_DATA_1_LINE;
    //QSPI_Command->NbData             =
    //QSPI_Command->DdrMode            = QSPI_DDR_MODE_DISABLE;
    //QSPI_Command->DdrHoldHalfCycle   = QSPI_DDR_HHC_ANALOG_DELAY;
    //QSPI_Command->SIOOMode           = QSPI_SIOO_INST_EVERY_CMD;
    break;
#endif  // defined(MXIC_SNOR_FAST_READ_111_CMD) || defined(MXIC_SNOR_4BYTE_ADDRESS_FAST_READ_111_CMD)

#if defined(MXIC_SNOR_FAST_READ_112_CMD) || defined(MXIC_SNOR_4BYTE_ADDRESS_FAST_READ_112_CMD)
  case MXIC_SNOR_FREAD_112 :        // 1-1-2 fast read command
    QSPI_Command->InstructionMode    = QSPI_INSTRUCTION_1_LINE;

#ifdef SUPPORT_4BYTE_ADDRESS_COMMAND
    QSPI_Command->Instruction        = MXIC_SNOR_4BYTE_ADDRESS_FAST_READ_112_CMD;
#else
    QSPI_Command->Instruction        = MXIC_SNOR_FAST_READ_112_CMD;
#endif  // SUPPORT_4BYTE_ADDRESS_COMMAND

    QSPI_Command->AddressMode        = QSPI_ADDRESS_1_LINE;
    //QSPI_Command->AddressSize        =
    //QSPI_Command->Address            =
    //QSPI_Command->AlternateByteMode  =
    //QSPI_Command->AlternateBytesSize =
    //QSPI_Command->AlternateBytes     =
    QSPI_Command->DummyCycles        = MXIC_SNOR_DUMMY_CLOCK_112;
    QSPI_Command->DataMode           = QSPI_DATA_2_LINES;
    //QSPI_Command->NbData             =
    //QSPI_Command->DdrMode            = QSPI_DDR_MODE_DISABLE;
    //QSPI_Command->DdrHoldHalfCycle   = QSPI_DDR_HHC_ANALOG_DELAY;
    //QSPI_Command->SIOOMode           = QSPI_SIOO_INST_EVERY_CMD;
    break;
#endif  // defined(MXIC_SNOR_FAST_READ_112_CMD) || defined(MXIC_SNOR_4BYTE_ADDRESS_FAST_READ_112_CMD)

#if defined(MXIC_SNOR_FAST_READ_114_CMD) || defined(MXIC_SNOR_4BYTE_ADDRESS_FAST_READ_114_CMD)
  case MXIC_SNOR_FREAD_114 :        // 1-1-4 fast read command
    QSPI_Command->InstructionMode    = QSPI_INSTRUCTION_1_LINE;

#ifdef SUPPORT_4BYTE_ADDRESS_COMMAND
    QSPI_Command->Instruction        = MXIC_SNOR_4BYTE_ADDRESS_FAST_READ_114_CMD;
#else
    QSPI_Command->Instruction        = MXIC_SNOR_FAST_READ_114_CMD;
#endif  // SUPPORT_4BYTE_ADDRESS_COMMAND

    QSPI_Command->AddressMode        = QSPI_ADDRESS_1_LINE;
    //QSPI_Command->AddressSize        =
    //QSPI_Command->Address            =
    //QSPI_Command->AlternateByteMode  =
    //QSPI_Command->AlternateBytesSize =
    //QSPI_Command->AlternateBytes     =
    QSPI_Command->DummyCycles        = MXIC_SNOR_DUMMY_CLOCK_114;
    QSPI_Command->DataMode           = QSPI_DATA_4_LINES;
    //QSPI_Command->NbData             =
    //QSPI_Command->DdrMode            = QSPI_DDR_MODE_DISABLE;
    //QSPI_Command->DdrHoldHalfCycle   = QSPI_DDR_HHC_ANALOG_DELAY;
    //QSPI_Command->SIOOMode           = QSPI_SIOO_INST_EVERY_CMD;
    break;
#endif  // defined(MXIC_SNOR_FAST_READ_114_CMD) || defined(MXIC_SNOR_4BYTE_ADDRESS_FAST_READ_114_CMD)

#if defined(MXIC_SNOR_FAST_READ_122_CMD) || defined(MXIC_SNOR_4BYTE_ADDRESS_FAST_READ_122_CMD)
  case MXIC_SNOR_FREAD_122 :        // 1-2-2 fast read command
    QSPI_Command->InstructionMode    = QSPI_INSTRUCTION_1_LINE;

#ifdef SUPPORT_4BYTE_ADDRESS_COMMAND
    QSPI_Command->Instruction        = MXIC_SNOR_4BYTE_ADDRESS_FAST_READ_122_CMD;
#else
    QSPI_Command->Instruction        = MXIC_SNOR_FAST_READ_122_CMD;
#endif  // SUPPORT_4BYTE_ADDRESS_COMMAND

    QSPI_Command->AddressMode        = QSPI_ADDRESS_2_LINES;
    //QSPI_Command->AddressSize        =
    //QSPI_Command->Address            =
    //QSPI_Command->AlternateByteMode  =
    //QSPI_Command->AlternateBytesSize =
    //QSPI_Command->AlternateBytes     =
    QSPI_Command->DummyCycles        = MXIC_SNOR_DUMMY_CLOCK_122;
    QSPI_Command->DataMode           = QSPI_DATA_2_LINES;
    //QSPI_Command->NbData             =
    //QSPI_Command->DdrMode            = QSPI_DDR_MODE_DISABLE;
    //QSPI_Command->DdrHoldHalfCycle   = QSPI_DDR_HHC_ANALOG_DELAY;
    //QSPI_Command->SIOOMode           = QSPI_SIOO_INST_EVERY_CMD;
    break;
#endif  // defined(MXIC_SNOR_FAST_READ_122_CMD) || defined(MXIC_SNOR_4BYTE_ADDRESS_FAST_READ_122_CMD)

#if defined(MXIC_SNOR_FAST_READ_144_CMD) || defined(MXIC_SNOR_4BYTE_ADDRESS_FAST_READ_144_CMD)
  case MXIC_SNOR_FREAD_144 :        // 1-4-4 fast read command
    QSPI_Command->InstructionMode    = QSPI_INSTRUCTION_1_LINE;

#ifdef SUPPORT_4BYTE_ADDRESS_COMMAND
    QSPI_Command->Instruction        = MXIC_SNOR_4BYTE_ADDRESS_FAST_READ_144_CMD;
#else
    QSPI_Command->Instruction        = MXIC_SNOR_FAST_READ_144_CMD;
#endif  // SUPPORT_4BYTE_ADDRESS_COMMAND

    QSPI_Command->AddressMode        = QSPI_ADDRESS_4_LINES;
    //QSPI_Command->AddressSize        =
    //QSPI_Command->Address            =

#ifdef SUPPORT_PERFORMANCE_ENHANCE_READ
    QSPI_Command->AlternateByteMode  = QSPI_ALTERNATE_BYTES_4_LINES;
    QSPI_Command->AlternateBytesSize = QSPI_ALTERNATE_BYTES_8_BITS;
    QSPI_Command->AlternateBytes     = MXIC_SNOR_PERFORMANCE_ENHANCE_INDICATOR;
#endif  // SUPPORT_PERFORMANCE_ENHANCE_READ

    QSPI_Command->DummyCycles        = (QSPI_Command->AlternateByteMode == QSPI_ALTERNATE_BYTES_NONE) ? MXIC_SNOR_DUMMY_CLOCK_144 : (MXIC_SNOR_DUMMY_CLOCK_144 - 2);
    QSPI_Command->DataMode           = QSPI_DATA_4_LINES;
    //QSPI_Command->NbData             =
    //QSPI_Command->DdrMode            = QSPI_DDR_MODE_DISABLE;
    //QSPI_Command->DdrHoldHalfCycle   = QSPI_DDR_HHC_ANALOG_DELAY;
    QSPI_Command->SIOOMode           = (QSPI_Command->AlternateByteMode == QSPI_ALTERNATE_BYTES_NONE) ? QSPI_SIOO_INST_EVERY_CMD : QSPI_SIOO_INST_ONLY_FIRST_CMD;
    break;
#endif  // defined(MXIC_SNOR_FAST_READ_144_CMD) || defined(MXIC_SNOR_4BYTE_ADDRESS_FAST_READ_144_CMD)

#if defined(MXIC_SNOR_FAST_READ_144_4DC_CMD)
  case MXIC_SNOR_FREAD_144_4DC :    // 1-4-4 fast read command with 4 dummy clock
    QSPI_Command->InstructionMode    = QSPI_INSTRUCTION_1_LINE;
    QSPI_Command->Instruction        = MXIC_SNOR_FAST_READ_144_4DC_CMD;
    QSPI_Command->AddressMode        = QSPI_ADDRESS_4_LINES;
    //QSPI_Command->AddressSize        =
    //QSPI_Command->Address            =
    //QSPI_Command->AlternateByteMode  =
    //QSPI_Command->AlternateBytesSize =
    //QSPI_Command->AlternateBytes     =
    QSPI_Command->DummyCycles        = 4;
    QSPI_Command->DataMode           = QSPI_DATA_4_LINES;
    //QSPI_Command->NbData             =
    //QSPI_Command->DdrMode            = QSPI_DDR_MODE_DISABLE;
    //QSPI_Command->DdrHoldHalfCycle   = QSPI_DDR_HHC_ANALOG_DELAY;
    //QSPI_Command->SIOOMode           = QSPI_SIOO_INST_EVERY_CMD;
    break;
#endif  // defined(MXIC_SNOR_FAST_READ_144_4DC_CMD)

#if defined(MXIC_SNOR_FAST_READ_444_CMD) || defined(MXIC_SNOR_4BYTE_ADDRESS_FAST_READ_444_CMD)
/*
 * 3 Byte or 3/4 & 4 Byte address STR 4-4-4 command
 * MXIC_SNOR_FREAD_444 mode will use 1 command only that come from 0xEA, 0xEB, 0xEC.
 * 0xEA/0xEB/0xEC dummy clock dependents on DC[1:0] define & support performance enhance read.
 * Define used command as MXIC_SNOR_FAST_READ_444_CMD
 */
  case MXIC_SNOR_FREAD_444 :        // 4-4-4 fast read QPI command
    QSPI_Command->InstructionMode    = QSPI_INSTRUCTION_4_LINES;

#ifdef SUPPORT_4BYTE_ADDRESS_COMMAND
    QSPI_Command->Instruction        = MXIC_SNOR_4BYTE_ADDRESS_FAST_READ_444_CMD;
#else
    QSPI_Command->Instruction        = MXIC_SNOR_FAST_READ_444_CMD;
#endif  //

    QSPI_Command->AddressMode        = QSPI_ADDRESS_4_LINES;
    //QSPI_Command->AddressSize        =
    //QSPI_Command->Address            =

#ifdef SUPPORT_PERFORMANCE_ENHANCE_READ
    QSPI_Command->AlternateByteMode  = QSPI_ALTERNATE_BYTES_4_LINES;
    QSPI_Command->AlternateBytesSize = QSPI_ALTERNATE_BYTES_8_BITS;
    QSPI_Command->AlternateBytes     = MXIC_SNOR_PERFORMANCE_ENHANCE_INDICATOR;
#endif  // SUPPORT_PERFORMANCE_ENHANCE_READ

    QSPI_Command->DummyCycles        = (QSPI_Command->AlternateByteMode == QSPI_ALTERNATE_BYTES_NONE) ? MXIC_SNOR_DUMMY_CLOCK_444 : (MXIC_SNOR_DUMMY_CLOCK_444 - 2);
    QSPI_Command->DataMode           = QSPI_DATA_4_LINES;
    //QSPI_Command->NbData             =
    //QSPI_Command->DdrMode            = QSPI_DDR_MODE_DISABLE;
    //QSPI_Command->DdrHoldHalfCycle   = QSPI_DDR_HHC_ANALOG_DELAY;
    QSPI_Command->SIOOMode           = (QSPI_Command->AlternateByteMode == QSPI_ALTERNATE_BYTES_NONE) ? QSPI_SIOO_INST_EVERY_CMD : QSPI_SIOO_INST_ONLY_FIRST_CMD;
    break;
#endif  // defined(MXIC_SNOR_FAST_READ_444_CMD) || defined(MXIC_SNOR_4BYTE_ADDRESS_FAST_READ_444_CMD)

#if defined(MXIC_SNOR_FAST_READ_444_4DC_CMD)
// 0x0B 1-1-1/4-4-4 fast read command with 4 dummy clock. Not support performance enhance read.
// 0xE7 1-4-4/4-4-4 fast read command with 4 dummy clock. Not support performance enhance read.
  case MXIC_SNOR_FREAD_444_4DC :    // 4-4-4 fast read command with 4 dummy clock
    QSPI_Command->InstructionMode    = QSPI_INSTRUCTION_4_LINES;
    QSPI_Command->Instruction        = MXIC_SNOR_FAST_READ_444_4DC_CMD;
    QSPI_Command->AddressMode        = QSPI_ADDRESS_4_LINES;
    //QSPI_Command->AddressSize        =
    //QSPI_Command->Address            =
    //QSPI_Command->AlternateByteMode  =
    //QSPI_Command->AlternateBytesSize =
    //QSPI_Command->AlternateBytes     =
    QSPI_Command->DummyCycles        = 4;
    QSPI_Command->DataMode           = QSPI_DATA_4_LINES;
    //QSPI_Command->NbData             =
    //QSPI_Command->DdrMode            = QSPI_DDR_MODE_DISABLE;
    //QSPI_Command->DdrHoldHalfCycle   = QSPI_DDR_HHC_ANALOG_DELAY;
    //QSPI_Command->SIOOMode           = QSPI_SIOO_INST_EVERY_CMD;
    break;
#endif  // defined(MXIC_SNOR_FAST_READ_444_4DC_CMD)
  }

  return MXIC_SNOR_ERROR_NONE;
}

/*
 * #SUPPORT_3BYTE_ADDRESS_COMMAND
 * MXIC_SNOR_NORMAL_READ_111_CMD                   0x03    // READ, Normal Read 3/4 Byte Address with 0 dummy clock; 1-1-1/1-1-8
 * MXIC_SNOR_FAST_READ_111_CMD                     0x0B    // FAST READ, Fast Read 3/4 Byte Address; 1-1-1/4-4-4
 * MXIC_SNOR_FAST_READ_112_CMD                     0x3B    // DREAD, 1I 2O Read 3/4 Byte Address; 1-1-2
 * MXIC_SNOR_FAST_READ_114_CMD                     0x6B    // QREAD, 1I 4O Read 3/4 Byte Address; 1-1-4
 * MXIC_SNOR_NORMAL_READ_118_CMD                   0x03    // READ, Normal Read 3/4 Byte Address with 0 dummy clock; 1-1-1/1-1-8
 * MXIC_SNOR_FAST_READ_122_CMD                     0xBB    // 2READ, 2 x I/O Read 3/4 Byte Address; 1-2-2
 * MXIC_SNOR_FAST_READ_144_CMD                     0xEB    // 4READ, 4 I/O Read 3/4 Byte Address; 1-4-4/4-4-4 + Enhance
 * MXIC_SNOR_FAST_READ_144_CMD1                    0xEA    // 4READ, 4 I/O Read 3/4 Byte Address Top; 1-4-4/4-4-4 + Enhance
 * MXIC_SNOR_FAST_READ_144_4DC_CMD                 0xE7    // W4READ, 4 I/O Read 3/4 Byte Address with 4 dummy clock; 1-4-4/4-4-4
 * MXIC_SNOR_FAST_READ_444_CMD                     0xEB    // 4READ, 4 I/O Read 3/4 Byte Address; 1-4-4/4-4-4 + Enhance
 * MXIC_SNOR_FAST_READ_444_CMD1                    0xEA    // 4READ, 4 I/O Read 3/4 Byte Address Top; 1-4-4/4-4-4 + Enhance
 * MXIC_SNOR_FAST_READ_444_CMD2                    0x0B    // FAST READ, Fast Read 3/4 Byte Address; 1-1-1/4-4-4 + Enhance
 * MXIC_SNOR_FAST_READ_444_4DC_CMD                 0x0B    // FAST READ, Fast Read 3/4 Byte Address; 1-1-1/4-4-4
 * MXIC_SNOR_FAST_READ_444_4DC_CMD1                0xE7    // W4READ, 4 I/O Read 3/4 Byte Address with 4 dummy clock; 1-4-4/4-4-4
 *
 * #SUPPORT_4BYTE_ADDRESS_COMMAND
 * MXIC_SNOR_4BYTE_ADDRESS_NORMAL_READ_111_CMD     0x13    // READ4B, Normal Read 4 Byte address with 0 dummy clock; 1-1-1
 * MXIC_SNOR_4BYTE_ADDRESS_FAST_READ_111_CMD       0x0C    // FAST READ4B, Fast Read 4 Byte address; 1-1-1
 * MXIC_SNOR_4BYTE_ADDRESS_FAST_READ_112_CMD       0x3C    // DREAD4B, Read by Dual Output 4 Byte address; 1-1-2
 * MXIC_SNOR_4BYTE_ADDRESS_FAST_READ_114_CMD       0x6C    // QREAD4B, Read by Quad Output 4 Byte address; 1-1-4
 * MXIC_SNOR_4BYTE_ADDRESS_FAST_READ_118_CMD       0x7C    // OREAD4B, Read by 8 x I/O 4 Byte address; 1-1-8/8S-8S-8S
 * MXIC_SNOR_4BYTE_ADDRESS_FAST_READ_122_CMD       0xBC    // 2READ4B, Read by 2 x I/O 4 Byte address; 1-2-2
 * MXIC_SNOR_4BYTE_ADDRESS_FAST_READ_144_CMD       0xEC    // 4READ4B, Read by 4 x I/O 4 Byte address; 1-4-4/4-4-4/8S-8S-8S + Enhance
 * MXIC_SNOR_4BYTE_ADDRESS_FAST_READ_444_CMD       0xEC    // 4READ4B, Read by 4 x I/O 4 Byte address; 1-4-4/4-4-4/8S-8S-8S + Enhance
 * MXIC_SNOR_4BYTE_ADDRESS_FAST_READ_888_CMD       0xEC    // 8READ,   Read by 8 x I/O 4 Byte address; 8S-8S-8S
 * MXIC_SNOR_4BYTE_ADDRESS_FAST_READ_888_CMD1      0x7C    // OREAD4B, Read by 8 x I/O 4 Byte address; 1-1-8/8S-8S-8S
 *
 * @brief  Trigger MCU Enter Memory Mapped Mode (STR)
 * @param  *Ctx : Device handle
 *         Mode : Command interface
 * @retval MXIC_xSPINORErrorTypeDef
 */
MXIC_xSPINORErrorTypeDef MXIC_SNOR_EnableMemoryMappedModeSTR(void *Ctx, MXIC_SNOR_Mode_t Mode)
{
  QSPI_CommandTypeDef      s_command;
  QSPI_MemoryMappedTypeDef s_mem_mapped_cfg;

  if(MXIC_SNOR_SetupReadCommandSTR(&s_command, Mode) != MXIC_SNOR_ERROR_NONE) return MXIC_SNOR_ERROR_NOT_SUPPORTED;

  /* Configure the memory mapped mode */
  s_mem_mapped_cfg.TimeOutActivation = QSPI_TIMEOUT_COUNTER_DISABLE;
  s_mem_mapped_cfg.TimeOutPeriod     = 0;

  if (HAL_QSPI_MemoryMapped(Ctx, &s_command, &s_mem_mapped_cfg) != HAL_OK)
  {
    return MXIC_SNOR_ERROR_MEMORY_MAPPED;
  }

  return MXIC_SNOR_ERROR_NONE;
}

/*
 * #SUPPORT_3BYTE_ADDRESS_COMMAND
 * MXIC_SNOR_NORMAL_READ_111_CMD                   0x03    // READ, Normal Read 3/4 Byte Address with 0 dummy clock; 1-1-1/1-1-8
 * MXIC_SNOR_FAST_READ_111_CMD                     0x0B    // FAST READ, Fast Read 3/4 Byte Address; 1-1-1/4-4-4
 * MXIC_SNOR_FAST_READ_112_CMD                     0x3B    // DREAD, 1I 2O Read 3/4 Byte Address; 1-1-2
 * MXIC_SNOR_FAST_READ_114_CMD                     0x6B    // QREAD, 1I 4O Read 3/4 Byte Address; 1-1-4
 * MXIC_SNOR_NORMAL_READ_118_CMD                   0x03    // READ, Normal Read 3/4 Byte Address with 0 dummy clock; 1-1-1/1-1-8
 * MXIC_SNOR_FAST_READ_122_CMD                     0xBB    // 2READ, 2 x I/O Read 3/4 Byte Address; 1-2-2
 * MXIC_SNOR_FAST_READ_144_CMD                     0xEB    // 4READ, 4 I/O Read 3/4 Byte Address; 1-4-4/4-4-4 + Enhance
 * MXIC_SNOR_FAST_READ_144_CMD1                    0xEA    // 4READ, 4 I/O Read 3/4 Byte Address Top; 1-4-4/4-4-4 + Enhance
 * MXIC_SNOR_FAST_READ_144_4DC_CMD                 0xE7    // W4READ, 4 I/O Read 3/4 Byte Address with 4 dummy clock; 1-4-4/4-4-4
 * MXIC_SNOR_FAST_READ_444_CMD                     0xEB    // 4READ, 4 I/O Read 3/4 Byte Address; 1-4-4/4-4-4 + Enhance
 * MXIC_SNOR_FAST_READ_444_CMD1                    0xEA    // 4READ, 4 I/O Read 3/4 Byte Address Top; 1-4-4/4-4-4 + Enhance
 * MXIC_SNOR_FAST_READ_444_CMD2                    0x0B    // FAST READ, Fast Read 3/4 Byte Address; 1-1-1/4-4-4 + Enhance
 * MXIC_SNOR_FAST_READ_444_4DC_CMD                 0x0B    // FAST READ, Fast Read 3/4 Byte Address; 1-1-1/4-4-4
 * MXIC_SNOR_FAST_READ_444_4DC_CMD1                0xE7    // W4READ, 4 I/O Read 3/4 Byte Address with 4 dummy clock; 1-4-4/4-4-4
 *
 * #SUPPORT_4BYTE_ADDRESS_COMMAND
 * MXIC_SNOR_4BYTE_ADDRESS_NORMAL_READ_111_CMD     0x13    // READ4B, Normal Read 4 Byte address with 0 dummy clock; 1-1-1
 * MXIC_SNOR_4BYTE_ADDRESS_FAST_READ_111_CMD       0x0C    // FAST READ4B, Fast Read 4 Byte address; 1-1-1
 * MXIC_SNOR_4BYTE_ADDRESS_FAST_READ_112_CMD       0x3C    // DREAD4B, Read by Dual Output 4 Byte address; 1-1-2
 * MXIC_SNOR_4BYTE_ADDRESS_FAST_READ_114_CMD       0x6C    // QREAD4B, Read by Quad Output 4 Byte address; 1-1-4
 * MXIC_SNOR_4BYTE_ADDRESS_FAST_READ_118_CMD       0x7C    // OREAD4B, Read by 8 x I/O 4 Byte address; 1-1-8/8S-8S-8S
 * MXIC_SNOR_4BYTE_ADDRESS_FAST_READ_122_CMD       0xBC    // 2READ4B, Read by 2 x I/O 4 Byte address; 1-2-2
 * MXIC_SNOR_4BYTE_ADDRESS_FAST_READ_144_CMD       0xEC    // 4READ4B, Read by 4 x I/O 4 Byte address; 1-4-4/4-4-4/8S-8S-8S + Enhance
 * MXIC_SNOR_4BYTE_ADDRESS_FAST_READ_444_CMD       0xEC    // 4READ4B, Read by 4 x I/O 4 Byte address; 1-4-4/4-4-4/8S-8S-8S + Enhance
 * MXIC_SNOR_4BYTE_ADDRESS_FAST_READ_888_CMD       0xEC    // 8READ,   Read by 8 x I/O 4 Byte address; 8S-8S-8S
 * MXIC_SNOR_4BYTE_ADDRESS_FAST_READ_888_CMD1      0x7C    // OREAD4B, Read by 8 x I/O 4 Byte address; 1-1-8/8S-8S-8S
 *
 * @brief  STR read command indirect mode
 * @param  *Ctx            : Device handle
 *         Handle->Mode    : Command interface
 *         Handle->Address : Read start address
 *         Handle->Size    : Read size in Byte
 *         Handle->pBuffer : Buffer pointer for store data
 * @retval MXIC_xSPINORErrorTypeDef
 */
MXIC_xSPINORErrorTypeDef MXIC_SNOR_ReadSTR(void *Ctx, MXIC_SNOR_CommandHandle_t *Handle)
{
  QSPI_CommandTypeDef s_command;
  QSPI_HandleTypeDef *hQSPI = Ctx;

  if(MXIC_SNOR_SetupReadCommandSTR(&s_command, Handle->Mode) != MXIC_SNOR_ERROR_NONE)
  {
    return MXIC_SNOR_ERROR_NOT_SUPPORTED;
  } // Check if command setup to performance enhance read, Indirect mode don't execute enhance read
  else if(s_command.AlternateByteMode != QSPI_ALTERNATE_BYTES_NONE)
  {
    s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    s_command.DummyCycles      += 2;
    s_command.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;
  }
  s_command.Address = Handle->Address;
  s_command.NbData  = Handle->Size;

  /* Configure the command */
  if (HAL_QSPI_Command(Ctx, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return MXIC_SNOR_ERROR_COMMAND;
  }

  /* Set S# timing for Read command */
  MODIFY_REG(hQSPI->Instance->DCR, QUADSPI_DCR_CSHT, QSPI_CS_HIGH_TIME_1_CYCLE);

  /* Reception of the data */
  if (HAL_QSPI_Receive(Ctx, Handle->pBuffer, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return MXIC_SNOR_ERROR_RECEIVE;
  }

  /* Restore S# timing for nonRead commands */
  MODIFY_REG(hQSPI->Instance->DCR, QUADSPI_DCR_CSHT, QSPI_CS_HIGH_TIME_4_CYCLE);

  return MXIC_SNOR_ERROR_NONE;
}

#ifdef SUPPORT_DTR
/*
 * #SUPPORT_3BYTE_ADDRESS_COMMAND
 * MXIC_SNOR_FAST_READ_111_DTR_CMD                 0x0D    // FASTDTRD, Fast DTR Read 3/4 Byte Address; 1-1-1
 * MXIC_SNOR_FAST_READ_122_DTR_CMD                 0xBD    // 2DTRD, Dual I/O DTR Read 3/4 Byte Address; 1-2-2
 * MXIC_SNOR_FAST_READ_144_DTR_CMD                 0xED    // 4DTRD, Quad I/O DTR Read 3/4 Byte Address; 1-4-4/4-4-4 + Enhance
 * MXIC_SNOR_FAST_READ_444_DTR_CMD                 0xED    // 4DTRD, Quad I/O DTR Read 3/4 Byte Address; 1-4-4/4-4-4 + Enhance
 *
 * #SUPPORT_4BYTE_ADDRESS_COMMAND
 * MXIC_SNOR_4BYTE_ADDRESS_FAST_READ_111_DTR_CMD   0x0E    // FRDTRD4B, Fast DTR Read 4 Byte address; 1-1-1
 * MXIC_SNOR_4BYTE_ADDRESS_FAST_READ_122_DTR_CMD   0xBE    // 2DTRD4B, DTR read by 2 x I/O 4 Byte address; 1-2-2
 * MXIC_SNOR_4BYTE_ADDRESS_FAST_READ_144_DTR_CMD   0xEE    // 4DTRD4B, Quad I/O DTR Read 4 Byte address; 1-4-4/4-4-4/8D-8D-8D + Enhance
 * MXIC_SNOR_4BYTE_ADDRESS_FAST_READ_444_DTR_CMD   0xEE    // 4DTRD4B, Quad I/O DTR Read 4 Byte address; 1-4-4/4-4-4/8D-8D-8D + Enhance
 * MXIC_SNOR_4BYTE_ADDRESS_FAST_READ_888_DTR_CMD   0xEE    // 8DTRD,   Octal I/O DTR Read 4 Byte address; 8D-8D-8D
 *
 * @brief  Setup DTR read command for MMP mode usage.
 *         MMP mode running in performance enhance mode if device supported.
 *         Indirect read will issue command on every read cycle.
 *         Exist 1 dummy clock cycle difference between MMP & Indirect mode.
 * @param  *QSPI_Command : Pointer for setup command to
 *         Mode          : Command interface
 * @retval MXIC_xSPINORErrorTypeDef
 *         OperationType not setup
 */
static MXIC_xSPINORErrorTypeDef MXIC_SNOR_SetupReadCommandDTR(QSPI_CommandTypeDef *QSPI_Command, MXIC_SNOR_Mode_t Mode)
{
  /* Setup command structure */
  //QSPI_Command->InstructionMode    =
  //QSPI_Command->Instruction        =
  //QSPI_Command->AddressMode        =

#if defined(SUPPORT_4BYTE_ADDRESS_COMMAND) || defined(SUPPORT_34BYTE_ADDRESS_SWITCH) || defined(FORCE_USE_4BYTE_ADDRESS)
  QSPI_Command->AddressSize        = QSPI_ADDRESS_32_BITS;
#else
  QSPI_Command->AddressSize        = QSPI_ADDRESS_24_BITS;
#endif  // defined(SUPPORT_4BYTE_ADDRESS_COMMAND) || defined(SUPPORT_34BYTE_ADDRESS_SWITCH)

  //QSPI_Command->Address            = Handle->Address;
  QSPI_Command->AlternateByteMode  = QSPI_ALTERNATE_BYTES_NONE;     // Modifyed by SUPPORT_PERFORMANCE_ENHANCE_READ
  //QSPI_Command->AlternateBytesSize =
  //QSPI_Command->AlternateBytes     =
  //QSPI_Command->DummyCycles        =
  //QSPI_Command->DataMode           =
  //QSPI_Command->NbData             = Handle->Size;
  QSPI_Command->DdrMode            = QSPI_DDR_MODE_ENABLE;
  QSPI_Command->DdrHoldHalfCycle   = QSPI_DDR_HHC_HALF_CLK_DELAY;
  QSPI_Command->SIOOMode           = QSPI_SIOO_INST_EVERY_CMD;      // Modifyed by SUPPORT_PERFORMANCE_ENHANCE_READ

  switch(Mode.IO)
  {
  default :
  //case MXIC_SNOR_NREAD_111 :        // 1-1-1 normal read command with 0 dummy cycle, Not support DTR mode
  //case MXIC_SNOR_FREAD_112 :        // 1-1-2 fast read command, Not support DTR mode
  //case MXIC_SNOR_FREAD_114 :        // 1-1-4 fast read command, Not support DTR mode
  //case MXIC_SNOR_FREAD_118 :        // 1-1-8 fast read command or Parallel mode command, Not support DTR mode      0x03, 0x7C
  //case MXIC_SNOR_FREAD_144_4DC :    // 1-4-4 fast read command with 4 dummy clock, Not support DTR mode            0xE7
  //case MXIC_SNOR_FREAD_444_4DC :    // 4-4-4 fast read QPI command with 4 dummy clock, Not support DTR mode        0x0B, 0xE7
  //case MXIC_SNOR_FREAD_888 :        // 8-8-8 fast read Octal command, H/W IP not supported
    return MXIC_SNOR_ERROR_NOT_SUPPORTED;

#if defined(MXIC_SNOR_FAST_READ_111_DTR_CMD) || defined(MXIC_SNOR_4BYTE_ADDRESS_FAST_READ_111_DTR_CMD)
  case MXIC_SNOR_NREAD_111 :        // 1-1-1 normal read command with 0 dummy cycle, Not support DTR mode
  case MXIC_SNOR_FREAD_111 :        // 1-1-1 fast read command, Power on H/W default setting
    QSPI_Command->InstructionMode    = QSPI_INSTRUCTION_1_LINE;

#ifdef SUPPORT_4BYTE_ADDRESS_COMMAND
    QSPI_Command->Instruction        = MXIC_SNOR_4BYTE_ADDRESS_FAST_READ_111_DTR_CMD;
#else
    QSPI_Command->Instruction        = MXIC_SNOR_FAST_READ_111_DTR_CMD;
#endif  // SUPPORT_4BYTE_ADDRESS_COMMAND

    QSPI_Command->AddressMode        = QSPI_ADDRESS_1_LINE;
    //QSPI_Command->AddressSize        =
    //QSPI_Command->Address            =
    //QSPI_Command->AlternateByteMode  = QSPI_ALTERNATE_BYTES_NONE;
    //QSPI_Command->AlternateBytesSize =
    //QSPI_Command->AlternateBytes     =
    QSPI_Command->DummyCycles        = MXIC_SNOR_DUMMY_CLOCK_111_DTR;
    QSPI_Command->DataMode           = QSPI_DATA_1_LINE;
    //QSPI_Command->NbData             =
    //QSPI_Command->DdrMode            = QSPI_DDR_MODE_DISABLE;
    //QSPI_Command->DdrHoldHalfCycle   = QSPI_DDR_HHC_ANALOG_DELAY;
    //QSPI_Command->SIOOMode           = QSPI_SIOO_INST_EVERY_CMD;
    break;
#endif  // defined(MXIC_SNOR_FAST_READ_111_DTR_CMD) || defined(MXIC_SNOR_4BYTE_ADDRESS_FAST_READ_111_DTR_CMD)

#if defined(MXIC_SNOR_FAST_READ_122_DTR_CMD) || defined(MXIC_SNOR_4BYTE_ADDRESS_FAST_READ_122_DTR_CMD)
  case MXIC_SNOR_FREAD_112 :        // 1-1-2 fast read command, Not support DTR mode
  case MXIC_SNOR_FREAD_122 :        // 1-2-2 fast read command
    QSPI_Command->InstructionMode    = QSPI_INSTRUCTION_1_LINE;

#ifdef SUPPORT_4BYTE_ADDRESS_COMMAND
    QSPI_Command->Instruction        = MXIC_SNOR_4BYTE_ADDRESS_FAST_READ_122_DTR_CMD;
#else
    QSPI_Command->Instruction        = MXIC_SNOR_FAST_READ_122_DTR_CMD;
#endif  // SUPPORT_4BYTE_ADDRESS_COMMAND

    QSPI_Command->AddressMode        = QSPI_ADDRESS_2_LINES;
    //QSPI_Command->AddressSize        =
    //QSPI_Command->Address            =
    //QSPI_Command->AlternateByteMode  = QSPI_ALTERNATE_BYTES_NONE;
    //QSPI_Command->AlternateBytesSize =
    //QSPI_Command->AlternateBytes     =
    QSPI_Command->DummyCycles        = MXIC_SNOR_DUMMY_CLOCK_122_DTR;
    QSPI_Command->DataMode           = QSPI_DATA_2_LINES;
    //QSPI_Command->NbData             =
    //QSPI_Command->DdrMode            = QSPI_DDR_MODE_DISABLE;
    //QSPI_Command->DdrHoldHalfCycle   = QSPI_DDR_HHC_ANALOG_DELAY;
    //QSPI_Command->SIOOMode           = QSPI_SIOO_INST_EVERY_CMD;
    break;
#endif  // defined(MXIC_SNOR_FAST_READ_122_DTR_CMD) || defined(MXIC_SNOR_4BYTE_ADDRESS_FAST_READ_122_DTR_CMD)

#if defined(MXIC_SNOR_FAST_READ_144_DTR_CMD) || defined(MXIC_SNOR_4BYTE_ADDRESS_FAST_READ_144_DTR_CMD)
  case MXIC_SNOR_FREAD_114 :        // 1-1-4 fast read command
  case MXIC_SNOR_FREAD_144 :        // 1-4-4 fast read command
    QSPI_Command->InstructionMode    = QSPI_INSTRUCTION_1_LINE;

#ifdef SUPPORT_4BYTE_ADDRESS_COMMAND
    QSPI_Command->Instruction        = MXIC_SNOR_4BYTE_ADDRESS_FAST_READ_144_DTR_CMD;
#else
    QSPI_Command->Instruction        = MXIC_SNOR_FAST_READ_144_DTR_CMD;
#endif  // SUPPORT_4BYTE_ADDRESS_COMMAND

    QSPI_Command->AddressMode        = QSPI_ADDRESS_4_LINES;
    //QSPI_Command->AddressSize        =
    //QSPI_Command->Address            =

#ifdef SUPPORT_PERFORMANCE_ENHANCE_READ
    QSPI_Command->AlternateByteMode  = QSPI_ALTERNATE_BYTES_4_LINES;
    QSPI_Command->AlternateBytesSize = QSPI_ALTERNATE_BYTES_8_BITS;
    QSPI_Command->AlternateBytes     = MXIC_SNOR_PERFORMANCE_ENHANCE_INDICATOR;
#endif  // SUPPORT_PERFORMANCE_ENHANCE_READ

    QSPI_Command->DummyCycles        = (QSPI_Command->AlternateByteMode == QSPI_ALTERNATE_BYTES_NONE) ? MXIC_SNOR_DUMMY_CLOCK_144_DTR : (MXIC_SNOR_DUMMY_CLOCK_144_DTR - 1);
    QSPI_Command->DataMode           = QSPI_DATA_4_LINES;
    //QSPI_Command->NbData             =
    //QSPI_Command->DdrMode            = QSPI_DDR_MODE_DISABLE;
    //QSPI_Command->DdrHoldHalfCycle   = QSPI_DDR_HHC_ANALOG_DELAY;
    QSPI_Command->SIOOMode           = (QSPI_Command->AlternateByteMode == QSPI_ALTERNATE_BYTES_NONE) ? QSPI_SIOO_INST_EVERY_CMD : QSPI_SIOO_INST_ONLY_FIRST_CMD;
    break;
#endif  // defined(MXIC_SNOR_FAST_READ_144_DTR_CMD) || defined(MXIC_SNOR_4BYTE_ADDRESS_FAST_READ_144_DTR_CMD)

#if defined(MXIC_SNOR_FAST_READ_444_DTR_CMD) || defined(MXIC_SNOR_4BYTE_ADDRESS_FAST_READ_444_DTR_CMD)
  case MXIC_SNOR_FREAD_444 :        // 4-4-4 fast read QPI command
    QSPI_Command->InstructionMode    = QSPI_INSTRUCTION_4_LINES;

#ifdef SUPPORT_4BYTE_ADDRESS_COMMAND
    QSPI_Command->Instruction        = MXIC_SNOR_4BYTE_ADDRESS_FAST_READ_444_DTR_CMD;
#else
    QSPI_Command->Instruction        = MXIC_SNOR_FAST_READ_444_DTR_CMD;
#endif  // SUPPORT_4BYTE_ADDRESS_COMMAND

    QSPI_Command->AddressMode        = QSPI_ADDRESS_4_LINES;
    //QSPI_Command->AddressSize        =
    //QSPI_Command->Address            =

#ifdef SUPPORT_PERFORMANCE_ENHANCE_READ
    QSPI_Command->AlternateByteMode  = QSPI_ALTERNATE_BYTES_4_LINES;
    QSPI_Command->AlternateBytesSize = QSPI_ALTERNATE_BYTES_8_BITS;
    QSPI_Command->AlternateBytes     = MXIC_SNOR_PERFORMANCE_ENHANCE_INDICATOR;
#endif  // SUPPORT_PERFORMANCE_ENHANCE_READ

    QSPI_Command->DummyCycles        = (QSPI_Command->AlternateByteMode == QSPI_ALTERNATE_BYTES_NONE) ? MXIC_SNOR_DUMMY_CLOCK_444_DTR : (MXIC_SNOR_DUMMY_CLOCK_444_DTR - 1);
    QSPI_Command->DataMode           = QSPI_DATA_4_LINES;
    //QSPI_Command->NbData             =
    //QSPI_Command->DdrMode            = QSPI_DDR_MODE_DISABLE;
    //QSPI_Command->DdrHoldHalfCycle   = QSPI_DDR_HHC_ANALOG_DELAY;
    QSPI_Command->SIOOMode           = (QSPI_Command->AlternateByteMode == QSPI_ALTERNATE_BYTES_NONE) ? QSPI_SIOO_INST_EVERY_CMD : QSPI_SIOO_INST_ONLY_FIRST_CMD;
    break;
#endif  // defined(MXIC_SNOR_FAST_READ_444_DTR_CMD) || defined(MXIC_SNOR_4BYTE_ADDRESS_FAST_READ_444_DTR_CMD)
  }

  return MXIC_SNOR_ERROR_NONE;
}

/*
 * #SUPPORT_3BYTE_ADDRESS_COMMAND
 * MXIC_SNOR_FAST_READ_111_DTR_CMD                 0x0D    // FASTDTRD, Fast DTR Read 3/4 Byte Address; 1-1-1
 * MXIC_SNOR_FAST_READ_122_DTR_CMD                 0xBD    // 2DTRD, Dual I/O DTR Read 3/4 Byte Address; 1-2-2
 * MXIC_SNOR_FAST_READ_144_DTR_CMD                 0xED    // 4DTRD, Quad I/O DTR Read 3/4 Byte Address; 1-4-4/4-4-4 + Enhance
 * MXIC_SNOR_FAST_READ_444_DTR_CMD                 0xED    // 4DTRD, Quad I/O DTR Read 3/4 Byte Address; 1-4-4/4-4-4 + Enhance
 *
 * #SUPPORT_4BYTE_ADDRESS_COMMAND
 * MXIC_SNOR_4BYTE_ADDRESS_FAST_READ_111_DTR_CMD   0x0E    // FRDTRD4B, Fast DTR Read 4 Byte address; 1-1-1
 * MXIC_SNOR_4BYTE_ADDRESS_FAST_READ_122_DTR_CMD   0xBE    // 2DTRD4B, DTR read by 2 x I/O 4 Byte address; 1-2-2
 * MXIC_SNOR_4BYTE_ADDRESS_FAST_READ_144_DTR_CMD   0xEE    // 4DTRD4B, Quad I/O DTR Read 4 Byte address; 1-4-4/4-4-4/8D-8D-8D + Enhance
 * MXIC_SNOR_4BYTE_ADDRESS_FAST_READ_444_DTR_CMD   0xEE    // 4DTRD4B, Quad I/O DTR Read 4 Byte address; 1-4-4/4-4-4/8D-8D-8D + Enhance
 *
 * @brief  Trigger MCU Enter Memory Mapped Mode (DTR)
 * @param  *Ctx         : Device handle
 *         Handle->Mode : Command interface
 * @retval MXIC_xSPINORErrorTypeDef
 */
MXIC_xSPINORErrorTypeDef MXIC_SNOR_EnableMemoryMappedModeDTR(void *Ctx, MXIC_SNOR_Mode_t Mode)
{
  QSPI_CommandTypeDef      s_command;
  QSPI_MemoryMappedTypeDef s_mem_mapped_cfg;

  if(MXIC_SNOR_SetupReadCommandDTR(&s_command, Mode) != MXIC_SNOR_ERROR_NONE) return MXIC_SNOR_ERROR_NOT_SUPPORTED;

  /* Configure the memory mapped mode */
  s_mem_mapped_cfg.TimeOutActivation = QSPI_TIMEOUT_COUNTER_DISABLE;
  s_mem_mapped_cfg.TimeOutPeriod     = 0;

  if (HAL_QSPI_MemoryMapped(Ctx, &s_command, &s_mem_mapped_cfg) != HAL_OK)
  {
    return MXIC_SNOR_ERROR_MEMORY_MAPPED;
  }

  return MXIC_SNOR_ERROR_NONE;
}

/*
 * #SUPPORT_3BYTE_ADDRESS_COMMAND
 * MXIC_SNOR_FAST_READ_111_DTR_CMD                 0x0D    // FASTDTRD, Fast DTR Read 3/4 Byte Address; 1-1-1
 * MXIC_SNOR_FAST_READ_122_DTR_CMD                 0xBD    // 2DTRD, Dual I/O DTR Read 3/4 Byte Address; 1-2-2
 * MXIC_SNOR_FAST_READ_144_DTR_CMD                 0xED    // 4DTRD, Quad I/O DTR Read 3/4 Byte Address; 1-4-4/4-4-4
 * MXIC_SNOR_FAST_READ_444_DTR_CMD                 0xED    // 4DTRD, Quad I/O DTR Read 3/4 Byte Address; 1-4-4/4-4-4
 *
 * #SUPPORT_4BYTE_ADDRESS_COMMAND
 * MXIC_SNOR_4BYTE_ADDRESS_FAST_READ_111_DTR_CMD   0x0E    // FRDTRD4B, Fast DTR Read 4 Byte address; 1-1-1
 * MXIC_SNOR_4BYTE_ADDRESS_FAST_READ_122_DTR_CMD   0xBE    // 2DTRD4B, DTR read by 2 x I/O 4 Byte address; 1-2-2
 * MXIC_SNOR_4BYTE_ADDRESS_FAST_READ_144_DTR_CMD   0xEE    // 4DTRD4B, Quad I/O DTR Read 4 Byte address; 1-4-4/4-4-4/8D-8D-8D
 * MXIC_SNOR_4BYTE_ADDRESS_FAST_READ_444_DTR_CMD   0xEE    // 4DTRD4B, Quad I/O DTR Read 4 Byte address; 1-4-4/4-4-4/8D-8D-8D
 *
 * @brief  DTR read command indirect mode
 * @param  *Ctx            : Device handle
 *         Handle->Mode    : Command interface
 *         Handle->Address : Read start address
 *         Handle->Size    : Read size in Byte
 *         Handle->pBuffer : Buffer pointer for store data
 * @retval MXIC_xSPINORErrorTypeDef
 */
MXIC_xSPINORErrorTypeDef MXIC_SNOR_ReadDTR(void *Ctx, MXIC_SNOR_CommandHandle_t *Handle)
{
  QSPI_CommandTypeDef s_command;

  if(MXIC_SNOR_SetupReadCommandDTR(&s_command, Handle->Mode) != MXIC_SNOR_ERROR_NONE)
  {
    return MXIC_SNOR_ERROR_NOT_SUPPORTED;
  } // Check if command setup to performance enhance read, Indirect mode don't execute enhance read
  else if(s_command.AlternateBytes != QSPI_ALTERNATE_BYTES_NONE)
  {
    s_command.AlternateBytes = QSPI_ALTERNATE_BYTES_NONE;
    s_command.DummyCycles   += 1;
    s_command.SIOOMode       = QSPI_SIOO_INST_EVERY_CMD;
  }
  s_command.Address = Handle->Address;
  s_command.NbData  = Handle->Size;

  /* Configure the command */
  if (HAL_QSPI_Command(Ctx, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return MXIC_SNOR_ERROR_COMMAND;
  }

  /* Reception of the data */
  if (HAL_QSPI_Receive(Ctx, Handle->pBuffer, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return MXIC_SNOR_ERROR_RECEIVE;
  }

  return MXIC_SNOR_ERROR_NONE;
}
#endif  // SUPPORT_DTR


/**************************************************************************************************
 * Macronix xSPI-NOR Flash Page Program Command Pool
 *************************************************************************************************/
/*
 * #SUPPORT_3BYTE_ADDRESS_COMMAND
 * MXIC_SNOR_PAGE_PROG_111_CMD                     0x02   // PP, Page Program 3/4 Byte Address; 1-1-1/4-4-4/1-1-8
 * MXIC_SNOR_CONTINUOUSLY_PROG_111_CMD             0xAD   // CP, Continuously Program 2 Byte data + 3/4 Byte Address; 1-1-1
 * MXIC_SNOR_PAGE_PROG_114_CMD                     0x32   // QPP, 1I4P Page Program 3 Byte Address; 1-1-4
 * MXIC_SNOR_PAGE_PROG_118_CMD                     0x02   // PP, Page Program 3/4 Byte Address; 1-1-1/4-4-4/1-1-8
 * MXIC_SNOR_PAGE_PROG_144_CMD                     0x38   // 4PP, Quad Page Program 3/4 Byte Address; 1-4-4
 * MXIC_SNOR_PAGE_PROG_444_CMD                     0x02   // PP, Page Program 3/4 Byte Address; 1-1-1/4-4-4/1-1-8
 *
 * #SUPPORT_4BYTE_ADDRESS_COMMAND
 * MXIC_SNOR_4BYTE_ADDRESS_PAGE_PROG_111_CMD       0x12   // PP4B, Page Program 4 Byte address; 1-1-1/4-4-4/8S-8S-8S/8D-8D-8D
 * MXIC_SNOR_4BYTE_ADDRESS_PAGE_PROG_144_CMD       0x3E   // 4PP4B, Quad Input Page Program 4 Byte address; 1-4-4
 * MXIC_SNOR_4BYTE_ADDRESS_PAGE_PROG_444_CMD       0x12   // PP4B, Page Program 4 Byte address; 1-1-1/4-4-4/8S-8S-8S/8D-8D-8D
 * MXIC_SNOR_4BYTE_ADDRESS_PAGE_PROG_888_CMD       0x12   // PP4B, Page Program 4 Byte address; 1-1-1/4-4-4/8S-8S-8S/8D-8D-8D
 *
 * @brief  Page Program command; max. write 1 page data to Flash
 *         This driver not support MXIC_SNOR_CONTINUOUSLY_PROG_111_CMD (0xAD) command.
 *         Due to it can replace by MXIC_SNOR_PAGE_PROG_111_CMD (0x02) command.
 *         User need modify this program to support it if required.
 * @param  *Ctx            : Device handle
 *         Handle->Mode.IO : Program Mode
 *         Handle->Address : Program start address
 *         Handle->Size    : Program size in Byte
 *         Handle->pBuffer : Buffer pointer of write data
 * @retval MXIC_xSPINORErrorTypeDef
 */
MXIC_xSPINORErrorTypeDef MXIC_SNOR_PageProgramSTR(void *Ctx, MXIC_SNOR_CommandHandle_t *Handle)
{
  QSPI_CommandTypeDef s_command;

  /* Setup command structure */
  //s_command.InstructionMode    =
  //s_command.Instruction        =
  //s_command.AddressMode        =

#if defined(SUPPORT_4BYTE_ADDRESS_COMMAND) || defined(SUPPORT_34BYTE_ADDRESS_SWITCH) || defined(FORCE_USE_4BYTE_ADDRESS)
  s_command.AddressSize       = QSPI_ADDRESS_32_BITS;
#else
  s_command.AddressSize       = QSPI_ADDRESS_24_BITS;
#endif  // defined(SUPPORT_4BYTE_ADDRESS_COMMAND) || defined(SUPPORT_34BYTE_ADDRESS_SWITCH) || defined(FORCE_USE_4BYTE_ADDRESS)

  s_command.Address            = Handle->Address;
  s_command.AlternateByteMode  = QSPI_ALTERNATE_BYTES_NONE;
  //s_command.AlternateBytesSize =
  //s_command.AlternateBytes     =
  s_command.DummyCycles        = 0;
  //s_command.DataMode           =
  s_command.NbData             = Handle->Size;
  s_command.DdrMode            = QSPI_DDR_MODE_DISABLE;
  s_command.DdrHoldHalfCycle   = QSPI_DDR_HHC_ANALOG_DELAY;
  s_command.SIOOMode           = QSPI_SIOO_INST_EVERY_CMD;

  switch(Handle->Mode.IO)
  {
  default :     // Default use 1-1-1 page program command; 1-1-8/8-8-8 H/W IP not supported

#if defined(MXIC_SNOR_PAGE_PROG_111_CMD) || defined(MXIC_SNOR_4BYTE_ADDRESS_PAGE_PROG_111_CMD)
  case MXIC_SNOR_NREAD_111 :        // 1-1-1 normal read command with 0 dummy cycle;          0x03, 0x13
  case MXIC_SNOR_FREAD_111 :        // 1-1-1 fast read command, Power on H/W default setting; 0x0B, 0x0C, 0x0D, 0x0E
  case MXIC_SNOR_FREAD_112 :        // 1-1-2 fast read command;                               0x3B, 0x3C
  case MXIC_SNOR_FREAD_122 :        // 1-2-2 fast read command;                               0xBB, 0xBC, 0xBD, 0xBE
    s_command.InstructionMode    = QSPI_INSTRUCTION_1_LINE;

#ifdef SUPPORT_4BYTE_ADDRESS_COMMAND
    s_command.Instruction        = MXIC_SNOR_4BYTE_ADDRESS_PAGE_PROG_111_CMD;
#else
    s_command.Instruction        = MXIC_SNOR_PAGE_PROG_111_CMD;
#endif  // SUPPORT_4BYTE_ADDRESS_COMMAND

    s_command.AddressMode        = QSPI_ADDRESS_1_LINE;
    //s_command.AddressSize        =
    //s_command.Address            =
    //s_command.AlternateByteMode  = QSPI_ALTERNATE_BYTES_NONE;
    //s_command.AlternateBytesSize =
    //s_command.AlternateBytes     =
    //s_command.DummyCycles        =
    s_command.DataMode           = QSPI_DATA_1_LINE;
    //s_command.NbData             =
    //s_command.DdrMode            = QSPI_DDR_MODE_DISABLE;
    //s_command.DdrHoldHalfCycle   = QSPI_DDR_HHC_ANALOG_DELAY;
    //s_command.SIOOMode           = QSPI_SIOO_INST_EVERY_CMD;
    break;
#endif  // defined(MXIC_SNOR_PAGE_PROG_111_CMD) || defined(MXIC_SNOR_4BYTE_ADDRESS_PAGE_PROG_111_CMD)

#if defined(MXIC_SNOR_PAGE_PROG_114_CMD)
  case MXIC_SNOR_FREAD_114 :        // 1-1-4 fast read command;                               0x6B, 0x6C
    s_command.InstructionMode    = QSPI_INSTRUCTION_1_LINE;
    s_command.Instruction        = MXIC_SNOR_PAGE_PROG_114_CMD;
    s_command.AddressMode        = QSPI_ADDRESS_1_LINE;
    //s_command.AddressSize       =
    //s_command.Address            =
    //s_command.AlternateByteMode  =
    //s_command.AlternateBytesSize =
    //s_command.AlternateBytes     =
    //s_command.DummyCycles        =
    s_command.DataMode           = QSPI_DATA_4_LINES;
    //s_command.NbData             = Handle->Size;
    //s_command.DdrMode            = QSPI_DDR_MODE_DISABLE;
    //s_command.DdrHoldHalfCycle   = QSPI_DDR_HHC_ANALOG_DELAY;
    //s_command.SIOOMode           = QSPI_SIOO_INST_EVERY_CMD;
    break;
#endif  // defined(MXIC_SNOR_PAGE_PROG_114_CMD)

#if defined(MXIC_SNOR_PAGE_PROG_144_CMD) || defined(MXIC_SNOR_4BYTE_ADDRESS_PAGE_PROG_144_CMD)
#ifndef MXIC_SNOR_PAGE_PROG_114_CMD
  case MXIC_SNOR_FREAD_114 :        // 1-1-4 fast read command;                               0x6B, 0x6C
#endif
  case MXIC_SNOR_FREAD_144 :        // 1-4-4 fast read command;                               0xEA, 0xEB, 0xEC, 0xED, 0xEE
  case MXIC_SNOR_FREAD_144_4DC :    // 1-4-4 fast read command with 4 dummy clock;            0xE7
    s_command.InstructionMode    = QSPI_INSTRUCTION_1_LINE;

#ifdef SUPPORT_4BYTE_ADDRESS_COMMAND
    s_command.Instruction        = MXIC_SNOR_4BYTE_ADDRESS_PAGE_PROG_144_CMD;
#else
    s_command.Instruction        = MXIC_SNOR_PAGE_PROG_144_CMD;
#endif  // SUPPORT_4BYTE_ADDRESS_COMMAND

    s_command.AddressMode        = QSPI_ADDRESS_4_LINES;
    //s_command.AddressSize        =
    //s_command.Address            =
    //s_command.AlternateByteMode  = QSPI_ALTERNATE_BYTES_NONE;
    //s_command.AlternateBytesSize =
    //s_command.AlternateBytes     =
    //s_command.DummyCycles        =
    s_command.DataMode           = QSPI_DATA_4_LINES;
    //s_command.NbData             =
    //s_command.DdrMode            = QSPI_DDR_MODE_DISABLE;
    //s_command.DdrHoldHalfCycle   = QSPI_DDR_HHC_ANALOG_DELAY;
    //s_command.SIOOMode           = QSPI_SIOO_INST_EVERY_CMD;
    break;
#endif  // defined(MXIC_SNOR_PAGE_PROG_144_CMD) || defined(MXIC_SNOR_4BYTE_ADDRESS_PAGE_PROG_144_CMD)

#if defined(MXIC_SNOR_PAGE_PROG_444_CMD) || defined(MXIC_SNOR_4BYTE_ADDRESS_PAGE_PROG_444_CMD)
  case MXIC_SNOR_FREAD_444 :        // 4-4-4 fast read QPI command;                           0x0B, 0xEA, 0xEB, 0xEC, 0xED, 0xEE
  case MXIC_SNOR_FREAD_444_4DC :    // 4-4-4 fast read QPI command with 4 dummy clock;        0x0B, 0xE7
    s_command.InstructionMode    = QSPI_INSTRUCTION_4_LINES;

#ifdef SUPPORT_4BYTE_ADDRESS_COMMAND
    s_command.Instruction        = MXIC_SNOR_4BYTE_ADDRESS_PAGE_PROG_444_CMD;
#else
    s_command.Instruction        = MXIC_SNOR_PAGE_PROG_444_CMD;
#endif  // SUPPORT_4BYTE_ADDRESS_COMMAND

    s_command.AddressMode        = QSPI_ADDRESS_4_LINES;
    //s_command.AddressSize        =
    //s_command.Address            =
    //s_command.AlternateByteMode  = QSPI_ALTERNATE_BYTES_NONE;
    //s_command.AlternateBytesSize =
    //s_command.AlternateBytes     =
    //s_command.DummyCycles        =
    s_command.DataMode           = QSPI_DATA_4_LINES;
    //s_command.NbData             =
    //s_command.DdrMode            = QSPI_DDR_MODE_DISABLE;
    //s_command.DdrHoldHalfCycle   = QSPI_DDR_HHC_ANALOG_DELAY;
    //s_command.SIOOMode           = QSPI_SIOO_INST_EVERY_CMD;
    break;
#endif // defineed(MXIC_SNOR_PAGE_PROG_444_CMD) || defined(MXIC_SNOR_4BYTE_ADDRESS_PAGE_PROG_444_CMD)
  }

  /* Configure the command */
  if (HAL_QSPI_Command(Ctx, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return MXIC_SNOR_ERROR_COMMAND;
  }

  /* Transmission of the data */
  if (HAL_QSPI_Transmit(Ctx, Handle->pBuffer, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return MXIC_SNOR_ERROR_TRANSMIT;
  }

  return MXIC_SNOR_ERROR_NONE;
}

/**************************************************************************************************
 * Macronix xSPI-NOR Flash Erase Command Pool
 *************************************************************************************************/
/*
 * #SUPPORT_3BYTE_ADDRESS_COMMAND
 * MXIC_SNOR_ERASE_4K_CMD                          0x20   // SE, Sector Erase 4KB 3/4 Byte Address; 1-1-0/4-4-0
 * MXIC_SNOR_ERASE_32K_CMD                         0x52   // BE32K, Block Erase 32KB 3/4 Byte Address; 1-1-0/4-4-0
 * MXIC_SNOR_ERASE_64K_CMD                         0xD8   // BE, Block Erase 64KB 3/4 Byte Address; 1-1-0/4-4-0
 *
 * #SUPPORT_4BYTE_ADDRESS_COMMAND
 * MXIC_SNOR_4BYTE_ADDRESS_ERASE_4K_CMD            0x21   // SE4B, Sector Erase 4KB 4 Byte address; 1-1-0/4-4-0/8S-8S-0/8D-8D-0
 * MXIC_SNOR_4BYTE_ADDRESS_ERASE_32K_CMD           0x5C   // BE32K4B, Block Erase 32KB 4 Byte address; 1-1-0/4-4-0
 * MXIC_SNOR_4BYTE_ADDRESS_ERASE_64K_CMD           0xDC   // BE4B, Block Erase 64KB 4 Byte address; 1-1-0/4-4-0/8S-8S-0/8D-8D-0
 *
 * #CHIP_ERASE
 * MXIC_SNOR_ERASE_CHIP_CMD                        0x60   // CE, Chip Erase 0 Byte Address; 1-0-0/4-0-0/8S-0-0/8D-0-0
 * MXIC_SNOR_ERASE_CHIP_CMD1                       0xC7   // CE, Chip Erase 0 Byte Address; 1-0-0/4-0-0/8S-0-0/8D-0-0
 *
 * @brief  Erases the specified address of block.
 * @param  *Ctx            : Device handle
 *         Handle->Mode    : Command interface
 *         Handle->Address : Erase address
 *         Handle->Size    : Block size; MXIC_SNOR_EraseTypeDef
 * @retval MXIC_xSPINORErrorTypeDef
 */
MXIC_xSPINORErrorTypeDef MXIC_SNOR_BlockErase(void *Ctx, MXIC_SNOR_CommandHandle_t *Handle)
{
  QSPI_CommandTypeDef s_command;

  // Check which size for erase & setup instruction
  switch(Handle->Size)
  {
  default :
    return MXIC_SNOR_ERROR_PARAMETER;

#if defined(MXIC_SNOR_ERASE_4K_CMD) || defined(MXIC_SNOR_4BYTE_ADDRESS_ERASE_4K_CMD)
  case MXIC_SNOR_ERASE_4K   :       // 4K size Sector erase
#ifdef MXIC_SNOR_4BYTE_ADDRESS_ERASE_4K_CMD
    s_command.Instruction        = MXIC_SNOR_4BYTE_ADDRESS_ERASE_4K_CMD;
#else
    s_command.Instruction        = MXIC_SNOR_ERASE_4K_CMD;
#endif  // MXIC_SNOR_4BYTE_ADDRESS_ERASE_4K_CMD
    break;
#endif  // defined(MXIC_SNOR_ERASE_4K_CMD) || defined(MXIC_SNOR_4BYTE_ADDRESS_ERASE_4K_CMD)

#if defined(MXIC_SNOR_ERASE_32K_CMD) || defined(MXIC_SNOR_4BYTE_ADDRESS_ERASE_32K_CMD)
  case MXIC_SNOR_ERASE_32K  :       // 32K size Block erase
#ifdef MXIC_SNOR_4BYTE_ADDRESS_ERASE_32K_CMD
    s_command.Instruction        = MXIC_SNOR_4BYTE_ADDRESS_ERASE_32K_CMD;
#else
    s_command.Instruction        = MXIC_SNOR_ERASE_32K_CMD;
#endif  // MXIC_SNOR_4BYTE_ADDRESS_ERASE_32K_CMD
    break;
#endif  // defined(MXIC_SNOR_ERASE_32K_CMD) || defined(MXIC_SNOR_4BYTE_ADDRESS_ERASE_32K_CMD)

#if defined(MXIC_SNOR_ERASE_64K_CMD) || defined(MXIC_SNOR_4BYTE_ADDRESS_ERASE_64K_CMD)
  case MXIC_SNOR_ERASE_64K  :       // 64K size Block erase
#ifdef MXIC_SNOR_4BYTE_ADDRESS_ERASE_64K_CMD
    s_command.Instruction        = MXIC_SNOR_4BYTE_ADDRESS_ERASE_64K_CMD;
#else
    s_command.Instruction        = MXIC_SNOR_ERASE_64K_CMD;
#endif  // MXIC_SNOR_4BYTE_ADDRESS_ERASE_64K_CMD
    break;
#endif  // defined(MXIC_SNOR_ERASE_64K_CMD) || defined(MXIC_SNOR_4BYTE_ADDRESS_ERASE_64K_CMD)

#ifdef MXIC_SNOR_ERASE_CHIP_CMD
  case MXIC_SNOR_ERASE_CHIP :       // Whole chip erase
    return MXIC_SNOR_ChipErase(Ctx, Handle->Mode);
#endif  // MXIC_SNOR_ERASE_CHIP_CMD
  }

  /* Setup command structure */
  s_command.InstructionMode    = ((Handle->Mode.IO == MXIC_SNOR_FREAD_444) || (Handle->Mode.IO == MXIC_SNOR_FREAD_444_4DC)) ? QSPI_INSTRUCTION_4_LINES : QSPI_INSTRUCTION_1_LINE;
  //s_command.Instruction        =
  s_command.AddressMode        = ((Handle->Mode.IO == MXIC_SNOR_FREAD_444) || (Handle->Mode.IO == MXIC_SNOR_FREAD_444_4DC)) ? QSPI_ADDRESS_4_LINES : QSPI_ADDRESS_1_LINE;

#if defined(SUPPORT_4BYTE_ADDRESS_COMMAND) || defined(SUPPORT_34BYTE_ADDRESS_SWITCH) || defined(FORCE_USE_4BYTE_ADDRESS)
  s_command.AddressSize        = QSPI_ADDRESS_32_BITS;
#else
  s_command.AddressSize        = QSPI_ADDRESS_24_BITS;
#endif  // defined(SUPPORT_4BYTE_ADDRESS_COMMAND) || defined(SUPPORT_34BYTE_ADDRESS_SWITCH) || defined(FORCE_USE_4BYTE_ADDRESS)

  s_command.Address            = Handle->Address;
  s_command.AlternateByteMode  = QSPI_ALTERNATE_BYTES_NONE;
  //s_command.AlternateBytesSize =
  //s_command.AlternateBytes     =
  s_command.DummyCycles        = 0;
  s_command.DataMode           = QSPI_DATA_NONE;
  //s_command.NbData             =
  s_command.DdrMode            = QSPI_DDR_MODE_DISABLE;
  s_command.DdrHoldHalfCycle   = QSPI_DDR_HHC_ANALOG_DELAY;
  s_command.SIOOMode           = QSPI_SIOO_INST_EVERY_CMD;

  /* Configure the command */
  if (HAL_QSPI_Command(Ctx, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return MXIC_SNOR_ERROR_COMMAND;
  }

  return MXIC_SNOR_ERROR_NONE;
}

#ifdef MXIC_SNOR_ERASE_CHIP_CMD
/*
 * MXIC_SNOR_ERASE_CHIP_CMD                        0x60   // CE, Chip Erase 0 Byte Address; 1-0-0/4-0-0/8S-0-0/8D-0-0
 * MXIC_SNOR_ERASE_CHIP_CMD1                       0xC7   // CE, Chip Erase 0 Byte Address; 1-0-0/4-0-0/8S-0-0/8D-0-0
 *
 * @brief  Whole chip erase.
 * @param  *Ctx  : Device handle
 *          Mode : Command interface
 * @retval MXIC_xSPINORErrorTypeDef
 */
MXIC_xSPINORErrorTypeDef MXIC_SNOR_ChipErase(void *Ctx, MXIC_SNOR_Mode_t Mode)
{
  return x00_Command(Ctx, MXIC_SNOR_ERASE_CHIP_CMD, Mode.IO);
}
#endif  // MXIC_SNOR_ERASE_CHIP_CMD


/**************************************************************************************************
 * Macronix xSPI-NOR Flash Write Buffer Access Command Pool; RWW (Read While Write)
 *************************************************************************************************/
#ifdef MXIC_SNOR_4BYTE_ADDRESS_WRITE_BUF_READ_CMD
/*
 * MXIC_SNOR_4BYTE_ADDRESS_WRITE_BUF_READ_CMD      0x25   // RDBUF, Write Buffer Read    (4 Byte address); 1-1-1/4-4-4/8S-8S-8S/8D-8D-8D
 * @brief  Write/Page Buffer Read
 * @param  *Ctx            : Device handle
 *         Handle->Mode    : Command interface
 *         Handle->Address : Read start address
 *         Handle->Size    : Read size in Byte
 *         Handle->pBuffer : Buffer pointer for store data
 * @retval MXIC_xSPINORErrorTypeDef
 *         *Handle->pBuffer = Data
 */
MXIC_xSPINORErrorTypeDef MXIC_SNOR_WriteBufferRead(void *Ctx, MXIC_SNOR_CommandHandle_t *Handle)
{
  QSPI_CommandTypeDef s_command;

  /* Setup command structure */
  SetupRegisterCommandSPIQPIxxx(&s_command, MXIC_SNOR_4BYTE_ADDRESS_WRITE_BUF_READ_CMD, Handle->Mode.IO, Handle->Size);
  s_command.Address            = Handle->Address;
  s_command.DummyCycles        = 8;                     // 8 dummy clock for 1-1-1/4-4-4

  /* Configure the command */
  if (HAL_QSPI_Command(Ctx, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return MXIC_SNOR_ERROR_COMMAND;
  }

  /* Reception of the data */
  if (HAL_QSPI_Receive(Ctx, Handle->pBuffer, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return MXIC_SNOR_ERROR_RECEIVE;
  }

  return MXIC_SNOR_ERROR_NONE;
}
#endif  // MXIC_SNOR_4BYTE_ADDRESS_WRITE_BUF_READ_CMD

#ifdef MXIC_SNOR_4BYTE_ADDRESS_WRITE_BUF_INITIAL_CMD
/*
 * MXIC_SNOR_4BYTE_ADDRESS_WRITE_BUF_INITIAL_CMD   0x22   // WRBI, Write Buffer Initial  (4 Byte address); 1-1-1/4-4-4/8S-8S-8S/8D-8D-8D
 * @brief  Write/Page Buffer Initial
 * @param  *Ctx            : Device handle
 *         Handle->Mode    : Command interface
 *         Handle->Address : Write start address
 *         Handle->Size    : Write size in Byte
 *         Handle->pBuffer : Data Buffer pointer
 * @retval MXIC_xSPINORErrorTypeDef
 */
MXIC_xSPINORErrorTypeDef MXIC_SNOR_WriteBufferInitial(void *Ctx, MXIC_SNOR_CommandHandle_t *Handle)
{
  QSPI_CommandTypeDef s_command;

  /* Setup command structure */
  SetupRegisterCommandSPIQPIxxx(&s_command, MXIC_SNOR_4BYTE_ADDRESS_WRITE_BUF_INITIAL_CMD, Handle->Mode.IO, Handle->Size);
  s_command.Address            = Handle->Address;
  s_command.DummyCycles        = 0;

  /* Configure the command */
  if (HAL_QSPI_Command(Ctx, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return MXIC_SNOR_ERROR_COMMAND;
  }

  if(Handle->Size)
  {
    /* Transmission of the data */
    if (HAL_QSPI_Transmit(Ctx, Handle->pBuffer, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
      return MXIC_SNOR_ERROR_TRANSMIT;
    }
  }

  return MXIC_SNOR_ERROR_NONE;
}
#endif  // MXIC_SNOR_4BYTE_ADDRESS_WRITE_BUF_INITIAL_CMD

#ifdef MXIC_SNOR_4BYTE_ADDRESS_WRITE_BUF_CONTINUE_CMD
/*
 * MXIC_SNOR_4BYTE_ADDRESS_WRITE_BUF_CONTINUE_CMD  0x24   // WRCT, Write Buffer Continue (4 Byte address); 1-1-1/4-4-4/8S-8S-8S/8D-8D-8D
 * @brief  Write/Page Buffer Continue
 * @param  *Ctx            : Device handle
 *         Handle->Mode    : Command interface
 *         Handle->Address : Write start address
 *         Handle->Size    : Write size in Byte
 *         Handle->pBuffer : Data Buffer pointer
 * @retval MXIC_xSPINORErrorTypeDef
 */
MXIC_xSPINORErrorTypeDef MXIC_SNOR_WriteBufferContinue(void *Ctx, MXIC_SNOR_CommandHandle_t *Handle)
{
  QSPI_CommandTypeDef s_command;

  /* Setup command structure */
  SetupRegisterCommandSPIQPIxxx(&s_command, MXIC_SNOR_4BYTE_ADDRESS_WRITE_BUF_CONTINUE_CMD, Handle->Mode.IO, Handle->Size);
  s_command.Address            = Handle->Address;
  s_command.DummyCycles        = 0;

  /* Configure the command */
  if (HAL_QSPI_Command(Ctx, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return MXIC_SNOR_ERROR_COMMAND;
  }

  if(Handle->Size)
  {
    /* Transmission of the data */
    if (HAL_QSPI_Transmit(Ctx, Handle->pBuffer, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
      return MXIC_SNOR_ERROR_TRANSMIT;
    }
  }

  return MXIC_SNOR_ERROR_NONE;
}
#endif  // MXIC_SNOR_4BYTE_ADDRESS_WRITE_BUF_CONTINUE_CMD

#ifdef MXIC_SNOR_4BYTE_ADDRESS_WRITE_BUF_CONFIRM_CMD
/*
 * MXIC_SNOR_4BYTE_ADDRESS_WRITE_BUF_CONFIRM_CMD   0x31   // WRCF, Write Buffer Confirm  (4 Byte address); 1-0-0/4-0-0/8S-0-0/8D-0-0
 * @brief  Write/Page Buffer Confirm
 * @param  *Ctx : Device handle
 *         Mode : Command interface
 * @retval MXIC_xSPINORErrorTypeDef
 */
MXIC_xSPINORErrorTypeDef MXIC_SNOR_WriteBufferConfirm(void *Ctx, MXIC_SNOR_Mode_t Mode)
{
  return x00_Command(Ctx, MXIC_SNOR_4BYTE_ADDRESS_WRITE_BUF_CONFIRM_CMD, Mode.IO);
}
#endif  // MXIC_SNOR_4BYTE_ADDRESS_WRITE_BUF_CONFIRM_CMD


/**************************************************************************************************
 * Macronix xSPI-NOR Flash Register Access Command Pool
 *************************************************************************************************/
#ifdef MXIC_SNOR_READ_STATUS_REG_CMD
/*
 * MXIC_SNOR_READ_STATUS_REG_CMD                   0x05   // RDSR, Read Status Register; 1-0-1/4-0-4/8S-8S-8S/8D-8D-8D
 * @brief  Read Status Register
 * @param  *Ctx            : Device handle
 *         Handle->Mode    : Command interface
 *         Handle->pBuffer : Buffer pointer
 * @retval MXIC_xSPINORErrorTypeDef
 *         *Handle->pBuffer = Status Register value
 */
MXIC_xSPINORErrorTypeDef MXIC_SNOR_ReadStatusRegister(void *Ctx, MXIC_SNOR_CommandHandle_t *Handle)
{
  QSPI_CommandTypeDef s_command;

  /* Setup command structure */
  SetupRegisterCommandSPIQPIx0x(&s_command, MXIC_SNOR_READ_STATUS_REG_CMD, Handle->Mode.IO, sizeof(MXIC_SNOR_StatusRegister_t));

  /* Configure the command */
  if (HAL_QSPI_Command(Ctx, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return MXIC_SNOR_ERROR_COMMAND;
  }

  /* Reception of the data */
  if (HAL_QSPI_Receive(Ctx, Handle->pBuffer, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return MXIC_SNOR_ERROR_RECEIVE;
  }
  return MXIC_SNOR_ERROR_NONE;
}
#endif  // MXIC_SNOR_READ_STATUS_REG_CMD

#ifdef MXIC_SNOR_READ_CONFIGURATION_REG_CMD
/*
 * MXIC_SNOR_READ_CONFIGURATION_REG_CMD            0x15   // RDCR, Read Configuration Register; 1-0-1/4-0-4/8S-8S-8S/8D-8D-8D
 * @brief  Read Flash Configuration register value
 * @param  *Ctx            : Device handle
 *         Handle->Mode    : Command interface
 *         Handle->Size    : Read size in Byte
 *         Handle->pBuffer : Buffer pointer
 * @retval MXIC_xSPINORErrorTypeDef
 *         *Handle->pBuffer = Configuration Register value
 */
MXIC_xSPINORErrorTypeDef MXIC_SNOR_ReadConfigurationRegister(void *Ctx, MXIC_SNOR_CommandHandle_t *Handle)
{
  QSPI_CommandTypeDef s_command;

  /* Setup command structure */
  SetupRegisterCommandSPIQPIx0x(&s_command, MXIC_SNOR_READ_CONFIGURATION_REG_CMD, Handle->Mode.IO, Handle->Size);

  /* Configure the command */
  if (HAL_QSPI_Command(Ctx, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return MXIC_SNOR_ERROR_COMMAND;
  }

  /* Reception of the data */
  if (HAL_QSPI_Receive(Ctx, Handle->pBuffer, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return MXIC_SNOR_ERROR_RECEIVE;
  }
  return MXIC_SNOR_ERROR_NONE;
}

/*
 * @brief  Write Configuration Register need a procedure (1-0-1/4-0-4/8-8-8)
 *         RDSR -> WREN -> WRSR + SR + CR (+ CR2)
 *         8-8-8 WREN -> WRCR + Address (1)
 * @param  *Ctx            : Device handle
 *         Handle->Mode       : Command interface
 *         Handle->Size       : Write Configuration Register size in Byte
 *         Handle->pBuffer[0] : CR value
 *         Handle->pBuffer[1] : CR2 value; Some device support CR2 reference to data sheet
 * @retval MXIC_xSPINORErrorTypeDef
 */
MXIC_xSPINORErrorTypeDef MXIC_SNOR_WriteConfigurationRegister(void *Ctx, MXIC_SNOR_CommandHandle_t *Handle)
{
  MXIC_xSPINORErrorTypeDef ret = MXIC_SNOR_ERROR_NONE;
  uint32_t Size;      // SPI/QPI need write SR first then write CR
  static uint8_t  Data[3];   // SR, CR, CR2

  // Save input value
  Size    = Handle->Size + 1;     // Total Register size SR + CR + CR2
  Data[1] = Handle->pBuffer[0];   // CR
  Data[2] = Handle->pBuffer[1];   // CR2

  // Read out Status Register to Data[0]
  Handle->pBuffer = &Data[0];
  ret = MXIC_SNOR_ReadStatusRegister(Ctx, Handle);

  /* Write Status Configuration Register, SR -> CR -> CR2  */
  Handle->Size    = Size;
  Handle->pBuffer = Data;

  if(ret == MXIC_SNOR_ERROR_NONE)
  {
    /* Enable write operations */
    ret = MXIC_SNOR_WriteEnable(Ctx, Handle->Mode);
    if(ret == MXIC_SNOR_ERROR_NONE)
    {
      ret = MXIC_SNOR_WriteStatusConfigurationRegister(Ctx, Handle);
      if(ret == MXIC_SNOR_ERROR_NONE)
      {
        ret = MXIC_SNOR_AutoPollingMemReady(Ctx, Handle->Mode);
      }
    }
  }
  /* Return BSP status */
  return ret;
}
#endif  // MXIC_SNOR_READ_CONFIGURATION_REG_CMD

#ifdef MXIC_SNOR_WRITE_STATUS_CONFIGURATION_REG_CMD
/*
 * MXIC_SNOR_WRITE_STATUS_CONFIGURATION_REG_CMD    0x01   // WRSR, Write Status/Configuration Register; 1-0-1/4-0-4/8S-8S-8S/8D-8D-8D
 * @brief  Write Flash Status & Configuration Register
 *         1-0-1/4-0-4 maximum can write 3 Byte : SR, CR, CR1
 *         8-8-8 address 0 = SR, 1 = CR
 * @param  *Ctx               : Device handle
 *         Handle->Mode       : Interface mode
 *         Handle->Address    : 8-8-8 need a address 0 = Status, 1 = Configuration
 *         Handle->Size       : Write data length in Byte
 *         Handle->pBuffer[0] : Status Register value
 *         Handle->pBuffer[1] : Configuration Register value
 *         Handle->pBuffer[2] : Configuration Register 2 value; Some device support CR2
 * @retval MXIC_xSPINORErrorTypeDef
 */
MXIC_xSPINORErrorTypeDef MXIC_SNOR_WriteStatusConfigurationRegister(void *Ctx, MXIC_SNOR_CommandHandle_t *Handle)
{
  QSPI_CommandTypeDef s_command;

  /* Setup command structure */
  SetupRegisterCommandSPIQPIx0x(&s_command, MXIC_SNOR_WRITE_STATUS_CONFIGURATION_REG_CMD, Handle->Mode.IO, Handle->Size);

  /* Configure the write volatile configuration register command */
  if (HAL_QSPI_Command(Ctx, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return MXIC_SNOR_ERROR_COMMAND;
  }

  /* Transmission of the data */
  if (HAL_QSPI_Transmit(Ctx, Handle->pBuffer, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return MXIC_SNOR_ERROR_TRANSMIT;
  }

  return MXIC_SNOR_ERROR_NONE;
}
#endif  // MXIC_SNOR_WRITE_STATUS_CONFIGURATION_REG_CMD

#ifdef MXIC_SNOR_WRITE_ENABLE_CMD
/*
 * MXIC_SNOR_WRITE_ENABLE_CMD                      0x06   // WREN, Write Enable; 1-0-0/4-0-0/8S-0-0/8D-0-0
 * @brief  Send Write Enable command to Flash and polling WEL bit = 1
 * @param  *Ctx            : Device handle
 *         Handle->Mode    : Interface Mode
 * @retval MXIC_xSPINORErrorTypeDef
 */
MXIC_xSPINORErrorTypeDef MXIC_SNOR_WriteEnable(void *Ctx, MXIC_SNOR_Mode_t Mode)
{
  MXIC_xSPINORErrorTypeDef ret;

  ret = x00_Command(Ctx, MXIC_SNOR_WRITE_ENABLE_CMD, Mode.IO);

  if(ret == MXIC_SNOR_ERROR_NONE)
  {
    /* Polling WEL = 1, Handle->Parameter.Type32 = Timeout */
    uint8_t MatchMask[2] = {MXIC_SNOR_SR_WEL, MXIC_SNOR_SR_WEL};
    MXIC_SNOR_CommandHandle_t CommandHandle;

    CommandHandle.Mode             = Mode;
    CommandHandle.pBuffer          = MatchMask;
    CommandHandle.Parameter.Type32 = HAL_QPSI_TIMEOUT_DEFAULT_VALUE;
    ret = MXIC_SNOR_PollingStatusRegister(Ctx, &CommandHandle);
  }

  return ret;
}
#endif  // MXIC_SNOR_WRITE_ENABLE_CMD

#ifdef MXIC_SNOR_WRITE_DISABLE_CMD
/*
 * MXIC_SNOR_WRITE_DISABLE_CMD                     0x04   // WRDI, Write Disable; 1-0-0/4-0-0/8S-0-0/8D-0-0
 * @brief  This function reset the (WEL) Write Enable Latch bit.
 * @param  *Ctx            : Device handle
 * @retval MXIC_xSPINORErrorTypeDef
 */
MXIC_xSPINORErrorTypeDef MXIC_SNOR_WriteDisable(void *Ctx, MXIC_SNOR_Mode_t Mode)
{
  return x00_Command(Ctx, MXIC_SNOR_WRITE_DISABLE_CMD, Mode.IO);
}
#endif  // MXIC_SNOR_WRITE_DISABLE_CMD

#ifdef MXIC_SNOR_READ_ID_CMD
/*
 * MXIC_SNOR_READ_ID_CMD                           0x9F   // RDID, Read IDentification; 1-0-1/1-0-8/8S-8S-8S/8D-8D-8D
 *
 * @brief  Read Flash 3 Byte IDs. Manufacturer ID, Memory type, Memory density
 * @param  *Ctx            : Device handle
 *         Handle->pBuffer : Buffer pointer
 * @retval MXIC_xSPINORErrorTypeDef
 *         Handle->pBuffer[0] = Manufacturer ID
 *         Handle->pBuffer[1] = Memory type
 *         Handle->pBuffer[2] = Memory density
 */
MXIC_xSPINORErrorTypeDef MXIC_SNOR_ReadID(void *Ctx, MXIC_SNOR_CommandHandle_t *Handle)
{
  QSPI_CommandTypeDef s_command;

  /* Setup command structure */
  SetupRegisterCommandSPIQPIx0x(&s_command, MXIC_SNOR_READ_ID_CMD, MXIC_SNOR_FREAD_111, 3);

  /* Configure the command */
  if (HAL_QSPI_Command(Ctx, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return MXIC_SNOR_ERROR_COMMAND;
  }

  /* Reception of the data */
  if (HAL_QSPI_Receive(Ctx, Handle->pBuffer, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return MXIC_SNOR_ERROR_RECEIVE;
  }

  return MXIC_SNOR_ERROR_NONE;
}
#endif  // MXIC_SNOR_READ_ID_CMD

#ifdef MXIC_SNOR_READ_ELECTRONIC_ID_CMD
/*
 * MXIC_SNOR_READ_ELECTRONIC_ID_CMD                0xAB   // RES, Read Electronic ID; 1-1-1/1-1-8/4-4-4
 * @brief  Read Flash Electronic Signature
 * @param  *Ctx            : Device handle
 *         Handle->Mode    : Command interface
 *         Handle->pBuffer : Buffer pointer
 * @retval MXIC_xSPINORErrorTypeDef
 *         Handle->pBuffer[0] = Signature
 */
MXIC_xSPINORErrorTypeDef MXIC_SNOR_ReadElectronicSignature(void *Ctx, MXIC_SNOR_CommandHandle_t *Handle)
{
  QSPI_CommandTypeDef s_command;

  /* Setup command structure */
  SetupRegisterCommandSPIQPIxxx(&s_command, MXIC_SNOR_READ_ELECTRONIC_ID_CMD, Handle->Mode.IO, 1);
  s_command.AddressSize = QSPI_ADDRESS_24_BITS;         // Dummy address 3 Byte always
  s_command.Address     = 0;                            // Dummy address value
  s_command.DummyCycles = 0;

  /* Configure the command */
  if (HAL_QSPI_Command(Ctx, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return MXIC_SNOR_ERROR_COMMAND;
  }

  /* Reception of the data */
  if (HAL_QSPI_Receive(Ctx, Handle->pBuffer, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return MXIC_SNOR_ERROR_RECEIVE;
  }

  return MXIC_SNOR_ERROR_NONE;
}
#endif  // MXIC_SNOR_READ_ELECTRONIC_ID_CMD

#if defined(MXIC_SNOR_READ_ELECTRONIC_MANFACTURER_ID_CMD)  || defined(MXIC_SNOR_READ_ELECTRONIC_MANFACTURER_ID_CMD2) || \
    defined(MXIC_SNOR_READ_ELECTRONIC_MANFACTURER_ID_CMD4) || defined(MXIC_SNOR_READ_ELECTRONIC_MANFACTURER_ID_CMD4D)
/*
 * MXIC_SNOR_READ_ELECTRONIC_MANFACTURER_ID_CMD    0x90   // REMS, Read Electronic Manufacturer ID & Device ID; 1-1-1/1-1-8
 * MXIC_SNOR_READ_ELECTRONIC_MANFACTURER_ID_CMD2   0xEF   // REMS2, Read Electronic Manufacturer ID & Device ID; 1-2-2
 * MXIC_SNOR_READ_ELECTRONIC_MANFACTURER_ID_CMD4   0xDF   // REMS4, Read Electronic Manufacturer ID & Device ID; 1-4-4
 * MXIC_SNOR_READ_ELECTRONIC_MANFACTURER_ID_CMD4D  0xCF   // REMS4D, Read Electronic Manufacturer ID & Device ID; 1-4-4 DTR
 *
 * @brief  Read Flash Electronic Manufacturer & Device ID
 * @param  *Ctx            : Device handle
 *         Handle->Mode    : Command interface
 *         Handle->Address : Address bit 0 Select ID output order
 *                           0 = Manufacturer ID first
 *                           1 = Device ID first
 *         Handle->pBuffer : Buffer pointer
 * @retval MXIC_xSPINORErrorTypeDef
 *         Handle->Address bit 0 = 0 :
 *          Handle->pBuffer[0] = Manufacturer ID
 *          Handle->pBuffer[1] = Device ID
 *         Handle->Address bit 0 = 1 :
 *          Handle->pBuffer[0] = Device ID
 *          Handle->pBuffer[1] = Manufacturer ID
 */
MXIC_xSPINORErrorTypeDef MXIC_SNOR_ReadElectronicManufacturerDeviceID(void *Ctx, MXIC_SNOR_CommandHandle_t *Handle)
{
  QSPI_CommandTypeDef s_command;

  /* Setup command structure */
  switch(Handle->Mode.IO)
  {
  default :     // Default use 1-1-1 command
  //case MXIC_SNOR_FREAD_444 :
  //case MXIC_SNOR_FREAD_444_4DC :
  //case MXIC_SNOR_FREAD_118 :
  //case MXIC_SNOR_FREAD_888 :        // 8-8-8 fast read Octal command, H/W IP not supported

#if defined(MXIC_SNOR_READ_ELECTRONIC_MANFACTURER_ID_CMD)
  case MXIC_SNOR_NREAD_111 :
  case MXIC_SNOR_FREAD_111 :
    SetupRegisterCommandSPIQPIxxx(&s_command, MXIC_SNOR_READ_ELECTRONIC_MANFACTURER_ID_CMD, MXIC_SNOR_FREAD_111, 2);
    s_command.AddressSize = QSPI_ADDRESS_24_BITS;
    s_command.Address     = Handle->Address;
    s_command.DummyCycles = 0;
    break;
#endif  // defined(MXIC_SNOR_READ_ELECTRONIC_MANFACTURER_ID_CMD)

#if defined(MXIC_SNOR_READ_ELECTRONIC_MANFACTURER_ID_CMD2)
  case MXIC_SNOR_FREAD_112 :
  case MXIC_SNOR_FREAD_122 :
    s_command.InstructionMode    = QSPI_INSTRUCTION_1_LINE;
    s_command.Instruction        = MXIC_SNOR_READ_ELECTRONIC_MANFACTURER_ID_CMD2;
    s_command.AddressMode        = QSPI_ADDRESS_2_LINES;
    s_command.AddressSize        = QSPI_ADDRESS_24_BITS;
    s_command.Address            = Handle->Address;
    s_command.AlternateByteMode  = QSPI_ALTERNATE_BYTES_NONE;
    //s_command.AlternateBytesSize =
    //s_command.AlternateBytes     =
    s_command.DummyCycles        = 0;
    s_command.DataMode           = QSPI_DATA_2_LINES;
    s_command.NbData             = 2;
    s_command.DdrMode            = QSPI_DDR_MODE_DISABLE;
    s_command.DdrHoldHalfCycle   = QSPI_DDR_HHC_ANALOG_DELAY;
    s_command.SIOOMode           = QSPI_SIOO_INST_EVERY_CMD;
    break;
#endif  // defined(MXIC_SNOR_READ_ELECTRONIC_MANFACTURER_ID_CMD2)

#if defined(MXIC_SNOR_READ_ELECTRONIC_MANFACTURER_ID_CMD4) || defined(MXIC_SNOR_READ_ELECTRONIC_MANFACTURER_ID_CMD4D)
  case MXIC_SNOR_FREAD_114 :
  case MXIC_SNOR_FREAD_144 :
  case MXIC_SNOR_FREAD_144_4DC :
    s_command.InstructionMode    = QSPI_INSTRUCTION_1_LINE;

#if defined(MXIC_SNOR_READ_ELECTRONIC_MANFACTURER_ID_CMD4D)
    s_command.Instruction        = (Handle->Mode.Rate == MXIC_SNOR_STR) ? MXIC_SNOR_READ_ELECTRONIC_MANFACTURER_ID_CMD4 : MXIC_SNOR_READ_ELECTRONIC_MANFACTURER_ID_CMD4D;
#else
    s_command.Instruction        = MXIC_SNOR_READ_ELECTRONIC_MANFACTURER_ID_CMD4;
#endif

    s_command.AddressMode        = QSPI_ADDRESS_4_LINES;
    s_command.AddressSize        = QSPI_ADDRESS_24_BITS;
    s_command.Address            = Handle->Address;
    s_command.AlternateByteMode  = QSPI_ALTERNATE_BYTES_NONE;
    //s_command.AlternateBytesSize =
    //s_command.AlternateBytes     =
    s_command.DummyCycles        = 0;
    s_command.DataMode           = QSPI_DATA_4_LINES;
    s_command.NbData             = 2;

#if defined(MXIC_SNOR_READ_ELECTRONIC_MANFACTURER_ID_CMD4D)
    s_command.DdrMode            = (Handle->Mode.Rate == MXIC_SNOR_STR) ? QSPI_DDR_MODE_DISABLE : QSPI_DDR_MODE_ENABLE;
    s_command.DdrHoldHalfCycle   = (Handle->Mode.Rate == MXIC_SNOR_STR) ? QSPI_DDR_HHC_ANALOG_DELAY : QSPI_DDR_HHC_HALF_CLK_DELAY;
#else
    s_command.DdrMode            = QSPI_DDR_MODE_DISABLE;
    s_command.DdrHoldHalfCycle   = QSPI_DDR_HHC_ANALOG_DELAY;
#endif

    s_command.SIOOMode           = QSPI_SIOO_INST_EVERY_CMD;
    break;
#endif  // defined(MXIC_SNOR_READ_ELECTRONIC_MANFACTURER_ID_CMD4) || defined(MXIC_SNOR_READ_ELECTRONIC_MANFACTURER_ID_CMD4D)
  }

  /* Configure the command */
  if (HAL_QSPI_Command(Ctx, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return MXIC_SNOR_ERROR_COMMAND;
  }

  /* Reception of the data */
  if (HAL_QSPI_Receive(Ctx, Handle->pBuffer, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return MXIC_SNOR_ERROR_RECEIVE;
  }

  return MXIC_SNOR_ERROR_NONE;
}
#endif  // defined(MXIC_SNOR_READ_ELECTRONIC_MANFACTURER_ID_CMD) ...

#ifdef MXIC_SNOR_MULTIPLE_IO_READ_ID_CMD
/*
 * MXIC_SNOR_MULTIPLE_IO_READ_ID_CMD               0xAF   // QPIID, QPI ID Read; QPI 4-0-4
 *
 * @brief  4-0-4 format Read Flash 3 Byte IDs. Manufacturer ID, Memory type, Memory density
 * @param  *Ctx            : Device handle
 *         Handle->pBuffer : Buffer pointer
 * @retval MXIC_xSPINORErrorTypeDef
 *         Handle->pBuffer[0] = Manufacturer ID
 *         Handle->pBuffer[1] = Memory type
 *         Handle->pBuffer[2] = Memory density
 */
MXIC_xSPINORErrorTypeDef MXIC_SNOR_QPIIDRead(void *Ctx, MXIC_SNOR_CommandHandle_t *Handle)
{
  QSPI_CommandTypeDef s_command;

  /* Setup command structure */
  SetupRegisterCommandSPIQPIx0x(&s_command, MXIC_SNOR_MULTIPLE_IO_READ_ID_CMD, MXIC_SNOR_FREAD_444, 3);

  /* Configure the command */
  if (HAL_QSPI_Command(Ctx, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return MXIC_SNOR_ERROR_COMMAND;
  }

  /* Reception of the data */
  if (HAL_QSPI_Receive(Ctx, Handle->pBuffer, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return MXIC_SNOR_ERROR_RECEIVE;
  }

  return MXIC_SNOR_ERROR_NONE;
}
#endif  // MXIC_SNOR_MULTIPLE_IO_READ_ID_CMD

#ifdef MXIC_SNOR_ENABLE_QSPI_CMD
/*
 * MXIC_SNOR_ENABLE_QSPI_CMD                       0x35   // EQIO, Enable QPI; 1-0-0
 * @brief  This function trigger Flash running Quad I/O mode (4-x-x) from SPI mode.
 *         SPI -> QPI; 1-x-x -> 4-x-x
 * @param  *Ctx            : Device handle
 * @retval MXIC_xSPINORErrorTypeDef
 */
MXIC_xSPINORErrorTypeDef MXIC_SNOR_EnterQPIMode(void *Ctx)
{
  return x00_Command(Ctx, MXIC_SNOR_ENABLE_QSPI_CMD, MXIC_SNOR_FREAD_111);
}
#endif  // MXIC_SNOR_ENABLE_QSPI_CMD

#ifdef MXIC_SNOR_RESET_QSPI_CMD
/*
 * MXIC_SNOR_RESET_QSPI_CMD                        0xF5   // RSTQIO, Reset QPI; QPI 4-0-0
 * @brief  This function trigger Flash running Single I/O mode (1-x-x) from QPI mode.
 *         QPI -> SPI; 4-x-x -> 1-x-x
 * @param  *Ctx            : Device handle
 * @retval MXIC_xSPINORErrorTypeDef
 */
MXIC_xSPINORErrorTypeDef MXIC_SNOR_ExitQPIMode(void *Ctx)
{
  return x00_Command(Ctx, MXIC_SNOR_RESET_QSPI_CMD, MXIC_SNOR_FREAD_444);
}
#endif  // MXIC_SNOR_RESET_QSPI_CMD

#ifdef MXIC_SNOR_READ_CONFIGURATION_REG2_CMD
/*
 * MXIC_SNOR_READ_CONFIGURATION_REG2_CMD           0x71   // RDCR2, Read Configuration Register 2; 1-1-1/8S-8S-8S/8D-8D-8D + 4 Byte Address
 * @brief  Read Flash Configuration register 2 value
 * @param  *Ctx            : Device handle
 *         Handle->Mode    : Command interface
 *         Handle->Address : CR2 address
 *         Handle->pBuffer : Buffer pointer
 * @retval MXIC_xSPINORErrorTypeDef
 *         Handle->pBuffer[0] = CR2 value
 */
MXIC_xSPINORErrorTypeDef MXIC_SNOR_ReadConfigurationRegister2(void *Ctx, MXIC_SNOR_CommandHandle_t *Handle)
{
  QSPI_CommandTypeDef s_command;

  /* Setup command structure */
  SetupRegisterCommandSPIQPIxxx(&s_command, MXIC_SNOR_READ_CONFIGURATION_REG2_CMD, MXIC_SNOR_FREAD_111, 1);
  s_command.AddressSize = QSPI_ADDRESS_32_BITS;     // 4 Byte address, always
  s_command.Address     = Handle->Address;
  s_command.DummyCycles = 0;

  /* Configure the command */
  if (HAL_QSPI_Command(Ctx, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return MXIC_SNOR_ERROR_COMMAND;
  }

  /* Reception of the data */
  if (HAL_QSPI_Receive(Ctx, Handle->pBuffer, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return MXIC_SNOR_ERROR_RECEIVE;
  }

  return MXIC_SNOR_ERROR_NONE;
}
#endif  // MXIC_SNOR_READ_CONFIGURATION_REG2_CMD

#ifdef  MXIC_SNOR_WRITE_CONFIGURATION_REG2_CMD
/*
 * MXIC_SNOR_WRITE_CONFIGURATION_REG2_CMD          0x72   // WRCR2, Write Configuration Register 2; 1-1-1/8S-8S-8S/8D-8D-8D + 4 Byte Address
 * @brief  Write configuration register 2
 * @param  *Ctx               : Device handle
 *         Handle->Address    : CR2 address
 *         Handle->pBuffer[0] : CR2 value
 * @retval MXIC_xSPINORErrorTypeDef
 */
MXIC_xSPINORErrorTypeDef MXIC_SNOR_WriteConfigurationRegister2(void *Ctx, MXIC_SNOR_CommandHandle_t *Handle)
{
  QSPI_CommandTypeDef s_command;

  /* Setup command structure */
  SetupRegisterCommandSPIQPIxxx(&s_command, MXIC_SNOR_WRITE_CONFIGURATION_REG2_CMD, MXIC_SNOR_FREAD_111, 1);
  s_command.AddressSize = QSPI_ADDRESS_32_BITS;     // 4 Byte address, always
  s_command.Address     = Handle->Address;
  s_command.DummyCycles = 0;

  /* Configure the write volatile configuration register command */
  if (HAL_QSPI_Command(Ctx, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return MXIC_SNOR_ERROR_COMMAND;
  }

  /* Transmission of the data */
  if (HAL_QSPI_Transmit(Ctx, Handle->pBuffer, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return MXIC_SNOR_ERROR_TRANSMIT;
  }

  return MXIC_SNOR_ERROR_NONE;
}
#endif  //  MXIC_SNOR_WRITE_CONFIGURATION_REG2_CMD


/**************************************************************************************************
 * 3/4 Byte addressing mode Operations
 *  Enter/Exit 4 Byte address command mode; Let 3 Byte address read command accept 4 Byte address
 * Extended Address Register Operations
 *  Change 128M bit memory bank by write Extended Address Register
 *  3 Byte address command no any change
 *************************************************************************************************/
#ifdef MXIC_SNOR_ENTER_4BYTE_ADDRESS_MODE_CMD
/*
 * MXIC_SNOR_ENTER_4BYTE_ADDRESS_MODE_CMD          0xB7   // EN4B, Enter 4-Byte mode (3/4 Byte address commands); 1-0-0/4-0-0
 * @brief  Flash enter 4 Byte address mode. Effect 3/4 address byte commands only.
 * @param  *Ctx : Device handle
 * @retval MXIC_xSPINORErrorTypeDef
 */
MXIC_xSPINORErrorTypeDef MXIC_SNOR_Enter4BytesAddressMode(void *Ctx, MXIC_SNOR_Mode_t Mode)
{
  return x00_Command(Ctx, MXIC_SNOR_ENTER_4BYTE_ADDRESS_MODE_CMD, Mode.IO);
}
#endif  // MXIC_SNOR_ENTER_4BYTE_ADDRESS_MODE_CMD

#ifdef MXIC_SNOR_EXIT_4BYTE_ADDRESS_MODE_CMD
/*
 * MXIC_SNOR_EXIT_4BYTE_ADDRESS_MODE_CMD           0xE9   // EX4B, Exit 4-Byte mode (3/4 Byte address commands); 1-0-0/4-0-0
 * @brief  Flash exit 4 Byte address mode. Effect 3/4 address byte commands only.
 * @param  *Ctx : Device handle
 * @retval MXIC_xSPINORErrorTypeDef
 */
MXIC_xSPINORErrorTypeDef MXIC_SNOR_Exit4BytesAddressMode(void *Ctx, MXIC_SNOR_Mode_t Mode)
{
  return x00_Command(Ctx, MXIC_SNOR_EXIT_4BYTE_ADDRESS_MODE_CMD, Mode.IO);
}
#endif  // MXIC_SNOR_EXIT_4BYTE_ADDRESS_MODE_CMD

#ifdef MXIC_SNOR_READ_EXTENDED_ADDRESS_REG_CMD
/*
 * MXIC_SNOR_READ_EXTENDED_ADDRESS_REG_CMD         0xC8   // RDEAR, Read Extended Address Register; 1-0-1/4-0-4
 * @brief  Read Extended Address Register.
 *         This Register define Address[31:24] of 3 Byte address read command
 * @param  *Ctx            : Device handle
 *         Handle->pBuffer : Buffer pointer
 * @retval MXIC_xSPINORErrorTypeDef
 *         Handle->pBuffer[0] = EAR value
 */
MXIC_xSPINORErrorTypeDef MXIC_SNOR_ReadExtendedAddressRegister(void *Ctx, MXIC_SNOR_CommandHandle_t *Handle)
{
  QSPI_CommandTypeDef s_command;

  /* Setup command structure */
  SetupRegisterCommandSPIQPIx0x(&s_command, MXIC_SNOR_READ_EXTENDED_ADDRESS_REG_CMD, Handle->Mode.IO, 1);

  /* Configure the command */
  if (HAL_QSPI_Command(Ctx, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return MXIC_SNOR_ERROR_COMMAND;
  }

  /* Reception of the data */
  if (HAL_QSPI_Receive(Ctx, Handle->pBuffer, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return MXIC_SNOR_ERROR_RECEIVE;
  }

  return MXIC_SNOR_ERROR_NONE;
}
#endif  // MXIC_SNOR_READ_EXTENDED_ADDRESS_REG_CMD

#ifdef MXIC_SNOR_WRITE_EXTENDED_ADDRESS_REG_CMD
/*
 * MXIC_SNOR_WRITE_EXTENDED_ADDRESS_REG_CMD        0xC5   // WREAR, Write Extended Address Register; 1-0-1/4-0-4
 * @brief  Write Extended Address Register.
 *         This Register define Address[31:24] of 3 Byte address read command
 * @param  *Ctx               : Device handle
 *         Handle->pBuffer[0] : EAR value
 * @retval MXIC_xSPINORErrorTypeDef
 */
MXIC_xSPINORErrorTypeDef MXIC_SNOR_WriteExtendedAddressRegister(void *Ctx, MXIC_SNOR_CommandHandle_t *Handle)
{
  QSPI_CommandTypeDef s_command;

  /* Setup command structure */
  SetupRegisterCommandSPIQPIx0x(&s_command, MXIC_SNOR_WRITE_EXTENDED_ADDRESS_REG_CMD, Handle->Mode.IO, 1);

  /* Configure the command */
  if (HAL_QSPI_Command(Ctx, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return MXIC_SNOR_ERROR_COMMAND;
  }

  /* Transmission of the data */
  if (HAL_QSPI_Transmit(Ctx, Handle->pBuffer, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return MXIC_SNOR_ERROR_TRANSMIT;
  }

  return MXIC_SNOR_ERROR_NONE;
}
#endif  // MXIC_SNOR_WRITE_EXTENDED_ADDRESS_REG_CMD

/* Fast Boot Register Operations */
#ifdef MXIC_SNOR_READ_FAST_BOOT_REGISTER_CMD
/*
 * MXIC_SNOR_READ_FAST_BOOT_REGISTER_CMD           0x16   // RDFBR, Read Fast Boot Register; 1-0-1/8S-8S-8S/8D-8D-8D
 * @brief  Read flash fast boot register value
 * @param  *Ctx            : Device handle
 *         Handle->pBuffer : Buffer pointer
 * @retval MXIC_xSPINORErrorTypeDef
 *         *Handle->pBuffer = (uint32_t) Value of Fast Boot Register
 */
MXIC_xSPINORErrorTypeDef MXIC_SNOR_ReadFastBootRegister(void *Ctx, MXIC_SNOR_CommandHandle_t *Handle)
{
  QSPI_CommandTypeDef s_command;

  /* Setup command structure */
  SetupRegisterCommandSPIQPIx0x(&s_command, MXIC_SNOR_READ_FAST_BOOT_REGISTER_CMD, MXIC_SNOR_FREAD_111, sizeof(MXIC_SNOR_FastBootRegister_t));

  /* Configure the command */
  if (HAL_QSPI_Command(Ctx, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return MXIC_SNOR_ERROR_COMMAND;
  }

  /* Reception of the data */
  if (HAL_QSPI_Receive(Ctx, Handle->pBuffer, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return MXIC_SNOR_ERROR_RECEIVE;
  }

  return MXIC_SNOR_ERROR_NONE;
}
#endif  // MXIC_SNOR_READ_FAST_BOOT_REGISTER_CMD

#ifdef MXIC_SNOR_WRITE_FAST_BOOT_REGISTER_CMD
/*
 * MXIC_SNOR_WRITE_FAST_BOOT_REGISTER_CMD          0x17   // WRFBR, Write Fast Boot Register; 1-0-1/8S-8S-8S/8D-8D-8D
 * @brief  Write fast boot register. Always write 4 byte data
 * @param  *Ctx            : Device handle
 *         Handle->pBuffer : Buffer pointer of Fast Boot Register value
 * @retval MXIC_xSPINORErrorTypeDef
 */
MXIC_xSPINORErrorTypeDef MXIC_SNOR_WriteFastBootRegister(void *Ctx, MXIC_SNOR_CommandHandle_t *Handle)
{
  QSPI_CommandTypeDef s_command;

  /* Setup command structure */
  SetupRegisterCommandSPIQPIx0x(&s_command, MXIC_SNOR_WRITE_FAST_BOOT_REGISTER_CMD, MXIC_SNOR_FREAD_111, sizeof(MXIC_SNOR_FastBootRegister_t));

  /* Configure the command */
  if (HAL_QSPI_Command(Ctx, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return MXIC_SNOR_ERROR_COMMAND;
  }

  /* Transmission of the data */
  if (HAL_QSPI_Transmit(Ctx, Handle->pBuffer, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return MXIC_SNOR_ERROR_TRANSMIT;
  }

  return MXIC_SNOR_ERROR_NONE;
}
#endif  // MXIC_SNOR_WRITE_FAST_BOOT_REGISTER_CMD

#ifdef MXIC_SNOR_ERASE_FAST_BOOT_REGISTER_CMD
/*
 * MXIC_SNOR_ERASE_FAST_BOOT_REGISTER_CMD          0x18   // ESFBR, Erase Fast Boot Register; 1-0-0/8S-0-0/8D-0-0
 * @brief  Erases fast boot register
 * @param  *Ctx : Device handle
 *         Mode : Command interface
 * @retval MXIC_xSPINORErrorTypeDef
 */
MXIC_xSPINORErrorTypeDef MXIC_SNOR_EraseFastBootRegister(void *Ctx, MXIC_SNOR_Mode_t Mode)
{
  UNUSED(Mode);   // 8xx command usage
  return x00_Command(Ctx, MXIC_SNOR_ERASE_FAST_BOOT_REGISTER_CMD, MXIC_SNOR_FREAD_111);
}
#endif  // MXIC_SNOR_ERASE_FAST_BOOT_REGISTER_CMD


/**************************************************************************************************
 * Macronix xSPI-NOR Flash Protection Command Pool
 *************************************************************************************************/
/* Secured OTP Memory Array Operations */
#ifdef MXIC_SNOR_ENTER_SECURED_OTP_CMD
/*
 * MXIC_SNOR_ENTER_SECURED_OTP_CMD                 0xB1   // ENSO, Enter Secured OTP; 1-0-0/4-0-0/8S-0-0/8D-0-0
 * @brief  Enter Secured OTP mode
 * @param  *Ctx : Device handle
 * @retval MXIC_xSPINORErrorTypeDef
 */
MXIC_xSPINORErrorTypeDef MXIC_SNOR_EnterSecuredOTP(void *Ctx, MXIC_SNOR_Mode_t Mode)
{
  return x00_Command(Ctx, MXIC_SNOR_ENTER_SECURED_OTP_CMD, Mode.IO);
}
#endif  // MXIC_SNOR_ENTER_SECURED_OTP_CMD

#ifdef MXIC_SNOR_EXIT_SECURED_OTP_CMD
/*
 * MXIC_SNOR_EXIT_SECURED_OTP_CMD                  0xC1   // EXSO, Exit Secured OTP; 1-0-0/4-0-0/8S-0-0/8D-0-0
 * @brief  Exit Secured OTP mode
 * @param  *Ctx : Device handle
 * @retval MXIC_xSPINORErrorTypeDef
 */
MXIC_xSPINORErrorTypeDef MXIC_SNOR_ExitSecuredOTP(void *Ctx, MXIC_SNOR_Mode_t Mode)
{
  return x00_Command(Ctx, MXIC_SNOR_EXIT_SECURED_OTP_CMD, Mode.IO);
}
#endif  // MXIC_SNOR_EXIT_SECURED_OTP_CMD

#ifdef MXIC_SNOR_WRITE_SECURITY_REGISTER_CMD
/*
 * MXIC_SNOR_WRITE_SECURITY_REGISTER_CMD           0x2F   // WRSCUR, Write Security Register; 1-0-0/4-0-0/8S-0-0/8D-0-0
 * @brief  Write Security Register. To set the Lock-down Secured OTP (LDSO) bit = 1.
 * @param  *Ctx : Device handle
 *         Mode : Command interface
 * @retval MXIC_xSPINORErrorTypeDef
 */
MXIC_xSPINORErrorTypeDef MXIC_SNOR_WriteSecurityRegister(void *Ctx, MXIC_SNOR_Mode_t Mode)
{
  return x00_Command(Ctx, MXIC_SNOR_WRITE_SECURITY_REGISTER_CMD, Mode.IO);
}
#endif  // MXIC_SNOR_WRITE_SECURITY_REGISTER_CMD

#ifdef MXIC_SNOR_ENTER_4K_BIT_MODE_CMD
/*
 * MXIC_SNOR_ENTER_4K_BIT_MODE_CMD                 0xA5   // EN4K, Enter 4K bit Mode; 1-0-0
 * @brief  Enter 4K bit Mode
 * @param  *Ctx : Device handle
 * @retval MXIC_xSPINORErrorTypeDef
 */
MXIC_xSPINORErrorTypeDef MXIC_SNOR_Enter4KbitMode(void *Ctx)
{
  return x00_Command(Ctx, MXIC_SNOR_ENTER_4K_BIT_MODE_CMD, MXIC_SNOR_FREAD_111);
}
#endif  // MXIC_SNOR_ENTER_4K_BIT_MODE_CMD

#ifdef MXIC_SNOR_EXIT_4K_BIT_MODE_CMD
/*
 * MXIC_SNOR_EXIT_4K_BIT_MODE_CMD                  0xB5   // EX4K, Exit 4K bit Mode; 1-0-0
 * @brief  Exit 4K bit Mode
 * @param  *Ctx            : Device handle
 * @retval MXIC_xSPINORErrorTypeDef
 */
MXIC_xSPINORErrorTypeDef MXIC_SNOR_Exit4KbitMode(void *Ctx)
{
  return x00_Command(Ctx, MXIC_SNOR_EXIT_4K_BIT_MODE_CMD, MXIC_SNOR_FREAD_111);
}
#endif  // MXIC_SNOR_EXIT_4K_BIT_MODE_CMD

/* Security Register Operations */
#ifdef MXIC_SNOR_READ_SECURITY_REGISTER_CMD
/*
 * MXIC_SNOR_READ_SECURITY_REGISTER_CMD            0x2B   // RDSCUR, Read Security Register; 1-0-1/4-0-4/8S-8S-8S/8D-8D-8D
 * @brief  Read Security Register value
 * @param  *Ctx            : Device handle
 *         Handle->Mode    : Command interface
 *         Handle->pBuffer : Buffer pointer
 * @retval MXIC_xSPINORErrorTypeDef
 *         Handle->pBuffer[0] = Security Register value
 */
MXIC_xSPINORErrorTypeDef MXIC_SNOR_ReadSecurityRegister(void *Ctx, MXIC_SNOR_CommandHandle_t *Handle)
{
  QSPI_CommandTypeDef s_command;

  /* Setup command structure */
  SetupRegisterCommandSPIQPIx0x(&s_command, MXIC_SNOR_READ_SECURITY_REGISTER_CMD, Handle->Mode.IO, sizeof(MXIC_SNOR_SecurityRegister_t));

  /* Configure the command */
  if (HAL_QSPI_Command(Ctx,  &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return MXIC_SNOR_ERROR_COMMAND;
  }

  /* Reception of the data */
  if (HAL_QSPI_Receive(Ctx, Handle->pBuffer, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return MXIC_SNOR_ERROR_RECEIVE;
  }

  return MXIC_SNOR_ERROR_NONE;
}
#endif  // MXIC_SNOR_READ_SECURITY_REGISTER_CMD

#ifdef MXIC_SNOR_CLEAR_SECURITY_REGISTER_FAIL_FLAG_CMD
/*
 * MXIC_SNOR_CLEAR_SECURITY_REGISTER_FAIL_FLAG_CMD 0x30   // CLSR, Clear Security Register bit 6 & bit 5; 1-0-0
 * @brief  Enable SO to output RY/BY# during CP mode
 * @param  *Ctx : Device handle
 * @retval MXIC_xSPINORErrorTypeDef
 */
MXIC_xSPINORErrorTypeDef MXIC_SNOR_ClearSecurityRegisterFailFlags(void *Ctx)
{
  return x00_Command(Ctx, MXIC_SNOR_CLEAR_SECURITY_REGISTER_FAIL_FLAG_CMD, MXIC_SNOR_FREAD_111);
}
#endif  // MXIC_SNOR_CLEAR_SECURITY_REGISTER_FAIL_FLAG_CMD

/* SPB/DPB/Lock Operations; Advanced Sector Protection */
#ifdef MXIC_SNOR_WRITE_PROTECT_SELECTION_CMD
/*
 * MXIC_SNOR_WRITE_PROTECT_SELECTION_CMD           0x68   // WPSEL, Write Protect Selection; 1-0-0/4-0-0/8S-0-0/8D-0-0
 * @brief  Write Protect Selection; WPSEL,
 *         Enter and enable individual block protect mode
 * @param  *Ctx : Device handle
 * @retval MXIC_xSPINORErrorTypeDef
 */
MXIC_xSPINORErrorTypeDef MXIC_SNOR_WriteProtectSelection(void *Ctx, MXIC_SNOR_Mode_t Mode)
{
  return x00_Command(Ctx, MXIC_SNOR_WRITE_PROTECT_SELECTION_CMD, Mode.IO);
}
#endif  // MXIC_SNOR_WRITE_PROTECT_SELECTION_CMD

#ifdef MXIC_SNOR_GANG_BLOCK_LOCK_CMD
/*
 * MXIC_SNOR_GANG_BLOCK_LOCK_CMD                   0x7E   // GBLK, Gang Block Lock; 1-0-0/4-0-0/8S-0-0/8D-0-0
 * @brief  Gang block lock. Whole chip write protect.
 * @param  *Ctx : Device handle
 * @retval MXIC_xSPINORErrorTypeDef
 */
MXIC_xSPINORErrorTypeDef MXIC_SNOR_GangBlockLock(void *Ctx, MXIC_SNOR_Mode_t Mode)
{
  return x00_Command(Ctx, MXIC_SNOR_GANG_BLOCK_LOCK_CMD, Mode.IO);
}
#endif  // MXIC_SNOR_GANG_BLOCK_LOCK_CMD

#ifdef MXIC_SNOR_GANG_BLOCK_UNLOCK_CMD
/*
 * MXIC_SNOR_GANG_BLOCK_UNLOCK_CMD                 0x98   // GBULK, Gang Block Unlock; 1-0-0/4-0-0/8S-0-0/8D-0-0
 * @brief  Gang block unlock. Whole chip unprotect.
 * @param  *Ctx : Device handle
 * @retval MXIC_xSPINORErrorTypeDef
 */
MXIC_xSPINORErrorTypeDef MXIC_SNOR_GangBlockUnlock(void *Ctx, MXIC_SNOR_Mode_t Mode)
{
  return x00_Command(Ctx, MXIC_SNOR_GANG_BLOCK_UNLOCK_CMD, Mode.IO);
}
#endif  // MXIC_SNOR_GANG_BLOCK_UNLOCK_CMD

#ifdef MXIC_SNOR_SINGLE_BLOCK_LOCK_CMD
/*
 * MXIC_SNOR_SINGLE_BLOCK_LOCK_CMD                 0x36   // SBLK, Single Block Lock; 1-1-0/4-4-0 + 3/4 Byte Address
 * @brief  Single Block Lock
 * @param  *Ctx            : Device handle
 *         Handle->Mode    : Command interface
 *         Handle->Address : Block address
 * @retval MXIC_xSPINORErrorTypeDef
 */
MXIC_xSPINORErrorTypeDef MXIC_SNOR_SingleBlockLock(void *Ctx, MXIC_SNOR_CommandHandle_t *Handle)
{
  QSPI_CommandTypeDef s_command;

  /* Setup command structure */
  SetupRegisterCommandSPIQPIxxx(&s_command, MXIC_SNOR_SINGLE_BLOCK_LOCK_CMD, Handle->Mode.IO, 0);
  s_command.Address     = Handle->Address;
  s_command.DummyCycles = 0;

  /* Configure the command */
  if (HAL_QSPI_Command(Ctx, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return MXIC_SNOR_ERROR_COMMAND;
  }

  return MXIC_SNOR_ERROR_NONE;
}
#endif  // MXIC_SNOR_SINGLE_BLOCK_LOCK_CMD

#ifdef MXIC_SNOR_SINGLE_BLOCK_UNLOCK_CMD
/*
 * MXIC_SNOR_SINGLE_BLOCK_UNLOCK_CMD               0x39   // SBULK, Single Block Un-Lock; 1-1-0/4-4-0 + 3/4 Byte Address
 * @brief  Single Block Un-Lock
 * @param  *Ctx            : Device handle
 *         Handle->Mode    : Command interface
 *         Handle->Address : Block address
 * @retval MXIC_xSPINORErrorTypeDef
 */
MXIC_xSPINORErrorTypeDef MXIC_SNOR_SingleBlockUnLock(void *Ctx, MXIC_SNOR_CommandHandle_t *Handle)
{
  QSPI_CommandTypeDef s_command;

  /* Setup command structure */
  SetupRegisterCommandSPIQPIxxx(&s_command, MXIC_SNOR_SINGLE_BLOCK_UNLOCK_CMD, Handle->Mode.IO, 0);
  s_command.Address     = Handle->Address;
  s_command.DummyCycles = 0;

  /* Configure the command */
  if (HAL_QSPI_Command(Ctx, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return MXIC_SNOR_ERROR_COMMAND;
  }

  return MXIC_SNOR_ERROR_NONE;
}
#endif  // MXIC_SNOR_SINGLE_BLOCK_UNLOCK_CMD

#ifdef MXIC_SNOR_READ_BLOCK_LOCK_STATUS_CMD
/*
 * MXIC_SNOR_READ_BLOCK_LOCK_STATUS_CMD            0x3C   // RDBLOCK, Read Block Lock Status; 1-1-1/4-4-4 + 3/4 Byte Address;
 * @brief  Read Block Lock Status
 * @param  *Ctx            : Device handle
 *         Handle->Mode    : Command interface
 *         Handle->Address : Block address
 *         Handle->pBuffer : Buffer pointer
 * @retval MXIC_xSPINORErrorTypeDef
 *         Handle->pBuffer[0] = Block Lock Status value
 */
MXIC_xSPINORErrorTypeDef MXIC_SNOR_ReadBlockLockStatus(void *Ctx, MXIC_SNOR_CommandHandle_t *Handle)
{
  QSPI_CommandTypeDef s_command;

  /* Setup command structure */
  SetupRegisterCommandSPIQPIxxx(&s_command, MXIC_SNOR_READ_BLOCK_LOCK_STATUS_CMD, Handle->Mode.IO, 1);
  s_command.Address     = Handle->Address;
  s_command.DummyCycles = 0;

  /* Configure the command */
  if (HAL_QSPI_Command(Ctx, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return MXIC_SNOR_ERROR_COMMAND;
  }

  /* Reception of the data */
  if (HAL_QSPI_Receive(Ctx, Handle->pBuffer, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return MXIC_SNOR_ERROR_RECEIVE;
  }

  return MXIC_SNOR_ERROR_NONE;
}
#endif  // MXIC_SNOR_READ_BLOCK_LOCK_STATUS_CMD

#ifdef MXIC_SNOR_READ_DPB_REGISTER_CMD
/*
 * MXIC_SNOR_READ_DPB_REGISTER_CMD                 0xE0   // RDDPB, Read DPB Register; 1-1-1/8S-8S-8S/8D-8D-8D + 4 Byte Address
 * @brief  Read DPB register, 1 Byte.
 * @param  *Ctx            : Device handle
 *         Handle->Address : DPB address
 *         Handle->pBuffer : Buffer pointer
 * @retval MXIC_xSPINORErrorTypeDef
 *         Handle->pBuffer[0] = MXIC_SNOR_DPBRegisterTypeDef DPB Register value
 */
MXIC_xSPINORErrorTypeDef MXIC_SNOR_ReadDPBRegister(void *Ctx, MXIC_SNOR_CommandHandle_t *Handle)
{
  QSPI_CommandTypeDef s_command;

  /* Setup command structure */
  SetupRegisterCommandSPIQPIxxx(&s_command, MXIC_SNOR_READ_DPB_REGISTER_CMD, MXIC_SNOR_FREAD_111, 1);
  s_command.AddressSize = QSPI_ADDRESS_32_BITS;     // 4 Byte address, always
  s_command.Address     = Handle->Address;
  s_command.DummyCycles = 0;

  /* Configure the command */
  if (HAL_QSPI_Command(Ctx, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return MXIC_SNOR_ERROR_COMMAND;
  }

  /* Reception of the data */
  if (HAL_QSPI_Receive(Ctx, Handle->pBuffer, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return MXIC_SNOR_ERROR_RECEIVE;
  }

  return MXIC_SNOR_ERROR_NONE;
}
#endif  // MXIC_SNOR_READ_DPB_REGISTER_CMD

#ifdef MXIC_SNOR_WRITE_DPB_REGISTER_CMD
/*
 * MXIC_SNOR_WRITE_DPB_REGISTER_CMD                0xE1   // WRDPB, Write DPB Register; 1-1-1/8S-8S-8S/8D-8D-8D + 4 Byte Address
 * @brief  Write DPB register
 * @param  *Ctx            : Device handle
 *         Handle->Address : DPB address
 *         Handle->pBuffer : Buffer pointer of DPB Register value
 * @retval MXIC_xSPINORErrorTypeDef
 *         *Handle->pBuffer = MXIC_SNOR_DPBRegisterTypeDef data
 */
MXIC_xSPINORErrorTypeDef MXIC_SNOR_WriteDPBRegister(void *Ctx, MXIC_SNOR_CommandHandle_t *Handle)
{
  QSPI_CommandTypeDef s_command;

  /* Setup command structure */
  SetupRegisterCommandSPIQPIxxx(&s_command, MXIC_SNOR_WRITE_DPB_REGISTER_CMD, MXIC_SNOR_FREAD_111, 1);
  s_command.AddressSize = QSPI_ADDRESS_32_BITS;     // 4 Byte address, always
  s_command.Address     = Handle->Address;
  s_command.DummyCycles = 0;

  /* Configure the command */
  if (HAL_QSPI_Command(Ctx, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return MXIC_SNOR_ERROR_COMMAND;
  }

  /* Transmission of the data */
  if (HAL_QSPI_Transmit(Ctx, Handle->pBuffer, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return MXIC_SNOR_ERROR_TRANSMIT;
  }

  return MXIC_SNOR_ERROR_NONE;
}
#endif  // MXIC_SNOR_WRITE_DPB_REGISTER_CMD

#ifdef MXIC_SNOR_READ_SPB_STATUS_CMD
/*
 * MXIC_SNOR_READ_SPB_STATUS_CMD                   0xE2   // RDSPB, Read SPB Status; 1-1-1/8S-8S-8S/8D-8D-8D + 4 Byte Address
 * @brief  Read SPB status
 * @param  *Ctx            : Device handle
 *         Handle->Address : SPB address
 *         Handle->pBuffer : Buffer pointer
 * @retval MXIC_xSPINORErrorTypeDef
 *         Handle->pBuffer[0] = MXIC_SNOR_SPBRegisterTypeDef SPB Register value
 */
MXIC_xSPINORErrorTypeDef MXIC_SNOR_ReadSPBStatus(void *Ctx, MXIC_SNOR_CommandHandle_t *Handle)
{
  QSPI_CommandTypeDef s_command;

  /* Setup command structure */
  SetupRegisterCommandSPIQPIxxx(&s_command, MXIC_SNOR_READ_SPB_STATUS_CMD, MXIC_SNOR_FREAD_111, 1);
  s_command.AddressSize = QSPI_ADDRESS_32_BITS;     // 4 Byte address, always
  s_command.Address     = Handle->Address;
  s_command.DummyCycles = 0;

  /* Configure the command */
  if (HAL_QSPI_Command(Ctx, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return MXIC_SNOR_ERROR_COMMAND;
  }

  /* Reception of the data */
  if (HAL_QSPI_Receive(Ctx, Handle->pBuffer, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return MXIC_SNOR_ERROR_RECEIVE;
  }

  return MXIC_SNOR_ERROR_NONE;
}
#endif  // MXIC_SNOR_READ_SPB_STATUS_CMD

#ifdef MXIC_SNOR_WRITE_SPB_CMD
/*
 * MXIC_SNOR_WRITE_SPB_CMD                         0xE3   // WRSPB, SPB bit program; 1-1-0/8S-8S-0/8D-8D-0 + 4 Byte Address
 * @brief  Write SPB. SPB bit program
 * @param  *Ctx            : Device handle
 *         Handle->Address : SPB address
 * @retval MXIC_xSPINORErrorTypeDef
 */
MXIC_xSPINORErrorTypeDef MXIC_SNOR_WriteSPB(void *Ctx, MXIC_SNOR_CommandHandle_t *Handle)
{
  QSPI_CommandTypeDef s_command;

  /* Setup command structure */
  SetupRegisterCommandSPIQPIxxx(&s_command, MXIC_SNOR_WRITE_SPB_CMD, MXIC_SNOR_FREAD_111, 0);
  s_command.AddressSize = QSPI_ADDRESS_32_BITS;     // 4 Byte address, always
  s_command.Address     = Handle->Address;
  s_command.DummyCycles = 0;

  /* Configure the command */
  if (HAL_QSPI_Command(Ctx, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return MXIC_SNOR_ERROR_COMMAND;
  }

  return MXIC_SNOR_ERROR_NONE;
}
#endif  // MXIC_SNOR_WRITE_SPB_CMD

#ifdef MXIC_SNOR_ERASE_SPB_CMD
/*
 * MXIC_SNOR_ERASE_SPB_CMD                         0xE4   // ESSPB, All SPB bit Erase; 1-0-0/8S-0-0/8D-0-0
 * @brief  Erase SPB (ESSPB), All SPB bit erase.
 * @param  *Ctx : Device handle
 * @retval MXIC_xSPINORErrorTypeDef
 */
MXIC_xSPINORErrorTypeDef MXIC_SNOR_EraseSPB(void *Ctx, MXIC_SNOR_Mode_t Mode)
{
  UNUSED(Mode);   // 8xx command usage
  return x00_Command(Ctx, MXIC_SNOR_ERASE_SPB_CMD, MXIC_SNOR_FREAD_111);
}
#endif  // MXIC_SNOR_ERASE_SPB_CMD

/* Lock Register Operations **********************************************************************/
#ifdef MXIC_SNOR_READ_LOCK_REGISTER_CMD
/*
 * MXIC_SNOR_READ_LOCK_REGISTER_CMD                0x2D   // RDLR, Read Lock Register; 1-0-1/8S-8S-8S/8D-8D-8D
 * @brief  Read Lock Register value. 1/2 Byte
 * @param  *Ctx            : Device handle
 *         Handle->pBuffer : Buffer pointer
 * @retval MXIC_xSPINORErrorTypeDef
 *         Handle->pBuffer[0] = LR[7:0]
 *         Handle->pBuffer[1] = LR[15:8]
 */
MXIC_xSPINORErrorTypeDef MXIC_SNOR_ReadLockRegister(void *Ctx, MXIC_SNOR_CommandHandle_t *Handle)
{
  QSPI_CommandTypeDef s_command;

  /* Setup command structure */
  SetupRegisterCommandSPIQPIx0x(&s_command, MXIC_SNOR_READ_LOCK_REGISTER_CMD, MXIC_SNOR_FREAD_111, sizeof(MXIC_SNOR_LockRegister_t));

  /* Configure the command */
  if (HAL_QSPI_Command(Ctx, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return MXIC_SNOR_ERROR_COMMAND;
  }

  /* Reception of the data */
  if (HAL_QSPI_Receive(Ctx, Handle->pBuffer, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return MXIC_SNOR_ERROR_RECEIVE;
  }

  return MXIC_SNOR_ERROR_NONE;
}
#endif  // MXIC_SNOR_READ_LOCK_REGISTER_CMD

#ifdef MXIC_SNOR_WRITE_LOCK_REGISTER_CMD
/*
 * MXIC_SNOR_WRITE_LOCK_REGISTER_CMD               0x2C   // WRLR, Write Lock Register; 1-0-1/8S-8S-8S/8D-8D-8D
 * @brief  Write lock register. 1/2 Byte
 * @param  *Ctx               : Device handle
 *         Handle->pBuffer[0] : LR[7:0]
 *         Handle->pBuffer[1] : LR[15:8]
 * @retval MXIC_xSPINORErrorTypeDef
 */
MXIC_xSPINORErrorTypeDef MXIC_SNOR_WriteLockRegister(void *Ctx, MXIC_SNOR_CommandHandle_t *Handle)
{
  QSPI_CommandTypeDef s_command;

  /* Setup command structure */
  SetupRegisterCommandSPIQPIx0x(&s_command, MXIC_SNOR_WRITE_LOCK_REGISTER_CMD, MXIC_SNOR_FREAD_111, sizeof(MXIC_SNOR_LockRegister_t));

  /* Configure the command */
  if (HAL_QSPI_Command(Ctx,  &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return MXIC_SNOR_ERROR_COMMAND;
  }

  /* Transmission of the data */
  if (HAL_QSPI_Transmit(Ctx, Handle->pBuffer, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return MXIC_SNOR_ERROR_TRANSMIT;
  }

  return MXIC_SNOR_ERROR_NONE;
}
#endif  // MXIC_SNOR_WRITE_LOCK_REGISTER_CMD

#ifdef MXIC_SNOR_SPB_LOCK_SET_CMD
/*
 * MXIC_SNOR_SPB_LOCK_SET_CMD                      0xA6   // SPBLK, SPB Lock set; 1-0-0
 * @brief  SPB lock set.
 * @param  *Ctx            : Device handle
 * @retval MXIC_xSPINORErrorTypeDef
 */
MXIC_xSPINORErrorTypeDef MXIC_SNOR_SPBLockSet(void *Ctx)
{
  return x00_Command(Ctx, MXIC_SNOR_SPB_LOCK_SET_CMD, MXIC_SNOR_FREAD_111);
}
#endif  // MXIC_SNOR_SPB_LOCK_SET_CMD

#ifdef MXIC_SNOR_READ_SPB_LOCK_REGISTER_CMD
/*
 * MXIC_SNOR_READ_SPB_LOCK_REGISTER_CMD            0xA7   // RDSPBLK, SPB Lock Register Read; 1-0-1
 * @brief  Read SPB lock register value.
 * @param  *Ctx            : Device handle
 *         Handle->pBuffer : Buffer pointer
 * @retval MXIC_xSPINORErrorTypeDef
 *         Handle->pBuffer[0] = SPB lock register value
 */
MXIC_xSPINORErrorTypeDef MXIC_SNOR_ReadSPBLockRegister(void *Ctx, MXIC_SNOR_CommandHandle_t *Handle)
{
  QSPI_CommandTypeDef s_command;

  /* Setup command structure */
  SetupRegisterCommandSPIQPIx0x(&s_command, MXIC_SNOR_READ_SPB_LOCK_REGISTER_CMD, MXIC_SNOR_FREAD_111, sizeof(MXIC_SNOR_SPBLockRegister_t));

  /* Configure the command */
  if (HAL_QSPI_Command(Ctx, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return MXIC_SNOR_ERROR_COMMAND;
  }

  /* Reception of the data */
  if (HAL_QSPI_Receive(Ctx, Handle->pBuffer, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return MXIC_SNOR_ERROR_RECEIVE;
  }

  return MXIC_SNOR_ERROR_NONE;
}
#endif  // MXIC_SNOR_READ_SPB_LOCK_REGISTER_CMD

/* Permanent Lock Operations *********************************************************************/
#ifdef MXIC_SNOR_READ_PERMANENT_LOCK_BIT_LOCK_DOWN_CMD
/*
 * MXIC_SNOR_READ_PERMANENT_LOCK_BIT_LOCK_DOWN_CMD 0xA7   // RDPLLK, Read Permanent Lock bit Lock Down; 1-1-1
 * @brief  Read Permanent Lock bit Lock Down
 *         Address = 8 bit 0 always
 * @param  *Ctx            : Device handle
 *         Handle->pBuffer : Buffer pointer
 * @retval MXIC_xSPINORErrorTypeDef
 *         Handle->pBuffer[0] = MXIC_SNOR_PermanentLockDownRegister_t Permanent Lock bit
 */
MXIC_xSPINORErrorTypeDef MXIC_SNOR_ReadPermanentLockbitLockDown(void *Ctx, MXIC_SNOR_CommandHandle_t *Handle)
{
  QSPI_CommandTypeDef s_command;

  /* Setup command structure */
  SetupRegisterCommandSPIQPIxxx(&s_command, MXIC_SNOR_READ_PERMANENT_LOCK_BIT_LOCK_DOWN_CMD, MXIC_SNOR_FREAD_111, sizeof(MXIC_SNOR_PermanentLockDownRegister_t));
  s_command.AddressSize = QSPI_ADDRESS_8_BITS;        // 8 bit address 0
  s_command.Address     = 0;                          // Address = 0 always
  s_command.DummyCycles = 0;

  /* Configure the command */
  if (HAL_QSPI_Command(Ctx, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return MXIC_SNOR_ERROR_COMMAND;
  }

  /* Reception of the data */
  if (HAL_QSPI_Receive(Ctx, Handle->pBuffer, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return MXIC_SNOR_ERROR_RECEIVE;
  }

  return MXIC_SNOR_ERROR_NONE;
}
#endif  // MXIC_SNOR_READ_PERMANENT_LOCK_BIT_LOCK_DOWN_CMD

#ifdef MXIC_SNOR_PERMANENT_LOCK_BIT_LOCK_DOWN_CMD
/*
 * MXIC_SNOR_PERMANENT_LOCK_BIT_LOCK_DOWN_CMD      0xA6   // PLLK, Permanent Lock bit Lock Down; 1-0-0
 * @brief  Permanent Lock bit Lock Down
 * @param  *Ctx : Device handle
 * @retval MXIC_xSPINORErrorTypeDef
 */
MXIC_xSPINORErrorTypeDef MXIC_SNOR_PermanentLockbitLockDown(void *Ctx)
{
  return x00_Command(Ctx, MXIC_SNOR_PERMANENT_LOCK_BIT_LOCK_DOWN_CMD, MXIC_SNOR_FREAD_111);
}
#endif  // MXIC_SNOR_PERMANENT_LOCK_BIT_LOCK_DOWN_CMD

#ifdef MXIC_SNOR_READ_READ_LOCK_CONFIGURATION_CMD
/*
 * MXIC_SNOR_READ_READ_LOCK_CONFIGURATION_CMD      0xD5   // RRLCR, Read Read-Lock Area Configuration Register; 1-0-1
 * MXIC_SNOR_READ_READ_LOCK_CONFIGURATION_CMD1     0x26   // RRLCR, Read Read-Lock Area Configuration Register; 1-0-1
 *
 * @brief  Read Read-Lock Area Configuration Register
 * @param  *Ctx            : Device handle
 *         Handle->pBuffer : Buffer pointer
 * @retval MXIC_xSPINORErrorTypeDef
 *         Handle->pBuffer[0] = Read-Lock Area Configuration Register value
 */
MXIC_xSPINORErrorTypeDef MXIC_SNOR_ReadReadLockAreaConfigurationRegister(void *Ctx, MXIC_SNOR_CommandHandle_t *Handle)
{
  QSPI_CommandTypeDef s_command;

  /* Setup command structure */
  SetupRegisterCommandSPIQPIx0x(&s_command, MXIC_SNOR_READ_READ_LOCK_CONFIGURATION_CMD, MXIC_SNOR_FREAD_111, sizeof(MXIC_SNOR_ReadLockRegister_t));

  /* Configure the command */
  if (HAL_QSPI_Command(Ctx, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return MXIC_SNOR_ERROR_COMMAND;
  }

  /* Reception of the data */
  if (HAL_QSPI_Receive(Ctx, Handle->pBuffer, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return MXIC_SNOR_ERROR_RECEIVE;
  }

  return MXIC_SNOR_ERROR_NONE;
}
#endif  // MXIC_SNOR_READ_READ_LOCK_CONFIGURATION_CMD

#ifdef MXIC_SNOR_WRITE_READ_LOCK_CONFIGURATION_CMD
/*
 * MXIC_SNOR_WRITE_READ_LOCK_CONFIGURATION_CMD     0xDD   // WRLCR, Write Read-Lock Area Configuration Register; 1-0-1
 * MXIC_SNOR_WRITE_READ_LOCK_CONFIGURATION_CMD1    0x28   // WRLCR, Write Read-Lock Area Configuration Register; 1-0-1
 *
 * @brief  Write Read-Lock Area Configuration Register
 * @param  *Ctx            : Device handle
 *         Handle->pBuffer : Buffer pointer of Read-Lock Area Configuration Register value
 * @retval MXIC_xSPINORErrorTypeDef
 */
MXIC_xSPINORErrorTypeDef MXIC_SNOR_WriteReadLockAreaConfigurationRegister(void *Ctx, MXIC_SNOR_CommandHandle_t *Handle)
{
  QSPI_CommandTypeDef s_command;

  /* Setup command structure */
  SetupRegisterCommandSPIQPIx0x(&s_command, MXIC_SNOR_WRITE_READ_LOCK_CONFIGURATION_CMD, MXIC_SNOR_FREAD_111, sizeof(MXIC_SNOR_ReadLockRegister_t));

  /* Configure the command */
  if (HAL_QSPI_Command(Ctx, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return MXIC_SNOR_ERROR_COMMAND;
  }

  /* Transmission of the data */
  if (HAL_QSPI_Transmit(Ctx, Handle->pBuffer, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return MXIC_SNOR_ERROR_TRANSMIT;
  }

  return MXIC_SNOR_ERROR_NONE;
}
#endif  // MXIC_SNOR_WRITE_READ_LOCK_CONFIGURATION_CMD

#ifdef MXIC_SNOR_WRITE_READ_LOCK_BIT_CMD
/*
 * MXIC_SNOR_WRITE_READ_LOCK_BIT_CMD               0x21   // WRLB, Write Read-Lock Bit; 1-0-1
 * @brief  Write Read-Lock Bit. Change bit 2 of Security Register only.
 * @param  *Ctx               : Device handle
 *         Handle->pBuffer[0] : Buffer pointer of Read-Lock Bit value
 * @retval MXIC_xSPINORErrorTypeDef
 */
MXIC_xSPINORErrorTypeDef MXIC_SNOR_WriteReadLockBit(void *Ctx, MXIC_SNOR_CommandHandle_t *Handle)
{
  QSPI_CommandTypeDef s_command;

  /* Setup command structure */
  SetupRegisterCommandSPIQPIx0x(&s_command, MXIC_SNOR_WRITE_READ_LOCK_BIT_CMD, MXIC_SNOR_FREAD_111, 1);

  /* Configure the command */
  if (HAL_QSPI_Command(Ctx, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return MXIC_SNOR_ERROR_COMMAND;
  }

  /* Transmission of the data */
  if (HAL_QSPI_Transmit(Ctx, Handle->pBuffer, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return MXIC_SNOR_ERROR_TRANSMIT;
  }

  return MXIC_SNOR_ERROR_NONE;
}
#endif  // MXIC_SNOR_WRITE_READ_LOCK_BIT_CMD

#ifdef MXIC_SNOR_BLOCK_WRITE_LOCK_PROTECTION_CMD
/*
 * MXIC_SNOR_BLOCK_WRITE_LOCK_PROTECTION_CMD       0xE2   // BLOCKP, Block Write Lock Protection; 1-1-0
 * @brief  Block Write Lock Protection
 * @param  *Ctx            : Device handle
 *         Handle->Address : Block address
 * @retval MXIC_xSPINORErrorTypeDef
 */
MXIC_xSPINORErrorTypeDef MXIC_SNOR_BlockWriteLockProtection(void *Ctx, MXIC_SNOR_CommandHandle_t *Handle)
{
  QSPI_CommandTypeDef s_command;

  /* Setup command structure */
  SetupRegisterCommandSPIQPIxxx(&s_command, MXIC_SNOR_BLOCK_WRITE_LOCK_PROTECTION_CMD, MXIC_SNOR_FREAD_111, 0);
  s_command.Address     = Handle->Address;
  s_command.DummyCycles = 0;

  /* Configure the command */
  if (HAL_QSPI_Command(Ctx, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return MXIC_SNOR_ERROR_COMMAND;
  }

  return MXIC_SNOR_ERROR_NONE;
}
#endif  // MXIC_SNOR_BLOCK_WRITE_LOCK_PROTECTION_CMD

#ifdef MXIC_SNOR_PERMANENT_LOCK_CMD
/*
 * MXIC_SNOR_PERMANENT_LOCK_CMD                    0x64   // PLOCK, Permanent Lock; 1-1-0 + 3/4 Byte Address
 * @brief  Permanent Lock
 * @param  *Ctx            : Device handle
 *         Handle->Address : Permanent Lock address
 * @retval MXIC_xSPINORErrorTypeDef
 */
MXIC_xSPINORErrorTypeDef MXIC_SNOR_PermanentLock(void *Ctx, MXIC_SNOR_CommandHandle_t *Handle)
{
  QSPI_CommandTypeDef s_command;

  /* Setup command structure */
  SetupRegisterCommandSPIQPIxxx(&s_command, MXIC_SNOR_PERMANENT_LOCK_CMD, MXIC_SNOR_FREAD_111, 0);
  s_command.Address     = Handle->Address;
  s_command.DummyCycles = 0;

  /* Configure the command */
  if (HAL_QSPI_Command(Ctx, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return MXIC_SNOR_ERROR_COMMAND;
  }

  return MXIC_SNOR_ERROR_NONE;
}
#endif  // MXIC_SNOR_PERMANENT_LOCK_CMD

#ifdef MXIC_SNOR_READ_BLOCK_WRITE_LOCK_STATUS_CMD
/*
 * MXIC_SNOR_READ_BLOCK_WRITE_LOCK_STATUS_CMD      0xFB   // RDBLOCK, Read Block Write Lock status; 1-1-1
 * @brief  Read Block Protection Lock status
 * @param  Device handle
 *         Handle->Address : Block address
 *         Handle->pBuffer : Buffer pointer
 * @retval MXIC_xSPINORErrorTypeDef
 *         Handle->pBuffer[0] = Status Register value
 */
MXIC_xSPINORErrorTypeDef MXIC_SNOR_ReadBlockWriteLockStatus(void *Ctx, MXIC_SNOR_CommandHandle_t *Handle)
{
  QSPI_CommandTypeDef s_command;

  /* Setup command structure */
  SetupRegisterCommandSPIQPIxxx(&s_command, MXIC_SNOR_READ_BLOCK_WRITE_LOCK_STATUS_CMD, MXIC_SNOR_FREAD_111, 1);
  s_command.Address     = Handle->Address;
  s_command.DummyCycles = 8;

  /* Configure the command */
  if (HAL_QSPI_Command(Ctx, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return MXIC_SNOR_ERROR_COMMAND;
  }

  /* Reception of the data */
  if (HAL_QSPI_Receive(Ctx, Handle->pBuffer, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return MXIC_SNOR_ERROR_RECEIVE;
  }

  return MXIC_SNOR_ERROR_NONE;
}
#endif  // MXIC_SNOR_READ_BLOCK_WRITE_LOCK_STATUS_CMD

#if defined(MXIC_SNOR_READ_PERMANENT_LOCK_STATUS_CMD) || defined(MXIC_SNOR_READ_PERMANENT_LOCK_STATUS_8DC_CMD)
/*
 * MXIC_SNOR_READ_PERMANENT_LOCK_STATUS_CMD        0x3F   // RDPLOCK, Read Block Permanent Lock Status; 1-1-1 + 3/4 Byte Address
 * MXIC_SNOR_READ_PERMANENT_LOCK_STATUS_8DC_CMD    0x3F   // RDPLOCK, Read Block Permanent Lock Status; 1-1-1 + 3 Byte Address + 8 DC
 *
 * @brief  Read Block Permanent Lock Status
 * @param  *Ctx            : Device handle
 *         Handle->Address : Block address
 *         Handle->pBuffer : Buffer pointer
 * @retval MXIC_xSPINORErrorTypeDef
 *         Handle->pBuffer[0] = Block Permanent Lock Status value
 */
MXIC_xSPINORErrorTypeDef MXIC_SNOR_ReadBlockPermanentLockStatus(void *Ctx, MXIC_SNOR_CommandHandle_t *Handle)
{
  QSPI_CommandTypeDef s_command;

  /* Setup command structure */
#ifdef MXIC_SNOR_READ_PERMANENT_LOCK_STATUS_CMD
  SetupRegisterCommandSPIQPIxxx(&s_command, MXIC_SNOR_READ_PERMANENT_LOCK_STATUS_CMD, MXIC_SNOR_FREAD_111, 1);
  s_command.Address     = Handle->Address;
  s_command.DummyCycles = 0;

#else
  SetupRegisterCommandSPIQPIxxx(&s_command, MXIC_SNOR_READ_PERMANENT_LOCK_STATUS_8DC_CMD, MXIC_SNOR_FREAD_111, 1);
  s_command.Address     = Handle->Address;
  s_command.DummyCycles = 8;
#endif  // MXIC_SNOR_READ_PERMANENT_LOCK_STATUS_CMD

  /* Configure the command */
  if (HAL_QSPI_Command(Ctx, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return MXIC_SNOR_ERROR_COMMAND;
  }

  /* Reception of the data */
  if (HAL_QSPI_Receive(Ctx, Handle->pBuffer, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return MXIC_SNOR_ERROR_RECEIVE;
  }

  return MXIC_SNOR_ERROR_NONE;
}
#endif  // defined(MXIC_SNOR_READ_PERMANENT_LOCK_STATUS_CMD) || defined(MXIC_SNOR_READ_PERMANENT_LOCK_STATUS_8DC_CMD)

#ifdef MXIC_SNOR_CHIP_UNPROCECT_CMD
/*
 * MXIC_SNOR_CHIP_UNPROCECT_CMD                    0xF3   // UNLOCK, Chip unprotect; 1-0-0
 * @brief  Reset Block Write Lock Protection bit whole chip
 * @param  *Ctx            : Device handle
 * @retval MXIC_xSPINORErrorTypeDef
 */
MXIC_xSPINORErrorTypeDef MXIC_SNOR_ChipUnprotect(void *Ctx)
{
  return x00_Command(Ctx, MXIC_SNOR_CHIP_UNPROCECT_CMD, MXIC_SNOR_FREAD_111);
}
#endif  // MXIC_SNOR_CHIP_UNPROCECT_CMD

/* Password Register Operations */
#ifdef MXIC_SNOR_READ_PASSWORD_REGISTER_CMD
/*
 * MXIC_SNOR_READ_PASSWORD_REGISTER_CMD            0x27   // RDPASS, Read Password Register; 1-0-1/8S-8S-8S/8D-8D-8D
 * @brief  Read password register value. 8 Byte
 * @param  *Ctx            : Device handle
 *         Handle->pBuffer : Buffer pointer
 * @retval MXIC_xSPINORErrorTypeDef
 *         *Handle->pBuffer = Password (8 Byte)
 */
MXIC_xSPINORErrorTypeDef MXIC_SNOR_ReadPasswordRegister(void *Ctx, MXIC_SNOR_CommandHandle_t *Handle)
{
  QSPI_CommandTypeDef s_command;

  /* Setup command structure */
  SetupRegisterCommandSPIQPIx0x(&s_command, MXIC_SNOR_READ_PASSWORD_REGISTER_CMD, MXIC_SNOR_FREAD_111, 8);

  /* Configure the command */
  if (HAL_QSPI_Command(Ctx, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return MXIC_SNOR_ERROR_COMMAND;
  }

  /* Reception of the data */
  if (HAL_QSPI_Receive(Ctx, Handle->pBuffer, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return MXIC_SNOR_ERROR_RECEIVE;
  }

  return MXIC_SNOR_ERROR_NONE;
}
#endif  // MXIC_SNOR_READ_PASSWORD_REGISTER_CMD

#ifdef MXIC_SNOR_WRITE_PASSWORD_REGISTER_CMD
/*
 * MXIC_SNOR_WRITE_PASSWORD_REGISTER_CMD           0x28   // WRPASS, Write Password Register; 1-0-1/8S-8S-8S/8D-8D-8D
 * @brief  Write password register. Always write 8 byte data
 * @param  *Ctx            : Device handle
 *         Handle->pBuffer : Buffer pointer of Password (8 Byte)
 * @retval MXIC_xSPINORErrorTypeDef
 */
MXIC_xSPINORErrorTypeDef MXIC_SNOR_WritePasswordRegister(void *Ctx, MXIC_SNOR_CommandHandle_t *Handle)
{
  QSPI_CommandTypeDef s_command;

  /* Setup command structure */
  SetupRegisterCommandSPIQPIx0x(&s_command, MXIC_SNOR_WRITE_PASSWORD_REGISTER_CMD, MXIC_SNOR_FREAD_111, 8);

  /* Configure the command */
  if (HAL_QSPI_Command(Ctx, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return MXIC_SNOR_ERROR_COMMAND;
  }

  /* Transmission of the data */
  if (HAL_QSPI_Transmit(Ctx, Handle->pBuffer, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return MXIC_SNOR_ERROR_TRANSMIT;
  }

  return MXIC_SNOR_ERROR_NONE;
}
#endif  // MXIC_SNOR_WRITE_PASSWORD_REGISTER_CMD

#ifdef MXIC_SNOR_PASSWORD_UNLOCK_CMD
/*
 * MXIC_SNOR_PASSWORD_UNLOCK_CMD                   0x29   // PASSULK, Password Unlock; 1-0-1/8S-8S-8S/8D-8D-8D
 * @brief  Write password register. Always write 8 byte data
 * @param  *Ctx            : Device handle
 *         Handle->Mode    : Command interface
 *         Handle->pBuffer : Buffer pointer of password
 * @retval MXIC_xSPINORErrorTypeDef
 */
MXIC_xSPINORErrorTypeDef MXIC_SNOR_PasswordUnlock(void *Ctx, MXIC_SNOR_CommandHandle_t *Handle)
{
  QSPI_CommandTypeDef s_command;

  /* Setup command structure */
  SetupRegisterCommandSPIQPIx0x(&s_command, MXIC_SNOR_PASSWORD_UNLOCK_CMD, MXIC_SNOR_FREAD_111, 8);

  /* Configure the command */
  if (HAL_QSPI_Command(Ctx, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return MXIC_SNOR_ERROR_COMMAND;
  }

  /* Transmission of the data */
  if (HAL_QSPI_Transmit(Ctx, Handle->pBuffer, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return MXIC_SNOR_ERROR_TRANSMIT;
  }

  return MXIC_SNOR_ERROR_NONE;
}
#endif  // MXIC_SNOR_PASSWORD_UNLOCK_CMD

/* Replay Protected Monotonic Counter ************************************************************/
#ifdef MXIC_SNOR_RPMC_OP1_CMD
/*
 * MXIC_SNOR_RPMC_OP1_CMD                          0x9B   // OP1, RPMC Operation 1; SPI 1-0-1
 * WRITE_ROOT_KEY_REGISTER :
 *      Byte      |   0    |    1    |       2       |    3   |  4-35  |     36-63
 * ---------------+--------+---------+---------------+--------+--------+-------------------
 * Write Root Key |OP1 0x9B|CmdType 0|Counter Address|Reserved|Root Key|Truncated Signature
 *                |Command |                  Data
 *
 * UPDATE_HMAC_KEY_REGISTER :
 *      Byte      |   0    |    1    |       2       |    3   |  4-7   |     8-39
 * ---------------+--------+---------+---------------+--------+--------+-------------------
 * Update HMAC Key|OP1 0x9B|CmdType 1|Counter Address|Reserved|Key Data|     Signature
 *                |Command |                  Data
 *
 * INCREMENT_MONOTONIC_COUNTER :
 *      Byte      |   0    |    1    |       2       |    3   |    4-7   |   8-39
 * ---------------+--------+---------+---------------+--------+----------+-----------------
 * Inc Counter    |OP1 0x9B|CmdType 2|Counter Address|Reserved|Count Data|   Signature
 *                |Command |                  Data
 *
 * REQUEST_MONOTONIC_COUNTER :
 *      Byte      |   0    |    1    |       2       |    3   |4-15|     16-47
 * ---------------+--------+---------+---------------+--------+----+-----------------------
 * Request Counter|OP1 0x9B|CmdType 3|Counter Address|Reserved|Tag |    Signature
 *                |Command |                  Data
 *
 * @brief  RPMC OP1 command; write
 * @param  *Ctx            : Device handle
 *         Handle->Address : CmdType + Counter Address + Reserved 24 bit structure
 *         Handle->Size    : Data size in buffer
 *         Handle->pBuffer : RPMC data pointer
 * @retval MXIC_xSPINORErrorTypeDef
 */
MXIC_xSPINORErrorTypeDef MXIC_SNOR_RPMCOP1(void *Ctx, MXIC_SNOR_CommandHandle_t *Handle)
{
  QSPI_CommandTypeDef s_command;

  /* Setup command structure */
  SetupRegisterCommandSPIQPIx0x(&s_command, MXIC_SNOR_RPMC_OP1_CMD, MXIC_SNOR_FREAD_111, Handle->Size);

  /* Configure the command */
  if (HAL_QSPI_Command(Ctx,  &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return MXIC_SNOR_ERROR_COMMAND;
  }

  /* Transmission of the data */
  if (HAL_QSPI_Transmit(Ctx, Handle->pBuffer, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return MXIC_SNOR_ERROR_TRANSMIT;
  }

  return MXIC_SNOR_ERROR_NONE;
}
#endif  // MXIC_SNOR_RPMC_OP1_CMD

#ifdef MXIC_SNOR_RPMC_OP2_CMD
/*
 * MXIC_SNOR_RPMC_OP2_CMD                          0x96   // OP2, RPMC Operation 2; SPI 1-0-1
 * RPMC OP2, Read Data. 1-1-1
 * Byte|   0    |   1   |       2       |3-14|   15-18    | 19-50
 * ----+--------+-------+---------------+----+------------+---------
 * Name|OP2 0x96|Dummy 8|Extended Status|Tag |Counter Data|Signature
 *     |Command |       |     Data
 *
 * @brief  RPMC OP1 command; write
 * @param  *Ctx            : Device handle
 *         Handle->Size    : Data size in buffer
 *         Handle->pBuffer : RPMC data pointer
 * @retval MXIC_xSPINORErrorTypeDef
 */
MXIC_xSPINORErrorTypeDef MXIC_SNOR_RPMCOP2(void *Ctx, MXIC_SNOR_CommandHandle_t *Handle)
{
  QSPI_CommandTypeDef s_command;

  /* Setup command structure */
  SetupRegisterCommandSPIQPIx0x(&s_command, MXIC_SNOR_RPMC_OP2_CMD, MXIC_SNOR_FREAD_111, Handle->Size);
  s_command.DummyCycles = 8;                // Need 8 dummy clocks

  /* Configure the command */
  if (HAL_QSPI_Command(Ctx,  &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return MXIC_SNOR_ERROR_COMMAND;
  }

  /* Reception of the data */
  if (HAL_QSPI_Receive(Ctx, Handle->pBuffer, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return MXIC_SNOR_ERROR_RECEIVE;
  }

  return MXIC_SNOR_ERROR_NONE;
}

/*
 * MXIC_SNOR_RPMC_OP2_CMD                          0x96   // OP2, RPMC Operation 2; SPI 1-0-1
 * @brief  Use RPMC 0x96 command polling busy bit, MXIC_SNOR_RPMC_ESR_BUSY
 * @param  *Ctx : Device handle
 * @retval MXIC_xSPINORErrorTypeDef
 */
MXIC_xSPINORErrorTypeDef MXIC_SNOR_RPMCPollingReady(void *Ctx)
{
  QSPI_CommandTypeDef     s_command;
  QSPI_AutoPollingTypeDef s_config;

  /* Setup command structure */
  SetupRegisterCommandSPIQPIx0x(&s_command, MXIC_SNOR_RPMC_OP2_CMD, MXIC_SNOR_FREAD_111, 1);
  s_command.DummyCycles = 8;                // Need 8 dummy clocks

  /* Setup auto polling mask & match structure */
  s_config.Match           = 0;
  s_config.Mask            = MXIC_SNOR_RPMC_ESR_BUSY;
  s_config.MatchMode       = QSPI_MATCH_MODE_AND;
  s_config.StatusBytesSize = 1;
  s_config.Interval        = 0x10;
  s_config.AutomaticStop   = QSPI_AUTOMATIC_STOP_ENABLE;

  if (HAL_QSPI_AutoPolling(Ctx, &s_command, &s_config, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return MXIC_SNOR_ERROR_POLLING;
  }

  return MXIC_SNOR_ERROR_NONE;
}
#endif  // MXIC_SNOR_RPMC_OP2_CMD

/**************************************************************************************************
 * Macronix xSPI-NOR Security Flash Packet Command
 *************************************************************************************************/
/* Armor Flash Enter/Exit Security Field Operations **********************************************/
#ifdef MXIC_SNOR_ENTER_SECURITY_FIELD_CMD
/*
 * MXIC_SNOR_ENTER_SECURITY_FIELD_CMD              0xB2   // ENSF, Enter Security Field; SPI 1-0-0
 * @brief  Armor Flash Enter Security Field.
 * @param  *Ctx            : Device handle
 * @retval MXIC_xSPINORErrorTypeDef
 */
MXIC_xSPINORErrorTypeDef MXIC_SNOR_EnterSecurityField(void *Ctx)
{
  return x00_Command(Ctx, MXIC_SNOR_ENTER_SECURITY_FIELD_CMD, MXIC_SNOR_FREAD_111);
}
#endif  // MXIC_SNOR_ENTER_SECURITY_FIELD_CMD

#ifdef MXIC_SNOR_EXIT_SECURITY_FIELD_CMD
/*
 * MXIC_SNOR_EXIT_SECURITY_FIELD_CMD               0xC2   // EXSF, Exit Security Field; SPI 1-0-0
 * @brief  Armor Flash Exit Security Field.
 * @param  *Ctx            : Device handle
 * @retval MXIC_xSPINORErrorTypeDef
 */
MXIC_xSPINORErrorTypeDef MXIC_SNOR_ExitSecurityField(void *Ctx)
{
  return x00_Command(Ctx, MXIC_SNOR_EXIT_SECURITY_FIELD_CMD, MXIC_SNOR_FREAD_111);
}
#endif  // MXIC_SNOR_EXIT_SECURITY_FIELD_CMD

#ifdef SUPPORT_SECURITY_FIELD_COMMAND
/*
 *   Write a Packet Command to Packet Buffer
 * @brief  Armor 1/2 Flash security command         // SPI; 1-1-1
 * @param  *Ctx            : Device handle
 *         Handle->Address : MXIC_SNOR_PACKET_BUFFER_ADDRESS or MXIC_SNOR_PACKET_BUFFER_RESET_ADDRESS
 *         Handle->Size    : Program size in Byte or 0
 *         Handle->pBuffer : Buffer pointer of write data
 * @retval OSPI memory status
 */
MXIC_xSPINORErrorTypeDef MXIC_SNOR_WritePacket(void *Ctx, MXIC_SNOR_CommandHandle_t *Handle)
{
  QSPI_CommandTypeDef s_command;

  /* Setup command structure */
  SetupRegisterCommandSPIQPIxxx(&s_command, MXIC_SNOR_PAGE_PROG_111_CMD, MXIC_SNOR_FREAD_111, Handle->Size);
  s_command.Address     = Handle->Address;
  s_command.DummyCycles = 0;

  /* Configure the command */
  if (HAL_QSPI_Command(Ctx, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return MXIC_SNOR_ERROR_COMMAND;
  }

  /* Transmission of the data */
  if(s_command.NbData)
  {
    if (HAL_QSPI_Transmit(Ctx, Handle->pBuffer, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
      return MXIC_SNOR_ERROR_TRANSMIT;
    }
  }
  return MXIC_SNOR_ERROR_NONE;
}

/*
 *   Read data from Packet Buffer
 * @brief  Armor 1/2 Flash security command     // SPI; 1-1-1
 * @param  *Ctx            : Device handle
 *         Handle->Size    : Read size in Byte
 *         Handle->pBuffer : Buffer pointer for store data
 * @retval OSPI memory status
 */
MXIC_xSPINORErrorTypeDef MXIC_SNOR_ReadPacket(void *Ctx, MXIC_SNOR_CommandHandle_t *Handle)
{
  QSPI_CommandTypeDef s_command;

  /* Setup command structure */
#ifdef MXIC_SNOR_PACKET_OUT_CMD
  SetupRegisterCommandSPIQPIxxx(&s_command, MXIC_SNOR_PACKET_OUT_CMD, MXIC_SNOR_FREAD_111, Handle->Size);
  s_command.Address     = MXIC_SNOR_PACKET_BUFFER_ADDRESS;
  s_command.DummyCycles = MXIC_SNOR_DUMMY_CLOCK_111;

#else
  SetupRegisterCommandSPIQPIxxx(&s_command, MXIC_SNOR_NORMAL_READ_111_CMD, MXIC_SNOR_FREAD_111, Handle->Size);
  s_command.Address     = MXIC_SNOR_PACKET_BUFFER_ADDRESS;
  s_command.DummyCycles = 0;
#endif  // MXIC_SNOR_PACKET_OUT_CMD

  /* Configure the command */
  if (HAL_QSPI_Command(Ctx, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return MXIC_SNOR_ERROR_COMMAND;
  }

  /* Reception of the data */
  if (HAL_QSPI_Receive(Ctx, Handle->pBuffer, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return MXIC_SNOR_ERROR_RECEIVE;
  }

  return MXIC_SNOR_ERROR_NONE;
}
#endif  // SUPPORT_SECURITY_FIELD_COMMAND

/**************************************************************************************************
 * Macronix xSPI-NOR Flash Miscellaneous Command Pool
 *************************************************************************************************/
#ifdef MXIC_SNOR_NO_OPERATION_CMD
/*
 * MXIC_SNOR_NO_OPERATION_CMD                      0x00   // NOP, No Operation; 1-0-0/4-0-0/8S-0-0/8D-0-0
 * @brief  No operation, Use to terminate RSTEN (Reset Enable) command only
 * @param  *Ctx : Device handle
 * @retval MXIC_xSPINORErrorTypeDef
 */
MXIC_xSPINORErrorTypeDef MXIC_SNOR_NoOperation(void *Ctx, MXIC_SNOR_Mode_t Mode)
{
  return x00_Command(Ctx, MXIC_SNOR_NO_OPERATION_CMD, Mode.IO);
}
#endif  // MXIC_SNOR_NO_OPERATION_CMD

#ifdef MXIC_SNOR_RESET_ENABLE_CMD
/*
 * MXIC_SNOR_RESET_ENABLE_CMD                      0x66   // RSTEN, Reset Enable; 1-0-0/4-0-0/8S-0-0/8D-0-0
 * @brief  Software Reset; Reset Enable -> Reset Memory
 * @param  *Ctx : Device handle
 * @retval MXIC_xSPINORErrorTypeDef
 */
MXIC_xSPINORErrorTypeDef MXIC_SNOR_ResetEnable(void *Ctx, MXIC_SNOR_Mode_t Mode)
{
  return x00_Command(Ctx, MXIC_SNOR_RESET_ENABLE_CMD, Mode.IO);
}
#endif  // MXIC_SNOR_RESET_ENABLE_CMD

#ifdef MXIC_SNOR_RESET_MEMORY_CMD
/*
 * MXIC_SNOR_RESET_MEMORY_CMD                      0x99   // RST, Reset Memory; 1-0-0/4-0-0/8S-0-0/8D-0-0
 * @brief  Software Reset; Reset Enable -> Reset Memory
 * @param  *Ctx : Device handle
 * @retval MXIC_xSPINORErrorTypeDef
 */
MXIC_xSPINORErrorTypeDef MXIC_SNOR_ResetMemory(void *Ctx, MXIC_SNOR_Mode_t Mode)
{
  return x00_Command(Ctx, MXIC_SNOR_RESET_MEMORY_CMD, Mode.IO);
}
#endif  // MXIC_SNOR_RESET_MEMORY_CMD

#ifdef MXIC_SNOR_SOFTWARE_RESET_CMD
/*
 * MXIC_SNOR_SOFTWARE_RESET_CMD                    0xFF   // SWRES, Software Reset; 1-0-0
 * @brief  Software Reset;
 * @param  *Ctx : Device handle
 * @retval MXIC_xSPINORErrorTypeDef
 */
MXIC_xSPINORErrorTypeDef MXIC_SNOR_SoftwareReset(void *Ctx)
{
  return x00_Command(Ctx, MXIC_SNOR_SOFTWARE_RESET_CMD, MXIC_SNOR_FREAD_111);
}
#endif  // MXIC_SNOR_SOFTWARE_RESET_CMD

/* Read Serial Flash Discoverable Parameter command */
#ifdef MXIC_SNOR_READ_SFDP_CMD
/*
 * MXIC_SNOR_READ_SFDP_CMD                         0x5A    // RDSFDP, Read Serial Flash Discoverable Parameter; 1-1-1/8S-8S-8S/8D-8D-8D
 * @brief  Reads an amount of SFDP data from the OSPI memory.
 *         SFDP : Serial Flash Discoverable Parameter
 * @param  *Ctx            : Device handle
 *         Handle->Mode    : Command interface
 *         Handle->Address : Read SFDP table address
 *         Handle->Size    : Read SFDP table size in Byte
 *         Handle->pBuffer : Data buffer pointer
 * @retval MXIC_xSPINORErrorTypeDef
 *         *Handle->pBuffer = Data
 */
MXIC_xSPINORErrorTypeDef MXIC_SNOR_ReadSFDP(void *Ctx, MXIC_SNOR_CommandHandle_t *Handle)
{
  QSPI_CommandTypeDef s_command;

  /* Setup command structure */
  SetupRegisterCommandSPIQPIxxx(&s_command, MXIC_SNOR_READ_SFDP_CMD, MXIC_SNOR_FREAD_111, Handle->Size);
  s_command.AddressSize = QSPI_ADDRESS_24_BITS;     // SFDP address 24 bit always
  s_command.Address     = Handle->Address;
  s_command.DummyCycles = 8;

  /* Configure the command */
  if (HAL_QSPI_Command(Ctx, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return MXIC_SNOR_ERROR_COMMAND;
  }

  /* Reception of the data */
  if (HAL_QSPI_Receive(Ctx, Handle->pBuffer, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return MXIC_SNOR_ERROR_RECEIVE;
  }

  return MXIC_SNOR_ERROR_NONE;
}
#endif  // MXIC_SNOR_READ_SFDP_CMD

/* Enter/Release from Program/Erase Suspend Operations */
#ifdef MXIC_SNOR_PROGRAM_ERASE_SUSPEND_CMD
/*
 * MXIC_SNOR_PROGRAM_ERASE_SUSPEND_CMD             0xB0   // PGM/ERS Suspend, Suspends Program/Erase; 1-0-0/4-0-0/8S-0-0/8D-0-0
 * MXIC_SNOR_PROGRAM_ERASE_SUSPEND_CMD1            0x75   // PGM/ERS Suspend, Suspends Program/Erase; 1-0-0/4-0-0
 * @brief  Program/Erases suspend. Interruption Program/Erase operations.
 *         After the device has entered Erase-Suspended mode,
 *         system can read any address except the block/sector being Program/Erased.
 * @param  *Ctx            : Device handle
 * @retval MXIC_xSPINORErrorTypeDef
 */
MXIC_xSPINORErrorTypeDef MXIC_SNOR_ProgramEraseSuspend(void *Ctx, MXIC_SNOR_Mode_t Mode)
{
  return x00_Command(Ctx, MXIC_SNOR_PROGRAM_ERASE_SUSPEND_CMD, Mode.IO);
}
#endif  // MXIC_SNOR_PROGRAM_ERASE_SUSPEND_CMD

#ifdef MXIC_SNOR_PROGRAM_ERASE_RESUME_CMD
/*
 * MXIC_SNOR_PROGRAM_ERASE_RESUME_CMD              0x30   // PGM/ERS Resume, Resumes Program/Erase; 1-0-0/4-0-0/8S-0-0/8D-0-0
 * MXIC_SNOR_PROGRAM_ERASE_RESUME_CMD1             0x7A   // PGM/ERS Resume, Resumes Program/Erase; 1-0-0/4-0-0/8S-0-0/8D-0-0
 *
 * @brief  Program/Erases resume. Resume from Program/Erases suspend.
 * @param  *Ctx : Device handle
 * @retval MXIC_xSPINORErrorTypeDef
 */
MXIC_xSPINORErrorTypeDef MXIC_SNOR_ProgramEraseResume(void *Ctx, MXIC_SNOR_Mode_t Mode)
{
  return x00_Command(Ctx, MXIC_SNOR_PROGRAM_ERASE_RESUME_CMD, Mode.IO);
}
#endif  // MXIC_SNOR_PROGRAM_ERASE_RESUME_CMD

/* Deep Power Down Operations */
#ifdef MXIC_SNOR_DEEP_POWER_DOWN_CMD
/*
 * MXIC_SNOR_DEEP_POWER_DOWN_CMD                   0xB9   // DP, Deep power down; 1-0-0/4-0-0/8S-0-0/8D-0-0
 * @brief  Enter deep power down.
 *         The device is not active and all Write/Program/Erase instruction are ignored.
 *         tDP time is required before enter Deep power down after CS# goes high.
 * @param  *Ctx : Device handle
 * @retval MXIC_xSPINORErrorTypeDef
 */
MXIC_xSPINORErrorTypeDef MXIC_SNOR_EnterDeepPowerDown(void *Ctx, MXIC_SNOR_Mode_t Mode)
{
  return x00_Command(Ctx, MXIC_SNOR_DEEP_POWER_DOWN_CMD, Mode.IO);
}
#endif  // MXIC_SNOR_DEEP_POWER_DOWN_CMD

#ifdef MXIC_SNOR_RELEASE_FROM_DEEP_POWER_DOWN_CMD
/*
 * MXIC_SNOR_RELEASE_FROM_DEEP_POWER_DOWN_CMD      0xAB   // RDP, Release from Deep Power down; 1-0-0/4-0-0/8S-0-0/8D-0-0
 * @brief  Release from deep power down.
 *         After CS# go high, system need wait tRES1 time for device ready.
 * @param  *Ctx : Device handle
 * @retval MXIC_xSPINORErrorTypeDef
 */
MXIC_xSPINORErrorTypeDef MXIC_SNOR_ReleaseFromDeepPowerDown(void *Ctx, MXIC_SNOR_Mode_t Mode)
{
  return x00_Command(Ctx, MXIC_SNOR_RELEASE_FROM_DEEP_POWER_DOWN_CMD, Mode.IO);
}
#endif  // MXIC_SNOR_RELEASE_FROM_DEEP_POWER_DOWN_CMD

/* Factory Mode Enable Operations */
#ifdef MXIC_SNOR_FACTORY_MODE_ENABLE_CMD
/*
 * MXIC_SNOR_FACTORY_MODE_ENABLE_CMD               0x41   // FMEN, Factory Mode Enable; 1-0-0/4-0-0
 * @brief  Factory mode enable. For accelerate erase/program speed.
 *         Suspend command is not acceptable under factory mode.
 * @param  *Ctx : Device handle
 * @retval MXIC_xSPINORErrorTypeDef
 */
MXIC_xSPINORErrorTypeDef MXIC_SNOR_FactoryModeEnable(void *Ctx, MXIC_SNOR_Mode_t Mode)
{
  return x00_Command(Ctx, MXIC_SNOR_FACTORY_MODE_ENABLE_CMD, Mode.IO);
}
#endif  // MXIC_SNOR_FACTORY_MODE_ENABLE_CMD

#ifdef MXIC_SNOR_READ_FACTORY_MODE_STATUS_REGISTER_CMD
/*
 * MXIC_SNOR_READ_FACTORY_MODE_STATUS_REGISTER_CMD 0x44   // RDFMSR, Read Factory Mode Status Register; 1-0-1/4-0-4
 * @brief  Factory mode enable. For accelerate erase/program speed.
 *         Suspend command is not acceptable under factory mode.
 * @param  *Ctx            : Device handle
 *         Handle->Mode    : Command interface
 *         Handle->pBuffer : Buffer pointer
 * @retval MXIC_xSPINORErrorTypeDef
 *         Handle->pBuffer[0] = Factory Mode Status Register value
 *                              0x00 = Not in Factory Mode
 *                              0xFF = In Factory Mode
 */
MXIC_xSPINORErrorTypeDef MXIC_SNOR_ReadFactoryModeStatusRegister(void *Ctx, MXIC_SNOR_CommandHandle_t *Handle)
{
  QSPI_CommandTypeDef s_command;

  /* Setup command structure */
  SetupRegisterCommandSPIQPIx0x(&s_command, MXIC_SNOR_READ_FACTORY_MODE_STATUS_REGISTER_CMD, Handle->Mode.IO, 1);

  /* Configure the command */
  if (HAL_QSPI_Command(Ctx, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return MXIC_SNOR_ERROR_COMMAND;
  }

  /* Reception of the data */
  if (HAL_QSPI_Receive(Ctx, Handle->pBuffer, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return MXIC_SNOR_ERROR_RECEIVE;
  }

  return MXIC_SNOR_ERROR_NONE;
}
#endif  // MXIC_SNOR_READ_FACTORY_MODE_STATUS_REGISTER_CMD

/* Burst Read Wrap Around Operations */
#ifdef MXIC_SNOR_SET_BURST_LENGTH_CMD
/*
 * MXIC_SNOR_SET_BURST_LENGTH_CMD                  0xC0   // SBL, Set burst length; 1-0-1/4-0-4/8S-8S-8S/8D-8D-8D
 * MXIC_SNOR_SET_BURST_LENGTH_CMD1                 0x77   // SBL, Set burst length; 1-0-1/4-0-4
 *
 * @brief  Set burst length.
 *         The SPI and QPI mode 4READ and 4READ4B read commands support the wrap around feature.
 *         Read commands QPI "EBh" "ECh" and SPI "EBh" "ECh" support this feature.
 * @param  *Ctx               : Device handle
 *         Handle->Mode.IO    : I/O mode
 *         Handle->pBuffer[0] : Burst length setting
 * @retval MXIC_xSPINORErrorTypeDef
 */
MXIC_xSPINORErrorTypeDef MXIC_SNOR_SetBurstLength(void *Ctx, MXIC_SNOR_CommandHandle_t *Handle)
{
  QSPI_CommandTypeDef s_command;

  /* Setup command structure */
  SetupRegisterCommandSPIQPIx0x(&s_command, MXIC_SNOR_SET_BURST_LENGTH_CMD, Handle->Mode.IO, 1);

  /* Configure the command */
  if (HAL_QSPI_Command(Ctx, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return MXIC_SNOR_ERROR_COMMAND;
  }

  /* Transmission of the data */
  if (HAL_QSPI_Transmit(Ctx, Handle->pBuffer, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return MXIC_SNOR_ERROR_TRANSMIT;
  }

  return MXIC_SNOR_ERROR_NONE;
}
#endif  // MXIC_SNOR_SET_BURST_LENGTH_CMD

/**************************************************************************************************
 * Macronix xSPI-NOR Flash Special Command Pool
 *************************************************************************************************/
#ifdef MXIC_SNOR_ENABLE_SO_OUTPUT_RYBY_CMD
/*
 * MXIC_SNOR_ENABLE_SO_OUTPUT_RYBY_CMD             0x70   // ESRY, Enable SO to output RY/BY# during CP mode; 1-0-0
 * @brief  Enable SO to output RY/BY# during CP mode
 * @param  *Ctx : Device handle
 * @retval MXIC_xSPINORErrorTypeDef
 */
MXIC_xSPINORErrorTypeDef MXIC_SNOR_EnableSOOutputRYBY(void *Ctx)
{
  return x00_Command(Ctx, MXIC_SNOR_ENABLE_SO_OUTPUT_RYBY_CMD, MXIC_SNOR_FREAD_111);
}
#endif  // MXIC_SNOR_ENABLE_SO_OUTPUT_RYBY_CMD

#ifdef MXIC_SNOR_DISABLE_SO_OUTPUT_RYBY_CMD
/*
 * MXIC_SNOR_DISABLE_SO_OUTPUT_RYBY_CMD            0x80   // DSRY, Disable SO to output RY/BY# during CP mode; 1-0-0
 * @brief  Disable SO to output RY/BY# during CP mode
 * @param  *Ctx : Device handle
 * @retval MXIC_xSPINORErrorTypeDef
 */
MXIC_xSPINORErrorTypeDef MXIC_SNOR_DisableSOOutputRYBY(void *Ctx)
{
  return x00_Command(Ctx, MXIC_SNOR_DISABLE_SO_OUTPUT_RYBY_CMD, MXIC_SNOR_FREAD_111);
}
#endif  // MXIC_SNOR_DISABLE_SO_OUTPUT_RYBY_CMD

#ifdef MXIC_SNOR_HOLD_ENABLE_CMD
/*
 * MXIC_SNOR_HOLD_ENABLE_CMD                       0xAA   // HDE, Hold Enable, Enable HOLD# pin function; 1-0-0
 * @brief  Enable HOLD# pin function
 * @param  *Ctx : Device handle
 * @retval MXIC_xSPINORErrorTypeDef
 */
MXIC_xSPINORErrorTypeDef MXIC_SNOR_HoldEnable(void *Ctx)
{
  return x00_Command(Ctx, MXIC_SNOR_HOLD_ENABLE_CMD, MXIC_SNOR_FREAD_111);
}
#endif  // MXIC_SNOR_HOLD_ENABLE_CMD

#ifdef MXIC_SNOR_ENTER_PARALLEL_MODE_CMD
/*
 * MXIC_SNOR_ENTER_PARALLEL_MODE_CMD               0x55   // ENPLM, Enter Parallel Mode; 1-0-0
 * @brief  Enter Parallel Mode, Flash accept 1-1-8 command
 * @param  *Ctx : Device handle
 * @retval MXIC_xSPINORErrorTypeDef
 */
MXIC_xSPINORErrorTypeDef MXIC_SNOR_EnterParallelMode(void *Ctx)
{
  return x00_Command(Ctx, MXIC_SNOR_ENTER_PARALLEL_MODE_CMD, MXIC_SNOR_FREAD_111);
}
#endif  // MXIC_SNOR_ENTER_PARALLEL_MODE_CMD

#ifdef MXIC_SNOR_EXIT_PARALLEL_MODE_CMD
/*
 * MXIC_SNOR_EXIT_PARALLEL_MODE_CMD                0x45   // EXPLM, Exit Parallel Mode; 1-0-0
 * @brief  Exit Parallel Mode
 * @param  *Ctx : Device handle
 * @retval MXIC_xSPINORErrorTypeDef
 */
MXIC_xSPINORErrorTypeDef MXIC_SNOR_ExitParallelMode(void *Ctx)
{
  return x00_Command(Ctx, MXIC_SNOR_EXIT_PARALLEL_MODE_CMD, MXIC_SNOR_FREAD_111);
}
#endif  // MXIC_SNOR_EXIT_PARALLEL_MODE_CMD

#ifdef MXIC_SNOR_IDLE_ERASE_CMD
/*
 * MXIC_SNOR_IDLE_ERASE_CMD                        0x5E   // IE, Idle Erase; 1-0-0
 * @brief  Idele Erase
 * @param  *Ctx : Device handle
 * @retval MXIC_xSPINORErrorTypeDef
 */
MXIC_xSPINORErrorTypeDef MXIC_SNOR_IdleErase(void *Ctx)
{
  return x00_Command(Ctx, MXIC_SNOR_IDLE_ERASE_CMD, MXIC_SNOR_FREAD_111);
}
#endif  // MXIC_SNOR_IDLE_ERASE_CMD

#ifdef MXIC_SNOR_RELEASE_FROM_IDLE_ERASE_CMD
/*
 * MXIC_SNOR_RELEASE_FROM_IDLE_ERASE_CMD           0x5F   // RIE, Release from Idle Erase; 1-0-0
 * @brief  Release from Idle Erase
 * @param  *Ctx : Device handle
 * @retval MXIC_xSPINORErrorTypeDef
 */
MXIC_xSPINORErrorTypeDef MXIC_SNOR_ReleaseFromIdeleErase(void *Ctx)
{
  return x00_Command(Ctx, MXIC_SNOR_RELEASE_FROM_IDLE_ERASE_CMD, MXIC_SNOR_FREAD_111);
}
#endif  // MXIC_SNOR_RELEASE_FROM_IDLE_ERASE_CMD

#ifdef MXIC_SNOR_READ_IDLE_ERASE_STATUS_CMD
/*
 * MXIC_SNOR_READ_IDLE_ERASE_STATUS_CMD            0x14   // RDIEST, Read Idle Erase Status; 1-0-1
 * @brief  Read Idle Erase Status
 * @param  *Ctx : Device handle
 * @retval MXIC_xSPINORErrorTypeDef
 *         Handle->pBuffer[0] = Idle Erase Status
 */
MXIC_xSPINORErrorTypeDef MXIC_SNOR_ReadIdeleEraseStatus(void *Ctx, MXIC_SNOR_CommandHandle_t *Handle)
{
  QSPI_CommandTypeDef s_command;

  /* Setup command structure */
  SetupRegisterCommandSPIQPIx0x(&s_command, MXIC_SNOR_READ_IDLE_ERASE_STATUS_CMD, MXIC_SNOR_FREAD_111, 1);

  /* Configure the command */
  if (HAL_QSPI_Command(Ctx, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return MXIC_SNOR_ERROR_COMMAND;
  }

  /* Reception of the data */
  if (HAL_QSPI_Receive(Ctx, Handle->pBuffer, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return MXIC_SNOR_ERROR_RECEIVE;
  }

  return MXIC_SNOR_ERROR_NONE;
}
#endif  // MXIC_SNOR_READ_IDLE_ERASE_STATUS_CMD

#ifdef MXIC_SNOR_BP4_KEY1_CMD
/*
 * MXIC_SNOR_BP4_KEY1_CMD                          0xC3   // KEY1, Key command to set BP4; 1-0-0
 * @brief  Key command to set BP4
 * @param  *Ctx : Device handle
 * @retval MXIC_xSPINORErrorTypeDef
 */
MXIC_xSPINORErrorTypeDef MXIC_SNOR_BP4Key1(void *Ctx)
{
  return x00_Command(Ctx, MXIC_SNOR_BP4_KEY1_CMD, MXIC_SNOR_FREAD_111);
}
#endif  // MXIC_SNOR_BP4_KEY1_CMD

#ifdef MXIC_SNOR_BP4_KEY2_CMD
/*
 * MXIC_SNOR_BP4_KEY2_CMD                          0xA5   // KEY2, Key command to set BP4; 1-0-0
 * @brief  Key command to set BP4
 * @param  *Ctx : Device handle
 * @retval MXIC_xSPINORErrorTypeDef
 */
MXIC_xSPINORErrorTypeDef MXIC_SNOR_BP4Key2(void *Ctx)
{
  return x00_Command(Ctx, MXIC_SNOR_BP4_KEY2_CMD, MXIC_SNOR_FREAD_111);
}
#endif  // MXIC_SNOR_BP4_KEY2_CMD

/*
 * @brief  X-0-0 QSPI Command send
 * @param  Device handle
 * @retval MXIC_xSPINORErrorTypeDef
 * @retval QSPI memory status
 */
static MXIC_xSPINORErrorTypeDef x00_Command(void *Ctx, uint8_t OPCode, MXIC_SNOR_IOTypeDef IO)
{
  QSPI_CommandTypeDef s_command;

  /* Setup command structure */
  SetupRegisterCommandSPIQPIx0x(&s_command, OPCode, IO, 0);

  /* Configure the command */
  if (HAL_QSPI_Command(Ctx, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return MXIC_SNOR_ERROR_COMMAND;
  }

  return MXIC_SNOR_ERROR_NONE;
}

/*
 *           | 100 | 101 | 400 | 404
 * ----------+-----+-----+-----+-----
 * Address   |         None
 * ----------+-----+-----+-----+-----
 *  Dummy    |           0
 * ----------+-----+-----+-----+-----
 *   Size    |  0  | ~0  |  0  | ~0
 * ----------+-----+-----+-----+-----
 * Data Rate |None | STR |None | STR
 *
 * @brief  Setup 1-0-1/4-0-4 Register Command
 * @param  *OSPI_Command : Command setup pointer
 *         OPCode        : Instruction Operation Code
 *         Mode          : Command interface
 *         Size          : Data length ~0 setup to 1-0-1/4-0-4 format
 *                         Data length  0 setup to 1-0-0/4-0-0 format
 * @retval OSPI memory status
 */
static void SetupRegisterCommandSPIQPIx0x(QSPI_CommandTypeDef *QSPI_Command, uint8_t OPCode, MXIC_SNOR_IOTypeDef IO, uint32_t Size)
{
  SetupRegisterCommandSPIQPIxxx(QSPI_Command, OPCode, IO, Size);
  QSPI_Command->AddressMode = QSPI_ADDRESS_NONE;
  QSPI_Command->DummyCycles = 0;
}

/*
 *           | 110 | 111 | 440 | 444
 * ----------+-----+-----+-----+-----
 * Address   |     Yes, Not setup
 * ----------+-----+-----+-----+-----
 *  Dummy    |     Yes, Not setup
 * ----------+-----+-----+-----+-----
 *   Size    |  0  | ~0  |  0  | ~0
 * ----------+-----+-----+-----+-----
 * Data Rate |None | STR |None | STR
 *
 * @brief  Setup 1-1-x/4-4-x Register Command
 * @param  *OSPI_Command : Command setup pointer
 *         OPCode        : Instruction Operation Code
 *         Mode          : Command interface
 *         Size          : Data length ~0 setup to 1-1-1/4-4-4 format
 *                         Data length  0 setup to 1-1-0/4-4-0 format
 * @retval OSPI memory status
 *         Address & DummyCycles not setuped
 */
static void SetupRegisterCommandSPIQPIxxx(QSPI_CommandTypeDef *QSPI_Command, uint8_t OPCode, MXIC_SNOR_IOTypeDef IO, uint32_t Size)
{
  /* Setup command structure */
  QSPI_Command->InstructionMode    = ((IO == MXIC_SNOR_FREAD_444) || (IO == MXIC_SNOR_FREAD_444_4DC)) ? QSPI_INSTRUCTION_4_LINES : QSPI_INSTRUCTION_1_LINE;
  QSPI_Command->Instruction        = OPCode;
  QSPI_Command->AddressMode        = ((IO == MXIC_SNOR_FREAD_444) || (IO == MXIC_SNOR_FREAD_444_4DC)) ? QSPI_ADDRESS_4_LINES : QSPI_ADDRESS_1_LINE;

#if defined(SUPPORT_4BYTE_ADDRESS_COMMAND) || defined(SUPPORT_34BYTE_ADDRESS_SWITCH) || defined(FORCE_USE_4BYTE_ADDRESS)
  QSPI_Command->AddressSize        = QSPI_ADDRESS_32_BITS;;
#else
  QSPI_Command->AddressSize        = QSPI_ADDRESS_24_BITS;;
#endif  // defined(SUPPORT_4BYTE_ADDRESS_COMMAND) || defined(SUPPORT_34BYTE_ADDRESS_SWITCH) || defined(FORCE_USE_4BYTE_ADDRESS)

  //QSPI_Command->Address            =
  QSPI_Command->AlternateByteMode  = QSPI_ALTERNATE_BYTES_NONE;
  //QSPI_Command->AlternateBytesSize =
  //QSPI_Command->AlternateBytes     =
  //QSPI_Command->DummyCycles        =
  QSPI_Command->DataMode           = Size ? (((IO == MXIC_SNOR_FREAD_444) || (IO == MXIC_SNOR_FREAD_444_4DC)) ? QSPI_DATA_4_LINES : QSPI_DATA_1_LINE) : QSPI_DATA_NONE;
  QSPI_Command->NbData             = Size;
  QSPI_Command->DdrMode            = QSPI_DDR_MODE_DISABLE;
  QSPI_Command->DdrHoldHalfCycle   = QSPI_DDR_HHC_ANALOG_DELAY;
  QSPI_Command->SIOOMode           = QSPI_SIOO_INST_EVERY_CMD;
}
/********************** (C) COPYRIGHT Macronix *************** END OF FILE****/
