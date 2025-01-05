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

#include "cis_scan.h"

#include "cis_linearCal.h"
#include "cis_polyCal.h"

#include "cis.h"

/* Private includes ----------------------------------------------------------*/

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static volatile CIS_BUFF_StateTypeDef  cisHalfBufferState[CIS_ADC_OUT_LANES] = {0};
static volatile CIS_BUFF_StateTypeDef  cisFullBufferState[CIS_ADC_OUT_LANES] = {0};

/* Variable containing ADC conversions data */

/* Private function prototypes -----------------------------------------------*/
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
void cis_init(void)
{
    printf("------ CIS HARDWARE INIT ------\n");

    /* Enable 5V power DC/DC for display */
    HAL_GPIO_WritePin(EN_5V_GPIO_Port, EN_5V_Pin, GPIO_PIN_SET);

    cis_initAdc();

    cis_configure(shared_config.cis_dpi);
}

/**
 * @brief  CIS configuration
 *         Configures the CIS, including DPI setting and variables dependent on DPI.
 * @param  dpi: Desired resolution in DPI (200 or 400)
 * @retval None
 */
void cis_configure(uint16_t dpi)
{
    float32_t leds_duration_us;

    printf("------ CIS CONFIGURATION ------\n");

    /* Stop any ongoing capture before reconfiguring */
    cis_stopCapture();

    cis_linearCalibrationInit();

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

    osDelay(100);

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
    memset(cisData, 0, cisConfig.adc_buff_size * 3 * sizeof(uint16_t));
    memset(cisDataCpy_f32, 0, cisConfig.adc_buff_size * 3 * sizeof(float32_t));

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

    for (int32_t packet = UDP_MAX_NB_PACKET_PER_LINE; --packet >= 0;)
    {
        packet_Image[packet].total_fragments = cisConfig.udp_nb_packet_per_line;
    }

    /* Start capture with new configuration */
    cis_startCapture();
}

/**
 * @brief  Manages Image process.
 * @param cis image buffer ptr
 * @retval None
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
 *
 *	DMA data conversion :
 *
 *	16bits RED :	 _______________
 *	ADC DATA => 	|   R	|	|	|
 *	16 to 8  => 	| R	|	|	|	|	>> 8
 *	                ^
 *					Rx
 *
 *	16bits GREEN : 	 _______________
 *	ADC DATA => 	|   G	|	|	|
 *	16 to 8  => 	| G	|	|	|	|	>> 8
 *	offset   => 	| 	| G	|	|	|	<< 8
 *	                ^
 *					Gx
 *
 *	16bits BLUE : 	 _______________
 *	ADC DATA => 	|   B	|	|	|
 *	16 to 8  => 	| B	|	|	|	|	>> 8
 *	offset   => 	| 	| 	| B	|	|	<< 16
 *					^
 *					Bx
 *
 * 	---------------------------------------------------
 *	OUTPUT :
 *	Data buffer in 8bits increment
 *
 *              |                LANE1                  ||                LANE2                  ||                LANE3                  |
 *	 	HALF => |RG--|RG--|RG--|RG--|R---|R---|R---|R---||RG--|RG--|RG--|RG--|R---|R---|R---|R---||RG--|RG--|RG--|RG--|R---|R---|R---|R---|
 * 		FULL => |--B-|--B-|--B-|--B-|-GB-|-GB-|-GB-|-GB-||--B-|--B-|--B-|--B-|-GB-|-GB-|-GB-|-GB-||--B-|--B-|--B-|--B-|-GB-|-GB-|-GB-|-GB-|
 * 		OUT  => |RGB-|RGB-|RGB-|RGB-|RGB-|RGB-|RGB-|RGB-||RGB-|RGB-|RGB-|RGB-|RGB-|RGB-|RGB-|RGB-||RGB-|RGB-|RGB-|RGB-|RGB-|RGB-|RGB-|RGB-|
 * 		        ^                                        ^                                        ^
 *              M1                                       M2                                       M3
 *
 */
#pragma GCC push_options
#pragma GCC optimize ("unroll-loops")
void cis_getRAWImage(float32_t* cisDataCpy_f32, uint8_t overSampling)
{
    int32_t acc = 0;
    int32_t lane, i;

    if (overSampling > 1)
    {
        arm_fill_f32(0, cisDataCpy_f32, cisConfig.adc_buff_size * 3); // Clear the buffer
    }

    while (acc < overSampling)
    {
        // Read and copy the half-filled DMA buffers
        for (lane = CIS_ADC_OUT_LANES; --lane >= 0;)
        {
            while (cisHalfBufferState[lane] != CIS_BUFFER_OFFSET_HALF);

            SCB_InvalidateDCache_by_Addr((uint32_t *)&cisData[cisConfig.adc_buff_size * lane], (cisConfig.adc_buff_size * sizeof(uint16_t)) / 2);
            for (i = (cisConfig.adc_buff_size / 2); --i >= 0;)
            {
                if (overSampling > 1)
                {
                    cisDataCpy_f32[cisConfig.adc_buff_size * lane + i] += (float32_t)(cisData[cisConfig.adc_buff_size * lane + i]);
                }
                else
                {
                    cisDataCpy_f32[cisConfig.adc_buff_size * lane + i] = (float32_t)(cisData[cisConfig.adc_buff_size * lane + i]);
                }
            }

            cisHalfBufferState[lane] = CIS_BUFFER_OFFSET_NONE;
        }

        // Read and copy the full DMA buffers
        for (lane = CIS_ADC_OUT_LANES; --lane >= 0;)
        {
            while (cisFullBufferState[lane] != CIS_BUFFER_OFFSET_FULL);

            SCB_InvalidateDCache_by_Addr((uint32_t *)&cisData[(cisConfig.adc_buff_size * lane) + (cisConfig.adc_buff_size / 2)], (cisConfig.adc_buff_size * sizeof(uint16_t)) / 2);
            for (i = (cisConfig.adc_buff_size / 2); --i >= 0;)
            {
                if (overSampling > 1)
                {
                    cisDataCpy_f32[(cisConfig.adc_buff_size * lane) + (cisConfig.adc_buff_size / 2) + i] += (float32_t)(cisData[(cisConfig.adc_buff_size * lane) + (cisConfig.adc_buff_size / 2) + i]);
                }
                else
                {
                    cisDataCpy_f32[(cisConfig.adc_buff_size * lane) + (cisConfig.adc_buff_size / 2) + i] = (float32_t)(cisData[(cisConfig.adc_buff_size * lane) + (cisConfig.adc_buff_size / 2) + i]);
                }
            }

            cisFullBufferState[lane] = CIS_BUFFER_OFFSET_NONE;
        }

        acc++;
    }

    if (overSampling > 1)
    {
        arm_scale_f32(cisDataCpy_f32, 1.0f / (float32_t)overSampling, cisDataCpy_f32, cisConfig.adc_buff_size * 3);
    }
}

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

    cis_getRAWImage(cisDataCpy_f32, shared_config.cis_oversampling);
    cis_convertRAWImageToFloatArray(cisDataCpy_f32, &RAWImage);
    cis_applyCalibration(&RAWImage, &cisRGB_Calibration);
    cis_convertRAWImageToRGBImage(&RAWImage, cis_buff);
}


/**
 * @brief  Manages Image process.
 * @param cis image buffer ptr
 * @retval None
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
void cis_imageProcess(float32_t* cisDataCpy_f32, struct packet_Image *imageBuffers)
{
    int32_t lane, i, ii, packet, offsetIndex, startIdx, endIdx;

	cis_getRAWImage(cisDataCpy_f32, shared_config.cis_oversampling);

	cis_applyLinearCalibration(cisDataCpy_f32, 255);

    for (packet = cisConfig.udp_nb_packet_per_line; --packet >= 0;)
    {
        lane = packet / (cisConfig.udp_nb_packet_per_line / CIS_ADC_OUT_LANES);

        if (shared_config.cis_handedness)
        {
            startIdx = (cisConfig.pixels_nb / cisConfig.udp_nb_packet_per_line) * (packet + 1 - (cisConfig.udp_nb_packet_per_line / CIS_ADC_OUT_LANES) * lane) - 1;
            endIdx = (cisConfig.pixels_nb / cisConfig.udp_nb_packet_per_line) * (packet - (cisConfig.udp_nb_packet_per_line / CIS_ADC_OUT_LANES) * lane);

            for (i = startIdx; i >= endIdx; --i)
            {
            	offsetIndex = i - endIdx;

                imageBuffers[packet].imageData_R[offsetIndex] = (uint8_t)cisDataCpy_f32[i + cisConfig.red_lane_offset + (lane * cisConfig.adc_buff_size)];
                imageBuffers[packet].imageData_G[offsetIndex] = (uint8_t)cisDataCpy_f32[i + cisConfig.green_lane_offset + (lane * cisConfig.adc_buff_size)];
                imageBuffers[packet].imageData_B[offsetIndex] = (uint8_t)cisDataCpy_f32[i + cisConfig.blue_lane_offset + (lane * cisConfig.adc_buff_size)];
            }
        }
        else
        {
            startIdx = (cisConfig.pixels_nb / cisConfig.udp_nb_packet_per_line) * (packet - (cisConfig.udp_nb_packet_per_line / CIS_ADC_OUT_LANES) * lane);
            endIdx = (cisConfig.pixels_nb / cisConfig.udp_nb_packet_per_line) * (packet + 1 - (cisConfig.udp_nb_packet_per_line / CIS_ADC_OUT_LANES) * lane);

            for (i = startIdx; i < endIdx; i++)
            {
            	offsetIndex = i - startIdx;

                ii = startIdx - 1 - i + (cisConfig.pixels_nb / cisConfig.udp_nb_packet_per_line) + startIdx;

                imageBuffers[cisConfig.udp_nb_packet_per_line - 1 - packet].imageData_R[offsetIndex] = (uint8_t)cisDataCpy_f32[ii + cisConfig.red_lane_offset + (lane * cisConfig.adc_buff_size)];
                imageBuffers[cisConfig.udp_nb_packet_per_line - 1 - packet].imageData_G[offsetIndex] = (uint8_t)cisDataCpy_f32[ii + cisConfig.green_lane_offset + (lane * cisConfig.adc_buff_size)];
                imageBuffers[cisConfig.udp_nb_packet_per_line - 1 - packet].imageData_B[offsetIndex] = (uint8_t)cisDataCpy_f32[ii + cisConfig.blue_lane_offset + (lane * cisConfig.adc_buff_size)];
            }
        }

        imageBuffers[packet].fragment_id = packet;
        imageBuffers[packet].line_id = shared_var.cis_process_cnt;
    }
}

#pragma GCC pop_options
/**
 * @brief  cis_ImageProcessRGB_Calibration
 * @param  cis calibration buffer ptr
 * @retval None
 */
void cis_imageProcessRGB_Calibration(float32_t *cisCalData, uint16_t iterationNb)
{
    static int32_t iteration;
    shared_var.cis_cal_progressbar = 0;

    arm_fill_f32(0, cisCalData, cisConfig.adc_buff_size * 3); // Clear buffer
    arm_fill_f32(0, cisDataCpy_f32, cisConfig.adc_buff_size * 3); // Clear buffer

    for (iteration = 0; iteration < iterationNb; iteration++)
    {
        cis_getRAWImage(cisDataCpy_f32, 1);
        arm_add_f32(cisCalData, cisDataCpy_f32, cisCalData, cisConfig.adc_buff_size * 3);
        shared_var.cis_cal_progressbar = iteration * 100 / (iterationNb);
    }

    arm_scale_f32(cisCalData, 1.0f / (float32_t)iterationNb, cisCalData, cisConfig.adc_buff_size * 3);
}

#pragma GCC push_options
#pragma GCC optimize ("O0")

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
        cisHalfBufferState[i] = CIS_BUFFER_OFFSET_NONE;
        cisFullBufferState[i] = CIS_BUFFER_OFFSET_NONE;
    }

    HAL_ADC_Start_DMA(&hadc1, (uint32_t *)&cisData[0], cisConfig.adc_buff_size);
    HAL_ADC_Start_DMA(&hadc2, (uint32_t *)&cisData[cisConfig.adc_buff_size], cisConfig.adc_buff_size);
    HAL_ADC_Start_DMA(&hadc3, (uint32_t *)&cisData[cisConfig.adc_buff_size * 2], cisConfig.adc_buff_size);

    /* Start ADC Main Timer #######################################*/
    __HAL_TIM_SET_COUNTER(&htim1, 0);
    __HAL_TIM_SET_COUNTER(&htim8, cisConfig.lane_size - CIS_SP_WIDTH);
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);

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
        cisHalfBufferState[i] = CIS_BUFFER_OFFSET_NONE;
        cisFullBufferState[i] = CIS_BUFFER_OFFSET_NONE;
    }
}
#pragma GCC pop_options

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

/**
 * @brief  Conversion DMA half-transfer callback in non-blocking mode
 * @param  hadc: ADC handle
 * @retval None
 */
void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef* hadc)
{
	if (hadc->Instance == ADC1)
	{
		cisHalfBufferState[0] = CIS_BUFFER_OFFSET_HALF;
	}
	if (hadc->Instance == ADC2)
	{
		cisHalfBufferState[1] = CIS_BUFFER_OFFSET_HALF;
	}
	if (hadc->Instance == ADC3)
	{
		cisHalfBufferState[2] = CIS_BUFFER_OFFSET_HALF;
	}
}

/**
 * @brief  Conversion complete callback in non-blocking mode
 * @param  hadc: ADC handle
 * @retval None
 */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
	if (hadc->Instance == ADC1)
	{
		cisFullBufferState[0] = CIS_BUFFER_OFFSET_FULL;
	}
	if (hadc->Instance == ADC2)
	{
		cisFullBufferState[1] = CIS_BUFFER_OFFSET_FULL;
	}
	if (hadc->Instance == ADC3)
	{
		cisFullBufferState[2] = CIS_BUFFER_OFFSET_FULL;
	}
}
