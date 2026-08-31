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

#include "cis.h"

/* Private includes ----------------------------------------------------------*/

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static volatile CIS_BUFF_StateTypeDef  cisBufferState[CIS_ADC_OUT_LANES] = {0};

/* Faux des qu'un HAL_ADC_Stop_DMA a echoue : cis_startCapture() repart alors d'un
   ADC/DMA reinitialise au lieu d'essayer de relancer un handle laisse en erreur. */
static bool cisAdcStopClean = true;

/**
 * @brief  Vrai si le capteur a bouge recemment, d'apres la centrale inertielle.
 *
 * Meme detecteur que l'economiseur d'ecran (somme des ecarts entre echantillons
 * consecutifs, seuils shared_config.motion_threshold_acc / _gyro), avec deux
 * differences imposees par le contexte :
 *
 *  - la boucle de calibration tourne a la cadence ligne (~1000/s) et la tache HID
 *    publie l'IMU a la sienne. Comparer deux lectures du meme echantillon donnerait
 *    un ecart nul, donc un faux "immobile" : on ne recalcule que lorsque seq change.
 *  - le mouvement se manifeste par a-coups. Sans remanence la porte battrait a chaque
 *    echantillon, d'ou la fenetre CIS_CAL_MOTION_HOLD_MS.
 *
 * shared_imu est ecrit par la tache HID sur ce meme coeur : pas de probleme de
 * coherence de cache ici. La double lecture de seq encadre la copie pour ne pas
 * melanger deux echantillons.
 */
bool cis_isMoving(void)
{
    static uint32_t lastSeq = 0;
    static float    lastAcc[3]  = {0.0f, 0.0f, 0.0f};
    static float    lastGyro[3] = {0.0f, 0.0f, 0.0f};
    static uint32_t lastMotionTick = 0;
    static bool     primed = false;

    const uint32_t seq = shared_imu.seq;

    if (seq != lastSeq)
    {
        float acc[3], gyro[3];
        for (int32_t i = 0; i < 3; i++)
        {
            acc[i]  = shared_imu.acc[i];
            gyro[i] = shared_imu.gyro[i];
        }

        /* Echantillon coupe en deux par une mise a jour : on le laisse passer, le
           suivant arrive dans la milliseconde. */
        if (shared_imu.seq == seq)
        {
            lastSeq = seq;

            if (!primed)
            {
                memcpy(lastAcc, acc, sizeof(lastAcc));
                memcpy(lastGyro, gyro, sizeof(lastGyro));
                primed = true;
            }
            else
            {
                const float accDelta  = fabsf(acc[0] - lastAcc[0])
                                      + fabsf(acc[1] - lastAcc[1])
                                      + fabsf(acc[2] - lastAcc[2]);
                const float gyroDelta = fabsf(gyro[0] - lastGyro[0])
                                      + fabsf(gyro[1] - lastGyro[1])
                                      + fabsf(gyro[2] - lastGyro[2]);

                memcpy(lastAcc, acc, sizeof(lastAcc));
                memcpy(lastGyro, gyro, sizeof(lastGyro));

                if (accDelta  > shared_config.motion_threshold_acc ||
                    gyroDelta > shared_config.motion_threshold_gyro)
                {
                    lastMotionTick = HAL_GetTick();
                }
            }
        }
    }

    return (primed && (lastMotionTick != 0U) &&
            ((HAL_GetTick() - lastMotionTick) < CIS_CAL_MOTION_HOLD_MS));
}

/* Variable containing ADC conversions data */

/* Private function prototypes -----------------------------------------------*/
static CIS_StatusTypeDef cis_configure(void);
static void cis_resetStart(void);
static void cis_initTimClock();
static void cis_initTimStartPulse();
static void cis_initTimLedRed();
static void cis_initTimLedGreen();
static void cis_initTimLedBlue();
static void cis_initAdc(bool freshOffsetCalibration);
static HAL_StatusTypeDef cis_startMdma(void);

/* Private user code ---------------------------------------------------------*/

/**
 * @brief  CIS hardware initialization
 *         Initializes STM32 peripherals independent of the CIS configuration.
 * @param  None
 * @retval None
 */
CIS_StatusTypeDef cis_init(void)
{
    cis_initAdc(true);

    if (cis_configure() != CIS_OK)
    {
    	return CIS_ERROR;
    }

    MDMA_Init();

#ifdef USE_WDG
    MX_IWDG1_Init();
#endif

    /* Start capture with new configuration */
    cis_startCapture();

    return CIS_OK;
}

/**
 * @brief  CIS power management (ON/OFF)
 *         Controls the 5V power supply for the CIS sensor.
 * @param  powerOn: true = Power ON, false = Power OFF
 * @retval CIS_StatusTypeDef
 */
CIS_StatusTypeDef cis_Power(bool powerOn)
{
    if (powerOn)
    {
        /* Enable 5V power DC/DC for CIS */
        HAL_GPIO_WritePin(EN_5V_GPIO_Port, EN_5V_Pin, GPIO_PIN_SET);

        printf("CIS Power ON\n");
    }
    else
    {
        /* Disable 5V power DC/DC for CIS */
        HAL_GPIO_WritePin(EN_5V_GPIO_Port, EN_5V_Pin, GPIO_PIN_RESET);
        
        /* Stop CLK generation ###################################*/
        HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_2);

        printf("CIS Power OFF\n");
    }

    return CIS_OK;
}

/**
 * @brief  CIS configuration
 *         Configures the CIS, including DPI setting and variables dependent on DPI.
 * @param  dpi: Desired resolution in DPI (200 or 400)
 * @retval None
 */
static CIS_StatusTypeDef cis_configure(void)
{
    float32_t leds_duration_us;

    if (cis_linearCalibrationInit() != CISCALIBRATION_OK)
    {
    	printf("CIS load calibration ERROR\n");
    	return CIS_ERROR;
    }

    /* Initialize variables based on the desired DPI */
    if (shared_config.cis_dpi == 400)
    {
        /* Variables for 400 DPI */
        cisConfig.pixels_per_color_per_lane = CIS_400DPI_PIXELS_PER_LANE;
        /* Set GPIO pin to RESET for 400 DPI */
        HAL_GPIO_WritePin(CIS_RS_GPIO_Port, CIS_RS_Pin, GPIO_PIN_RESET); // RESET : 400DPI
        leds_duration_us = CIS_400DPI_LED_DURATION_US;
        cisConfig.cis_clk_freq = CIS_400DPI_CLK_FREQ;
    }
    else // Default to 200 DPI
    {
        /* Variables for 200 DPI */
        cisConfig.pixels_per_color_per_lane = CIS_200DPI_PIXELS_PER_LANE;
        /* Set GPIO pin to SET for 200 DPI */
        HAL_GPIO_WritePin(CIS_RS_GPIO_Port, CIS_RS_Pin, GPIO_PIN_SET); // SET : 200DPI
        leds_duration_us = CIS_200DPI_LED_DURATION_US;
        cisConfig.cis_clk_freq = CIS_200DPI_CLK_FREQ;
    }

    osDelay(50);

    /* Common configurations */
    cisConfig.pixels_nb = cisConfig.pixels_per_color_per_lane * CIS_ADC_OUT_LANES;
    cisConfig.pixel_area_stop = CIS_INACTIVE_WIDTH + cisConfig.pixels_per_color_per_lane;
    cisConfig.start_offset = CIS_INACTIVE_WIDTH;
    cisConfig.lane_size = cisConfig.pixel_area_stop + CIS_OVER_SCAN;

    cisConfig.adc_buff_size = cisConfig.lane_size * COLOR_CHANNELS;

    /* Les esclaves de TIM1 (SP, LEDs) comptent DEUX ticks par periode CP (mesure) :
       leurs periodes et graines s'expriment donc directement en ECHANTILLONS, soit
       lane_size. lane_clocks ne sert qu'a la documentation du domaine temporel. */
    cisConfig.lane_clocks = cisConfig.lane_size / 2;

    /* Update lane offsets */
    cisConfig.red_lane_offset = cisConfig.start_offset;
    cisConfig.green_lane_offset = cisConfig.lane_size + cisConfig.start_offset;
    cisConfig.blue_lane_offset = (cisConfig.lane_size * 2) + cisConfig.start_offset;

    cisConfig.useful_data_size_per_color_per_lane = CIS_BLACK_PIXELS + cisConfig.pixels_per_color_per_lane;
    cisConfig.useful_data_size_per_lane = cisConfig.useful_data_size_per_color_per_lane * COLOR_CHANNELS;

    cisConfig.red_offset = CIS_BLACK_PIXELS;
    cisConfig.green_offset = cisConfig.useful_data_size_per_color_per_lane + CIS_BLACK_PIXELS;
    cisConfig.blue_offset = (cisConfig.useful_data_size_per_color_per_lane * 2) + CIS_BLACK_PIXELS;

    /* Initialize buffers */
    memset(cisData_ADC1, 0, cisConfig.adc_buff_size * sizeof(uint16_t));
    memset(cisData_ADC2, 0, cisConfig.adc_buff_size * sizeof(uint16_t));
    memset(cisData_ADC3, 0, cisConfig.adc_buff_size * sizeof(uint16_t));
    memset(cisDataCpy, 0, cisConfig.useful_data_size_per_lane * CIS_ADC_OUT_LANES * sizeof(uint32_t));

    /* Calculate the cycle duration in microseconds */
    float32_t cycle_duration_us = (1000000.0f / (float32_t)cisConfig.cis_clk_freq);

    /* Calculate LED OFF index */
    cisConfig.leds_off_index = (int)(leds_duration_us / cycle_duration_us) + CIS_LED_ON;

    /* Check that led_off_index does not exceed CIS_MAX_LANE_SIZE */
    if (cisConfig.leds_off_index > CIS_MAX_LANE_SIZE)
    {
    	printf("leds_off_index overflow %d \n", (int)(cisConfig.leds_off_index - CIS_MAX_LANE_SIZE));
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
        buffers_Scanline.scanline_buff1[packet].h.fragment_count = (uint8_t)cisConfig.udp_nb_packet_per_line;

        // Initialize second buffer (scanline_buff2)
        buffers_Scanline.scanline_buff2[packet].h.fragment_count = (uint8_t)cisConfig.udp_nb_packet_per_line;
    }

    return CIS_OK;
}

/**
 * @brief  CIS reConfiguration
 *         Configures the CIS on the fly, including DPI setting and variables dependent on DPI.
 * @param  dpi: Desired resolution in DPI (200 or 400)
 * @retval None
 */
CIS_StatusTypeDef cis_reConfigure(void)
{
    /* Stop any ongoing capture before reconfiguring */
    cis_stopCapture();

    if (cis_configure() != CIS_OK)
    {
    	return CIS_ERROR;
    }

    return CIS_OK;
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
void cis_imageProcess(int32_t *cisDataCpy, struct slp_line_cis *imageBuffers)
{
    int32_t lane, i, packet, iteration;
    uint32_t startTick;
    uint32_t numPackets = cisConfig.udp_nb_packet_per_line;
    const int32_t pixelPerPacket = cisConfig.pixels_nb / numPackets;   /* pixels FIL */
    const int32_t lanePackets = numPackets / CIS_ADC_OUT_LANES;

    // Outer loop (oversampling) in decrementing order
    for (iteration = shared_config.cis_oversampling; iteration-- > 0; )
    {
    	int32_t curIter = shared_config.cis_oversampling - iteration;  // curIter: 1 for the first oversample

        // Wait for all lanes to be ready (loop in decrementing order)
        startTick = HAL_GetTick();
        for (i = CIS_ADC_OUT_LANES; i-- > 0; )
        {
            while (cisBufferState[i] != CIS_BUFFER_COMPLETE)
            {
                if ((HAL_GetTick() - startTick) > CIS_CAPTURE_TIMEOUT)
                {
                    /* Narration indispensable : sans elle la boucle resetStart est muette
                       et le systeme parait simplement mort. Limitee a 1 message sur 8. */
                    static uint32_t toCnt = 0;
                    if ((toCnt++ & 7U) == 0U)
                    {
                        printf("Timeout lane %d (ADC1 st 0x%03lX NDTR %lu, #%lu)\n",
                               (int)i + 1,
                               (unsigned long)HAL_ADC_GetState(&hadc1),
                               (unsigned long)__HAL_DMA_GET_COUNTER(&hdma_adc1),
                               (unsigned long)toCnt);
                    }
                    cis_resetStart();
                    return;
                }
            }

            cisBufferState[i] = CIS_BUFFER_OFFSET_NONE;
        }

#if CIS_PHASE_DEBUG_ENABLED
        /* BALAYAGE DE PHASE (one-shot, ~7 s apres le boot, regime etabli).
           Fait glisser l'instant de declenchement ADC (CCR1 de TIM15) sur toute la
           periode CP et mesure le niveau brut a chaque position : trace la courbe
           d'etablissement REELLE du signal video a travers le S&H. Si le point de
           fonctionnement est sur une pente, toute variation de timing au boot (TXS sur
           CP, phase interne du capteur a la mise sous tension) devient un decalage de
           NIVEAU -- la signature exacte du defaut de reproductibilite. Compare entre
           boots, le deplacement de la courbe mesure directement cette variation. */
        {
            static bool scanDone = false;
            static uint32_t scanArm = 0;
            if (!scanDone && (++scanArm > 2000))
            {
                scanDone = true;
                const uint32_t saved = __HAL_TIM_GET_COMPARE(&htim15, TIM_CHANNEL_1);
                printf("PHASESCAN begin (saved CCR %lu)\n", (unsigned long)saved);

                for (uint32_t ccr = 2; ccr <= 48; ccr += 2)
                {
                    __HAL_TIM_SET_COMPARE(&htim15, TIM_CHANNEL_1, ccr);
                    osDelay(6);  /* plusieurs lignes a la nouvelle phase */

                    uint32_t inv = ((uint32_t)cisData_ADC1) & ~31U;
                    SCB_InvalidateDCache_by_Addr((uint32_t *)inv, CIS_MAX_ADC_BUFF_SIZE * 2 + 64);

                    /* zone effective de la voie 1, phases paire/impaire separees */
                    int64_t se = 0, so = 0;
                    const int32_t o = 60;
                    for (int32_t k = 0; k < 512; k += 2)
                    {
                        se += cisData_ADC1[o + k];
                        so += cisData_ADC1[o + k + 1];
                    }
                    printf("PHASESCAN %2lu %ld %ld\n", (unsigned long)ccr,
                           (long)(se / 256), (long)(so / 256));
                }

                __HAL_TIM_SET_COMPARE(&htim15, TIM_CHANNEL_1, saved);
                printf("PHASESCAN end\n");
            }
        }
#endif

        cis_applyLinearCalibration(cisDataCpy, 255);

        if (shared_config.cis_handedness)
        {
            for (packet = numPackets - 1; packet >= 0; packet--)
            {
                lane = packet / lanePackets;
                int32_t localPacketIndex = packet - (lane * lanePackets);
                int32_t startIdx = pixelPerPacket * (localPacketIndex + 1) - 1;
                int32_t endIdx = pixelPerPacket * localPacketIndex;

                int32_t *redBase = cisDataCpy + cisConfig.red_offset + lane * cisConfig.useful_data_size_per_lane;
                int32_t *greenBase = cisDataCpy + cisConfig.green_offset + lane * cisConfig.useful_data_size_per_lane;
                int32_t *blueBase = cisDataCpy + cisConfig.blue_offset + lane * cisConfig.useful_data_size_per_lane;

                // Inner loop
                for (i = startIdx; i >= endIdx; i--)
                {
                    int32_t offsetIndex = i - endIdx;
                    uint8_t sample_R = (uint8_t)redBase[i];
                    uint8_t sample_G = (uint8_t)greenBase[i];
                    uint8_t sample_B = (uint8_t)blueBase[i];

                    if (curIter == 1)
                    {
                        imageBuffers[packet].r[offsetIndex] = sample_R;
                        imageBuffers[packet].g[offsetIndex] = sample_G;
                        imageBuffers[packet].b[offsetIndex] = sample_B;
                    }
                    else
                    {
                        imageBuffers[packet].r[offsetIndex] += (sample_R - imageBuffers[packet].r[offsetIndex]) / curIter;
                        imageBuffers[packet].g[offsetIndex] += (sample_G - imageBuffers[packet].g[offsetIndex]) / curIter;
                        imageBuffers[packet].b[offsetIndex] += (sample_B - imageBuffers[packet].b[offsetIndex]) / curIter;
                    }
                }

                if (curIter == shared_config.cis_oversampling)
                {
                    imageBuffers[packet].h.fragment_index = packet;
                    imageBuffers[packet].h.line_id = shared_var.cis_process_cnt;
                }
            }
        }
        else
        {
        	for (packet = 0; packet < numPackets; packet++)
        	{
        	    lane = packet / lanePackets;
        	    int32_t localPacketIndex = packet - (lane * lanePackets);
        	    int32_t startIdx = pixelPerPacket * localPacketIndex;
        	    int32_t endIdx = pixelPerPacket * (localPacketIndex + 1);

        	    int32_t *redBase = cisDataCpy + cisConfig.red_offset + lane * cisConfig.useful_data_size_per_lane;
        	    int32_t *greenBase = cisDataCpy + cisConfig.green_offset + lane * cisConfig.useful_data_size_per_lane;
        	    int32_t *blueBase = cisDataCpy + cisConfig.blue_offset + lane * cisConfig.useful_data_size_per_lane;

                int32_t destPacket = numPackets - 1 - packet;

        	    for (i = endIdx; i-- > startIdx; )
        	    {
        	        int32_t offsetIndex = (endIdx - 1) - i;
        	        uint8_t sample_R = (uint8_t)redBase[i];
        	        uint8_t sample_G = (uint8_t)greenBase[i];
        	        uint8_t sample_B = (uint8_t)blueBase[i];

        	        if (curIter == 1)
        	        {
        	            imageBuffers[destPacket].r[offsetIndex] = sample_R;
        	            imageBuffers[destPacket].g[offsetIndex] = sample_G;
        	            imageBuffers[destPacket].b[offsetIndex] = sample_B;
        	        }
        	        else
        	        {
        	            imageBuffers[destPacket].r[offsetIndex] += (sample_R - imageBuffers[destPacket].r[offsetIndex]) / curIter;
        	            imageBuffers[destPacket].g[offsetIndex] += (sample_G - imageBuffers[destPacket].g[offsetIndex]) / curIter;
        	            imageBuffers[destPacket].b[offsetIndex] += (sample_B - imageBuffers[destPacket].b[offsetIndex]) / curIter;
        	        }
        	    }

        	    if (curIter == shared_config.cis_oversampling)
        	    {
        	        imageBuffers[packet].h.fragment_index = packet;
        	        imageBuffers[packet].h.line_id = shared_var.cis_process_cnt;
        	    }
        	}
        }

        /* Launch optimized MDMA transfers with selective copying */
        if (cis_startMdma() != HAL_OK)
        {
            printf("CIS: MDMA re-arm FAILED\n");
            Error_Handler();
        }
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
CIS_StatusTypeDef cis_imageProcessRGB_Calibration(int32_t *cisDataCpy, uint32_t *cisCalData,
                                                  uint16_t iterationNb,
                                                  uint8_t progressBase, uint8_t progressSpan)
{
    uint32_t totalElements = cisConfig.useful_data_size_per_lane * CIS_ADC_OUT_LANES;
    uint32_t i;
    uint16_t iteration;
    uint32_t startTick;
    const uint32_t motionDeadline = HAL_GetTick() + CIS_CAL_MOTION_TIMEOUT_MS;
    bool stalledReported = false;

    /* La barre couvre [progressBase, progressBase + progressSpan] et non 0..100 :
       le balayage des niveaux intermediaires enchaine plusieurs captures dans le MEME
       etat CIS_CAL_INTERMEDIATE, et l'afficheur du CM4 ne joue son while (barre < 99)
       qu'une fois par entree dans un etat. Une barre par capture se figerait des la
       premiere. */
    shared_var.cis_cal_progressbar = progressBase;

    /* Clear the calibration and copy buffers */
    for (i = 0; i < totalElements; i++)
    {
        cisCalData[i] = 0;
        cisDataCpy[i] = 0;
    }

    /* Les etats sont laisses COMPLETE par l'etape precedente : sans cette remise a zero
       la premiere iteration accumulerait cisDataCpy tel qu'il vient d'etre efface, soit
       une LIGNE NULLE dans la moyenne -- et de toute facon une ligne anterieure au
       reglage LED de cette etape. On attend donc un transfert frais. */
    for (i = CIS_ADC_OUT_LANES; i-- > 0; )
    {
        cisBufferState[i] = CIS_BUFFER_OFFSET_NONE;
    }
    if (cis_startMdma() != HAL_OK)
    {
        printf("CIS: MDMA re-arm FAILED\n");
        Error_Handler();
    }

    /* iteration n'avance QUE sur une ligne acquise en mouvement : voir plus bas. */
    for (iteration = 0; iteration < iterationNb; )
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
                    return CIS_ERROR;
                }
            }
            /* Reset the state for the next capture */
            cisBufferState[i] = CIS_BUFFER_OFFSET_NONE;
        }

#if CIS_CAL_REQUIRE_MOTION
        /* Capteur immobile : la ligne est jetee, pas comptabilisee. Moyenner iterationNb
           fois la meme portion de papier graverait son grain dans les gains par pixel.
           La barre de progression se fige, ce qui dit a l'operateur quoi faire. */
        if (!cis_isMoving())
        {
            if (!stalledReported)
            {
                printf("CIS cal: sensor still, waiting for motion (%u/%u lines)\n",
                       (unsigned)iteration, (unsigned)iterationNb);
                stalledReported = true;
            }

            if ((int32_t)(HAL_GetTick() - motionDeadline) >= 0)
            {
                printf("CIS cal: ABORTED, only %u/%u lines acquired in motion\n",
                       (unsigned)iteration, (unsigned)iterationNb);
                return CIS_ERROR;
            }

            /* Rearmer quand meme, sinon plus aucune ligne n'arrive et on ne pourrait
               jamais constater le retour du mouvement. */
            if (cis_startMdma() != HAL_OK)
            {
                printf("CIS: MDMA re-arm FAILED\n");
                Error_Handler();
            }
            continue;
        }
        stalledReported = false;
#endif

        /* Sum the acquired buffer into the calibration data */
        for (i = 0; i < totalElements; i++)
        {
            cisCalData[i] += cisDataCpy[i];
        }

        iteration++;

        /* Update the progress bar */
        shared_var.cis_cal_progressbar = progressBase + ((uint32_t)iteration * progressSpan) / iterationNb;

        /* Launch optimized MDMA transfers for calibration */
        if (cis_startMdma() != HAL_OK)
        {
            printf("CIS: MDMA re-arm FAILED\n");
            Error_Handler();
        }
    }

    /* Average the calibration data, kept in Q<CIS_CAL_FRAC_BITS> fixed point.
       Truncating the average of iterationNb lines back to an integer ADC count would
       throw away exactly the precision the averaging just bought; the callers convert
       back to integer counts themselves, so nothing downstream changes scale silently.
       Borne : CIS_CAL_ITER_ANCHOR (1000) lignes x CIS_ADC_FULL_SCALE (16383) = 1,6e7,
       decale de CIS_CAL_FRAC_BITS -> 2,6e8, tient dans un uint32_t. */
    configASSERT((uint64_t)iterationNb * CIS_ADC_FULL_SCALE << CIS_CAL_FRAC_BITS <= 0xFFFFFFFFULL);
    for (i = 0; i < totalElements; i++)
    {
        cisCalData[i] = ((cisCalData[i] << CIS_CAL_FRAC_BITS) + (iterationNb / 2U)) / iterationNb;
    }

    return CIS_OK;
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

    /* Start CLK generation ##################################*/
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);
    /* TIM15 = declencheur ADC en 400 dpi (deux fronts par periode CP, voir tim.c).
       Demarre avant le gel de TIM1 ; sa resynchronisation sur tim1_trgo(update) le met
       en phase des la premiere periode suivant le relachement. */
    CIS_TIM15_TriggerInit();
    HAL_TIM_PWM_Start(&htim15, TIM_CHANNEL_1);

    osDelay(300);

    /* GEL DU MAITRE #########################################
       TIM8 (SP) et TIM3/4/5 (LEDs) sont esclaves de TIM1 : leur phase par rapport aux
       sous-trames couleur du capteur est fixee par la valeur de leur compteur au moment
       ou TIM1 avance. Or TIM1 tourne depuis le demarrage de CH2 ci-dessus : ensemencer
       les esclaves pendant qu'il compte rend la phase dependante du temps d'execution
       jusqu'a la remise a zero de TIM1 -- une seule interruption dans cette fenetre
       (5 us = 19 echantillons a 4 MHz) fait glisser les plans couleur, d'ou images
       dedoublees et dominantes aleatoires apres chaque redemarrage de capture.
       L'ancien code re-ensemencait d'ailleurs TIM8 une seconde fois juste avant le
       lancement : la course etait connue, mais les trois timers LED restaient dedans.
       On gele donc TIM1 (plus de TRGO, les esclaves ne bougent plus), on regle TOUT a
       l'arret, et le HAL_TIM_PWM_Start(htim1, CH1) final relache l'ensemble d'un seul
       coup, en phase par construction. Pendant le gel l'horloge CIS est simplement
       suspendue (registre a decalage statique) ; la premiere ligne, surexposee, est de
       toute facon jetee. CLEAR_BIT direct : __HAL_TIM_DISABLE refuse d'agir tant qu'un
       canal est actif, et CH2 l'est. */
    CLEAR_BIT(htim1.Instance->CR1, TIM_CR1_CEN);

    /* TIM1 gele : plus aucun front TRGO. On coupe aussi TIM15 (il court sur sa propre
       horloge !) pour qu'AUCUN trigger ADC ne parte avant le relachement ; son mode
       esclave reset+trigger le redemarrera au premier TRGO, en phase exacte. */
    CLEAR_BIT(htim15.Instance->CR1, TIM_CR1_CEN);

    // Reset SP counter (CIS_PHASE_TRIM : voir config.h -- retarde toute la trame optique
    // pour que SP retombe sur les echantillons 0-1 du tampon ADC)
    __HAL_TIM_SET_COUNTER(&htim8, cisConfig.lane_size - CIS_SP_WIDTH - CIS_PHASE_TRIM);

    // Set RGB phase shift
    __HAL_TIM_SET_COUNTER(&htim4, (cisConfig.lane_size * 1) - CIS_LED_ON - CIS_PHASE_TRIM);  // R
    __HAL_TIM_SET_COUNTER(&htim5, (cisConfig.lane_size * 3) - CIS_LED_ON - CIS_PHASE_TRIM);  // G
    __HAL_TIM_SET_COUNTER(&htim3, (cisConfig.lane_size * 2) - CIS_LED_ON - CIS_PHASE_TRIM);  // B

    /* Start LEDs ############################################*/
    /* Start LED R generation ###############################*/
    HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_2);

    /* Start LED G generation ###############################*/
    HAL_TIM_PWM_Start(&htim5, TIM_CHANNEL_3);

    /* Start LED B generation ###############################*/
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);

    cis_ledPowerAdj(100, 100, 100);

    /* Start SP generation ##################################*/
    HAL_TIM_PWM_Start(&htim8, TIM_CHANNEL_3);
    __HAL_TIM_MOE_DISABLE(&htim8);


    /* Start DMA #############################################*/
    /* Initialize all buffer states as COMPLETE to skip the first read */
    for (int i = 0; i < CIS_ADC_OUT_LANES; i++)
    {
        cisBufferState[i] = CIS_BUFFER_COMPLETE;
    }

    /* Codes de retour testes : c'est leur silence qui rendait la panne invisible. */
    if (HAL_ADC_Start_DMA(&hadc1, (uint32_t *)cisData_ADC1, cisConfig.adc_buff_size) != HAL_OK)
    {
        printf("CIS startCapture: ADC1 start FAILED (state 0x%08lX)\n", (unsigned long)HAL_ADC_GetState(&hadc1));
    }
    if (HAL_ADC_Start_DMA(&hadc2, (uint32_t *)cisData_ADC2, cisConfig.adc_buff_size) != HAL_OK)
    {
        printf("CIS startCapture: ADC2 start FAILED (state 0x%08lX)\n", (unsigned long)HAL_ADC_GetState(&hadc2));
    }
    if (HAL_ADC_Start_DMA(&hadc3, (uint32_t *)cisData_ADC3, cisConfig.adc_buff_size) != HAL_OK)
    {
        printf("CIS startCapture: ADC3 start FAILED (state 0x%08lX)\n", (unsigned long)HAL_ADC_GetState(&hadc3));
    }

    /* LANCEMENT ############################################
       TIM1 est toujours gele : sa remise a zero et le PWM_Start(CH1) -- qui remet
       CR1.CEN a 1 -- relachent tous les timers en phase, quel que soit le temps passe
       dans les demarrages ADC ci-dessus. Le second ensemencement de TIM8 de l'ancien
       code n'a plus d'objet. Le MOE de TIM8 arrive apres le relachement : au pire le
       premier SP est masque, et cette ligne-la est deja jetee. */
    __HAL_TIM_SET_COUNTER(&htim1, 0);
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
    __HAL_TIM_MOE_ENABLE(&htim8);


    /* AUTO-LOCALISATION du plateau noir ####################
       La phase optique jitte de quelques echantillons d'un demarrage a l'autre (l'etat
       interne du capteur au moment du gel varie). Une position de copie FIXE fait donc
       porter la calibration par pixel sur les voisins des pixels mesures, et laisse la
       queue du transitoire entrer ou sortir de la fenetre de derive selon le boot :
       teintes differentes a chaque demarrage, banding. On DETECTE donc la position
       reelle : le transitoire plonge sous 500 puis le plateau noir (38 echantillons
       ~3500) commence net -- signature sans ambiguite, localisee par voie. */
    {
        osDelay(4);  /* au moins deux lignes completes dans les tampons */

        volatile uint16_t *const rawBuf[CIS_ADC_OUT_LANES] =
            { cisData_ADC1, cisData_ADC2, cisData_ADC3 };

        for (int lane = 0; lane < CIS_ADC_OUT_LANES; lane++)
        {
            uint32_t inv = ((uint32_t)rawBuf[lane]) & ~31U;
            SCB_InvalidateDCache_by_Addr((uint32_t *)inv, 96 * 2 + 64);

            int32_t start = -1;
            bool dipped = false;
            for (int i = 1; i < 90; i++)
            {
                uint16_t x = rawBuf[lane][i];
                if (x < 500U)
                {
                    dipped = true;
                }
                else if (dipped && x > 2000U && rawBuf[lane][i + 1] > 2000U)
                {
                    start = i;
                    break;
                }
            }
            if (start < 0)
            {
                start = CIS_SP_WIDTH + CIS_OVER_SCAN;  /* repli : position nominale */
                printf("CIS: black plateau ADC%d NOT found, fallback %ld\n", lane + 1, (long)start);
            }
            /* + pipeline : le S&H sur plateau etabli lit le pixel PRECEDENT (voir
               CIS_SAMPLE_PIPELINE) -- sans ce decalage, la premiere rangee effective
               de chaque ligne appartient a la ligne d'avant. */
            start += CIS_SAMPLE_PIPELINE;
            nodeConfigs[lane].SrcAddress = (uint32_t)&rawBuf[lane][start];
        }
        printf("CIS: black plateau at %lu %lu %lu\n",
               (unsigned long)((nodeConfigs[0].SrcAddress - (uint32_t)cisData_ADC1) / 2),
               (unsigned long)((nodeConfigs[1].SrcAddress - (uint32_t)cisData_ADC2) / 2),
               (unsigned long)((nodeConfigs[2].SrcAddress - (uint32_t)cisData_ADC3) / 2));
    }

    /* IDENTIFICATION DES COULEURS ##########################
       L'assignation ligne->couleur du tampon etait une CONVENTION (ligne 0 = R) ; or
       elle peut tourner d'un boot a l'autre : la calibration par couleur s'applique
       alors aux mauvaises couleurs -- changement d'aspect immediat et global (~10 %,
       l'ecart de luminosite entre LED), sans le moindre decalage geometrique. Mesure
       inter-boots : niveaux 6395 vs 7036/7055 a decalage nul, deux etats discrets.
       On MESURE donc : LED rouge seule quelques trames, la ligne la plus brillante est
       la ligne R, et les offsets couleur sont remappes en consequence. L'ordre optique
       apres R est G puis B (fenetres LED sequentielles). */
    {
        /* Identification DIFFERENTIELLE : (LED rouge seule) moins (toutes LED
           eteintes). La lumiere ambiante illumine les trois lignes en continu et noie
           un contraste absolu (vecu : trois lignes a ~9000 sous eclairage de banc) ;
           en differentiel, l'ambiant s'annule et seul l'apport de la LED rouge reste.
           La purge de 40 ms entre chaque etat couvre la charge integree pendant le gel
           (LED figees allumees) et le pipeline d'integration. */
        int32_t attempts = 3;
        const int32_t base0 = (int32_t)((nodeConfigs[0].SrcAddress - (uint32_t)cisData_ADC1) / 2);
        int64_t lit[3], dark[3];

      retry_ident:
        cis_ledPowerAdjFine(10000, 0, 0);
        osDelay(40);
        {
            uint32_t inv = ((uint32_t)cisData_ADC1) & ~31U;
            SCB_InvalidateDCache_by_Addr((uint32_t *)inv, CIS_MAX_ADC_BUFF_SIZE * 2 + 64);
            for (int32_t line = 0; line < 3; line++)
            {
                const int32_t o = base0 + line * cisConfig.lane_size + CIS_BLACK_PIXELS + 100;
                lit[line] = 0;
                for (int32_t k = 0; k < 256; k++) lit[line] += cisData_ADC1[o + k];
            }
        }
        cis_ledPowerAdjFine(0, 0, 0);
        osDelay(40);
        {
            uint32_t inv = ((uint32_t)cisData_ADC1) & ~31U;
            SCB_InvalidateDCache_by_Addr((uint32_t *)inv, CIS_MAX_ADC_BUFF_SIZE * 2 + 64);
            for (int32_t line = 0; line < 3; line++)
            {
                const int32_t o = base0 + line * cisConfig.lane_size + CIS_BLACK_PIXELS + 100;
                dark[line] = 0;
                for (int32_t k = 0; k < 256; k++) dark[line] += cisData_ADC1[o + k];
            }
        }

        int64_t mean[3];
        for (int32_t line = 0; line < 3; line++)
        {
            mean[line] = lit[line] - dark[line];   /* apport de la LED rouge seule */
        }

        int32_t rot = 0;
        if (mean[1] > mean[rot]) rot = 1;
        if (mean[2] > mean[rot]) rot = 2;

        const int64_t second = (rot == 0) ? ((mean[1] > mean[2]) ? mean[1] : mean[2])
                             : (rot == 1) ? ((mean[0] > mean[2]) ? mean[0] : mean[2])
                                          : ((mean[0] > mean[1]) ? mean[0] : mean[1]);
        if (mean[rot] > second * 2 && mean[rot] > 256 * 200)
        {
            cisConfig.red_offset   = rot * cisConfig.useful_data_size_per_color_per_lane + CIS_BLACK_PIXELS;
            cisConfig.green_offset = ((rot + 1) % 3) * cisConfig.useful_data_size_per_color_per_lane + CIS_BLACK_PIXELS;
            cisConfig.blue_offset  = ((rot + 2) % 3) * cisConfig.useful_data_size_per_color_per_lane + CIS_BLACK_PIXELS;
            printf("CIS: color rotation %ld (dR=%ld dG=%ld dB=%ld)\n", (long)rot,
                   (long)(mean[0] / 256), (long)(mean[1] / 256), (long)(mean[2] / 256));
        }
        else if (--attempts > 0)
        {
            osDelay(50);
            goto retry_ident;
        }
        else
        {
            printf("CIS: color rotation AMBIGUOUS (d=%ld %ld %ld), keeping previous\n",
                   (long)(mean[0] / 256), (long)(mean[1] / 256), (long)(mean[2] / 256));
        }

        cis_ledPowerAdjFine(10000, 10000, 10000);

    }

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
    /* Un arrêt qui échoue ne doit PAS figer la machine. Error_Handler() coupe les
       interruptions et boucle à l'infini : plus d'UART, plus de réseau, et le CM4 reste
       bloqué sur son dernier écran, le tout sans le moindre indice sur la cause. On
       signale et on continue — les trois HAL_ADC_Stop_DMA non testés en fin de fonction
       rattrapent de toute façon un arrêt partiel. */
    #define CIS_REPORT_ADC_STOP(h, name)                                              \
        do {                                                                          \
            if (HAL_ADC_Stop_DMA(&(h)) != HAL_OK)                                     \
            {                                                                         \
                printf("CIS stopCapture: %s stop FAILED (state 0x%08lX err 0x%08lX)\n",\
                       (name),                                                        \
                       (unsigned long)HAL_ADC_GetState(&(h)),                         \
                       (unsigned long)HAL_ADC_GetError(&(h)));                        \
                cisAdcStopClean = false;                                              \
            }                                                                         \
        } while (0)

    #define CIS_REPORT_TIM_STOP(h, ch, name)                                          \
        do {                                                                          \
            if (HAL_TIM_PWM_Stop(&(h), (ch)) != HAL_OK)                               \
            {                                                                         \
                printf("CIS stopCapture: %s stop FAILED\n", (name));                  \
            }                                                                         \
        } while (0)

    /* Stop ADC Timer ######################################*/
    CIS_REPORT_TIM_STOP(htim1, TIM_CHANNEL_1, "CLK TIM1");

    /* Stop SP generation ####################################*/
    CIS_REPORT_TIM_STOP(htim8, TIM_CHANNEL_3, "SP TIM8");

    /* Stop DMA ##############################################*/
    CIS_REPORT_ADC_STOP(hadc1, "ADC1");
    CIS_REPORT_ADC_STOP(hadc2, "ADC2");
    CIS_REPORT_ADC_STOP(hadc3, "ADC3");

    /* Stop LEDs ############################################*/
    CIS_REPORT_TIM_STOP(htim4, TIM_CHANNEL_2, "LED R TIM4");
    CIS_REPORT_TIM_STOP(htim5, TIM_CHANNEL_3, "LED G TIM5");
    CIS_REPORT_TIM_STOP(htim3, TIM_CHANNEL_1, "LED B TIM3");

    #undef CIS_REPORT_ADC_STOP
    #undef CIS_REPORT_TIM_STOP

    HAL_ADC_Stop_DMA(&hadc1);
    HAL_ADC_Stop_DMA(&hadc2);
    HAL_ADC_Stop_DMA(&hadc3);

    /*
    for (int i = 0; i < CIS_ADC_OUT_LANES; i++)
    {
    	cisBufferState[i] = CIS_BUFFER_OFFSET_NONE;
    }
    */

    /* Recuperation apres un arret sale ######################*/
    /* Le DMA des ADC tourne en circulaire avec FIFO et rafale memoire de 8. HAL_ADC_Stop_DMA
       coupe l'ADC AVANT d'abandonner le DMA : la coupure tombe en plein milieu d'une ligne,
       le FIFO garde une rafale incomplete que plus aucune donnee ne viendra terminer, et le
       bit EN ne retombe pas dans les 5 ms du timeout HAL. HAL_DMA_Abort laisse alors le
       handle en HAL_DMA_STATE_ERROR (et non READY), si bien que HAL_ADC_Start_DMA ne peut
       plus reprogrammer le DMA : l'ADC reconvertit mais plus rien n'est transfere, et
       l'image reste figee jusqu'au reboot. On repart donc d'un handle neuf. HAL_ADC_DeInit
       passe par MspDeInit, donc par HAL_DMA_DeInit, qui n'ecrit que des registres et ne peut
       pas se bloquer.

       Cette reprise doit imperativement rester ICI, et surtout PAS dans cis_startCapture().
       TIM3/4/5 (les LEDs) sont esclaves de TIM1 en SLAVEMODE_EXTERNAL1 : une fois demarres
       ils comptent les impulsions de l'horloge CIS, qui tourne deja. Or cis_startCapture()
       les demarre, puis remet SEUL TIM1 a zero avant de lancer l'acquisition. Glisser
       plusieurs millisecondes de reinitialisation ADC entre les deux les laisse avancer de
       milliers de comptages : la phase des LEDs par rapport aux sous-trames couleur du
       capteur devient arbitraire, l'eclairage deborde sur les mauvaises sous-trames, et
       l'image prend une dominante differente a chaque calibration. */
    if (!cisAdcStopClean)
    {
        printf("CIS: ADC/DMA recovery after a failed stop\n");
        (void)HAL_ADC_DeInit(&hadc1);
        (void)HAL_ADC_DeInit(&hadc2);
        (void)HAL_ADC_DeInit(&hadc3);
        cis_initAdc(false);
        cisAdcStopClean = true;
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
/* Version fine : puissances en centiemes de pourcent (0..10000), granularite CCR
   ~0,17 %. Utilisee par l'asservissement thermique, qui a besoin de pas bien plus
   fins que le pourcent de cis_ledPowerAdj. */
void cis_ledPowerAdjFine(int32_t red_x100, int32_t green_x100, int32_t blue_x100)
{
    TIM_HandleTypeDef *const tims[3] = { &htim4, &htim5, &htim3 };
    const uint32_t chans[3] = { TIM_CHANNEL_2, TIM_CHANNEL_3, TIM_CHANNEL_1 };
    const int32_t maxes[3] = { cisLeds_Calibration.redLed_maxPulse,
                               cisLeds_Calibration.greenLed_maxPulse,
                               cisLeds_Calibration.blueLed_maxPulse };
    const int32_t vals[3] = { red_x100, green_x100, blue_x100 };

    for (int i = 0; i < 3; i++)
    {
        int32_t v = vals[i];
        v = (v < 0) ? 0 : (v > 10000) ? 10000 : v;
        __HAL_TIM_SET_COMPARE(tims[i], chans[i], ((maxes[i] - 1) * v) / 10000);
    }
}

/* Asservissement thermique : a appeler ~1 Hz hors calibration. Corrige le duty LED de
   l'ecart entre la temperature courante et celle de la calibration. Sans cout dans la
   boucle pixel. */
void cis_ledThermalServo(void)
{
#if CIS_LED_TEMP_COEFF_X100 != 0
    if (shared_var.cis_cal_state != CIS_CAL_END || cisCals.cal_temp_c == 0.0f)
    {
        return;
    }
    const float dT = shared_imu.temp_c - cisCals.cal_temp_c;
    int32_t duty = 10000 + (int32_t)(dT * (float)CIS_LED_TEMP_COEFF_X100);
    cis_ledPowerAdjFine(duty, duty, duty);
#endif
}

void cis_ledPowerAdj(int32_t red_pwm, int32_t green_pwm, int32_t blue_pwm)
{
	int32_t pulseValue = 0;

	// Ensure that the power intensity is within the expected range
	red_pwm = red_pwm < 0 ? 0 : red_pwm > 100 ? 100 : red_pwm;
	green_pwm = green_pwm < 0 ? 0 : green_pwm > 100 ? 100 : green_pwm;
	blue_pwm = blue_pwm < 0 ? 0 : blue_pwm > 100 ? 100 : blue_pwm;

	pulseValue = cisLeds_Calibration.redLed_maxPulse - 1;

	pulseValue = (pulseValue * red_pwm) / 100;
	__HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_2, pulseValue);

	pulseValue = cisLeds_Calibration.greenLed_maxPulse - 1;

	pulseValue = (pulseValue * green_pwm) / 100;
	__HAL_TIM_SET_COMPARE(&htim5, TIM_CHANNEL_3, pulseValue);

	pulseValue = cisLeds_Calibration.blueLed_maxPulse - 1;

	pulseValue = (pulseValue * blue_pwm) / 100;
	__HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, pulseValue);
}

/**
 * @brief  CIS adc init
 * @param  None
 * @retval None
 */
/* Facteurs de calibration d'offset des trois ADC, releves au demarrage.

   Une calibration d'offset n'est pas exactement reproductible : elle rend un facteur qui
   varie de quelques LSB d'une execution a l'autre. Or les offsets et gains PAR PIXEL de
   cisCals ont ete mesures avec le facteur en vigueur au moment de la calibration capteur.
   En recalculer un nouveau ensuite decale la donnee brute sous une calibration qui ne
   l'attend plus. L'ecart est amplifie par gain[p], le plus eleve dans le bleu puisque
   c'est la reponse la plus faible du CIS : il en resulte une dominante coloree dont le
   signe change d'une calibration a l'autre, et qui disparait au redemarrage.

   La reprise apres un arret ADC rate doit donc RESTAURER les facteurs du demarrage, pas
   en calculer de nouveaux -- HAL_ADC_DeInit remet ADC_CALFACT a zero. */
static uint32_t cisAdcCalFactor[CIS_ADC_OUT_LANES] = {0};
static bool     cisAdcCalFactorValid = false;

void cis_initAdc(bool freshOffsetCalibration)
{
	ADC_HandleTypeDef *const adc[CIS_ADC_OUT_LANES] = { &hadc1, &hadc2, &hadc3 };

	MX_ADC1_Init();
	MX_ADC2_Init();
	MX_ADC3_Init();

	/* 400 dpi : 2 px/horloge -> chaque ADC convertit sur LES DEUX fronts de TIM1_CC3
	   (PWM asymetrique : fronts a 125 et 375 ns, au centre de chaque phase pixel), soit
	   4 Mech/s par voie -- le meme debit qu'avant, mais couvrant les 1152 pixels reels.
	   Ecrit ici plutot que dans adc.c : les MX_ADCx_Init sont regeneres par CubeMX.
	   CFGR n'est modifiable qu'ADC desactive : c'est le cas au sortir de MX_ADCx_Init. */
	if (shared_config.cis_dpi == 400)
	{
		ADC_HandleTypeDef *const adcAll[CIS_ADC_OUT_LANES] = { &hadc1, &hadc2, &hadc3 };
		for (int32_t i = 0; i < CIS_ADC_OUT_LANES; i++)
		{
			MODIFY_REG(adcAll[i]->Instance->CFGR,
			           ADC_CFGR_EXTSEL | ADC_CFGR_EXTEN,
			           (ADC_EXTERNALTRIG_T15_TRGO & ADC_CFGR_EXTSEL) | ADC_EXTERNALTRIGCONVEDGE_RISING);
		}
	}

	for (int32_t i = 0; i < CIS_ADC_OUT_LANES; i++)
	{
		if (freshOffsetCalibration || !cisAdcCalFactorValid)
		{
			/* ### Start calibration ############################################ */
			if (HAL_ADCEx_Calibration_Start(adc[i], ADC_CALIB_OFFSET, ADC_SINGLE_ENDED) != HAL_OK)
			{
				Error_Handler();
			}
			cisAdcCalFactor[i] = HAL_ADCEx_Calibration_GetValue(adc[i], ADC_SINGLE_ENDED);
		}

		if (HAL_ADCEx_LinearCalibration_FactorLoad(adc[i]) != HAL_OK)
		{
			Error_Handler();
		}

		if (!(freshOffsetCalibration || !cisAdcCalFactorValid))
		{
			/* ADC_CALFACT n'est accessible qu'ADC ACTIVE : HAL_ADCEx_Calibration_SetValue
			   exige LL_ADC_IsEnabled() != 0 et refuse sinon. Sorti de MX_ADCx_Init(), il
			   est desactive. On l'active donc ici, et on le LAISSE actif : le
			   HAL_ADC_Start_DMA de cis_startCapture() passe par ADC_Enable(), qui rend
			   HAL_OK immediatement sur un ADC deja actif. */
			LL_ADC_Enable(adc[i]->Instance);

			const uint32_t t0 = HAL_GetTick();
			while (LL_ADC_IsActiveFlag_ADRDY(adc[i]->Instance) == 0UL)
			{
				if ((HAL_GetTick() - t0) > 5U)
				{
					break;
				}
			}

			if (HAL_ADCEx_Calibration_SetValue(adc[i], ADC_SINGLE_ENDED, cisAdcCalFactor[i]) != HAL_OK)
			{
				printf("CIS: ADC%d offset factor restore FAILED\n", (int)i + 1);
			}
		}
	}

	cisAdcCalFactorValid = true;
}

/**
 * @brief  Démarrage des transferts MDMA optimisés avec linked lists et block address offset
 *         Copie seulement les données utiles (CIS_BLACK_LINE + pixels) en sautant SP_WIDTH et OVERSCAN
 *         Utilise les vraies linked lists avec block address offsets
 * @param  None
 * @retval HAL_StatusTypeDef
 */
static HAL_StatusTypeDef cis_startMdma(void)
{
    HAL_StatusTypeDef status = HAL_OK;

    status = HAL_MDMA_Start_IT(&hmdma_mdma_channel1_dma1_stream0_tc_0,
                               nodeConfigs[0].SrcAddress,
                               nodeConfigs[0].DstAddress,
                               nodeConfigs[0].BlockDataLength,
                               nodeConfigs[0].BlockCount);
    if (status != HAL_OK) return status;

    status = HAL_MDMA_Start_IT(&hmdma_mdma_channel2_dma1_stream1_tc_0,
                               nodeConfigs[1].SrcAddress,
                               nodeConfigs[1].DstAddress,
                               nodeConfigs[1].BlockDataLength,
                               nodeConfigs[1].BlockCount);
    if (status != HAL_OK) return status;

    status = HAL_MDMA_Start_IT(&hmdma_mdma_channel3_dma2_stream0_tc_0,
                               nodeConfigs[2].SrcAddress,
                               nodeConfigs[2].DstAddress,
                               nodeConfigs[2].BlockDataLength,
                               nodeConfigs[2].BlockCount);

    return status;
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
