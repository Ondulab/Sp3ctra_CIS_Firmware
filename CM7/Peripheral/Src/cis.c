/**
 ******************************************************************************
 * @file           : cis.c
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
#include "main.h"
#include "config.h"
#include "basetypes.h"
#include "globals.h"

#include "stdlib.h"
#include "stdio.h"
#include "stdbool.h"

#include "arm_math.h"

#include "FreeRTOS.h"
#include "cmsis_os.h"
#include "task.h"
#include "tim.h"
#include "adc.h"
#include "dma.h"
#include "mdma.h"
#include "iwdg.h"

#include "cis_scan.h"

#include "cis_linearCal.h"
#include "cis_polyCal.h"

#include "cis.h"

/* Private includes ----------------------------------------------------------*/

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static volatile CIS_BUFF_StateTypeDef  cisBufferState[CIS_ADC_OUT_LANES] = {0};

/* Variable containing ADC conversions data */

/* Private function prototypes -----------------------------------------------*/
static void cis_resetStart(void);
static void cis_initTimClock();
static void cis_initTimStartPulse();
static void cis_initTimLedRed();
static void cis_initTimLedGreen();
static void cis_initTimLedBlue();
static void cis_initAdc(void);

/* Private user code ---------------------------------------------------------*/

/**
 * @brief  CIS hardware initialization
 *         Initializes STM32 peripherals independent of the CIS configuration.
 * @param  None
 * @retval None
 */
CIS_StatusTypeDef cis_init(void)
{
    /* Enable 5V power DC/DC for display */
    HAL_GPIO_WritePin(EN_5V_GPIO_Port, EN_5V_Pin, GPIO_PIN_SET);

    cis_initAdc();

    if (cis_configure(shared_config.cis_dpi) != CIS_OK)
    {
    	return CIS_ERROR;
    }

#ifdef USE_WDG
    MX_IWDG1_Init();
#endif

    return CIS_OK;
}

/**
 * @brief  CIS configuration
 *         Configures the CIS, including DPI setting and variables dependent on DPI.
 * @param  dpi: Desired resolution in DPI (200 or 400)
 * @retval None
 */
CIS_StatusTypeDef cis_configure(uint16_t dpi)
{
    float32_t leds_duration_us;

    /* Stop any ongoing capture before reconfiguring */
    cis_stopCapture();

    /* Initialize variables based on the desired DPI */
    if (dpi == 400)
    {
        /* Variables for 400 DPI */
        cisConfig.pixels_per_lane = CIS_400DPI_PIXELS_PER_LANE;
        /* Set GPIO pin to RESET for 400 DPI */
        HAL_GPIO_WritePin(CIS_RS_GPIO_Port, CIS_RS_Pin, GPIO_PIN_RESET); // RESET : 400DPI
        leds_duration_us = CIS_400DPI_LED_DURATION_US;
    }
    else // Default to 200 DPI
    {
        /* Variables for 200 DPI */
        cisConfig.pixels_per_lane = CIS_200DPI_PIXELS_PER_LANE;
        /* Set GPIO pin to SET for 200 DPI */
        HAL_GPIO_WritePin(CIS_RS_GPIO_Port, CIS_RS_Pin, GPIO_PIN_SET); // SET : 200DPI
        leds_duration_us = CIS_200DPI_LED_DURATION_US;
    }

    osDelay(50);

    /* Common configurations */
    cisConfig.pixels_nb = cisConfig.pixels_per_lane * CIS_ADC_OUT_LANES;
    cisConfig.pixel_area_stop = CIS_INACTIVE_WIDTH + cisConfig.pixels_per_lane;
    cisConfig.start_offset = CIS_INACTIVE_WIDTH - CIS_SP_WIDTH + 2;
    cisConfig.lane_size = cisConfig.pixel_area_stop + CIS_OVER_SCAN;

    cisConfig.adc_buff_size = cisConfig.lane_size * CIS_ADC_OUT_LANES;

    /* Update lane offsets */
    cisConfig.red_lane_offset = cisConfig.start_offset;
    cisConfig.green_lane_offset = cisConfig.lane_size + cisConfig.start_offset;
    cisConfig.blue_lane_offset = (cisConfig.lane_size * 2) + cisConfig.start_offset;

    /* Initialize buffers */
    memset(cisData_ADC1, 0, cisConfig.adc_buff_size * sizeof(uint16_t));
    memset(cisData_ADC2, 0, cisConfig.adc_buff_size * sizeof(uint16_t));
    memset(cisData_ADC3, 0, cisConfig.adc_buff_size * sizeof(uint16_t));
    memset(cisDataCpy, 0, cisConfig.adc_buff_size * 3 * sizeof(uint32_t));

    /* Calculate the cycle duration in microseconds */
    float32_t cycle_duration_us = (1000000.0f / DEFAULT_CIS_CLK_FREQ);

    /* Calculate LED OFF index */
    cisConfig.leds_off_index = (int)(leds_duration_us / cycle_duration_us) + CIS_LED_ON;

    /* Check that led_off_index does not exceed CIS_MAX_LANE_SIZE */
    if (cisConfig.leds_off_index > CIS_MAX_LANE_SIZE)
    {
    	cisConfig.leds_off_index = CIS_MAX_LANE_SIZE;
    }

    /* Initialize calibration data */
    cisLeds_Calibration.redLed_maxPulse = cisConfig.leds_off_index;
    cisLeds_Calibration.greenLed_maxPulse = cisConfig.leds_off_index;
    cisLeds_Calibration.blueLed_maxPulse = cisConfig.leds_off_index;

    /* Update the number of UDP packets per line */
    cisConfig.udp_nb_packet_per_line = cisConfig.pixels_nb / UDP_LINE_FRAGMENT_SIZE;

    for (int32_t packet = 0; packet < UDP_MAX_NB_PACKET_PER_LINE; packet++)
    {
        // Initialize first buffer (scanline_buff1)
        buffers_Scanline.scanline_buff1[packet].total_fragments = cisConfig.udp_nb_packet_per_line;

        // Initialize second buffer (scanline_buff2)
        buffers_Scanline.scanline_buff2[packet].total_fragments = cisConfig.udp_nb_packet_per_line;
    }

    /* Start capture with new configuration */
    cis_startCapture();

    return CIS_OK;
}

/**
 * @brief  Convert RAW image to a float array.
 * @param  cisDataCpy_f32: Pointer to destination float array.
 * @param  RAWImage: Pointer to a RAWImage structure.
 * @retval None
 */
void cis_convertRAWImageToFloatArray(float32_t* cisDataCpy_f32, struct RAWImage* RAWImage)
{
    // Copy segments to the complete buffer of the red line
    arm_copy_f32(&cisDataCpy_f32[cisConfig.start_offset], RAWImage->redLine, cisConfig.pixels_per_lane);
    arm_copy_f32(&cisDataCpy_f32[cisConfig.start_offset + cisConfig.adc_buff_size], &RAWImage->redLine[cisConfig.pixels_per_lane], cisConfig.pixels_per_lane);
    arm_copy_f32(&cisDataCpy_f32[cisConfig.start_offset + cisConfig.adc_buff_size * 2], &RAWImage->redLine[cisConfig.pixels_per_lane * 2], cisConfig.pixels_per_lane);

    // Do the same for the green and blue lines
    arm_copy_f32(&cisDataCpy_f32[cisConfig.lane_size + cisConfig.start_offset], RAWImage->greenLine, cisConfig.pixels_per_lane);
    arm_copy_f32(&cisDataCpy_f32[cisConfig.lane_size + cisConfig.start_offset + cisConfig.adc_buff_size], &RAWImage->greenLine[cisConfig.pixels_per_lane], cisConfig.pixels_per_lane);
    arm_copy_f32(&cisDataCpy_f32[cisConfig.lane_size + cisConfig.start_offset + cisConfig.adc_buff_size * 2], &RAWImage->greenLine[cisConfig.pixels_per_lane * 2], cisConfig.pixels_per_lane);

    arm_copy_f32(&cisDataCpy_f32[cisConfig.lane_size * 2 + cisConfig.start_offset], RAWImage->blueLine, cisConfig.pixels_per_lane);
    arm_copy_f32(&cisDataCpy_f32[cisConfig.lane_size * 2 + cisConfig.start_offset + cisConfig.adc_buff_size], &RAWImage->blueLine[cisConfig.pixels_per_lane], cisConfig.pixels_per_lane);
    arm_copy_f32(&cisDataCpy_f32[cisConfig.lane_size * 2 + cisConfig.start_offset + cisConfig.adc_buff_size * 2], &RAWImage->blueLine[cisConfig.pixels_per_lane * 2], cisConfig.pixels_per_lane);
}

void cis_imageProcess_2(int32_t *cis_buff)
{
    static struct RAWImage RAWImage = {0};
    static float32_t cisDataCpy_f32[CIS_MAX_ADC_BUFF_SIZE * 3] = {0};

    //cis_getRAWImage(cisDataCpy_f32, shared_config.cis_oversampling);
    cis_convertRAWImageToFloatArray(cisDataCpy_f32, &RAWImage);
    cis_applyCalibration(&RAWImage, &cisRGB_Calibration);
    cis_convertRAWImageToRGBImage(&RAWImage, cis_buff);
}


/**
 * @brief  Process image and output an RGB buffer.
 * @param  cisDataCpy: Pointer to processed data buffer.
 * @param  imageBuffers: Pointer to an array of scanline packets.
 * @retval None
 *
 * Detailed explanation (INPUT/OUTPUT schema remains unchanged):
 *   - The function acquires raw image data.
 *   - Applies linear calibration.
 *   - Then partitions the data into UDP packet buffers.
 *
 * ---------------------------------------------------
 *	INPUT :
 *	DMA buffer in 32bits increment
 *	o = start offset
 *	e = over scan
 *	                                        X1                                   X2                                   X3
 *		 ___________________________________V ___________________________________V ___________________________________V
 *		|                DMA1               ||                DMA2               ||                DMA3               |
 * 		|       HALF      |       FULL      ||       HALF      |       FULL      ||       HALF      |       FULL      |
 * 		|     R     |     G     |     B     ||     R     |     G     |     B     ||     R     |     G     |     B     |
 * 		|o |      |e|o |      |e|o |      |e||o |      |e|o |      |e|o |      |e||o |      |e|o |      |e|o |      |e|
 * 		   ^           ^  ^        ^            ^           ^  ^        ^            ^           ^  ^        ^
 * 		   R1          G1 C1       B1           R2          G2 C2       B2           R3          G3 C3       B3
 *
 *   	R1 = CIS_START_OFFSET
 *      R2 = CIS_START_OFFSET + cis_adc_buff_size
 *     	R3 = CIS_START_OFFSET + cis_adc_buff_size * 2
 *
 *      G1 = cis_lane_size + CIS_START_OFFSET
 *      G2 = cis_lane_size + CIS_START_OFFSET + cis_adc_buff_size
 *      G3 = cis_lane_size + CIS_START_OFFSET + cis_adc_buff_size * 2
 *
 *      B1 = cis_lane_size * 2 + CIS_START_OFFSET
 *      B2 = cis_lane_size * 2 + CIS_START_OFFSET + cis_adc_buff_size
 *      B3 = cis_lane_size * 2 + CIS_START_OFFSET + cis_adc_buff_size * 2
 */
#pragma GCC push_options
#pragma GCC optimize ("unroll-loops")
void cis_imageProcess(uint32_t *cisDataCpy, struct packet_Scanline *imageBuffers)
{
    int32_t iteration;
    int32_t lane, i, packet;
    uint32_t startTick;
    int32_t numPackets = cisConfig.udp_nb_packet_per_line;
    const int32_t pixelPerPacket = cisConfig.pixels_nb / numPackets;
    const int32_t lanePackets = numPackets / CIS_ADC_OUT_LANES;

    // Outer loop (oversampling) in decrementing order
    for (iteration = shared_config.cis_oversampling; iteration-- > 0; )
    {
        int curIter = shared_config.cis_oversampling - iteration;  // curIter: 1 for the first oversample

        // Wait for all lanes to be ready (loop in decrementing order)
        startTick = HAL_GetTick();
        for (i = CIS_ADC_OUT_LANES; i-- > 0; )
        {
            while (cisBufferState[i] != CIS_BUFFER_COMPLETE)
            {
                if ((HAL_GetTick() - startTick) > CIS_CAPTURE_TIMEOUT)
                {
                    printf("Timeout: Full buffer state not reached for lane %d\n", (int)i + 1);
                    cis_resetStart();
                    return;
                }
            }
            cisBufferState[i] = CIS_BUFFER_OFFSET_NONE;
        }

        cis_applyLinearCalibration(cisDataCpy, 255);

        if (shared_config.cis_handedness)
        {
            // For handedness true, the outer packet loop est déjà décrémentée
            for (packet = numPackets - 1; packet >= 0; packet--)
            {
                lane = packet / lanePackets;
                int localPacketIndex = packet - (lane * lanePackets);
                int startIdx = pixelPerPacket * (localPacketIndex + 1) - 1;
                int endIdx = pixelPerPacket * localPacketIndex;

                uint32_t *redBase = cisDataCpy + cisConfig.red_lane_offset + lane * cisConfig.adc_buff_size;
                uint32_t *greenBase = cisDataCpy + cisConfig.green_lane_offset + lane * cisConfig.adc_buff_size;
                uint32_t *blueBase = cisDataCpy + cisConfig.blue_lane_offset + lane * cisConfig.adc_buff_size;

                // Inner loop en décrémentation (i de startIdx à endIdx)
                for (i = startIdx; i >= endIdx; i--)
                {
                    int offsetIndex = i - endIdx;
                    uint8_t sample_R = (uint8_t)redBase[i];
                    uint8_t sample_G = (uint8_t)greenBase[i];
                    uint8_t sample_B = (uint8_t)blueBase[i];

                    if (curIter == 1)
                    {
                        imageBuffers[packet].imageData_R[offsetIndex] = sample_R;
                        imageBuffers[packet].imageData_G[offsetIndex] = sample_G;
                        imageBuffers[packet].imageData_B[offsetIndex] = sample_B;
                    }
                    else
                    {
                        imageBuffers[packet].imageData_R[offsetIndex] += (sample_R - imageBuffers[packet].imageData_R[offsetIndex]) / curIter;
                        imageBuffers[packet].imageData_G[offsetIndex] += (sample_G - imageBuffers[packet].imageData_G[offsetIndex]) / curIter;
                        imageBuffers[packet].imageData_B[offsetIndex] += (sample_B - imageBuffers[packet].imageData_B[offsetIndex]) / curIter;
                    }
                }

                if (curIter == shared_config.cis_oversampling)
                {
                    imageBuffers[packet].fragment_id = packet;
                    imageBuffers[packet].line_id = shared_var.cis_process_cnt;
                }
            }
        }
        else
        {
            // For non handedness, convert the outer packet loop to a decrementing loop
            for (packet = numPackets; packet-- > 0; )
            {
                int origPacket = numPackets - 1 - packet;  // Convert to original ascending index
                lane = origPacket / lanePackets;
                int localPacketIndex = origPacket - (lane * lanePackets);
                int startIdx = pixelPerPacket * localPacketIndex;
                int endIdx = pixelPerPacket * (localPacketIndex + 1);
                int destPacket = numPackets - 1 - origPacket;

                uint32_t *redBase = cisDataCpy + cisConfig.red_lane_offset + lane * cisConfig.adc_buff_size;
                uint32_t *greenBase = cisDataCpy + cisConfig.green_lane_offset + lane * cisConfig.adc_buff_size;
                uint32_t *blueBase = cisDataCpy + cisConfig.blue_lane_offset + lane * cisConfig.adc_buff_size;

                // Inner loop en décrémentation : i va de endIdx-1 à startIdx
                for (i = endIdx; i-- > startIdx; )
                {
                    int offsetIndex = (endIdx - 1) - i;
                    uint8_t sample_R = (uint8_t)redBase[i];
                    uint8_t sample_G = (uint8_t)greenBase[i];
                    uint8_t sample_B = (uint8_t)blueBase[i];

                    if (curIter == 1)
                    {
                        imageBuffers[destPacket].imageData_R[offsetIndex] = sample_R;
                        imageBuffers[destPacket].imageData_G[offsetIndex] = sample_G;
                        imageBuffers[destPacket].imageData_B[offsetIndex] = sample_B;
                    }
                    else
                    {
                        imageBuffers[destPacket].imageData_R[offsetIndex] += (sample_R - imageBuffers[destPacket].imageData_R[offsetIndex]) / curIter;
                        imageBuffers[destPacket].imageData_G[offsetIndex] += (sample_G - imageBuffers[destPacket].imageData_G[offsetIndex]) / curIter;
                        imageBuffers[destPacket].imageData_B[offsetIndex] += (sample_B - imageBuffers[destPacket].imageData_B[offsetIndex]) / curIter;
                    }
                }

                if (curIter == shared_config.cis_oversampling)
                {
                    imageBuffers[destPacket].fragment_id = origPacket;
                    imageBuffers[destPacket].line_id = shared_var.cis_process_cnt;
                }
            }
        }

        // Launch MDMA transfers concurrently for the three channels
        HAL_MDMA_Start_IT(&hmdma_mdma_channel1_dma1_stream0_tc_0, (uint32_t)cisData_ADC1, (uint32_t)&cisDataCpy[0], cisConfig.adc_buff_size * sizeof(int16_t), 1);
        HAL_MDMA_Start_IT(&hmdma_mdma_channel2_dma1_stream1_tc_0, (uint32_t)cisData_ADC2, (uint32_t)&cisDataCpy[cisConfig.adc_buff_size], cisConfig.adc_buff_size * sizeof(int16_t), 1);
        HAL_MDMA_Start_IT(&hmdma_mdma_channel3_dma2_stream0_tc_0, (uint32_t)cisData_ADC3, (uint32_t)&cisDataCpy[cisConfig.adc_buff_size * 2], cisConfig.adc_buff_size * sizeof(int16_t), 1);
    }
}
#pragma GCC pop_options

/**
 * @brief  Perform image processing for RGB calibration.
 *         Acquires multiple images, sums them and then averages.
 * @param  cisCalData: Pointer to the calibration buffer (Q16.16 format).
 * @param  iterationNb: Number of iterations for averaging.
 * @retval None
 */
void cis_imageProcessRGB_Calibration(uint32_t *cisDataCpy, uint32_t *cisCalData, uint16_t iterationNb)
{
    uint32_t totalElements = cisConfig.adc_buff_size * 3;
    uint32_t i;
    uint16_t iteration;
    uint32_t startTick;

    shared_var.cis_cal_progressbar = 0;

    /* Clear the calibration and copy buffers */
    for (i = 0; i < totalElements; i++)
    {
        cisCalData[i] = 0;
        cisDataCpy[i] = 0;
    }

    for (iteration = 0; iteration < iterationNb; iteration++)
    {
        /* Wait for all lanes to be ready (loop in decrementing order) */
        startTick = HAL_GetTick();
        for (i = CIS_ADC_OUT_LANES; i-- > 0; )
        {
            while (cisBufferState[i] != CIS_BUFFER_COMPLETE)
            {
                if ((HAL_GetTick() - startTick) > CIS_CAPTURE_TIMEOUT)
                {
                    printf("Timeout: Full buffer state not reached for lane %d\n", (int)i + 1);
                    cis_resetStart();
                    return;
                }
            }
            /* Reset the state for the next capture */
            cisBufferState[i] = CIS_BUFFER_OFFSET_NONE;
        }

        /* Sum the acquired buffer into the calibration data */
        for (i = 0; i < totalElements; i++)
        {
            cisCalData[i] += cisDataCpy[i];
        }

        /* Update the progress bar */
        shared_var.cis_cal_progressbar = (iteration * 100U) / iterationNb;

        /* Launch MDMA transfers concurrently for the three channels */
        HAL_MDMA_Start_IT(&hmdma_mdma_channel1_dma1_stream0_tc_0, (uint32_t)cisData_ADC1, (uint32_t)&cisDataCpy[0], cisConfig.adc_buff_size * sizeof(int16_t), 1);
        HAL_MDMA_Start_IT(&hmdma_mdma_channel2_dma1_stream1_tc_0, (uint32_t)cisData_ADC2, (uint32_t)&cisDataCpy[cisConfig.adc_buff_size], cisConfig.adc_buff_size * sizeof(int16_t), 1);
        HAL_MDMA_Start_IT(&hmdma_mdma_channel3_dma2_stream0_tc_0, (uint32_t)cisData_ADC3, (uint32_t)&cisDataCpy[cisConfig.adc_buff_size * 2], cisConfig.adc_buff_size * sizeof(int16_t), 1);
    }

    /* Average the calibration data */
    for (i = 0; i < totalElements; i++)
    {
        cisCalData[i] /= iterationNb;
    }
}

/**
 * @brief Resets the CIS capture process by stopping, clearing buffer states, and restarting.
 */
void cis_resetStart(void)
{
    // Stop the capture process
    cis_stopCapture();

    // Reset buffer states
    for (int i = 0; i < CIS_ADC_OUT_LANES; i++)
    {
    	cisBufferState[i] = CIS_BUFFER_OFFSET_NONE;
    }

    // Restart the capture process
    cis_startCapture();
}

/**
 * @brief  CIS start captures
 * @param  None
 * @retval None
 */
void cis_startCapture()
{
    /* Initialize hardware peripherals */
    cis_initTimStartPulse();
    cis_initTimLedRed();
    cis_initTimLedGreen();
    cis_initTimLedBlue();
    cis_initTimClock();

    /* Reset CLKs ############################################*/
    // Reset CLK counter
    __HAL_TIM_SET_COUNTER(&htim1, 0);

    // Reset SP counter
    __HAL_TIM_SET_COUNTER(&htim8, cisConfig.lane_size - CIS_SP_WIDTH);

    // Set RGB phase shift
#ifndef CIS_MONOCHROME
    __HAL_TIM_SET_COUNTER(&htim4, (cisConfig.lane_size * 1) - CIS_LED_ON);  // R
    __HAL_TIM_SET_COUNTER(&htim5, (cisConfig.lane_size * 3) - CIS_LED_ON);  // G
    __HAL_TIM_SET_COUNTER(&htim3, (cisConfig.lane_size * 2) - CIS_LED_ON);  // B
#else
    __HAL_TIM_SET_COUNTER(&htim4, (cisConfig.lane_size * 1) - CIS_LED_ON);  // R
    __HAL_TIM_SET_COUNTER(&htim5, (cisConfig.lane_size * 1) - CIS_LED_ON);  // G
    __HAL_TIM_SET_COUNTER(&htim3, (cisConfig.lane_size * 1) - CIS_LED_ON);  // B
#endif

    /* Start LEDs ############################################*/
    /* Start LED R generation ###############################*/
    HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_2);

    /* Start LED G generation ###############################*/
    HAL_TIM_PWM_Start(&htim5, TIM_CHANNEL_3);

    /* Start LED B generation ###############################*/
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);

    /* Start SP generation ##################################*/
    HAL_TIM_PWM_Start(&htim8, TIM_CHANNEL_3);

    /* Start CLK generation ##################################*/
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);

    /* Start DMA #############################################*/
    /* Reset buffer states */
    for (int i = 0; i < CIS_ADC_OUT_LANES; i++)
    {
        cisBufferState[i] = CIS_BUFFER_OFFSET_NONE;
    }

    HAL_ADC_Start_DMA(&hadc1, (uint32_t *)cisData_ADC1, cisConfig.adc_buff_size);
    HAL_ADC_Start_DMA(&hadc2, (uint32_t *)cisData_ADC2, cisConfig.adc_buff_size);
    HAL_ADC_Start_DMA(&hadc3, (uint32_t *)cisData_ADC3, cisConfig.adc_buff_size);

    HAL_MDMA_Start_IT(&hmdma_mdma_channel1_dma1_stream0_tc_0, (uint32_t)cisData_ADC1, (uint32_t)&cisDataCpy[0], cisConfig.adc_buff_size * sizeof(int16_t), 1);
    HAL_MDMA_Start_IT(&hmdma_mdma_channel2_dma1_stream1_tc_0, (uint32_t)cisData_ADC2, (uint32_t)&cisDataCpy[cisConfig.adc_buff_size], cisConfig.adc_buff_size * sizeof(int16_t), 1);
    HAL_MDMA_Start_IT(&hmdma_mdma_channel3_dma2_stream0_tc_0, (uint32_t)cisData_ADC3, (uint32_t)&cisDataCpy[cisConfig.adc_buff_size * 2], cisConfig.adc_buff_size * sizeof(int16_t), 1);

    /* Start ADC Main Timer #######################################*/
    __HAL_TIM_SET_COUNTER(&htim1, 0);
    __HAL_TIM_SET_COUNTER(&htim8, cisConfig.lane_size - CIS_SP_WIDTH);
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);

#ifdef CIS_PRINT_COUNTER
	printf("=========== COUNTERS ==========\n");
    printf("adc1 DMA count : %d \n",(int)__HAL_DMA_GET_COUNTER(&hdma_adc1));
    printf("adc2 DMA count : %d \n",(int)__HAL_DMA_GET_COUNTER(&hdma_adc2));
    printf("adc3 DMA count : %d \n",(int)__HAL_DMA_GET_COUNTER(&hdma_adc3));
    printf("CLK  TIM count : %d \n",(int)__HAL_TIM_GET_COUNTER(&htim1));
    printf("SP   TIM count : %d \n",(int)__HAL_TIM_GET_COUNTER(&htim8));
    printf("LEDR TIM count : %d \n",(int)__HAL_TIM_GET_COUNTER(&htim4));
    printf("LEDG TIM count : %d \n",(int)__HAL_TIM_GET_COUNTER(&htim5));
    printf("LEDB TIM count : %d \n",(int)__HAL_TIM_GET_COUNTER(&htim3));
	printf("===============================\n");
#endif
}

/**
 * @brief  CIS stop captures
 * @param  None
 * @retval None
 */
void cis_stopCapture()
{
    /* Stop ADC Timer ######################################*/
    if(HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_1) != HAL_OK)
    {
        Error_Handler();
    }

    /* Stop CLK generation ###################################*/
    if(HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_2) != HAL_OK)
    {
        Error_Handler();
    }

    /* Stop SP generation ####################################*/
    if(HAL_TIM_PWM_Stop(&htim8, TIM_CHANNEL_3) != HAL_OK)
    {
        Error_Handler();
    }

    /* Stop DMA ##############################################*/
    if (HAL_ADC_Stop_DMA(&hadc1) != HAL_OK)
    {
        Error_Handler();
    }

    if (HAL_ADC_Stop_DMA(&hadc2) != HAL_OK)
    {
        Error_Handler();
    }

    if (HAL_ADC_Stop_DMA(&hadc3) != HAL_OK)
    {
        Error_Handler();
    }

    /* Stop LEDs ############################################*/
    /* Stop LED R generation ################################*/
    if(HAL_TIM_PWM_Stop(&htim4, TIM_CHANNEL_2) != HAL_OK)
    {
        Error_Handler();
    }
    /* Stop LED G generation ################################*/
    if(HAL_TIM_PWM_Stop(&htim5, TIM_CHANNEL_3) != HAL_OK)
    {
        Error_Handler();
    }
    /* Stop LED B generation ################################*/
    if(HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_1) != HAL_OK)
    {
        Error_Handler();
    }

    HAL_ADC_Stop_DMA(&hadc1);
    HAL_ADC_Stop_DMA(&hadc2);
    HAL_ADC_Stop_DMA(&hadc3);

    for (int i = 0; i < CIS_ADC_OUT_LANES; i++)
    {
    	cisBufferState[i] = CIS_BUFFER_OFFSET_NONE;
    }
}

/**
 * @brief  Init CIS clock Frequency
 * @param  None
 * @retval None
 */
void cis_initTimClock()
{
	MX_TIM1_Init();
}

/**
 * @brief  CIS start pulse timer init
 * @param  None
 * @retval None
 */
void cis_initTimStartPulse()
{
	MX_TIM8_Init();
}

/**
 * @brief  CIS red led timer init
 * @param  None
 * @retval None
 */
void cis_initTimLedBlue()
{
	MX_TIM3_Init();
}

/**
 * @brief  CIS green led timer init
 * @param  None
 * @retval None
 */
void cis_initTimLedRed()
{
	MX_TIM4_Init();
}

/**
 * @brief  CIS blue led timer init
 * @param  None
 * @retval None
 */
void cis_initTimLedGreen()
{
	MX_TIM5_Init();
}

/**
 * @brief  CIS leds on
 * @param  None
 * @retval None
 */
void cis_ledsOn()
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	GPIO_InitStruct.Pin = CIS_LED_R_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	GPIO_InitStruct.Alternate = GPIO_AF2_TIM5;
	HAL_GPIO_Init(CIS_LED_R_GPIO_Port, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = CIS_LED_G_Pin;
	GPIO_InitStruct.Alternate = GPIO_AF2_TIM3;
	HAL_GPIO_Init(CIS_LED_G_GPIO_Port, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = CIS_LED_B_Pin;
	GPIO_InitStruct.Alternate = GPIO_AF2_TIM4;
	HAL_GPIO_Init(CIS_LED_B_GPIO_Port, &GPIO_InitStruct);
}

/**
 * @brief  CIS leds off
 * @param  None
 * @retval None
 */
void cis_ledsOff()
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	HAL_GPIO_WritePin(CIS_LED_R_GPIO_Port, CIS_LED_R_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(CIS_LED_G_GPIO_Port, CIS_LED_G_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(CIS_LED_B_GPIO_Port, CIS_LED_B_Pin, GPIO_PIN_RESET);

	GPIO_InitStruct.Pin = CIS_LED_R_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(CIS_LED_R_GPIO_Port, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = CIS_LED_G_Pin;
	HAL_GPIO_Init(CIS_LED_G_GPIO_Port, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = CIS_LED_B_Pin;
	HAL_GPIO_Init(CIS_LED_B_GPIO_Port, &GPIO_InitStruct);
}

/**
 * @brief  CIS change led power
 * @param  None
 * @retval None
 */
void cis_ledPowerAdj(int32_t red_pwm, int32_t green_pwm, int32_t blue_pwm)
{
	int32_t pulseValue = 0;

	// Ensure that the power intensity is within the expected range
	red_pwm = red_pwm < 0 ? 0 : red_pwm > 100 ? 100 : red_pwm;
	green_pwm = green_pwm < 0 ? 0 : green_pwm > 100 ? 100 : green_pwm;
	blue_pwm = blue_pwm < 0 ? 0 : blue_pwm > 100 ? 100 : blue_pwm;

	pulseValue = cisLeds_Calibration.redLed_maxPulse - 1;
#ifdef CIS_MONOCHROME
	pulseValue /= 3;
#endif
	pulseValue = (pulseValue * red_pwm) / 100;
	__HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_2, pulseValue);

	pulseValue = cisLeds_Calibration.greenLed_maxPulse - 1;
#ifdef CIS_MONOCHROME
	pulseValue /= 3;
#endif
	pulseValue = (pulseValue * green_pwm) / 100;
	__HAL_TIM_SET_COMPARE(&htim5, TIM_CHANNEL_3, pulseValue);

	pulseValue = cisLeds_Calibration.blueLed_maxPulse - 1;
#ifdef CIS_MONOCHROME
	pulseValue /= 3;
#endif
	pulseValue = (pulseValue * blue_pwm) / 100;
	__HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, pulseValue);
}

/**
 * @brief  CIS adc init
 * @param  None
 * @retval None
 */
void cis_initAdc(void)
{
	MX_ADC1_Init();
	MX_ADC2_Init();
	MX_ADC3_Init();

	/* ### Start calibration ############################################ */
	if (HAL_ADCEx_Calibration_Start(&hadc1, ADC_CALIB_OFFSET, ADC_SINGLE_ENDED) != HAL_OK)
	{
		Error_Handler();
	}

	if (HAL_ADCEx_Calibration_Start(&hadc2, ADC_CALIB_OFFSET, ADC_SINGLE_ENDED) != HAL_OK)
	{
		Error_Handler();
	}

	if (HAL_ADCEx_Calibration_Start(&hadc3, ADC_CALIB_OFFSET, ADC_SINGLE_ENDED) != HAL_OK)
	{
		Error_Handler();
	}

	if (HAL_ADCEx_LinearCalibration_FactorLoad(&hadc1) != HAL_OK)
	{
		Error_Handler();
	}

	if (HAL_ADCEx_LinearCalibration_FactorLoad(&hadc2) != HAL_OK)
	{
		Error_Handler();
	}

	if (HAL_ADCEx_LinearCalibration_FactorLoad(&hadc3) != HAL_OK)
	{
		Error_Handler();
	}
}

void MDMA_XferCpltCallback(MDMA_HandleTypeDef *hmdma)
{

    if (hmdma == &hmdma_mdma_channel1_dma1_stream0_tc_0) //ADC1
    {
    	cisBufferState[0] = CIS_BUFFER_COMPLETE;
    }
    if (hmdma == &hmdma_mdma_channel2_dma1_stream1_tc_0) //ADC2
    {
    	cisBufferState[1] = CIS_BUFFER_COMPLETE;
    }
    if (hmdma == &hmdma_mdma_channel3_dma2_stream0_tc_0) //ADC3
    {
    	cisBufferState[2] = CIS_BUFFER_COMPLETE;
    }
}
