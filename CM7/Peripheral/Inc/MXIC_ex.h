/*************************************************************************************************
 *          Macronix xSPI-NOR Flash BSP driver extension header file for STM32 QUADSPI IP
 * @file    stm32xxx_QUADSPI_MXIC_ex.h
 * @brief   Define Flash specific commands except basic Erase/Program/Read functions.
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
 * 2022/12/08 Rename to stm32xxx_QUADSPI_MXIC_ex.h from stm32f769i_discovery_qspi_MXIC_ex.h
 * 2022/10/06 Move RPMC functions to stm32f769i_discovery_qspi_MXIC_ex.c
 * 2022/06/15 Release V1.0.0
 * 2022/05/04 Initial version.
 *************************************************************************************************/
/* Define to prevent recursive inclusion --------------------------------------------------------*/
#ifndef __STM32XXX_QUADSPI_MXIC_EX_H
#define __STM32XXX_QUADSPI_MXIC_EX_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Functions prototypes ------------------------------------------------------------------*/
#ifdef MXIC_SNOR_SECURE_OTP_SIZE
int32_t BSP_QSPI_EnterSecuredOTP(uint32_t Instance);
int32_t BSP_QSPI_ExitSecuredOTP(uint32_t Instance);
int32_t BSP_QSPI_WriteSecurityRegister(uint32_t Instance);
#endif  // MXIC_SNOR_SECURE_OTP_SIZE

#ifdef MXIC_SNOR_ADDITIONAL_MEMORY_SIZE
int32_t BSP_QSPI_Enter4KbitMode(uint32_t Instance);
int32_t BSP_QSPI_Exit4KbitMode(uint32_t Instance);
#endif  // MXIC_SNOR_ADDITIONAL_MEMORY_SIZE

#ifdef SUPPORT_PARALLEL_MODE
int32_t BSP_QSPI_EnterParallelMode(uint32_t Instance);
int32_t BSP_QSPI_ExitParallelMode(uint32_t Instance);
#endif  // SUPPORT_PARALLEL_MODE

#ifdef SUPPORT_SECURITY_FIELD_COMMAND
int32_t BSP_QSPI_EnterSecurityField(uint32_t Instance);
int32_t BSP_QSPI_ExitSecurityField(uint32_t Instance);
int32_t BSP_QSPI_WritePacket(uint32_t Instance, uint8_t *pData, uint32_t Size);
int32_t BSP_QSPI_ReadPacket(uint32_t Instance, uint8_t *pData, uint32_t Size);
#endif  // SUPPORT_SECURITY_FIELD_COMMAND

#ifdef SUPPORT_SFDP
int32_t BSP_QSPI_ReadSFDP(uint32_t Instance, uint8_t *pData, uint32_t Address, uint32_t Size);
#endif  // SUPPORT_SFDP

#ifdef SUPPORT_ADVANCED_SECTOR_PROTECTION
int32_t BSP_QSPI_EnableIndividualBlockProtectMode(uint32_t Instance);
int32_t BSP_QSPI_GangBlockLock(uint32_t Instance);
int32_t BSP_QSPI_GangBlockUnlock(uint32_t Instance);
int32_t BSP_QSPI_SingleBlockLock(uint32_t Instance, uint32_t Address);
int32_t BSP_QSPI_SingleBlockUnLock(uint32_t Instance, uint32_t Address);
int32_t BSP_QSPI_ReadBlockLockStatus(uint32_t Instance, MXIC_SNOR_BlockLockTypeDef *pData, uint32_t Address);

int32_t BSP_QSPI_ReadDPBRegister(uint32_t Instance, MXIC_SNOR_DPBRegisterTypeDef *pData, uint32_t Address);
int32_t BSP_QSPI_WriteDPBRegister(uint32_t Instance, MXIC_SNOR_DPBRegisterTypeDef Data, uint32_t Address);
int32_t BSP_QSPI_ReadSPBStatus(uint32_t Instance, MXIC_SNOR_SPBRegisterTypeDef *pData, uint32_t Address);
int32_t BSP_QSPI_WriteSPB(uint32_t Instance, uint32_t Address);
int32_t BSP_QSPI_EraseSPB(uint32_t Instance);

#ifdef MXIC_SNOR_READ_LOCK_REGISTER_CMD
int32_t BSP_QSPI_ReadLockRegister(uint32_t Instance, MXIC_SNOR_LockRegister_t *pData);
int32_t BSP_QSPI_WriteLockRegister(uint32_t Instance, MXIC_SNOR_LockRegister_t Data);
#endif

#ifdef SUPPORT_SPB_LOCK
int32_t BSP_QSPI_SPBLockSet(uint32_t Instance);
int32_t BSP_QSPI_ReadSPBLockRegister(uint32_t Instance, MXIC_SNOR_SPBLockRegister_t *pData);
#endif
#endif  // SUPPORT_ADVANCED_SECTOR_PROTECTION

#ifdef SUPPORT_BLOCK_PROTECTION
#ifdef MXIC_SNOR_WRITE_READ_LOCK_CONFIGURATION_CMD
int32_t BSP_QSPI_WriteReadLockAreaConfigurationRegister(uint32_t Instance, MXIC_SNOR_ReadLockRegister_t Data);
#endif

#ifdef MXIC_SNOR_READ_READ_LOCK_CONFIGURATION_CMD
int32_t BSP_QSPI_ReadReadLockAreaConfigurationRegister(uint32_t Instance, MXIC_SNOR_ReadLockRegister_t *pData);
#endif

#ifdef MXIC_SNOR_PERMANENT_LOCK_BIT_LOCK_DOWN_CMD
int32_t BSP_QSPI_PermanentLockbitLockDown(uint32_t Instance);
#endif

#ifdef MXIC_SNOR_READ_PERMANENT_LOCK_BIT_LOCK_DOWN_CMD
int32_t BSP_QSPI_ReadPermanentLockbitLockDown(uint32_t Instance, MXIC_SNOR_PermanentLockDownRegister_t *pData);
#endif

#ifdef MXIC_SNOR_WRITE_READ_LOCK_BIT_CMD
int32_t BSP_QSPI_WriteReadLockBit(uint32_t Instance, MXIC_SNOR_SecurityRegister_t Data);
#endif

#ifdef MXIC_SNOR_BLOCK_WRITE_LOCK_PROTECTION_CMD
int32_t BSP_QSPI_BlockWriteLockProtection(uint32_t Instance, uint32_t Address);
#endif

#ifdef MXIC_SNOR_PERMANENT_LOCK_CMD
int32_t BSP_QSPI_PermanentLock(uint32_t Instance, uint32_t Address);
#endif

#ifdef MXIC_SNOR_READ_BLOCK_WRITE_LOCK_STATUS_CMD
int32_t BSP_QSPI_ReadBlockWriteLockStatus(uint32_t Instance, uint8_t *pData, uint32_t Address);
#endif

#if defined(MXIC_SNOR_READ_PERMANENT_LOCK_STATUS_CMD) || defined(MXIC_SNOR_READ_PERMANENT_LOCK_STATUS_8DC_CMD)
int32_t BSP_QSPI_ReadBlockPermanentLockStatus(uint32_t Instance, MXIC_SNOR_BlockLockTypeDef *pData, uint32_t Address);
#endif

#ifdef MXIC_SNOR_CHIP_UNPROCECT_CMD
int32_t BSP_QSPI_ChipUnprotect(uint32_t Instance);
#endif
#endif  // SUPPORT_BLOCK_PROTECTION

#ifdef SUPPORT_BP4_KEY
int32_t BSP_QSPI_SetBP4Key1(uint32_t Instance);
int32_t BSP_QSPI_SetBP4Key2(uint32_t Instance);
#endif  // SUPPORT_BP4_KEY

#ifdef SUPPORT_PASSWORD
int32_t BSP_QSPI_ReadPasswordRegister(uint32_t Instance, uint8_t *pData);
int32_t BSP_QSPI_WritePasswordRegister(uint32_t Instance, uint8_t *pData);
int32_t BSP_QSPI_PasswordUnlock(uint32_t Instance, uint8_t *pData);
#endif  // SUPPORT_PASSWORD

#ifdef SUPPORT_SECURITY_REGISTER
int32_t BSP_QSPI_ReadSecurityRegister(uint32_t Instance, MXIC_SNOR_SecurityRegister_t *pData);
int32_t BSP_QSPI_ClearSecurityRegisterFailFlags(uint32_t Instance);
#endif  // SUPPORT_SECURITY_REGISTER

#ifdef SUPPORT_FAST_BOOT_REGISTER
int32_t BSP_QSPI_ReadFastBootRegister(uint32_t Instance, MXIC_SNOR_FastBootRegister_t *pData);
int32_t BSP_QSPI_WriteFastBootRegister(uint32_t Instance, MXIC_SNOR_FastBootRegister_t Data);
int32_t BSP_QSPI_EraseFastBootRegister(uint32_t Instance);
#endif  // SUPPORT_FAST_BOOT_REGISTER

#ifdef SUPPORT_PROGRAM_ERASE_SUSPEND
int32_t BSP_QSPI_ProgramEraseSuspend(uint32_t Instance);
int32_t BSP_QSPI_ProgramEraseResume(uint32_t Instance);
#endif  // SUPPORT_PROGRAM_ERASE_SUSPEND

#ifdef SUPPORT_DEEP_POWER_DOWN
int32_t BSP_QSPI_EnterDeepPowerDown(uint32_t Instance);
int32_t BSP_QSPI_ReleaseFromDeepPowerDown(uint32_t Instance);
#endif  // SUPPORT_DEEP_POWER_DOWN

#ifdef SUPPORT_FACTORY_MODE
int32_t BSP_QSPI_FactoryModeEnable(uint32_t Instance);
#endif  // SUPPORT_FACTORY_MODE

#ifdef SUPPORT_BURST_READ_WRAP_AROUND
int32_t BSP_QSPI_SetBurstLength(uint32_t Instance, MXIC_SNOR_WrapAroundTypeDef Data);
#endif  // SUPPORT_BURST_READ_WRAP_AROUND

#ifdef SUPPORT_RES_REMS_ID
int32_t BSP_QSPI_ReadElectronicSignature(uint32_t Instance, uint8_t *pData);
int32_t BSP_QSPI_ReadElectronicManufacturerDeviceID(uint32_t Instance, uint8_t *pData, uint32_t Address);
#endif  // SUPPORT_RES_REMS_ID

#ifdef SUPPORT_WRITE_BUFFER_ACCESS
int32_t BSP_QSPI_InterruptibleWrite(uint32_t Instance, uint8_t *pData, uint32_t Address, uint32_t Size);
int32_t BSP_QSPI_PageBufferRead(uint32_t Instance, uint8_t *pData, uint32_t Address, uint32_t Size);
int32_t BSP_QSPI_PageBufferContinueWrite(uint32_t Instance, uint8_t *pData, uint32_t Address, uint32_t Size);
int32_t BSP_QSPI_PageBufferConfirm(uint32_t Instance);
#endif  // SUPPORT_WRITE_BUFFER_ACCESS

#ifdef SUPPORT_SO_OUTPUT_RYBY
int32_t BSP_QSPI_EnableSOOutputRYBY(uint32_t Instance);
int32_t BSP_QSPI_DisableSOOutputRYBY(uint32_t Instance);
int32_t BSP_QSPI_EnableHoldPinFunction(uint32_t Instance);
#endif  // SUPPORT_RYBY_OUTPUT

#ifdef SUPPORT_IDLE_ERASE
int32_t BSP_QSPI_EnterIdleErase(uint32_t Instance);
int32_t BSP_QSPI_ReleaseFromIdeleErase(uint32_t Instance);
int32_t BSP_QSPI_ReadIdeleEraseStatus(uint32_t Instance, MXIC_SNOR_IdleEraseStatus_t *pData);
#endif  // SUPPORT_IDLE_ERASE

/*******************************************************************************
 * RPMC Functions
 ******************************************************************************/
#ifdef SUPPORT_RPMC
int32_t BSP_QSPI_RPMCWriteRootKeyRegister(uint32_t Instance, uint8_t CounterAddress, uint8_t *pRootKey);
int32_t BSP_QSPI_RPMCUpdateHMACKeyRegister(uint32_t Instance, uint8_t CounterAddress, uint8_t *pRootKey, uint8_t *pKeyData, uint8_t *pHMACKey);
int32_t BSP_QSPI_RPMCIncrementMonotonicCounter(uint32_t Instance, uint8_t CounterAddress, uint8_t *pHMACKey, uint32_t Counter);
int32_t BSP_QSPI_RPMCRequestMonotonicCounter(uint32_t Instance, uint8_t CounterAddress, uint8_t *pHMACKey, uint8_t *pTag, uint32_t *Counter);
int32_t BSP_QSPI_RPMCReadData(uint32_t Instance, uint8_t *pBuffer, uint8_t Size);
#endif  // SUPPORT_RPMC

#ifdef __cplusplus
}
#endif

#endif /* __STM32XXX_QUADSPI_MXIC_EX_H */
/************************ (C) COPYRIGHT Macronix *****END OF FILE****/
