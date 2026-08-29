/**
 ******************************************************************************
 * @file           : icm42688.c
 ******************************************************************************
 * @attention
 *
 * Copyright (C) 2018-present Reso-nance Numerique.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 *
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include <stdbool.h>
#include <stdlib.h>
#include "string.h"
#include "main.h"
#include "basetypes.h"
#include "icm42688_registers.h"
#include "stdio.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
#include "icm42688.h"
#include "spi.h"
#include "config.h"
#include "file_manager.h"
#include "globals.h"

// buffer for reading from sensor
static uint8_t _buffer[15] = {};

static volatile uint8_t _bufferDMA[16] = {};
static volatile IMU_StateTypeDef  IMU_State = IMU_INIT_NOK;

/* Diagnostic counters for timer and SPI callback invocations */
volatile uint32_t icm_tim_count = 0;
volatile uint32_t icm_spi_cb_count = 0;

#ifdef DEBUG_ICM42688
static void icm42688_diag_task(void *argument)
{
    (void) argument;
    for (;;)
    {
        printf("DIAG: TIM=%lu SPI_CB=%lu\n", icm_tim_count, icm_spi_cb_count);
        osDelay(1000);
    }
}
static const osThreadAttr_t icm_diag_attributes = {
    .name = "icm_diag",
    .stack_size = 512 * 4,
    .priority = (osPriority_t) osPriorityLow,
};
static osThreadId_t icm_diag_tid = NULL;
#endif

static uint8_t _bank = 0; ///< current user bank

// data buffer
static float _t = 0.0f;
static float _acc[3] = {};
static float _gyr[3] = {};

///\brief Full scale resolution factors
static float _accelScale = 0.0f;
static float _gyroScale = 0.0f;

///\brief Full scale selections
static AccelFS _accelFS;
static GyroFS _gyroFS;

///\brief Accel calibration
static volatile float _accBD[3] = {};
static float _accB[3] = {};
static float _accS[3] = {1.0f, 1.0f, 1.0f};

///\brief Gyro calibration
static volatile float _gyroBD[3] = {};
static float _gyrB[3] = {};

static ICM42688_FIFO icm42688_FIFO;

#ifdef DEBUG_ICM42688
// Debug buffer to safely store IMU data
static volatile float _dbg_acc[3];
static volatile float _dbg_gyr[3];
static volatile float _dbg_t;
// Flag to indicate new data is available
static volatile bool _newDataAvailable = false;
#endif

/**
 * @brief      Get accelerometer data, per axis
 *
 * @return     Acceleration in m/s² (converted from g's)
 */
#define GRAVITY_MS2 9.81f  // Standard gravity constant for G to m/s² conversion

float icm42688_accX()
{
	return _acc[0] * GRAVITY_MS2;
}
float icm42688_accY()
{
	return _acc[1] * GRAVITY_MS2;
}
float icm42688_accZ()
{
	return _acc[2] * GRAVITY_MS2;
}

/**
 * @brief      Get gyro data, per axis
 *
 * @return     Angular velocity in dps
 */
float icm42688_gyrX()
{
	return _gyr[0];
}
float icm42688_gyrY()
{
	return _gyr[1];
}
float icm42688_gyrZ()
{
	return _gyr[2];
}

/**
 * @brief      Get temperature of gyro die
 *
 * @return     Temperature in Celsius
 */
float icm42688_temp()
{
	return _t;
}

/* starts communication with the ICM42688 */
ICM42688_StatusTypeDef icm42688_init()
{
	icm42688_FIFO.enFifoAccel = false;
	icm42688_FIFO.enFifoGyro = false;
	icm42688_FIFO.enFifoTemp = false;
	icm42688_FIFO.fifoSize = 0;
	icm42688_FIFO.fifoFrameSize = 0;
	memset(icm42688_FIFO.axFifo, 0, FIFO_SIZE * sizeof(float));
	memset(icm42688_FIFO.ayFifo, 0, FIFO_SIZE * sizeof(float));
	memset(icm42688_FIFO.azFifo, 0, FIFO_SIZE * sizeof(float));

	icm42688_FIFO.aSize = 0;

	memset(icm42688_FIFO.gxFifo, 0, FIFO_SIZE * sizeof(float));
	memset(icm42688_FIFO.gyFifo, 0, FIFO_SIZE * sizeof(float));
	memset(icm42688_FIFO.gzFifo, 0, FIFO_SIZE * sizeof(float));

	icm42688_FIFO.gSize = 0;

	memset(icm42688_FIFO.tFifo, 0, FIFO_SIZE * sizeof(float));

	memset(_buffer, 0, 15 * sizeof(uint8_t));

	icm42688_FIFO.tSize = 0;

	// reset the ICM42688
	if (icm42688_reset() != ICM42688_OK)
	{
		return ICM42688_ERROR;
	}

	// check the WHO AM I byte
	if (icm42688_whoAmI() != ICM42688_OK)
	{
		printf("failed to reset IMU\n");
		return ICM42688_ERROR;
	}

	// turn on accel and gyro in Low Noise (LN) Mode
	if (icm42688_writeRegister(UB0_REG_PWR_MGMT0, 0x0F) != ICM42688_OK)
	{
		printf("failed to turn on IMU\n");
		return ICM42688_ERROR;
	}

	// Set accelerometer to configured sensitivity from flash (default: ±4G)
	// Validate range and fallback to gpm2 if invalid
	AccelFS accel_fs = (shared_config.imu_accel_sensitivity <= 0x03) ?
	                   (AccelFS)shared_config.imu_accel_sensitivity : gpm2;
	if (icm42688_setAccelFS(accel_fs) != ICM42688_OK)
	{
		printf("failed to set ACC FS IMU\n");
		return ICM42688_ERROR;
	}

	// Set gyroscope to configured sensitivity from flash (default: ±500 dps)
	// Validate range and fallback to dps500 if invalid
	GyroFS gyro_fs = (shared_config.imu_gyro_sensitivity <= 0x07) ?
	                 (GyroFS)shared_config.imu_gyro_sensitivity : dps500;
	if (icm42688_setGyroFS(gyro_fs) != ICM42688_OK)
	{
		printf("failed to set GYRO FS IMU\n");
		return ICM42688_ERROR;
	}

	// Set ODR to 1000Hz for both sensors (optimal for fast motion capture)
	if (icm42688_setAccelODR(odr1k) != ICM42688_OK)
	{
		printf("failed to set ACC ODR IMU\n");
		return ICM42688_ERROR;
	}

	if (icm42688_setGyroODR(odr1k) != ICM42688_OK)
	{
		printf("failed to set GYRO ODR IMU\n");
		return ICM42688_ERROR;
	}

	// Enable inner filters for cleaner signals (optimized for gesture analysis)
	if (icm42688_setFilters(true, true) != ICM42688_OK)
	{
		printf("failed to set filters IMU\n");
		return ICM42688_ERROR;
	}

#ifdef DEBUG_ICM42688
    {
        uint8_t val = 0;
        // read back critical config registers for verification
        if (icm42688_readRegisters(UB0_REG_ACCEL_CONFIG0, 1, &val) == ICM42688_OK)
        {
            printf("ICM42688 DBG: ACCEL_CONFIG0=0x%02X\n", val);
        }
        if (icm42688_readRegisters(UB0_REG_GYRO_CONFIG0, 1, &val) == ICM42688_OK)
        {
            printf("ICM42688 DBG: GYRO_CONFIG0=0x%02X\n", val);
        }
        // read static filter config in banks 1 and 2
        if (icm42688_setBank(1) == ICM42688_OK)
        {
            if (icm42688_readRegisters(UB1_REG_GYRO_CONFIG_STATIC2, 1, &val) == ICM42688_OK)
            {
                printf("ICM42688 DBG: GYRO_CONFIG_STATIC2=0x%02X\n", val);
            }
        }
        if (icm42688_setBank(2) == ICM42688_OK)
        {
            if (icm42688_readRegisters(UB2_REG_ACCEL_CONFIG_STATIC2, 1, &val) == ICM42688_OK)
            {
                printf("ICM42688 DBG: ACCEL_CONFIG_STATIC2=0x%02X\n", val);
            }
        }
        // return to bank 0
        icm42688_setBank(0);
    }
#endif

	osDelay(100);

	// Try to load calibration from flash
	if (icm42688_loadCalibration(IMU_CALIBRATION_FILE_PATH) == ICM42688_OK)
	{
		printf("IMU: Calibration loaded from flash\n");
		printf("=== IMU CALIBRATION DEBUG ===\n");
		printf("Accel FS: %d (0=16g, 1=8g, 2=4g, 3=2g)\n", _accelFS);
		printf("Accel Scale: %.10f\n", _accelScale);
		printf("Accel Bias [X/Y/Z]: %.6f, %.6f, %.6f g\n", _accB[0], _accB[1], _accB[2]);
		printf("Accel Scale [X/Y/Z]: %.6f, %.6f, %.6f\n", _accS[0], _accS[1], _accS[2]);
		printf("Gyro FS: %d (0=2000dps, 1=1000dps, 2=500dps, 3=250dps)\n", _gyroFS);
		printf("Gyro Scale: %.10f\n", _gyroScale);
		printf("Gyro Bias [X/Y/Z]: %.6f, %.6f, %.6f dps\n", _gyrB[0], _gyrB[1], _gyrB[2]);
		printf("=============================\n");
	}
	else
	{
		// No valid calibration found - perform automatic calibration
		printf("IMU: No valid calibration found, performing auto-calibration...\n");
		printf("IMU: Please keep device stationary...\n");

		if (icm42688_performCalibration() == ICM42688_OK)
		{
			printf("IMU: Auto-calibration successful and saved\n");
		}
		else
		{
			printf("IMU: Auto-calibration failed, using default values\n");
			// Set default values (zeros)
			_gyrB[0] = _gyrB[1] = _gyrB[2] = 0.0f;
			_accB[0] = _accB[1] = 0.0f;
			_accB[2] = -1.0f; // Gravity offset
			_accS[0] = _accS[1] = _accS[2] = 1.0f;
		}
	}

	// successful init, return 1
	IMU_State = IMU_INIT_OK;

#ifdef DEBUG_ICM42688
    /* create diagnostic task to print counters periodically (non-blocking) */
    if (icm_diag_tid == NULL)
    {
        icm_diag_tid = osThreadNew(icm42688_diag_task, NULL, &icm_diag_attributes);
    }
#endif
	printf("IMU initialization SUCCESS\n");
	return ICM42688_OK;
}

/* sets the accelerometer full scale range to values other than default */
ICM42688_StatusTypeDef icm42688_setAccelFS(AccelFS fssel)
{
	icm42688_setBank(0);

	// read current register value
	uint8_t reg;
	if (icm42688_readRegisters(UB0_REG_ACCEL_CONFIG0, 1, &reg) != ICM42688_OK)
	{
		return ICM42688_ERROR;
	}

	// only change FS_SEL in reg
	reg = (fssel << 5) | (reg & 0x1F);

	if (icm42688_writeRegister(UB0_REG_ACCEL_CONFIG0, reg) != ICM42688_OK)
	{
		return ICM42688_ERROR;
	}

	_accelScale = (float)(1 << (4 - fssel)) / 32768.0f;
	_accelFS = fssel;

	return ICM42688_OK;
}

/* sets the gyro full scale range to values other than default */
ICM42688_StatusTypeDef icm42688_setGyroFS(GyroFS fssel)
{
	icm42688_setBank(0);

	// read current register value
	uint8_t reg;
	if (icm42688_readRegisters(UB0_REG_GYRO_CONFIG0, 1, &reg) != ICM42688_OK)
	{
		return ICM42688_ERROR;
	}

	// only change FS_SEL in reg
	reg = (fssel << 5) | (reg & 0x1F);

	if (icm42688_writeRegister(UB0_REG_GYRO_CONFIG0, reg) != ICM42688_OK)
	{
		return ICM42688_ERROR;
	}

	_gyroScale = (2000.0f / (float)(1 << fssel)) / 32768.0f;
	_gyroFS = fssel;

	return ICM42688_OK;
}

ICM42688_StatusTypeDef icm42688_setAccelODR(ODR odr)
{
	icm42688_setBank(0);

	// read current register value
	uint8_t reg;
	if (icm42688_readRegisters(UB0_REG_ACCEL_CONFIG0, 1, &reg) != ICM42688_OK)
	{
		return ICM42688_ERROR;
	}

	// only change ODR in reg
	reg = odr | (reg & 0xF0);

	if (icm42688_writeRegister(UB0_REG_ACCEL_CONFIG0, reg) != ICM42688_OK)
	{
		return ICM42688_ERROR;
	}

	return ICM42688_OK;
}

ICM42688_StatusTypeDef icm42688_setGyroODR(ODR odr)
{
	icm42688_setBank(0);

	// read current register value
	uint8_t reg;
	if (icm42688_readRegisters(UB0_REG_GYRO_CONFIG0, 1, &reg) != ICM42688_OK)
	{
		return ICM42688_ERROR;
	}

	// only change ODR in reg
	reg = odr | (reg & 0xF0);

	if (icm42688_writeRegister(UB0_REG_GYRO_CONFIG0, reg) != ICM42688_OK)
	{
		return ICM42688_ERROR;
	}

	return ICM42688_OK;
}

ICM42688_StatusTypeDef icm42688_setFilters(uint8_t gyroFilters, uint8_t accFilters)
{
	if (icm42688_setBank(1) != ICM42688_OK)
	{
		return ICM42688_ERROR;
	}

	if (gyroFilters == true)
	{
		if (icm42688_writeRegister(UB1_REG_GYRO_CONFIG_STATIC2, GYRO_NF_ENABLE | GYRO_AAF_ENABLE) != ICM42688_OK)
		{
			return ICM42688_ERROR;
		}
	}
	else
	{
		if (icm42688_writeRegister(UB1_REG_GYRO_CONFIG_STATIC2, GYRO_NF_DISABLE | GYRO_AAF_DISABLE) != ICM42688_OK)
		{
			return ICM42688_ERROR;
		}
	}

	if (icm42688_setBank(2) != ICM42688_OK)
	{
		return ICM42688_ERROR;
	}

	if (accFilters == true)
	{
		// Configure AAF for 473Hz bandwidth (optimal for 1000Hz ODR fast motion capture)
		// From datasheet table: 473Hz -> DELT=1, DELTSQR=1, BITSHIFT=15

		// UB2_REG_ACCEL_CONFIG_STATIC2 (0x03): bits 6:1 = ACCEL_AAF_DELT (1), bit 0 = ACCEL_AAF_DIS (0=enable)
		uint8_t config_static2 = (1 << 1) | ACCEL_AAF_ENABLE;  // DELT=1, AAF enabled
		if (icm42688_writeRegister(UB2_REG_ACCEL_CONFIG_STATIC2, config_static2) != ICM42688_OK)
		{
			return ICM42688_ERROR;
		}

		// UB2_REG_ACCEL_CONFIG_STATIC3 (0x04): bits 7:0 = ACCEL_AAF_DELTSQR low byte (1 & 0xFF = 1)
		if (icm42688_writeRegister(UB2_REG_ACCEL_CONFIG_STATIC3, 1) != ICM42688_OK)
		{
			return ICM42688_ERROR;
		}

		// UB2_REG_ACCEL_CONFIG_STATIC4 (0x05): bits 3:0 = ACCEL_AAF_DELTSQR high nibble (1 >> 8 = 0), bits 7:4 = ACCEL_AAF_BITSHIFT (15)
		uint8_t config_static4 = (15 << 4) | ((1 >> 8) & 0x0F);  // BITSHIFT=15, DELTSQR high=0
		if (icm42688_writeRegister(UB2_REG_ACCEL_CONFIG_STATIC4, config_static4) != ICM42688_OK)
		{
			return ICM42688_ERROR;
		}

#ifdef DEBUG_ICM42688
		printf("ICM42688: AAF configured for 473Hz (DELT=1, DELTSQR=1, BITSHIFT=15)\n");
#endif
	}
	else
	{
		// Disable AAF by setting ACCEL_AAF_DIS bit
		if (icm42688_writeRegister(UB2_REG_ACCEL_CONFIG_STATIC2, ACCEL_AAF_DISABLE) != ICM42688_OK)
		{
			return ICM42688_ERROR;
		}
#ifdef DEBUG_ICM42688
		printf("ICM42688: AAF disabled\n");
#endif
	}

	if (icm42688_setBank(0) != ICM42688_OK)
	{
		return ICM42688_ERROR;
	}
	return ICM42688_OK;
}

ICM42688_StatusTypeDef icm42688_enableDataReadyInterrupt()
{
	// push-pull, pulsed, active HIGH interrupts
	if (icm42688_writeRegister(UB0_REG_INT_CONFIG, 0x18 | 0x03) != ICM42688_OK)
	{
		return ICM42688_ERROR;
	}

	// need to clear bit 4 to allow proper INT1 and INT2 operation
	uint8_t reg;
	if (icm42688_readRegisters(UB0_REG_INT_CONFIG1, 1, &reg) != ICM42688_OK)
	{
		return ICM42688_ERROR;
	}
	reg &= ~0x10;
	if (icm42688_writeRegister(UB0_REG_INT_CONFIG1, reg) != ICM42688_OK)
	{
		return ICM42688_ERROR;
	}

	// route UI data ready interrupt to INT1
	if (icm42688_writeRegister(UB0_REG_INT_SOURCE0, 0x18) != ICM42688_OK)
	{
		return ICM42688_ERROR;
	}

	return ICM42688_OK;
}

ICM42688_StatusTypeDef icm42688_disableDataReadyInterrupt()
{
	// set pin 4 to return to reset value
	uint8_t reg;
	if (icm42688_readRegisters(UB0_REG_INT_CONFIG1, 1, &reg) != ICM42688_OK)
	{
		return ICM42688_ERROR;
	}
	reg |= 0x10;
	if (icm42688_writeRegister(UB0_REG_INT_CONFIG1, reg) != ICM42688_OK)
	{
		return ICM42688_ERROR;
	}

	// return reg to reset value
	if (icm42688_writeRegister(UB0_REG_INT_SOURCE0, 0x10) != ICM42688_OK)
	{
		return ICM42688_ERROR;
	}

	return ICM42688_OK;
}

/* reads the most current data from ICM42688 and stores in buffer */
ICM42688_StatusTypeDef icm42688_getAGT()
{
	if (icm42688_readRegisters(UB0_REG_TEMP_DATA1, 14, _buffer) != ICM42688_OK)
	{
		return ICM42688_ERROR;
	}

	// combine bytes into 16 bit values
	int16_t rawMeas[7]; // temp, accel xyz, gyro xyz
	for (int32_t i=0; i < 7; i++)
	{
		rawMeas[i] = ((int16_t)_buffer[ i * 2] << 8) | _buffer[ i * 2 + 1];
	}

	_t = ((float)rawMeas[0] / TEMP_DATA_REG_SCALE) + TEMP_OFFSET;

	_acc[0] = ((rawMeas[1] * _accelScale) - _accB[0]) * _accS[0];
	_acc[1] = ((rawMeas[2] * _accelScale) - _accB[1]) * _accS[1];
	_acc[2] = ((rawMeas[3] * _accelScale) - _accB[2]) * _accS[2];

	_gyr[0] = (rawMeas[4] * _gyroScale) - _gyrB[0];
	_gyr[1] = (rawMeas[5] * _gyroScale) - _gyrB[1];
	_gyr[2] = (rawMeas[6] * _gyroScale) - _gyrB[2];

	return ICM42688_OK;
}

/* configures and enables the FIFO buffer  */
ICM42688_StatusTypeDef icm42688_FIFO_enableFifo(uint8_t accel,uint8_t gyro,uint8_t temp)
{
	if(icm42688_writeRegister(FIFO_EN,(accel*FIFO_ACCEL)|(gyro*FIFO_GYRO)|(temp*FIFO_TEMP_EN)) != ICM42688_OK)
	{
		return ICM42688_ERROR;
	}
	icm42688_FIFO.enFifoAccel = accel;
	icm42688_FIFO.enFifoGyro = gyro;
	icm42688_FIFO.enFifoTemp = temp;
	icm42688_FIFO.fifoFrameSize = accel*6 + gyro*6 + temp*2;

	return ICM42688_OK;
}

/* reads data from the ICM42688 FIFO and stores in buffer */
int icm42688_FIFO_readFifo()
{
	icm42688_readRegisters(UB0_REG_FIFO_COUNTH, 2, _buffer);
	icm42688_FIFO.fifoSize = (((uint16_t) (_buffer[0]&0x0F)) <<8) + (((uint16_t) _buffer[1]));
	// read and parse the buffer
	for (int32_t i=0; i < icm42688_FIFO.fifoSize/icm42688_FIFO.fifoFrameSize; i++)
	{
		// grab the data from the ICM42688
		if (icm42688_readRegisters(UB0_REG_FIFO_DATA, icm42688_FIFO.fifoFrameSize, _buffer) != ICM42688_OK)
		{
			return ICM42688_OK;
		}
		if (icm42688_FIFO.enFifoAccel)
		{
			// combine into 16 bit values
			int16_t rawMeas[3];
			rawMeas[0] = (((int16_t)_buffer[0]) << 8) | _buffer[1];
			rawMeas[1] = (((int16_t)_buffer[2]) << 8) | _buffer[3];
			rawMeas[2] = (((int16_t)_buffer[4]) << 8) | _buffer[5];
			// transform and convert to float values
			icm42688_FIFO.axFifo[i] = ((rawMeas[0] * _accelScale) - _accB[0]) * _accS[0];
			icm42688_FIFO.ayFifo[i] = ((rawMeas[1] * _accelScale) - _accB[1]) * _accS[1];
			icm42688_FIFO.azFifo[i] = ((rawMeas[2] * _accelScale) - _accB[2]) * _accS[2];
			icm42688_FIFO.aSize = icm42688_FIFO.fifoSize / icm42688_FIFO.fifoFrameSize;
		}
		if (icm42688_FIFO.enFifoTemp)
		{
			// combine into 16 bit values
			int16_t rawMeas = (((int16_t)_buffer[0 + icm42688_FIFO.enFifoAccel*6]) << 8) | _buffer[1 + icm42688_FIFO.enFifoAccel*6];
			// transform and convert to float values
			icm42688_FIFO.tFifo[i] = ((float)rawMeas / TEMP_DATA_REG_SCALE) + TEMP_OFFSET;
			icm42688_FIFO.tSize = icm42688_FIFO.fifoSize/icm42688_FIFO.fifoFrameSize;
		}
		if (icm42688_FIFO.enFifoGyro)
		{
			// combine into 16 bit values
			int16_t rawMeas[3];
			rawMeas[0] = (((int16_t)_buffer[0 + icm42688_FIFO.enFifoAccel*6 + icm42688_FIFO.enFifoTemp*2]) << 8) | _buffer[1 + icm42688_FIFO.enFifoAccel*6 + icm42688_FIFO.enFifoTemp*2];
			rawMeas[1] = (((int16_t)_buffer[2 + icm42688_FIFO.enFifoAccel*6 + icm42688_FIFO.enFifoTemp*2]) << 8) | _buffer[3 + icm42688_FIFO.enFifoAccel*6 + icm42688_FIFO.enFifoTemp*2];
			rawMeas[2] = (((int16_t)_buffer[4 + icm42688_FIFO.enFifoAccel*6 + icm42688_FIFO.enFifoTemp*2]) << 8) | _buffer[5 + icm42688_FIFO.enFifoAccel*6 + icm42688_FIFO.enFifoTemp*2];
			// transform and convert to float values
			icm42688_FIFO.gxFifo[i] = (rawMeas[0] * _gyroScale) - _gyrB[0];
			icm42688_FIFO.gyFifo[i] = (rawMeas[1] * _gyroScale) - _gyrB[1];
			icm42688_FIFO.gzFifo[i] = (rawMeas[2] * _gyroScale) - _gyrB[2];
			icm42688_FIFO.gSize = icm42688_FIFO.fifoSize/icm42688_FIFO.fifoFrameSize;
		}
	}
	return ICM42688_OK;
}

/* returns the accelerometer FIFO size and data in the x direction, m/s/s */
void icm42688_FIFO_getFifoAccelX_mss(int32_t *size, float* data)
{
	*size = icm42688_FIFO.aSize;
	memcpy(data, icm42688_FIFO.axFifo, icm42688_FIFO.aSize*sizeof(float));
}

/* returns the accelerometer FIFO size and data in the y direction, m/s/s */
void icm42688_FIFO_getFifoAccelY_mss(int32_t *size, float* data)
{
	*size = icm42688_FIFO.aSize;
	memcpy(data, icm42688_FIFO.ayFifo, icm42688_FIFO.aSize*sizeof(float));
}

/* returns the accelerometer FIFO size and data in the z direction, m/s/s */
void icm42688_FIFO_getFifoAccelZ_mss(int32_t *size, float* data)
{
	*size = icm42688_FIFO.aSize;
	memcpy(data, icm42688_FIFO.azFifo, icm42688_FIFO.aSize*sizeof(float));
}

/* returns the gyroscope FIFO size and data in the x direction, dps */
void icm42688_FIFO_getFifoGyroX(int32_t *size, float* data)
{
	*size = icm42688_FIFO.gSize;
	memcpy(data, icm42688_FIFO.gxFifo, icm42688_FIFO.gSize*sizeof(float));
}

/* returns the gyroscope FIFO size and data in the y direction, dps */
void icm42688_FIFO_getFifoGyroY(int32_t *size, float* data)
{
	*size = icm42688_FIFO.gSize;
	memcpy(data, icm42688_FIFO.gyFifo, icm42688_FIFO.gSize*sizeof(float));
}

/* returns the gyroscope FIFO size and data in the z direction, dps */
void icm42688_FIFO_getFifoGyroZ(int32_t *size, float* data)
{
	*size = icm42688_FIFO.gSize;
	memcpy(data, icm42688_FIFO.gzFifo, icm42688_FIFO.gSize*sizeof(float));
}

/* returns the die temperature FIFO size and data, C */
void icm42688_FIFO_getFifoTemperature_C(int32_t *size,float* data)
{
	*size = icm42688_FIFO.tSize;
	memcpy(data, icm42688_FIFO.tFifo, icm42688_FIFO.tSize*sizeof(float));
}

/* estimates the gyro biases */
ICM42688_StatusTypeDef icm42688_calibrateGyro()
{
	// Prevent concurrent DMA transfers during calibration
	IMU_StateTypeDef saved_state = IMU_State;
	IMU_State = IMU_INIT_NOK;

	// set at a lower range (more resolution) since IMU not moving
	const GyroFS current_fssel = _gyroFS;
	if (icm42688_setGyroFS(dps250) != ICM42688_OK)
	{
		IMU_State = saved_state;
		return ICM42688_ERROR;
	}

	// Reset biases to zero before measurement to avoid cumulative errors
	// This ensures we measure raw sensor values, not already-compensated values
	_gyrB[0] = 0.0f;
	_gyrB[1] = 0.0f;
	_gyrB[2] = 0.0f;

	// take samples and find bias
	_gyroBD[0] = 0;
	_gyroBD[1] = 0;
	_gyroBD[2] = 0;

	for (int32_t i = 0; i < NUM_CALIB_SAMPLES; i++)
	{
		icm42688_getAGT();
		_gyroBD[0] += icm42688_gyrX();
		_gyroBD[1] += icm42688_gyrY();
		_gyroBD[2] += icm42688_gyrZ();
		osDelay(1);
	}

	_gyrB[0] = _gyroBD[0] / NUM_CALIB_SAMPLES;
	_gyrB[1] = _gyroBD[1] / NUM_CALIB_SAMPLES;
	_gyrB[2] = _gyroBD[2] / NUM_CALIB_SAMPLES;

	// recover the full scale setting
	if (icm42688_setGyroFS(current_fssel) != ICM42688_OK)
	{
		IMU_State = saved_state;
		return ICM42688_ERROR;
	}

	// Restore IMU state to allow DMA transfers
	IMU_State = saved_state;
	return ICM42688_OK;
}

/* returns the gyro bias in the X direction, dps */
float icm42688_getGyroBiasX()
{
	return _gyrB[0];
}

/* returns the gyro bias in the Y direction, dps */
float icm42688_getGyroBiasY()
{
	return _gyrB[1];
}

/* returns the gyro bias in the Z direction, dps */
float icm42688_getGyroBiasZ()
{
	return _gyrB[2];
}

/* sets the gyro bias in the X direction to bias, dps */
void icm42688_setGyroBiasX(float bias)
{
	_gyrB[0] = bias;
}

/* sets the gyro bias in the Y direction to bias, dps */
void icm42688_setGyroBiasY(float bias)
{
	_gyrB[1] = bias;
}

/* sets the gyro bias in the Z direction to bias, dps */
void icm42688_setGyroBiasZ(float bias)
{
	_gyrB[2] = bias;
}

/* finds bias and scale factor calibration for the accelerometer,
this should be run for each axis in each direction (6 total) to find
the min and max values along each */
ICM42688_StatusTypeDef icm42688_calibrateAccel()
{
	// Prevent concurrent DMA transfers during calibration
	IMU_StateTypeDef saved_state = IMU_State;
	IMU_State = IMU_INIT_NOK;

	// set at a lower range (more resolution) since IMU not moving
	const AccelFS current_fssel = _accelFS;
	if (icm42688_setAccelFS(gpm2) != ICM42688_OK)
	{
		IMU_State = saved_state;
		return ICM42688_ERROR;
	}

	// Reset biases and scale to identity before measurement to avoid cumulative errors
	// This ensures we measure raw sensor values, not already-compensated values
	_accB[0] = 0.0f;
	_accB[1] = 0.0f;
	_accB[2] = 0.0f;
	_accS[0] = 1.0f;
	_accS[1] = 1.0f;
	_accS[2] = 1.0f;

	// take samples and find min / max
	_accBD[0] = 0;
	_accBD[1] = 0;
	_accBD[2] = 0;

	for (int32_t i = 0; i < NUM_CALIB_SAMPLES; i++)
	{
		icm42688_getAGT();
		_accBD[0] += icm42688_accX();
		_accBD[1] += icm42688_accY();
		_accBD[2] += icm42688_accZ();

		// Wait for ~5ms between samples to preserve temporal spacing
		// Safe to use osDelay during initialization since IMU_State = IMU_INIT_NOK prevents DMA conflicts
		if (i < NUM_CALIB_SAMPLES - 1) // Don't wait after last sample
		{
			osDelay(5); // 5ms delay between samples for stable readings
		}
	}
	_accBD[0] /= NUM_CALIB_SAMPLES;
	_accBD[1] /= NUM_CALIB_SAMPLES;
	_accBD[2] /= NUM_CALIB_SAMPLES;

	// For gesture detection: preserve gravity while removing small DC offsets
	// Detect IMU orientation based on Z-axis measurement
	_accB[0] = _accBD[0];  // Remove X bias (should be ~0g when horizontal)
	_accB[1] = _accBD[1];  // Remove Y bias (should be ~0g when horizontal)

	// Handle both normal and upside-down IMU mounting
	if (_accBD[2] < -0.5f) {
		// IMU mounted upside-down (Z pointing down): measured gravity is negative
		_accB[2] = _accBD[2] + 1.0f;  // Add 1g to compensate for inverted mounting
		printf("IMU CAL: Upside-down mounting detected\n");
	} else {
		// IMU mounted normally (Z pointing up): measured gravity is positive
		_accB[2] = _accBD[2] - 1.0f;  // Subtract 1g as before
		printf("IMU CAL: Normal mounting detected\n");
	}

	printf("IMU CAL: Raw averages: X=%.6fg Y=%.6fg Z=%.6fg\n", _accBD[0], _accBD[1], _accBD[2]);
	printf("IMU CAL: Computed bias: X=%.6fg Y=%.6fg Z=%.6fg\n", _accB[0], _accB[1], _accB[2]);

	// recover the full scale setting
	if (icm42688_setAccelFS(current_fssel) != ICM42688_OK)
	{
		IMU_State = saved_state;
		return ICM42688_ERROR;
	}

	// Let sensor settle after configuration changes
	osDelay(20);

	// Reinitialize SPI interface to clear any potential HAL state corruption
	HAL_SPI_DeInit(&hspi2);
	MX_SPI2_Init();

	// Clean DMA buffer cache to ensure coherency
	SCB_CleanDCache_by_Addr((uint32_t*)_bufferDMA, sizeof(_bufferDMA));

	// Restore IMU state to allow DMA transfers
	IMU_State = saved_state;

	return ICM42688_OK;
}

/* returns the accelerometer bias in the X direction, m/s/s */
float icm42688_getAccelBiasX_mss()
{
	return _accB[0];
}

/* returns the accelerometer scale factor in the X direction */
float icm42688_getAccelScaleFactorX()
{
	return _accS[0];
}

/* returns the accelerometer bias in the Y direction, m/s/s */
float icm42688_getAccelBiasY_mss()
{
	return _accB[1];
}

/* returns the accelerometer scale factor in the Y direction */
float icm42688_getAccelScaleFactorY()
{
	return _accS[1];
}

/* returns the accelerometer bias in the Z direction, m/s/s */
float icm42688_getAccelBiasZ_mss()
{
	return _accB[2];
}

/* returns the accelerometer scale factor in the Z direction */
float icm42688_getAccelScaleFactorZ()
{
	return _accS[2];
}

/* sets the accelerometer bias (m/s/s) and scale factor in the X direction */
void icm42688_setAccelCalX(float bias,float scaleFactor)
{
	_accB[0] = bias;
	_accS[0] = scaleFactor;
}

/* sets the accelerometer bias (m/s/s) and scale factor in the Y direction */
void icm42688_setAccelCalY(float bias,float scaleFactor)
{
	_accB[1] = bias;
	_accS[1] = scaleFactor;
}

/* sets the accelerometer bias (m/s/s) and scale factor in the Z direction */
void icm42688_setAccelCalZ(float bias,float scaleFactor)
{
	_accB[2] = bias;
	_accS[2] = scaleFactor;
}

/* writes a byte to ICM42688 register given a register address and data */
ICM42688_StatusTypeDef icm42688_writeRegister(uint8_t subAddress, uint8_t data)
{
	static uint8_t tx[2];

	tx[0] = subAddress;
	tx[1] = data;

	/* write data to device */
	while(HAL_SPI_GetState(&hspi2) != HAL_SPI_STATE_READY);

	HAL_StatusTypeDef hal_status = HAL_SPI_Transmit(&hspi2, tx, 2, 1000);
	if (hal_status != HAL_OK)
	{
		printf("ICM42688: WRITE HAL_ERROR reg=0x%02X status=%d\n", subAddress, hal_status);
		return ICM42688_ERROR;
	}

	/* read back the register */
	if (icm42688_readRegisters(subAddress, 1, _buffer) != ICM42688_OK)
	{
		return ICM42688_ERROR;
	}
	/* check the read back register against the written register and log result */
	if(_buffer[0] == data)
	{
#ifdef DEBUG_ICM42688
		printf("ICM42688: WRITE OK reg=0x%02X val=0x%02X\n", subAddress, data);
#endif
		return ICM42688_OK;
	}
	else
	{
#ifdef DEBUG_ICM42688
		printf("ICM42688: WRITE FAIL reg=0x%02X wrote=0x%02X readback=0x%02X\n", subAddress, data, _buffer[0]);
#endif
		return ICM42688_ERROR;
	}
}

/* reads registers from ICM42688 given a starting register address, number of bytes, and a pointer to store data */
ICM42688_StatusTypeDef icm42688_readRegisters(uint8_t subAddress, uint8_t count, uint8_t* dest)
{
	static uint8_t tx[20] = {0};
	static uint8_t rx[20] = {0};

	subAddress |= 0x80;

	tx[0] = subAddress;

	while(HAL_SPI_GetState(&hspi2) != HAL_SPI_STATE_READY);

	HAL_StatusTypeDef hal_status = HAL_SPI_TransmitReceive(&hspi2, tx, rx, count + 1, 1000);
	if (hal_status != HAL_OK)
	{
		printf("ICM42688: READ HAL_ERROR reg=0x%02X status=%d\n", subAddress & 0x7F, hal_status);
		return ICM42688_ERROR;
	}

	for (int i = 0; i < count; i++)
	{
		dest[i] = rx[i + 1];
	}

	return ICM42688_OK;
}

ICM42688_StatusTypeDef icm42688_setBank(uint8_t bank)
{
	// if we are already on this bank, bail
	if (_bank == bank)
		return ICM42688_OK;

	_bank = bank;

	return icm42688_writeRegister(REG_BANK_SEL, bank);
}

ICM42688_StatusTypeDef icm42688_reset()
{
	// First, let's check if we can communicate with the device at all
	uint8_t test_val = 0;

	// Try to read WHO_AM_I without setting bank (should work from any bank)
	if (icm42688_readRegisters(UB0_REG_WHO_AM_I, 1, &test_val) == ICM42688_OK)
	{

	}
	else
	{
		return ICM42688_ERROR;
	}

	// Ensure we're on bank 0
	if (icm42688_setBank(0) != ICM42688_OK)
	{
		printf("ICM42688: Failed to set bank 0\n");
		return ICM42688_ERROR;
	}

	// According to datasheet: DEVICE_CONFIG bit 0 (SOFT_RESET_CONFIG)
	// 0 = Normal (default), 1 = Enable reset
	// After writing 1, the bit auto-clears and readback will be 0x00 (normal behavior)

	// Write 0x01 to trigger reset - don't check readback as it auto-clears
	static uint8_t tx[2];
	tx[0] = UB0_REG_DEVICE_CONFIG;
	tx[1] = 0x01;

	while(HAL_SPI_GetState(&hspi2) != HAL_SPI_STATE_READY);

	HAL_StatusTypeDef hal_status = HAL_SPI_Transmit(&hspi2, tx, 2, 1000);
	if (hal_status != HAL_OK)
	{
		printf("ICM42688: Reset write HAL_ERROR status=%d\n", hal_status);
		return ICM42688_ERROR;
	}

	// Wait 1ms as specified in datasheet, then additional time for device to come back up
	osDelay(2); // 1ms minimum + margin

	// Verify device is responsive after reset
	for (int retry = 0; retry < 10; retry++)
	{
		if (icm42688_readRegisters(UB0_REG_WHO_AM_I, 1, &test_val) == ICM42688_OK && test_val == WHO_AM_I)
		{
			// Check that DEVICE_CONFIG is back to 0x00 (normal state)
			return ICM42688_OK;
		}
		osDelay(1); // Wait 1ms between attempts
	}

	printf("ICM42688: Device not responsive after reset\n");
	return ICM42688_ERROR;
}

/* gets the ICM42688 WHO_AM_I register value */
ICM42688_StatusTypeDef icm42688_whoAmI()
{
	icm42688_setBank(0);

	// read the WHO AM I register
	if (icm42688_readRegisters(UB0_REG_WHO_AM_I, 1, _buffer) != ICM42688_OK)
	{
		return ICM42688_ERROR;
	}

	if (_buffer[0] != WHO_AM_I)
	{
		return ICM42688_ERROR;
	}

	return ICM42688_OK;
}

void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi)
{
	if(hspi->Instance == SPI2)
	{
        /* increment diagnostic SPI callback counter (non-blocking) */
        icm_spi_cb_count++;

		if ( IMU_State == IMU_INIT_OK)
		{
			SCB_InvalidateDCache_by_Addr ((uint32_t *)_bufferDMA, 16);

			/*
		uint32_t tmp = 0;
		for (int i = 0; i < 15; i++)
			tmp += _bufferDMA[i];

		if (tmp == 0)
			HAL_GPIO_TogglePin(LED1_GPIO_Port, LED1_Pin);
			 */

			// combine bytes into 16 bit values
			int16_t rawMeas[7]; // temp, accel xyz, gyro xyz
			for (int32_t i = 7; --i >= 0;)
			{
				rawMeas[i] = ((int16_t)_bufferDMA[i * 2 + 1] << 8) | _bufferDMA[ i * 2 + 2];
			}

	_t = ((float)rawMeas[0] / TEMP_DATA_REG_SCALE) + TEMP_OFFSET;

	_acc[0] = ((rawMeas[1] * _accelScale) - _accB[0]) * _accS[0];
	_acc[1] = ((rawMeas[2] * _accelScale) - _accB[1]) * _accS[1];
	_acc[2] = ((rawMeas[3] * _accelScale) - _accB[2]) * _accS[2];

	_gyr[0] = (rawMeas[4] * _gyroScale) - _gyrB[0];
	_gyr[1] = (rawMeas[5] * _gyroScale) - _gyrB[1];
	_gyr[2] = (rawMeas[6] * _gyroScale) - _gyrB[2];

#ifdef DEBUG_ICM42688
	// Debug: Print every 200th sample to avoid flooding (~1Hz at 200Hz sampling)
	for (int i = 1; i <= 3; i++)
	{
		if (rawMeas[i] == 32767 || rawMeas[i] == -32768)
		{
			printf("ICM42688: ACC SAT axis=%d raw=%d\n", i - 1, rawMeas[i]);
		}
	}
	for (int i = 4; i <= 6; i++)
	{
		if (rawMeas[i] == 32767 || rawMeas[i] == -32768)
		{
			printf("ICM42688: GYR SAT axis=%d raw=%d\n", i - 4, rawMeas[i]);
		}
	}
#endif

#ifdef DEBUG_ICM42688
			_dbg_acc[0] = _acc[0];
			_dbg_acc[1] = _acc[1];
			_dbg_acc[2] = _acc[2];
			_dbg_gyr[0] = _gyr[0];
			_dbg_gyr[1] = _gyr[1];
			_dbg_gyr[2] = _gyr[2];
			_dbg_t = _t;
			_newDataAvailable = true;
#endif
		}
	}
}

/**
 * @brief  Performs complete IMU calibration (gyro + accel) and saves to flash.
 * @note   Device MUST be stationary during calibration (~1.2 seconds).
 * @return ICM42688_OK if successful, ICM42688_ERROR otherwise.
 */
ICM42688_StatusTypeDef icm42688_performCalibration(void)
{
	printf("IMU: Starting calibration (keep device stationary)...\n");

	// Calibrate gyroscope (~200ms, 200 samples @ 1ms)
	if (icm42688_calibrateGyro() != ICM42688_OK)
	{
		printf("IMU: Gyro calibration FAILED\n");
		return ICM42688_ERROR;
	}
	printf("IMU: Gyro calibration OK (bias: X=%.3f Y=%.3f Z=%.3f dps)\n",
	       _gyrB[0], _gyrB[1], _gyrB[2]);

	// Calibrate accelerometer (~1000ms, 200 samples @ 5ms)
	if (icm42688_calibrateAccel() != ICM42688_OK)
	{
		printf("IMU: Accel calibration FAILED\n");
		return ICM42688_ERROR;
	}
	printf("IMU: Accel calibration OK (bias: X=%.3f Y=%.3f Z=%.3f g)\n",
	       _accB[0], _accB[1], _accB[2]);

	// Save calibration to flash
	if (icm42688_saveCalibration(IMU_CALIBRATION_FILE_PATH) != ICM42688_OK)
	{
		printf("IMU: WARNING - Failed to save calibration to flash\n");
		printf("IMU: Calibration data remains in RAM only\n");
		// Continue anyway - calibration is valid in RAM
	}
	else
	{
		printf("IMU: Calibration saved to flash successfully\n");
	}

	return ICM42688_OK;
}

/**
 * @brief  Saves current IMU calibration to flash file.
 * @param  filePath  Path to calibration file.
 * @return ICM42688_OK if successful, ICM42688_ERROR otherwise.
 */
ICM42688_StatusTypeDef icm42688_saveCalibration(const char* filePath)
{
	struct imuCals cal_data;

	// Fill calibration structure from current values
	cal_data.gyroBiasX = _gyrB[0];
	cal_data.gyroBiasY = _gyrB[1];
	cal_data.gyroBiasZ = _gyrB[2];
	cal_data.accelBiasX = _accB[0];
	cal_data.accelBiasY = _accB[1];
	cal_data.accelBiasZ = _accB[2];
	cal_data.accelScaleX = _accS[0];
	cal_data.accelScaleY = _accS[1];
	cal_data.accelScaleZ = _accS[2];

	// Write to file system
	if (file_writeImuCals(filePath, &cal_data) != FILEMANAGER_OK)
	{
		return ICM42688_ERROR;
	}

	return ICM42688_OK;
}

/**
 * @brief  Loads IMU calibration from flash file and applies it.
 * @param  filePath  Path to calibration file.
 * @return ICM42688_OK if successful, ICM42688_ERROR if file missing/corrupt.
 */
ICM42688_StatusTypeDef icm42688_loadCalibration(const char* filePath)
{
	struct imuCals cal_data;

	// Read from file system
	if (file_readImuCals(filePath, &cal_data) != FILEMANAGER_OK)
	{
		printf("IMU: Calibration file not found\n");
		return ICM42688_ERROR;
	}

	// Apply calibration values
	_gyrB[0] = cal_data.gyroBiasX;
	_gyrB[1] = cal_data.gyroBiasY;
	_gyrB[2] = cal_data.gyroBiasZ;
	_accB[0] = cal_data.accelBiasX;
	_accB[1] = cal_data.accelBiasY;
	_accB[2] = cal_data.accelBiasZ;
	_accS[0] = cal_data.accelScaleX;
	_accS[1] = cal_data.accelScaleY;
	_accS[2] = cal_data.accelScaleZ;

	return ICM42688_OK;
}

void icm42688_TIM_Callback()
{
    /* increment diagnostic timer callback counter (non-blocking) */
    icm_tim_count++;

	static const uint8_t tx[16] = {UB0_REG_TEMP_DATA1 | 0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

	if ( IMU_State == IMU_INIT_OK)
	{
		HAL_SPI_TransmitReceive_DMA(&hspi2, tx, (uint8_t *)_bufferDMA, 15);
	}
}

#ifdef DEBUG_ICM42688
void icm42688_handle_debug_print()
{
    if (_newDataAvailable) {
        // Local copy to avoid race conditions
        float acc[3], gyr[3], t;

        // Simple critical section to copy data
        __disable_irq();
        acc[0] = _dbg_acc[0]; acc[1] = _dbg_acc[1]; acc[2] = _dbg_acc[2];
        gyr[0] = _dbg_gyr[0]; gyr[1] = _dbg_gyr[1]; gyr[2] = _dbg_gyr[2];
        t = _dbg_t;
        _newDataAvailable = false;
        __enable_irq();

        printf("IMU: Acc[X:%+1.3fg Y:%+1.3fg Z:%+1.3fg] Gyr[X:%+3.1fdps Y:%+3.1fdps Z:%+3.1fdps] T:%.1f°C\n",
               acc[0], acc[1], acc[2],
               gyr[0], gyr[1], gyr[2],
               t);
    }
}
#endif
