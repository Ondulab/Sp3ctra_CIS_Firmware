/**
 ******************************************************************************
 * @file           : icm42688.h
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __ICM42688_H__
#define __ICM42688_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>

typedef enum
{
	IMU_INIT_NOK = 0,
	IMU_INIT_OK,
}IMU_StateTypeDef;

typedef enum {
      dps2000 = 0x00,
      dps1000 = 0x01,
      dps500 = 0x02,
      dps250 = 0x03,
      dps125 = 0x04,
      dps62_5 = 0x05,
      dps31_25 = 0x06,
      dps15_625 = 0x07
} GyroFS;

typedef enum {
      gpm16 = 0x00,
      gpm8 = 0x01,
      gpm4 = 0x02,
      gpm2 = 0x03
} AccelFS;

typedef enum {
      odr32k = 0x01, // LN mode only
      odr16k = 0x02, // LN mode only
      odr8k = 0x03, // LN mode only
      odr4k = 0x04, // LN mode only
      odr2k = 0x05, // LN mode only
      odr1k = 0x06, // LN mode only
      odr200 = 0x07,
      odr100 = 0x08,
      odr50 = 0x09,
      odr25 = 0x0A,
      odr12_5 = 0x0B,
      odr6a25 = 0x0C, // LP mode only (accel only)
      odr3a125 = 0x0D, // LP mode only (accel only)
      odr1a5625 = 0x0E, // LP mode only (accel only)
      odr500 = 0x0F,
} ODR;

typedef struct {
    uint8_t address;
    uint8_t csPin;
} ICM42688;

///\brief Constants
#define WHO_AM_I			0xDB
#define NUM_CALIB_SAMPLES 	100 //1000

///\brief Conversion formula to get temperature in Celsius (Sec 4.13)
#define TEMP_DATA_REG_SCALE 132.48f
#define TEMP_OFFSET			25.0f

#define FIFO_EN				0x23
#define FIFO_TEMP_EN		0x04
#define FIFO_GYRO			0x02
#define FIFO_ACCEL			0x01

#define FIFO_SIZE			85
// #define FIFO_COUNT = 0x2E;
// #define FIFO_DATA = 0x30;

// BANK 1
// #define GYRO_CONFIG_STATIC2 = 0x0B;
#define GYRO_NF_ENABLE		0x00
#define GYRO_NF_DISABLE 	0x01
#define GYRO_AAF_ENABLE 	0x00
#define GYRO_AAF_DISABLE 	0x02

// BANK 2
// #define ACCEL_CONFIG_STATIC2 = 0x03;
#define ACCEL_AAF_ENABLE 	0x00
#define ACCEL_AAF_DISABLE 	0x01

// private functions
int icm42688_writeRegister(uint8_t subAddress, uint8_t data);
int icm42688_readRegisters(uint8_t subAddress, uint8_t count, uint8_t* dest);
int icm42688_setBank(uint8_t bank);

/**
 * @brief      Software reset of the device
 */
void icm42688_reset();

/**
 * @brief      Read the WHO_AM_I register
 *
 * @return     Value of WHO_AM_I register
 */
uint8_t icm42688_whoAmI();

typedef struct {
    uint8_t enFifoAccel;
    uint8_t enFifoGyro;
    uint8_t enFifoTemp;
    int32_t fifoSize;
    int32_t fifoFrameSize;
    float axFifo[FIFO_SIZE];
    float ayFifo[FIFO_SIZE];
    float azFifo[FIFO_SIZE];
    int32_t aSize;
    float gxFifo[FIFO_SIZE];
    float gyFifo[FIFO_SIZE];
    float gzFifo[FIFO_SIZE];
    int32_t gSize;
    float tFifo[256];
    int32_t tSize;
} ICM42688_FIFO;

    /**
     * @brief      Constructor for I2C communication
     *
     * @param      bus      I2C bus
     * @param[in]  address  Address of ICM 42688-p device
     */
    //ICM42688(TwoWire &bus, uint8_t address);

    /**
     * @brief      Constructor for SPI communication
     *
     * @param      bus    SPI bus
     * @param[in]  csPin  Chip Select pin
     */
    //ICM42688(SPIClass &bus, uint8_t csPin, uint32_t SPI_HS_CLK=8000000);

    /**
     * @brief      Initialize the device.
     *
     * @return     ret < 0 if error
     */
	int icm42688_init();

    /**
     * @brief      Sets the full scale range for the accelerometer
     *
     * @param[in]  fssel  Full scale selection
     *
     * @return     ret < 0 if error
     */
    int icm42688_setAccelFS(AccelFS fssel);

    /**
     * @brief      Sets the full scale range for the gyro
     *
     * @param[in]  fssel  Full scale selection
     *
     * @return     ret < 0 if error
     */
    int icm42688_setGyroFS(GyroFS fssel);

    /**
     * @brief      Set the ODR for accelerometer
     *
     * @param[in]  odr   Output data rate
     *
     * @return     ret < 0 if error
     */
    int icm42688_setAccelODR(ODR odr);

    /**
     * @brief      Set the ODR for gyro
     *
     * @param[in]  odr   Output data rate
     *
     * @return     ret < 0 if error
     */
    int icm42688_setGyroODR(ODR odr);

    int icm42688_setFilters(uint8_t gyroFilters, uint8_t accFilters);

    /**
     * @brief      Enables the data ready interrupt.
     *
     *             - routes UI data ready interrupt to INT1
     *             - push-pull, pulsed, active HIGH interrupts
     *
     * @return     ret < 0 if error
     */
    int icm42688_enableDataReadyInterrupt();

    /**
     * @brief      Masks the data ready interrupt
     *
     * @return     ret < 0 if error
     */
    int icm42688_disableDataReadyInterrupt();

    /**
     * @brief      Transfers data from ICM 42688-p to microcontroller.
     *             Must be called to access new measurements.
     *
     * @return     ret < 0 if error
     */
    int icm42688_getAGT();

    /**
     * @brief      Get accelerometer data, per axis
     *
     * @return     Acceleration in g's
     */
    float icm42688_accX();
    float icm42688_accY();
    float icm42688_accZ();

    /**
     * @brief      Get gyro data, per axis
     *
     * @return     Angular velocity in dps
     */
    float icm42688_gyrX();
    float icm42688_gyrY();
    float icm42688_gyrZ();

    /**
     * @brief      Get temperature of gyro die
     *
     * @return     Temperature in Celsius
     */
    float icm42688_temp();

    int icm42688_calibrateGyro();
    float icm42688_getGyroBiasX();
    float icm42688_getGyroBiasY();
    float icm42688_getGyroBiasZ();
    void icm42688_setGyroBiasX(float bias);
    void icm42688_setGyroBiasY(float bias);
    void icm42688_setGyroBiasZ(float bias);
    int icm42688_calibrateAccel();
    float icm42688_getAccelBiasX_mss();
    float icm42688_getAccelScaleFactorX();
    float icm42688_getAccelBiasY_mss();
    float icm42688_getAccelScaleFactorY();
    float icm42688_getAccelBiasZ_mss();
    float icm42688_getAccelScaleFactorZ();
    void icm42688_setAccelCalX(float bias,float scaleFactor);
    void icm42688_setAccelCalY(float bias,float scaleFactor);
    void icm42688_setAccelCalZ(float bias,float scaleFactor);
    void icm42688_TIM_Callback();

#endif // icm42688_H
