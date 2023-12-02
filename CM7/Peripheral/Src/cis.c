/**
 ******************************************************************************
 * @file           : cis_BW_.c
 * @brief          :
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "config.h"
#include "basetypes.h"
#include "shared.h"

#include "stdlib.h"
#include "stdio.h"
#include "stdbool.h"

#include "arm_math.h"

#include "tim.h"
#include "adc.h"
#include "dma.h"

#include "stm32_flash.h"
#include "cis_linearCal.h"
#include "cis_polyCal.h"

#include "cis.h"

/* Private includes ----------------------------------------------------------*/

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/
#define CIS_GREEN_HALF_SIZE			((CIS_ADC_BUFF_SIZE / 2) - (CIS_LINE_SIZE + CIS_INACTIVE_WIDTH))
#define CIS_GREEN_FULL_SIZE			((CIS_PIXELS_PER_LINE) - (CIS_GREEN_HALF_SIZE))

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

static volatile CIS_BUFF_StateTypeDef  cisHalfBufferState[3] = {0};
static volatile CIS_BUFF_StateTypeDef  cisFullBufferState[3] = {0};

/* Variable containing ADC conversions data */

/* Private function prototypes -----------------------------------------------*/
static void cis_TIM_MAIN_Init(void);
static void cis_TIM_CLK_Init(void);
static void cis_TIM_SP_Init(void);
static void cis_TIM_LED_R_Init(void);
static void cis_TIM_LED_G_Init(void);
static void cis_TIM_LED_B_Init(void);
static void cis_ADC_Init(void);

/* Private user code ---------------------------------------------------------*/

/**
 * @brief  CIS init
 * @param  Void
 * @retval None
 */
void cis_Init()
{
	printf("----------- CIS INIT ----------\n");

	printf("CIS END CAPTURE = %d\n", CIS_LINE_SIZE);

	// Enable 5V power DC/DC for display
	HAL_GPIO_WritePin(EN_5V_GPIO_Port, EN_5V_Pin, GPIO_PIN_SET);

	memset((int16_t *)&cisData[0], 0, CIS_ADC_BUFF_SIZE * 3 * sizeof(uint16_t));
	//memset((float32_t *)&cisDataCpy_q31[0], 0, CIS_ADC_BUFF_SIZE * 3 * sizeof(uint32_t));

	cisLeds_Calibration.redLed_maxPulse = CIS_LED_RED_OFF;
	cisLeds_Calibration.greenLed_maxPulse = CIS_LED_GREEN_OFF;
	cisLeds_Calibration.blueLed_maxPulse = CIS_LED_BLUE_OFF;

#ifdef CIS_400DPI
	HAL_GPIO_WritePin(CIS_RS_GPIO_Port, CIS_RS_Pin, GPIO_PIN_RESET); //SET : 200DPI   RESET : 400DPI
#else
	HAL_GPIO_WritePin(CIS_RS_GPIO_Port, CIS_RS_Pin, GPIO_PIN_SET); //SET : 200DPI   RESET : 400DPI
#endif

	cis_ADC_Init();
	cis_TIM_SP_Init();
	cis_TIM_LED_R_Init();
	cis_TIM_LED_G_Init();
	cis_TIM_LED_B_Init();
	cis_TIM_CLK_Init();
	cis_TIM_MAIN_Init();

	//cis_linealCalibrationInit();

	cis_Start_capture();
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
 *      R2 = CIS_START_OFFSET + CIS_ADC_BUFF_SIZE
 *     	R3 = CIS_START_OFFSET + CIS_ADC_BUFF_SIZE * 2
 *
 *      G1 = CIS_LINE_SIZE + CIS_START_OFFSET
 *      G2 = CIS_LINE_SIZE + CIS_START_OFFSET + CIS_ADC_BUFF_SIZE
 *      G3 = CIS_LINE_SIZE + CIS_START_OFFSET + CIS_ADC_BUFF_SIZE * 2
 *
 *      B1 = CIS_LINE_SIZE * 2 + CIS_START_OFFSET
 *      B2 = CIS_LINE_SIZE * 2 + CIS_START_OFFSET + CIS_ADC_BUFF_SIZE
 *      B3 = CIS_LINE_SIZE * 2 + CIS_START_OFFSET + CIS_ADC_BUFF_SIZE * 2
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
 *              |                LINE1                  ||                LINE2                  ||                LINE3                  |
 *	 	HALF => |RG--|RG--|RG--|RG--|R---|R---|R---|R---||RG--|RG--|RG--|RG--|R---|R---|R---|R---||RG--|RG--|RG--|RG--|R---|R---|R---|R---|
 * 		FULL => |--B-|--B-|--B-|--B-|-GB-|-GB-|-GB-|-GB-||--B-|--B-|--B-|--B-|-GB-|-GB-|-GB-|-GB-||--B-|--B-|--B-|--B-|-GB-|-GB-|-GB-|-GB-|
 * 		OUT  => |RGB-|RGB-|RGB-|RGB-|RGB-|RGB-|RGB-|RGB-||RGB-|RGB-|RGB-|RGB-|RGB-|RGB-|RGB-|RGB-||RGB-|RGB-|RGB-|RGB-|RGB-|RGB-|RGB-|RGB-|
 * 		        ^                                        ^                                        ^
 *              M1                                       M2                                       M3
 *
 */
#pragma GCC push_options
#pragma GCC optimize ("unroll-loops")
void cis_getRAWImage(float32_t* cisDataCpy_f32, uint16_t overSampling)
{
	int32_t acc = 0;
	static int32_t line, i;

	arm_fill_f32(0, cisDataCpy_f32, CIS_ADC_BUFF_SIZE * 3);

	while (acc < overSampling)
	{
		// Read and copy half DMAs buffers
		for (line = CIS_ADC_OUT_LINES; --line >= 0;)
		{
			/* 1st half DMA buffer Data represent Full R region + 1/2 of G region */
			while (cisHalfBufferState[line] != CIS_BUFFER_OFFSET_HALF);

			/* Invalidate Data Cache */
			SCB_InvalidateDCache_by_Addr((uint32_t *)&cisData[CIS_ADC_BUFF_SIZE * line], (CIS_ADC_BUFF_SIZE * sizeof(uint16_t)) / 2);
			for (i = (CIS_ADC_BUFF_SIZE / 2); --i >= 0;)
			{
				cisDataCpy_f32[CIS_ADC_BUFF_SIZE * line + i] += (float32_t)(cisData[CIS_ADC_BUFF_SIZE * line + i]);
			}

			cisHalfBufferState[line] = CIS_BUFFER_OFFSET_NONE;
		}

		// Read and copy full DMAs buffers
		for (line = CIS_ADC_OUT_LINES; --line >= 0;)
		{
			/* 2nd full DMA buffer Data represent last 1/2 of G region + Full B region */
			while (cisFullBufferState[line] != CIS_BUFFER_OFFSET_FULL);

			/* Invalidate Data Cache */
			SCB_InvalidateDCache_by_Addr((uint32_t *)&cisData[(CIS_ADC_BUFF_SIZE * line) + (CIS_ADC_BUFF_SIZE / 2)], (CIS_ADC_BUFF_SIZE * sizeof(uint16_t)) / 2);
			for (i = (CIS_ADC_BUFF_SIZE / 2); --i >= 0;)
			{
				cisDataCpy_f32[(CIS_ADC_BUFF_SIZE * line) + (CIS_ADC_BUFF_SIZE / 2) + i] += (float32_t)(cisData[(CIS_ADC_BUFF_SIZE * line) + (CIS_ADC_BUFF_SIZE / 2) + i]);
			}

			cisFullBufferState[line] = CIS_BUFFER_OFFSET_NONE;
		}

		acc ++;
	}

	if ( overSampling > 1)
	{
		arm_scale_f32(cisDataCpy_f32, 1.0 / (float32_t)overSampling, cisDataCpy_f32, CIS_ADC_BUFF_SIZE * 3);
	}
}

void cis_ConvertRAWImageToFloatArray(float32_t* cisDataCpy_f32, struct RAWImage* RAWImage)
{
	// Get the segments and copy them to the full red line buffer
	arm_copy_f32(&cisDataCpy_f32[CIS_START_OFFSET], RAWImage->redLine, CIS_PIXELS_PER_LINE);
	arm_copy_f32(&cisDataCpy_f32[CIS_START_OFFSET + CIS_ADC_BUFF_SIZE], &RAWImage->redLine[CIS_PIXELS_PER_LINE], CIS_PIXELS_PER_LINE);
	arm_copy_f32(&cisDataCpy_f32[CIS_START_OFFSET + CIS_ADC_BUFF_SIZE * 2], &RAWImage->redLine[CIS_PIXELS_PER_LINE * 2], CIS_PIXELS_PER_LINE);

	// Get the segments and copy them to the full green line buffer
	arm_copy_f32(&cisDataCpy_f32[CIS_LINE_SIZE + CIS_START_OFFSET], RAWImage->greenLine, CIS_PIXELS_PER_LINE);
	arm_copy_f32(&cisDataCpy_f32[CIS_LINE_SIZE + CIS_START_OFFSET + CIS_ADC_BUFF_SIZE], &RAWImage->greenLine[CIS_PIXELS_PER_LINE], CIS_PIXELS_PER_LINE);
	arm_copy_f32(&cisDataCpy_f32[CIS_LINE_SIZE + CIS_START_OFFSET + CIS_ADC_BUFF_SIZE * 2], &RAWImage->greenLine[CIS_PIXELS_PER_LINE * 2], CIS_PIXELS_PER_LINE);

	// Get the segments and copy them to the full blue line buffer
	arm_copy_f32(&cisDataCpy_f32[CIS_LINE_SIZE * 2 + CIS_START_OFFSET], RAWImage->blueLine, CIS_PIXELS_PER_LINE);
	arm_copy_f32(&cisDataCpy_f32[CIS_LINE_SIZE * 2 + CIS_START_OFFSET + CIS_ADC_BUFF_SIZE], &RAWImage->blueLine[CIS_PIXELS_PER_LINE], CIS_PIXELS_PER_LINE);
	arm_copy_f32(&cisDataCpy_f32[CIS_LINE_SIZE * 2 + CIS_START_OFFSET + CIS_ADC_BUFF_SIZE * 2], &RAWImage->blueLine[CIS_PIXELS_PER_LINE * 2], CIS_PIXELS_PER_LINE);
}

void cis_ImageProcessRGB_2(int32_t *cis_buff)
{
	static struct RAWImage RAWImage = {0};
	static float32_t cisDataCpy_f32[CIS_ADC_BUFF_SIZE * 3] = {0};

	cis_getRAWImage(cisDataCpy_f32, shared_var.cis_oversampling);
	cis_ConvertRAWImageToFloatArray(cisDataCpy_f32, &RAWImage);
	cis_ApplyCalibration(&RAWImage, &rgbCalibration);
	cis_ConvertRAWImageToRGBImage(&RAWImage, cis_buff);
}

void cis_ImageProcessRGB(int32_t *cis_buff)
{
	static uint32_t dataOffset_Rx, dataOffset_Gx, dataOffset_Cx, dataOffset_Bx, imageOffset;
	static int32_t tmp_cis_buff[CIS_PIXELS_NB];
	static int32_t line, i;

	cis_getRAWImage(cisDataCpy_f32, shared_var.cis_oversampling);

#ifndef CIS_DESACTIVATE_CALIBRATION
	for (line = CIS_ADC_OUT_LINES; --line >= 0;)
	{
		dataOffset_Rx = (CIS_ADC_BUFF_SIZE * line) + CIS_RED_LINE_OFFSET;									//Rx
		dataOffset_Gx = (CIS_ADC_BUFF_SIZE * line) + CIS_GREEN_LINE_OFFSET;									//Gx
		dataOffset_Bx = (CIS_ADC_BUFF_SIZE * line) + CIS_BLUE_LINE_OFFSET;									//Bx

		//offset compensation
		/*
		static float32_t tmpImmactiveAvrg_R = 0.0;
		static float32_t tmpImmactiveAvrg_G = 0.0;
		static float32_t tmpImmactiveAvrg_B = 0.0;
		arm_mean_f32(&cisDataCpy_f32[dataOffset_Rx - CIS_INACTIVE_WIDTH], CIS_INACTIVE_WIDTH, &tmpImmactiveAvrg_R);
		arm_mean_f32(&cisDataCpy_f32[dataOffset_Gx - CIS_INACTIVE_WIDTH], CIS_INACTIVE_WIDTH, &tmpImmactiveAvrg_G);
		arm_mean_f32(&cisDataCpy_f32[dataOffset_Bx - CIS_INACTIVE_WIDTH], CIS_INACTIVE_WIDTH, &tmpImmactiveAvrg_B);

		tmpImmactiveAvrg_R -= cisCals.whiteCal.red.inactiveAvrgPix[line];
		tmpImmactiveAvrg_G -= cisCals.whiteCal.green.inactiveAvrgPix[line];
		tmpImmactiveAvrg_B -= cisCals.whiteCal.blue.inactiveAvrgPix[line];

		arm_offset_f32(&cisDataCpy_f32[dataOffset_Rx], tmpImmactiveAvrg_R, &cisDataCpy_f32[dataOffset_Rx], CIS_PIXELS_PER_LINE);
		arm_offset_f32(&cisDataCpy_f32[dataOffset_Gx], tmpImmactiveAvrg_G, &cisDataCpy_f32[dataOffset_Gx], CIS_PIXELS_PER_LINE);
		arm_offset_f32(&cisDataCpy_f32[dataOffset_Bx], tmpImmactiveAvrg_B, &cisDataCpy_f32[dataOffset_Bx], CIS_PIXELS_PER_LINE);
		 */
		//end offset compensation

		arm_sub_f32(&cisDataCpy_f32[dataOffset_Rx], &cisCals.blackCal.data[dataOffset_Rx], &cisDataCpy_f32[dataOffset_Rx], CIS_PIXELS_PER_LINE);
		arm_sub_f32(&cisDataCpy_f32[dataOffset_Gx], &cisCals.blackCal.data[dataOffset_Gx], &cisDataCpy_f32[dataOffset_Gx], CIS_PIXELS_PER_LINE);
		arm_sub_f32(&cisDataCpy_f32[dataOffset_Bx], &cisCals.blackCal.data[dataOffset_Bx], &cisDataCpy_f32[dataOffset_Bx], CIS_PIXELS_PER_LINE);

		arm_mult_f32(&cisDataCpy_f32[dataOffset_Rx], &cisCals.gainsData[dataOffset_Rx], &cisDataCpy_f32[dataOffset_Rx], CIS_PIXELS_PER_LINE);
		arm_mult_f32(&cisDataCpy_f32[dataOffset_Gx], &cisCals.gainsData[dataOffset_Gx], &cisDataCpy_f32[dataOffset_Gx], CIS_PIXELS_PER_LINE);
		arm_mult_f32(&cisDataCpy_f32[dataOffset_Bx], &cisCals.gainsData[dataOffset_Bx], &cisDataCpy_f32[dataOffset_Bx], CIS_PIXELS_PER_LINE);

		arm_clip_f32(&cisDataCpy_f32[dataOffset_Rx], &cisDataCpy_f32[dataOffset_Rx], 0, 4095, CIS_PIXELS_PER_LINE);
		arm_clip_f32(&cisDataCpy_f32[dataOffset_Gx], &cisDataCpy_f32[dataOffset_Gx], 0, 4095, CIS_PIXELS_PER_LINE);
		arm_clip_f32(&cisDataCpy_f32[dataOffset_Bx], &cisDataCpy_f32[dataOffset_Bx], 0, 4095, CIS_PIXELS_PER_LINE);
	}
#endif

	for (i = (CIS_ADC_BUFF_SIZE * 3); --i >= 0;)
	{
		cisDataCpy_q31[i] = (int32_t)(cisDataCpy_f32[i]);
	}

	arm_fill_f32(0, cisDataCpy_f32, CIS_ADC_BUFF_SIZE * 3);

	for (line = CIS_ADC_OUT_LINES; --line >= 0;)
	{
		imageOffset = (CIS_PIXELS_PER_LINE * line);	//Mx

		dataOffset_Rx = (CIS_ADC_BUFF_SIZE * line) + CIS_RED_LINE_OFFSET;									//Rx
		dataOffset_Gx = (CIS_ADC_BUFF_SIZE * line) + CIS_GREEN_LINE_OFFSET;									//Gx

		arm_shift_q31(&cisDataCpy_q31[dataOffset_Rx], -4, &cisDataCpy_q31[dataOffset_Rx], (CIS_ADC_BUFF_SIZE / 2) - CIS_START_OFFSET); 	//R + Ghalf 8 bit conversion

		arm_shift_q31(&cisDataCpy_q31[dataOffset_Gx], 8, &cisDataCpy_q31[dataOffset_Gx], CIS_GREEN_HALF_SIZE); 	//Ghalf 8 bit shift

		arm_add_q31(&cisDataCpy_q31[dataOffset_Gx], &cisDataCpy_q31[dataOffset_Rx], &cisDataCpy_q31[dataOffset_Rx], CIS_GREEN_HALF_SIZE); //Add Green

		dataOffset_Cx = (CIS_ADC_BUFF_SIZE * line) + (CIS_ADC_BUFF_SIZE / 2);								//Cx
		dataOffset_Bx = (CIS_ADC_BUFF_SIZE * line) + CIS_BLUE_LINE_OFFSET;									//Bx

		arm_shift_q31(&cisDataCpy_q31[dataOffset_Cx], -4, &cisDataCpy_q31[dataOffset_Cx], CIS_ADC_BUFF_SIZE / 2); 	//Gfull + B 8 bit conversion

		arm_shift_q31(&cisDataCpy_q31[dataOffset_Cx], 8, &cisDataCpy_q31[dataOffset_Cx], CIS_GREEN_FULL_SIZE); 		//Gfull 8 bit shift
		arm_shift_q31(&cisDataCpy_q31[dataOffset_Bx], 16, &cisDataCpy_q31[dataOffset_Bx], CIS_PIXELS_PER_LINE); 	//B 16 bit shift

		arm_add_q31(&cisDataCpy_q31[dataOffset_Cx], &cisDataCpy_q31[dataOffset_Bx + CIS_GREEN_HALF_SIZE], &cisDataCpy_q31[dataOffset_Bx + CIS_GREEN_HALF_SIZE], CIS_GREEN_FULL_SIZE); //Add Green

		arm_add_q31(&cisDataCpy_q31[dataOffset_Rx], &cisDataCpy_q31[dataOffset_Bx], &tmp_cis_buff[imageOffset], CIS_PIXELS_PER_LINE); //Half + Full Data


		//		arm_fill_q31(0xFF0000 >> (line * 8), &cis_buff[imageOffset], CIS_PIXELS_PER_LINE); //RGB debug
	}

	if (shared_var.cis_scanDir)
	{
		for (i = CIS_PIXELS_NB; --i >= 0;)
		{
			cis_buff[i] = tmp_cis_buff[CIS_PIXELS_NB - i];
		}
	}
	else
	{
		for (i = CIS_PIXELS_NB; --i >= 0;)
		{
			cis_buff[i] = tmp_cis_buff[i];
		}
	}

	//	//debug
	//	for (line = CIS_ADC_OUT_LINES; --line >= 0;)
	//	{
	//		imageOffset = (CIS_PIXELS_PER_LINE * line);	//Mx
	//
	//		dataOffset_Rx = (CIS_ADC_BUFF_SIZE * line);									//Rx
	//
	//		arm_shift_q31(&cisDataCpy_q31[dataOffset_Rx], -4, &cisDataCpy_q31[dataOffset_Rx], (CIS_ADC_BUFF_SIZE / 3)); 	//R + Ghalf 8 bit conversion
	//		arm_copy_q31(&cisDataCpy_q31[dataOffset_Rx], &cis_buff[imageOffset], CIS_PIXELS_PER_LINE); //Half + Full Data
	//	}

	//fort transmit full data of one line
	//	arm_copy_q31(&cisDataCpy_q31[0], &cis_buff[0], CIS_PIXELS_PER_LINE * 3);
}

/**
 * @brief  cis_ImageProcessRGB_Calibration
 * @param  cis calibration buffer ptr
 * @retval None
 */
void cis_ImageProcessRGB_Calibration(float32_t *cisCalData, uint16_t iterationNb)
{
	static int32_t line, iteration, pix, i;
	shared_var.cis_cal_progressbar = 0;

	memset(cisCalData, 0, CIS_ADC_BUFF_SIZE * 3 * sizeof(uint32_t));

	for (iteration = 0; iteration < iterationNb; iteration++)
	{
		// Read and copy half DMAs buffers
		for (line = CIS_ADC_OUT_LINES; --line >= 0;)
		{
			/* 1st half DMA buffer Data represent Full R region + 1/2 of G region */
			while (cisHalfBufferState[line] != CIS_BUFFER_OFFSET_HALF)
			{
				//				printf("wait \n");
			}

			/* Invalidate Data Cache */
			SCB_InvalidateDCache_by_Addr((uint32_t *)&cisData[CIS_ADC_BUFF_SIZE * line], (CIS_ADC_BUFF_SIZE * sizeof(uint16_t)) / 2);
			for (i = (CIS_ADC_BUFF_SIZE / 2); --i >= 0;)
			{
				cisCalData[CIS_ADC_BUFF_SIZE * line + i] += (float32_t)(cisData[CIS_ADC_BUFF_SIZE * line + i]);
			}
			cisHalfBufferState[line] = CIS_BUFFER_OFFSET_NONE;
		}

		// Read and copy full DMAs buffers
		for (line = CIS_ADC_OUT_LINES; --line >= 0;)
		{
			/* 2nd full DMA buffer Data represent last 1/2 of G region + Full B region */
			while (cisFullBufferState[line] != CIS_BUFFER_OFFSET_FULL)
			{
				//				printf("wait \n");
			}


			/* Invalidate Data Cache */
			SCB_InvalidateDCache_by_Addr((uint32_t *)&cisData[(CIS_ADC_BUFF_SIZE * line) + (CIS_ADC_BUFF_SIZE / 2)], (CIS_ADC_BUFF_SIZE * sizeof(uint16_t)) / 2);
			for (i = (CIS_ADC_BUFF_SIZE / 2); --i >= 0;)
			{
				cisCalData[(CIS_ADC_BUFF_SIZE * line) + (CIS_ADC_BUFF_SIZE / 2) + i] += (float32_t)(cisData[(CIS_ADC_BUFF_SIZE * line) + (CIS_ADC_BUFF_SIZE / 2) + i]);
			}
			cisFullBufferState[line] = CIS_BUFFER_OFFSET_NONE;
		}
		shared_var.cis_cal_progressbar = iteration * 100 / (iterationNb);
	}

	for (pix = (CIS_ADC_BUFF_SIZE * 3); --pix >= 0;)
	{
		cisCalData[pix] /= iterationNb;

#ifdef PRINT_CIS_CALIBRATION
		printf("Pix = %d, Val = %d\n", (int)pix, (int)cisCalData[pix]);
#endif
	}
}
#pragma GCC pop_options

/**
 * @brief  CIS start captures
 * @param  None
 * @retval None
 */
void cis_Start_capture()
{
	/* Reset CLKs ############################################*/
	//Reset CLK counter
	__HAL_TIM_SET_COUNTER(&htim1, 0);

	//Reset SP counter
	__HAL_TIM_SET_COUNTER(&htim8, CIS_LINE_SIZE - CIS_SP_WIDTH);

	//Set RGB phase shift
#ifndef CIS_MONOCHROME
	__HAL_TIM_SET_COUNTER(&htim4, (CIS_LINE_SIZE * 1) - CIS_LED_RED_ON);	//R
	__HAL_TIM_SET_COUNTER(&htim5, (CIS_LINE_SIZE * 3) - CIS_LED_GREEN_ON);	//G
	__HAL_TIM_SET_COUNTER(&htim3, (CIS_LINE_SIZE * 2) - CIS_LED_BLUE_ON);	//B
#else
	__HAL_TIM_SET_COUNTER(&htim4, (CIS_LINE_SIZE * 1) - CIS_LED_RED_ON);	//R
	__HAL_TIM_SET_COUNTER(&htim5, (CIS_LINE_SIZE * 1) - CIS_LED_GREEN_ON);	//G
	__HAL_TIM_SET_COUNTER(&htim3, (CIS_LINE_SIZE * 1) - CIS_LED_BLUE_ON);	//B
#endif

	/* Start LEDs ############################################*/
	/* Start LED R generation ###############################*/
	HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_2);

	/* Start LED G generation ###############################*/
	HAL_TIM_PWM_Start(&htim5, TIM_CHANNEL_3);

	/* Start LED B generation ###############################*/
	HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);

	/* Start CLK generation ##################################*/
	HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);

	/* Start SP generation ##################################*/
	HAL_TIM_PWM_Start(&htim8, TIM_CHANNEL_3);

	/* Start ADC Timer #######################################*/
	HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);

	/* Start DMA #############################################*/
	HAL_ADC_Start_DMA(&hadc1, (uint32_t *)&cisData[0], CIS_ADC_BUFF_SIZE);
	HAL_ADC_Start_DMA(&hadc2, (uint32_t *)&cisData[CIS_ADC_BUFF_SIZE], CIS_ADC_BUFF_SIZE);
	HAL_ADC_Start_DMA(&hadc3, (uint32_t *)&cisData[CIS_ADC_BUFF_SIZE * 2], CIS_ADC_BUFF_SIZE);

	/* Start Main Timer ######################################*/
	HAL_TIM_PWM_Start(&htim15, TIM_CHANNEL_1);

	printf("adc1 DMA count : %d \n",(int)__HAL_DMA_GET_COUNTER(&hdma_adc1));
	printf("adc2 DMA count : %d \n",(int)__HAL_DMA_GET_COUNTER(&hdma_adc2));
	printf("adc3 DMA count : %d \n",(int)__HAL_DMA_GET_COUNTER(&hdma_adc3));
	printf("CLK  TIM count : %d \n",(int)__HAL_TIM_GET_COUNTER(&htim1));
	printf("SP   TIM count : %d \n",(int)__HAL_TIM_GET_COUNTER(&htim8));
	printf("LEDR TIM count : %d \n",(int)__HAL_TIM_GET_COUNTER(&htim4));
	printf("LEDG TIM count : %d \n",(int)__HAL_TIM_GET_COUNTER(&htim5));
	printf("LEDB TIM count : %d \n",(int)__HAL_TIM_GET_COUNTER(&htim3));
}

/**
 * @brief  CIS stop captures
 * @param  None
 * @retval None
 */
void cis_Stop_capture()
{
	/* Stop Main Timer #######################################*/
	HAL_TIM_PWM_Stop(&htim15, TIM_CHANNEL_1);

	/* Stop ADC Timer #####"##################################*/
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

	cisHalfBufferState[0] = CIS_BUFFER_OFFSET_NONE;
	cisHalfBufferState[1] = CIS_BUFFER_OFFSET_NONE;
	cisHalfBufferState[2] = CIS_BUFFER_OFFSET_NONE;
	cisFullBufferState[0] = CIS_BUFFER_OFFSET_NONE;
	cisFullBufferState[1] = CIS_BUFFER_OFFSET_NONE;
	cisFullBufferState[2] = CIS_BUFFER_OFFSET_NONE;
}

/**
 * @brief  Init Main CIS clock Frequency
 * @param  None
 * @retval None
 */
void cis_TIM_MAIN_Init()
{
	MX_TIM15_Init();
}

/**
 * @brief  Init CIS clock Frequency
 * @param  None
 * @retval None
 */
void cis_TIM_CLK_Init()
{
	MX_TIM1_Init();
}

/**
 * @brief  CIS start pulse timer init
 * @param  None
 * @retval None
 */
void cis_TIM_SP_Init()
{
	MX_TIM8_Init();
}

/**
 * @brief  CIS red led timer init
 * @param  None
 * @retval None
 */
void cis_TIM_LED_B_Init()
{
	MX_TIM3_Init();
}

/**
 * @brief  CIS green led timer init
 * @param  None
 * @retval None
 */
void cis_TIM_LED_R_Init()
{
	MX_TIM4_Init();
}

/**
 * @brief  CIS blue led timer init
 * @param  None
 * @retval None
 */
void cis_TIM_LED_G_Init()
{
	MX_TIM5_Init();
}

/**
 * @brief  CIS leds on
 * @param  None
 * @retval None
 */
void cis_LedsOn()
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
void cis_LedsOff()
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
void cis_LedPowerAdj(int32_t red_pwm, int32_t green_pwm, int32_t blue_pwm)
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
void cis_ADC_Init(void)
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
