/*************************************************************************************************
 *          Macronix xSPI-NOR Flash command pool driver header file
 * @file    MXIC_xSPINOR.h
 * @brief   Define command pool for the MXIC xSPI-NOR memory.
 * @Author  Peter Chang / Macronix; peterchang01@mxic.com.tw
 *************************************************************************************************
 *   MXIC_SNOR_IOTypeDef is base on Read Command defined. Other commands will mapping to it.
 *
 * Read Commands :
 *  Rate |    IO     | 3/4 Byte address | 4 Byte address | Enhance Read
 * ------+-----------+------------------+----------------+------------------
 *  STR  | 1-1-1+0DC | 0x03             | 0x13           |
 *       | 1-1-1     | 0x0B             | 0x0C           |
 *       | 1-1-2     | 0x3B             | 0x3C           |
 *       | 1-1-4     | 0x6B             | 0x6C           |
 *       | 1-1-8     | 0x03             | 0x7C           |
 *       | 1-2-2     | 0xBB             | 0xBC           |
 *       | 1-4-4     | 0xEA, 0xEB       | 0xEC           | 0xEA, 0xEB, 0xEC
 *       | 1-4-4+4DC | 0xE7             |       -        |
 *       | 4-4-4     | 0x0B, 0xEA, 0xEB | 0xEC           | 0xEA, 0xEB, 0xEC
 *       | 4-4-4+4DC | 0x0B, 0xE7       |       -        |
 *       | 8-8-8     |         -        | 0x7C, 0xEC     |
 * ------+-----------+------------------+----------------+------------------
 *  DTR  | 1-1-1+0DC |         -        |       -        |
 *       | 1-1-1     | 0x0D             | 0x0E           |
 *       | 1-1-2     |         -        |       -        |
 *       | 1-1-4     |         -        |       -        |
 *       | 1-1-8     |         -        |       -        |
 *       | 1-2-2     | 0xBD             | 0xBE           |
 *       | 1-4-4     | 0xED             | 0xEE           | 0xED, 0xEE
 *       | 1-4-4+4DC |         -        |       -        |
 *       | 4-4-4     | 0xED             | 0xEE           | 0xED, 0xEE
 *       | 4-4-4+4DC |         -        |       -        |
 *       | 8-8-8     |         -        | 0xEE           |
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
 *  STR  | 1-1-1+0DC | 1-0-0  0x60, 0xC7 (Chip Erase)
 *       | 1-1-1     | 1-1-0        Block Erase
 *       | 1-1-2     | 0x20, 0x52, 0xD8 | 0x21, 0x5C, 0xDC
 *       | 1-1-4     |                  |
 *       | 1-1-8     |                  |
 *       | 1-2-2     |                  |
 *       | 1-4-4     |                  |
 *       | 1-4-4+4DC |                  |
 *       +-----------+------------------+------------------
 *       | 4-4-4     | 4-0-0  0x60, 0xC7 (Chip Erase)
 *       | 4-4-4+4DC | 4-4-0        Block Erase
 *       |           | 0x20, 0x52, 0xD8 | 0x21, 0x5C, 0xDC
 *       +-----------+------------------+------------------
 *       | 8-8-8     | 8-0-0  0x60, 0xC7 (Chip Erase)
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
 *       | 8-8-8     | 8-0-0  0x60, 0xC7 (Chip Erase)
 *       |           | 8-8-0        Block Erase
 *       |           |         -        | 0x21, 0xDC
 *************************************************************************************************
 * History :
 * 2023/12/20 Release V2.0.0
 *            Move enum MXIC_SNOR_WrapAroundTypeDef form Flash header file to MXIC_xSPINOR.h
 *            Add Octal Flash constant define
 * 2023/01/30 Release V1.2.0
 * 2022/12/21 Add MXIC_SNOR_WritePacket() & MXIC_SNOR_ReadPacket() commands
 * 2022/12/12 Release V1.1.0
 * 2022/06/17 Release V1.0.1
 *            Remove test functions
 * 2022/06/15 Release V1.0.0
 * 2021/12/09 Initial version.
 *************************************************************************************************/
/* Define to prevent recursive inclusion --------------------------------------------------------*/
#ifndef __MXIC_xSPINOR_H
#define __MXIC_xSPINOR_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes Macronix xSPI-NOR Flash Specific Information ----------------------------------------*/
//#include "MXIC_SNOR_2G.h"
//#include "MXIC_SNOR_1G.h"
//#include "MXIC_SNOR_512M.h"
//#include "MXIC_SNOR_256M.h"
#include "MXIC_SNOR_128M.h"
//#include "MXIC_SNOR_64M.h"
//#include "MXIC_SNOR_32M.h"
//#include "MXIC_SNOR_16M.h"
//#include "MXIC_SNOR_8M.h"
//#include "MXIC_SNOR_4M.h"
//#include "MXIC_SNOR_2M.h"
//#include "MXIC_SNOR_1M.h"
//#include "MXIC_SNOR_512K.h"

/**************************************************************************************************
 * Constant
 *************************************************************************************************/
typedef enum    // Driver Error type
{
  MXIC_SNOR_ERROR_NOT_SUPPORTED = -8,
  MXIC_SNOR_ERROR_MEMORY_MAPPED = -7,
  MXIC_SNOR_ERROR_POLLING       = -6,       // Timeout
  MXIC_SNOR_ERROR_RECEIVE       = -5,
  MXIC_SNOR_ERROR_TRANSMIT      = -4,
  MXIC_SNOR_ERROR_COMMAND       = -3,
  MXIC_SNOR_ERROR_PARAMETER     = -2,
  MXIC_SNOR_ERROR_INIT          = -1,
  MXIC_SNOR_ERROR_NONE          =  0,
} MXIC_xSPINORErrorTypeDef;

typedef enum
{
  MXIC_SNOR_NREAD_111,          // 1-1-1 normal read command with 0 dummy cycle;          0x03, 0x13
  MXIC_SNOR_FREAD_111,          // 1-1-1 fast read command;                               0x0B, 0x0C, 0x0D, 0x0E
  MXIC_SNOR_FREAD_112,          // 1-1-2 fast read command;                               0x3B, 0x3C
  MXIC_SNOR_FREAD_114,          // 1-1-4 fast read command;                               0x6B, 0x6C
  MXIC_SNOR_FREAD_118,          // 1-1-8 fast read command or Parallel mode command;      0x03, 0x7C
  MXIC_SNOR_FREAD_122,          // 1-2-2 fast read command;                               0xBB, 0xBC, 0xBD, 0xBE
  MXIC_SNOR_FREAD_144,          // 1-4-4 fast read command;                               0xEA, 0xEB, 0xEC, 0xED, 0xEE
  MXIC_SNOR_FREAD_144_4DC,      // 1-4-4 fast read command with 4 dummy clock;            0xE7
  MXIC_SNOR_FREAD_444,          // 4-4-4 fast read QPI command;                           0x0B, 0xEA, 0xEB, 0xEC, 0xED, 0xEE
  MXIC_SNOR_FREAD_444_4DC,      // 4-4-4 fast read QPI command with 4 dummy clock;        0x0B, 0xE7
  MXIC_SNOR_FREAD_888,          // 8-8-8 fast read Octal command;                         0x7C, 0xEC, 0xEE
} MXIC_SNOR_IOTypeDef;

typedef enum
{
  MXIC_SNOR_STR = 0,            // STR Transfer rate
  MXIC_SNOR_DTR = 1,            // DTR Transfer rate
} MXIC_SNOR_TransferTypeDef;

typedef enum
{
#if defined(MXIC_SNOR_ERASE_4K_CMD) || defined(MXIC_SNOR_4BYTE_ADDRESS_ERASE_4K_CMD)
  MXIC_SNOR_ERASE_4K   = ( 4U * 1024U), // 4K size Sector erase
#else
  MXIC_SNOR_ERASE_4K   = 0,             // 4K size Sector erase not supported
#endif

#if defined(MXIC_SNOR_ERASE_32K_CMD) || defined(MXIC_SNOR_4BYTE_ADDRESS_ERASE_32K_CMD)
  MXIC_SNOR_ERASE_32K  = (32U * 1024U), // 32K size Block erase
#else
  MXIC_SNOR_ERASE_32K  = 0,             // 32K size Block erase not supported
#endif

#if defined(MXIC_SNOR_ERASE_64K_CMD) || defined(MXIC_SNOR_4BYTE_ADDRESS_ERASE_64K_CMD)
  MXIC_SNOR_ERASE_64K  = (64U * 1024U), // 64K size Block erase
#else
  MXIC_SNOR_ERASE_64K  = 0,             // 64K size Block erase not supported
#endif

#ifdef MXIC_SNOR_ERASE_CHIP_CMD
  MXIC_SNOR_ERASE_CHIP = ((uint32_t)0xFFFFFFFFU),    // Whole chip erase
#else
  MXIC_SNOR_ERASE_CHIP = 0,                          // Whole chip erase not supported
#endif
} MXIC_SNOR_EraseTypeDef;

#ifdef SUPPORT_BURST_READ_WRAP_AROUND   // Burst Read Wrap Around
typedef enum                    // Device Dependents
{
  MXIC_SNOR_WRAP_8 = 0,         // Wrap  8 Byte/Reserved on Octal Flash
  MXIC_SNOR_WRAP_16,            // Wrap 16 Byte
  MXIC_SNOR_WRAP_32,            // Wrap 32 Byte
  MXIC_SNOR_WRAP_64,            // Wrap 64 Byte
  MXIC_SNOR_WRAP_NONE = 0x1F,   // Disable wrap function
} MXIC_SNOR_WrapAroundTypeDef;
#endif

#ifdef SUPPORT_OCTAL                    // 8xx access command
#define OCTAL_READ_REGISTER_DUMMY_CLOCK         4U  // Dummy Clock for register read command
#define OCTAL_READ_REGISTER_DUMMY_CLOCK_DTR     5U  // Dummy Clock must > 4 on 8-8-8/DTR; OCTOSPI IP limitation
#define OCTAL_STATUS_REGISTER_ADDRESS           0U
#define OCTAL_CONFIGURATION_REGISTER_ADDRESS    1U
#endif

/**************************************************************************************************
 * Macronix xSPI-NOR Flash Data Structure Type
 *************************************************************************************************/
typedef struct                  // Macronix xSPI-NOR Flash Driver information
{
  uint32_t ID;                  // Driver Supported Flash ID
  uint32_t DeviceSize;          // Driver Supported Flash Size in Byte
  uint32_t PageSize;            // Driver Supported Flash Page Size in Byte
  uint32_t EraseType1;          // Driver Supported Flash Erase Type 1 Size in Byte; 0 = Not Supported
  uint32_t EraseType2;          // Driver Supported Flash Erase Type 2 Size in Byte; 0 = Not Supported
  uint32_t EraseType3;          // Driver Supported Flash Erase Type 3 Size in Byte; 0 = Not Supported
} MXIC_SNOR_DriverInfo_t;

typedef struct
{
  MXIC_SNOR_IOTypeDef       IO   : 7;   // MXIC_SNOR_IOTypeDef
  MXIC_SNOR_TransferTypeDef Rate : 1;   // MXIC_SNOR_TransferTypeDef; 0 = STR, 1 = DTR
} MXIC_SNOR_Mode_t;

typedef struct              // Macronix xSPI-NOR Flash Command information
{
  MXIC_SNOR_Mode_t Mode;    // Read/Program/Erase I/O Mode & STR/DTR Select; 1-1-1 ... 8D-8D-8D
  uint32_t Address;         // Read/Program/Erase Address; Command dependents
  uint32_t Size;            // Read/Program Size in Byte; Erase in Unit : 4K/32K/64K
  uint8_t  *pBuffer;        // Read/Program Data Buffer; Command dependents, could be input or output

  union
  {
    uint8_t  Type8[4];
    uint16_t Type16[2];
    uint32_t Type32;
  } Parameter;              // Function defined variable; Command dependents, could be input or output
} MXIC_SNOR_CommandHandle_t;

/**************************************************************************************************
 * Macronix xSPI-NOR Flash Function Pool
 *************************************************************************************************/
/* BSP Request Functions**************************************************************************/
MXIC_xSPINORErrorTypeDef MXIC_SNOR_GetDriverInfo(MXIC_SNOR_DriverInfo_t *pInfo);
MXIC_xSPINORErrorTypeDef MXIC_SNOR_AutoPollingMemReady(void *Ctx,  MXIC_SNOR_Mode_t Mode);
MXIC_xSPINORErrorTypeDef MXIC_SNOR_PollingStatusRegister(void *Ctx, MXIC_SNOR_CommandHandle_t *Handle);

#ifdef USE_AUTO_DC_CONFIGURATION
MXIC_xSPINORErrorTypeDef MXIC_SNOR_AutoDummyCycleConfigure(void *Ctx, MXIC_SNOR_CommandHandle_t *Handle);
#endif  // USE_AUTO_DC_CONFIGURATION

#ifdef SUPPORT_PERFORMANCE_ENHANCE_READ
MXIC_xSPINORErrorTypeDef MXIC_SNOR_PerformanceEnhanceModeReset(void *Ctx);
#endif  // SUPPORT_PERFORMANCE_ENHANCE_READ

/* Read/Write/Erase Memory Array Commands ********************************************************/
MXIC_xSPINORErrorTypeDef MXIC_SNOR_EnableMemoryMappedModeSTR(void *Ctx, MXIC_SNOR_Mode_t Mode);
MXIC_xSPINORErrorTypeDef MXIC_SNOR_ReadSTR(void *Ctx, MXIC_SNOR_CommandHandle_t *Handle);
MXIC_xSPINORErrorTypeDef MXIC_SNOR_PageProgramSTR(void *Ctx, MXIC_SNOR_CommandHandle_t *Handle);
MXIC_xSPINORErrorTypeDef MXIC_SNOR_BlockErase(void *Ctx, MXIC_SNOR_CommandHandle_t *Handle);
MXIC_xSPINORErrorTypeDef MXIC_SNOR_ChipErase(void *Ctx, MXIC_SNOR_Mode_t Mode);

#ifdef SUPPORT_DTR
MXIC_xSPINORErrorTypeDef MXIC_SNOR_EnableMemoryMappedModeDTR(void *Ctx, MXIC_SNOR_Mode_t Mode);
MXIC_xSPINORErrorTypeDef MXIC_SNOR_ReadDTR(void *Ctx, MXIC_SNOR_CommandHandle_t *Handle);
MXIC_xSPINORErrorTypeDef MXIC_SNOR_PageProgramDTR(void *Ctx, MXIC_SNOR_CommandHandle_t *Handle);
#endif  // SUPPORT_DTR

/**************************************************************************************************
 * Macronix xSPI-NOR Flash Write Buffer Access Command Pool; RWW (Read While Write)
 *************************************************************************************************/
#ifdef SUPPORT_WRITE_BUFFER_ACCESS
MXIC_xSPINORErrorTypeDef MXIC_SNOR_WriteBufferRead(void *Ctx, MXIC_SNOR_CommandHandle_t *Handle);
MXIC_xSPINORErrorTypeDef MXIC_SNOR_WriteBufferInitial(void *Ctx, MXIC_SNOR_CommandHandle_t *Handle);
MXIC_xSPINORErrorTypeDef MXIC_SNOR_WriteBufferContinue(void *Ctx, MXIC_SNOR_CommandHandle_t *Handle);
MXIC_xSPINORErrorTypeDef MXIC_SNOR_WriteBufferConfirm(void *Ctx, MXIC_SNOR_Mode_t Mode);
#endif  // SUPPORT_WRITE_BUFFER_ACCESS

/**************************************************************************************************
 * Macronix xSPI-NOR Flash Register Access Command Pool
 *************************************************************************************************/
MXIC_xSPINORErrorTypeDef MXIC_SNOR_ReadStatusRegister(void *Ctx, MXIC_SNOR_CommandHandle_t *Handle);

#ifdef MXIC_SNOR_READ_CONFIGURATION_REG_CMD
MXIC_xSPINORErrorTypeDef MXIC_SNOR_ReadConfigurationRegister(void *Ctx, MXIC_SNOR_CommandHandle_t *Handle);
MXIC_xSPINORErrorTypeDef MXIC_SNOR_WriteConfigurationRegister(void *Ctx, MXIC_SNOR_CommandHandle_t *Handle);
#endif  // MXIC_SNOR_READ_CONFIGURATION_REG_CMD

MXIC_xSPINORErrorTypeDef MXIC_SNOR_WriteStatusConfigurationRegister(void *Ctx, MXIC_SNOR_CommandHandle_t *Handle);
MXIC_xSPINORErrorTypeDef MXIC_SNOR_WriteEnable(void *Ctx, MXIC_SNOR_Mode_t Mode);
MXIC_xSPINORErrorTypeDef MXIC_SNOR_WriteDisable(void *Ctx, MXIC_SNOR_Mode_t Mode);

MXIC_xSPINORErrorTypeDef MXIC_SNOR_ReadID(void *Ctx, MXIC_SNOR_CommandHandle_t *Handle);

#ifdef SUPPORT_RES_REMS_ID
MXIC_xSPINORErrorTypeDef MXIC_SNOR_ReadElectronicSignature(void *Ctx, MXIC_SNOR_CommandHandle_t *Handle);
MXIC_xSPINORErrorTypeDef MXIC_SNOR_ReadElectronicManufacturerDeviceID(void *Ctx, MXIC_SNOR_CommandHandle_t *Handle);
#endif  // SUPPORT_RES_REMS_ID

#ifdef SUPPORT_QPI
MXIC_xSPINORErrorTypeDef MXIC_SNOR_QPIIDRead(void *Ctx, MXIC_SNOR_CommandHandle_t *Handle);
MXIC_xSPINORErrorTypeDef MXIC_SNOR_EnterQPIMode(void *Ctx);
MXIC_xSPINORErrorTypeDef MXIC_SNOR_ExitQPIMode(void *Ctx);
#endif  // SUPPORT_QPI

#ifdef SUPPORT_CONFIGURATION_REGISTER2
MXIC_xSPINORErrorTypeDef MXIC_SNOR_ReadConfigurationRegister2(void *Ctx, MXIC_SNOR_CommandHandle_t *Handle);
MXIC_xSPINORErrorTypeDef MXIC_SNOR_WriteConfigurationRegister2(void *Ctx, MXIC_SNOR_CommandHandle_t *Handle);
#endif  // SUPPORT_CONFIGURATION_REGISTER2

/* 3/4 Byte addressing mode Operations; Enter/Exit 3 Byte address command accept 4 Byte address mode */
#ifdef SUPPORT_34BYTE_ADDRESS_SWITCH
MXIC_xSPINORErrorTypeDef MXIC_SNOR_Enter4BytesAddressMode(void *Ctx, MXIC_SNOR_Mode_t Mode);
MXIC_xSPINORErrorTypeDef MXIC_SNOR_Exit4BytesAddressMode(void *Ctx, MXIC_SNOR_Mode_t Mode);
/* Extended Address Register Operations; Switch over 128M bit Flash bank for 3 Byte address command */
MXIC_xSPINORErrorTypeDef MXIC_SNOR_ReadExtendedAddressRegister(void *Ctx, MXIC_SNOR_CommandHandle_t *Handle);
MXIC_xSPINORErrorTypeDef MXIC_SNOR_WriteExtendedAddressRegister(void *Ctx, MXIC_SNOR_CommandHandle_t *Handle);
#endif  // SUPPORT_34BYTE_ADDRESS_SWITCH

/* Boot Register Operations */
#ifdef SUPPORT_FAST_BOOT_REGISTER
MXIC_xSPINORErrorTypeDef MXIC_SNOR_ReadFastBootRegister(void *Ctx, MXIC_SNOR_CommandHandle_t *Handle);
MXIC_xSPINORErrorTypeDef MXIC_SNOR_WriteFastBootRegister(void *Ctx, MXIC_SNOR_CommandHandle_t *Handle);
MXIC_xSPINORErrorTypeDef MXIC_SNOR_EraseFastBootRegister(void *Ctx, MXIC_SNOR_Mode_t Mode);
#endif  // SUPPORT_FAST_BOOT_REGISTER

/**************************************************************************************************
 * Macronix xSPI-NOR Flash Protection Command Pool
 *************************************************************************************************/
/* Secured OTP/ Additional Memory Array Operations */
#ifdef MXIC_SNOR_SECURE_OTP_SIZE
MXIC_xSPINORErrorTypeDef MXIC_SNOR_EnterSecuredOTP(void *Ctx, MXIC_SNOR_Mode_t Mode);
MXIC_xSPINORErrorTypeDef MXIC_SNOR_ExitSecuredOTP(void *Ctx, MXIC_SNOR_Mode_t Mode);
MXIC_xSPINORErrorTypeDef MXIC_SNOR_WriteSecurityRegister(void *Ctx, MXIC_SNOR_Mode_t Mode);
#elif defined(MXIC_SNOR_ADDITIONAL_MEMORY_SIZE)
MXIC_xSPINORErrorTypeDef MXIC_SNOR_Enter4KbitMode(void *Ctx);
MXIC_xSPINORErrorTypeDef MXIC_SNOR_Exit4KbitMode(void *Ctx);
#endif

/* Security Register Operations */
#ifdef SUPPORT_SECURITY_REGISTER
MXIC_xSPINORErrorTypeDef MXIC_SNOR_ReadSecurityRegister(void *Ctx, MXIC_SNOR_CommandHandle_t *Handle);
MXIC_xSPINORErrorTypeDef MXIC_SNOR_ClearSecurityRegisterFailFlags(void *Ctx);
#endif  // SUPPORT_SECURITY_REGISTER

/* SPB/DPB/Password/Lock Operations; Advanced Sector Protection */
#ifdef SUPPORT_ADVANCED_SECTOR_PROTECTION
/* SPB/DPB Operations; Advanced Sector Protection */
MXIC_xSPINORErrorTypeDef MXIC_SNOR_WriteProtectSelection(void *Ctx, MXIC_SNOR_Mode_t Mode);
MXIC_xSPINORErrorTypeDef MXIC_SNOR_GangBlockLock(void *Ctx, MXIC_SNOR_Mode_t Mode);
MXIC_xSPINORErrorTypeDef MXIC_SNOR_GangBlockUnlock(void *Ctx, MXIC_SNOR_Mode_t Mode);
MXIC_xSPINORErrorTypeDef MXIC_SNOR_SingleBlockLock(void *Ctx, MXIC_SNOR_CommandHandle_t *Handle);
MXIC_xSPINORErrorTypeDef MXIC_SNOR_SingleBlockUnLock(void *Ctx, MXIC_SNOR_CommandHandle_t *Handle);
MXIC_xSPINORErrorTypeDef MXIC_SNOR_ReadBlockLockStatus(void *Ctx, MXIC_SNOR_CommandHandle_t *Handle);
MXIC_xSPINORErrorTypeDef MXIC_SNOR_ReadDPBRegister(void *Ctx, MXIC_SNOR_CommandHandle_t *Handle);
MXIC_xSPINORErrorTypeDef MXIC_SNOR_WriteDPBRegister(void *Ctx, MXIC_SNOR_CommandHandle_t *Handle);
MXIC_xSPINORErrorTypeDef MXIC_SNOR_ReadSPBStatus(void *Ctx, MXIC_SNOR_CommandHandle_t *Handle);
MXIC_xSPINORErrorTypeDef MXIC_SNOR_WriteSPB(void *Ctx, MXIC_SNOR_CommandHandle_t *Handle);
MXIC_xSPINORErrorTypeDef MXIC_SNOR_EraseSPB(void *Ctx, MXIC_SNOR_Mode_t Mode);

/* Lock Register Operations */
MXIC_xSPINORErrorTypeDef MXIC_SNOR_ReadLockRegister(void *Ctx, MXIC_SNOR_CommandHandle_t *Handle);
MXIC_xSPINORErrorTypeDef MXIC_SNOR_WriteLockRegister(void *Ctx, MXIC_SNOR_CommandHandle_t *Handle);

#ifdef SUPPORT_SPB_LOCK                                        // Command 0xA6, 0xA7 & SPB Lock Register
MXIC_xSPINORErrorTypeDef MXIC_SNOR_SPBLockSet(void *Ctx);
MXIC_xSPINORErrorTypeDef MXIC_SNOR_ReadSPBLockRegister(void *Ctx, MXIC_SNOR_CommandHandle_t *Handle);
#endif  // SUPPORT_SPB_LOCK
#endif  // SUPPORT_ADVANCED_SECTOR_PROTECTION

/* 64K Block Write Lock Protection/Permanent Lock Operations*/
#ifdef SUPPORT_BLOCK_PROTECTION
MXIC_xSPINORErrorTypeDef MXIC_SNOR_ReadPermanentLockbitLockDown(void *Ctx, MXIC_SNOR_CommandHandle_t *Handle);
MXIC_xSPINORErrorTypeDef MXIC_SNOR_PermanentLockbitLockDown(void *Ctx);
MXIC_xSPINORErrorTypeDef MXIC_SNOR_ReadReadLockAreaConfigurationRegister(void *Ctx, MXIC_SNOR_CommandHandle_t *Handle);
MXIC_xSPINORErrorTypeDef MXIC_SNOR_WriteReadLockAreaConfigurationRegister(void *Ctx, MXIC_SNOR_CommandHandle_t *Handle);
MXIC_xSPINORErrorTypeDef MXIC_SNOR_WriteReadLockBit(void *Ctx, MXIC_SNOR_CommandHandle_t *Handle);
MXIC_xSPINORErrorTypeDef MXIC_SNOR_BlockWriteLockProtection(void *Ctx, MXIC_SNOR_CommandHandle_t *Handle);
MXIC_xSPINORErrorTypeDef MXIC_SNOR_PermanentLock(void *Ctx, MXIC_SNOR_CommandHandle_t *Handle);
MXIC_xSPINORErrorTypeDef MXIC_SNOR_ReadBlockWriteLockStatus(void *Ctx, MXIC_SNOR_CommandHandle_t *Handle);
MXIC_xSPINORErrorTypeDef MXIC_SNOR_ReadBlockPermanentLockStatus(void *Ctx, MXIC_SNOR_CommandHandle_t *Handle);
MXIC_xSPINORErrorTypeDef MXIC_SNOR_ChipUnprotect(void *Ctx);
#endif  // SUPPORT_BLOCK_PROTECTION

/* Password Register Operations */
#ifdef SUPPORT_PASSWORD
MXIC_xSPINORErrorTypeDef MXIC_SNOR_ReadPasswordRegister(void *Ctx, MXIC_SNOR_CommandHandle_t *Handle);
MXIC_xSPINORErrorTypeDef MXIC_SNOR_WritePasswordRegister(void *Ctx, MXIC_SNOR_CommandHandle_t *Handle);
MXIC_xSPINORErrorTypeDef MXIC_SNOR_PasswordUnlock(void *Ctx, MXIC_SNOR_CommandHandle_t *Handle);
#endif  // SUPPORT_PASSWORD

/* Replay Protected Monotonic Counter */
#ifdef SUPPORT_RPMC
MXIC_xSPINORErrorTypeDef MXIC_SNOR_RPMCOP1(void *Ctx, MXIC_SNOR_CommandHandle_t *Handle);
MXIC_xSPINORErrorTypeDef MXIC_SNOR_RPMCOP2(void *Ctx, MXIC_SNOR_CommandHandle_t *Handle);
MXIC_xSPINORErrorTypeDef MXIC_SNOR_RPMCPollingReady(void *Ctx);
#endif  // SUPPORT_RPMC

/* Armor Flash Enter/Exit Security Field Operations */
#ifdef SUPPORT_SECURITY_FIELD_COMMAND
MXIC_xSPINORErrorTypeDef MXIC_SNOR_EnterSecurityField(void *Ctx);
MXIC_xSPINORErrorTypeDef MXIC_SNOR_ExitSecurityField(void *Ctx);
MXIC_xSPINORErrorTypeDef MXIC_SNOR_WritePacket(void *Ctx, MXIC_SNOR_CommandHandle_t *Handle);
MXIC_xSPINORErrorTypeDef MXIC_SNOR_ReadPacket(void *Ctx, MXIC_SNOR_CommandHandle_t *Handle);
#endif  // SUPPORT_SECURITY_FIELD_COMMAND

/**************************************************************************************************
 * Macronix xSPI-NOR Flash Miscellaneous Command Pool
 *************************************************************************************************/
#ifdef SUPPORT_SOFTWARE_RESET
MXIC_xSPINORErrorTypeDef MXIC_SNOR_NoOperation(void *Ctx, MXIC_SNOR_Mode_t Mode);
MXIC_xSPINORErrorTypeDef MXIC_SNOR_ResetEnable(void *Ctx, MXIC_SNOR_Mode_t Mode);
MXIC_xSPINORErrorTypeDef MXIC_SNOR_ResetMemory(void *Ctx, MXIC_SNOR_Mode_t Mode);
MXIC_xSPINORErrorTypeDef MXIC_SNOR_SoftwareReset(void *Ctx);
#endif  // SUPPORT_SOFTWARE_RESET

/* Read Serial Flash Discoverable Parameter command */
#ifdef SUPPORT_SFDP
MXIC_xSPINORErrorTypeDef MXIC_SNOR_ReadSFDP(void *Ctx, MXIC_SNOR_CommandHandle_t *Handle);
#endif  // SUPPORT_SFDP

/* Enter/Release from Program/Erase Suspend Operations */
#ifdef SUPPORT_PROGRAM_ERASE_SUSPEND
MXIC_xSPINORErrorTypeDef MXIC_SNOR_ProgramEraseSuspend(void *Ctx, MXIC_SNOR_Mode_t Mode);
MXIC_xSPINORErrorTypeDef MXIC_SNOR_ProgramEraseResume(void *Ctx, MXIC_SNOR_Mode_t Mode);
#endif  // SUPPORT_PROGRAM_ERASE_SUSPEND

/* Deep Power Down Operations */
#ifdef SUPPORT_DEEP_POWER_DOWN
MXIC_xSPINORErrorTypeDef MXIC_SNOR_EnterDeepPowerDown(void *Ctx, MXIC_SNOR_Mode_t Mode);
MXIC_xSPINORErrorTypeDef MXIC_SNOR_ReleaseFromDeepPowerDown(void *Ctx, MXIC_SNOR_Mode_t Mode);
#endif  // SUPPORT_DEEP_POWER_DOWN

/* Factory Mode Enable Operations */
#ifdef SUPPORT_FACTORY_MODE
MXIC_xSPINORErrorTypeDef MXIC_SNOR_FactoryModeEnable(void *Ctx, MXIC_SNOR_Mode_t Mode);
MXIC_xSPINORErrorTypeDef MXIC_SNOR_ReadFactoryModeStatusRegister(void *Ctx, MXIC_SNOR_CommandHandle_t *Handle);
#endif  // SUPPORT_FACTORY_MODE

/* Burst Read Wrap Around Operations */
#ifdef SUPPORT_BURST_READ_WRAP_AROUND
MXIC_xSPINORErrorTypeDef MXIC_SNOR_SetBurstLength(void *Ctx, MXIC_SNOR_CommandHandle_t *Handle);
#endif  // SUPPORT_BURST_READ_WRAP_AROUND

/**************************************************************************************************
 * Macronix xSPI-NOR Flash Special Command Pool
 *************************************************************************************************/
#ifdef SUPPORT_SO_OUTPUT_RYBY
MXIC_xSPINORErrorTypeDef MXIC_SNOR_EnableSOOutputRYBY(void *Ctx);
MXIC_xSPINORErrorTypeDef MXIC_SNOR_DisableSOOutputRYBY(void *Ctx);
MXIC_xSPINORErrorTypeDef MXIC_SNOR_HoldEnable(void *Ctx);
#endif  // SUPPORT_RYBY_OUTPUT

#ifdef SUPPORT_PARALLEL_MODE
MXIC_xSPINORErrorTypeDef MXIC_SNOR_EnterParallelMode(void *Ctx);
MXIC_xSPINORErrorTypeDef MXIC_SNOR_ExitParallelMode(void *Ctx);
#endif  // SUPPORT_PARALLEL_MODE

#ifdef SUPPORT_IDLE_ERASE
MXIC_xSPINORErrorTypeDef MXIC_SNOR_IdleErase(void *Ctx);
MXIC_xSPINORErrorTypeDef MXIC_SNOR_ReleaseFromIdeleErase(void *Ctx);
MXIC_xSPINORErrorTypeDef MXIC_SNOR_ReadIdeleEraseStatus(void *Ctx, MXIC_SNOR_CommandHandle_t *Handle);
#endif  // SUPPORT_IDLE_ERASE

#ifdef SUPPORT_BP4_KEY
MXIC_xSPINORErrorTypeDef MXIC_SNOR_BP4Key1(void *Ctx);
MXIC_xSPINORErrorTypeDef MXIC_SNOR_BP4Key2(void *Ctx);
#endif  // SUPPORT_BP4_KEY

#ifdef __cplusplus
}
#endif

#endif /* __MXIC_xSPINOR_H */
/************************ (C) COPYRIGHT Macronix *****END OF FILE****/
