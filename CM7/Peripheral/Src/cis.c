/**
 ******************************************************************************
 * @file           : cis_BW_.c
 * @brief          :
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"
#include "main.h"
#include "config.h"
#include "basetypes.h"

#include "stdlib.h"
#include "stdio.h"
#include "stdbool.h"

#include "arm_math.h"

#include "tim.h"
#include "adc.h"

#include "buttons.h"
#include "ssd1362.h"

#include "shared.h"

#include "cis.h"

/* Private includes ----------------------------------------------------------*/

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/
#define CIS_GREEN_HALF_SIZE			((CIS_ADC_BUFF_SIZE / 2) - (CIS_LINE_SIZE + CIS_INACTIVE_AERA_STOP))
#define CIS_GREEN_FULL_SIZE			((CIS_PIXELS_PER_LINE) - (CIS_GREEN_HALF_SIZE))

#define CIS_RED_LINE_OFFSET 		(CIS_START_OFFSET)
#define CIS_GREEN_LINE_OFFSET 		((CIS_LINE_SIZE) + (CIS_START_OFFSET))
#define CIS_BLUE_LINE_OFFSET  		((CIS_LINE_SIZE * 2)) + (CIS_START_OFFSET)

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

static int32_t cisData[CIS_ADC_BUFF_SIZE * 3] = {0};
static int32_t cisDataCpy[CIS_ADC_BUFF_SIZE * 3] = {0};

#ifdef CIS_BW
static int32_t *cisWhiteCalData     = NULL;
static int32_t *cisBlackCalData     = NULL;
static int32_t *cisOffsetCalData    = NULL;
static float32_t *cisGainsCalData 	= NULL;

static int32_t minWhitePix = 0;
static int32_t maxWhitePix = 0;

static const uint32_t blackCalibVirtAddVar = ADDR_FLASH_SECTOR_6_BANK1;
static const uint32_t whiteCalibVirtAddVar = ADDR_FLASH_SECTOR_7_BANK1;
#else

#endif

static volatile CIS_BUFF_StateTypeDef  cisHalfBufferState[3] = {0};
static volatile CIS_BUFF_StateTypeDef  cisFullBufferState[3] = {0};

/* Variable containing ADC conversions data */

/* Private function prototypes -----------------------------------------------*/
static void cis_ImageProcessRGB_Calibration(int32_t *cisCalData, uint16_t iterationNb);
static void cis_TIM_CLK_Init(void);
static void cis_TIM_SP_Init(void);
static void cis_TIM_LED_R_Init(void);
static void cis_TIM_LED_G_Init(void);
static void cis_TIM_LED_B_Init(void);
static void cis_ADC_Init(void);
static void cis_RW_FlashCalibration(CIS_FlashRW_TypeDef RW);
static void cis_ComputeCalsExtremums(struct cisCalsTypes *currCisCals, CIS_Color_TypeDef color);
static void cis_ComputeCalsOffsets(CIS_Color_TypeDef color);
static void cis_ComputeCalsGains(CIS_Color_TypeDef color);
static void cis_StartCalibration(uint16_t iterationNb);

/* Private user code ---------------------------------------------------------*/

/**
 * @brief  CIS init
 * @param  Void
 * @retval None
 */
void cis_Init(void)
{
	printf("----------- CIS INIT ----------\n");
	printf("-------------------------------\n");

	printf("CIS END CAPTURE = %d\n", CIS_LINE_SIZE);

	// Enable 5V power DC/DC for display
	HAL_GPIO_WritePin(EN_5V_GPIO_Port, EN_5V_Pin, GPIO_PIN_SET);

#ifdef CIS_BW
	// Allocate the contiguous memory area for storage cis datas
	cisData = malloc(CIS_ADC_BUFF_SIZE * sizeof(uint32_t));
	if (cisData == NULL)
	{
		Error_Handler();
	}

	// Clear buffers
	memset(cisData, 0, CIS_ADC_BUFF_SIZE * sizeof(uint32_t));

	cisWhiteCalData = malloc(CIS_EFFECTIVE_PIXELS * sizeof(uint32_t));
	if (cisData == NULL)
	{
		Error_Handler();
	}
	cisBlackCalData = malloc(CIS_EFFECTIVE_PIXELS * sizeof(uint32_t));
	if (cisData == NULL)
	{
		Error_Handler();
	}
	cisOffsetCalData = malloc(CIS_EFFECTIVE_PIXELS * sizeof(uint32_t));
	if (cisData == NULL)
	{
		Error_Handler();
	}
	cisGainsCalData = malloc(CIS_EFFECTIVE_PIXELS * sizeof(float32_t));
	if (cisData == NULL)
	{
		Error_Handler();
	}

	// Clear buffers
	memset(cisWhiteCalData, 0, CIS_EFFECTIVE_PIXELS * sizeof(uint32_t));
	memset(cisBlackCalData, 0, CIS_EFFECTIVE_PIXELS * sizeof(uint32_t));
	memset(cisOffsetCalData, 0, CIS_EFFECTIVE_PIXELS * sizeof(uint32_t));
	memset(cisGainsCalData, 0, CIS_EFFECTIVE_PIXELS * sizeof(float32_t));
#else
	memset(&cisData[0], 0, CIS_ADC_BUFF_SIZE * 3 * sizeof(uint32_t));

	// Allocate the contiguous memory area for storage cis datas
	//	cisData = malloc(CIS_ADC_BUFF_SIZE * 3 * sizeof(uint32_t));
	//	if (cisData == NULL)
	//	{
	//		Error_Handler();
	//	}
	//
	//	// Clear buffers
	//	memset(cisData, 0, CIS_ADC_BUFF_SIZE * 3 * sizeof(uint32_t));
#endif

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

	cis_Start_capture();

	//	if (buttonState[SW1] == SWITCH_PRESSED)
	//	{
	//		cis_CalibrationMenu(&cisCals);
	//	}

	cis_StartCalibration(100);

	//	cis_RW_FlashCalibration(CIS_READ_CAL);
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
void cis_ImageProcessRGB(int32_t *cis_buff)
{
	static uint32_t dataOffset_Rx, dataOffset_Gx, dataOffset_Cx, dataOffset_Bx, imageOffset;
	static int32_t line;

	// Read and copy half DMAs buffers
	for (line = CIS_ADC_OUT_LINES; --line >= 0;)
	{
		/* 1st half DMA buffer Data represent Full R region + 1/2 of G region */
		while(cisHalfBufferState[line] != CIS_BUFFER_OFFSET_HALF);

		/* Invalidate Data Cache */
		SCB_InvalidateDCache_by_Addr((uint32_t *)&cisData[CIS_ADC_BUFF_SIZE * line], (CIS_ADC_BUFF_SIZE * sizeof(uint32_t)) / 2);
		arm_copy_q31(&cisData[CIS_ADC_BUFF_SIZE * line],&cisDataCpy[CIS_ADC_BUFF_SIZE * line], CIS_ADC_BUFF_SIZE / 2);
		cisHalfBufferState[line] = CIS_BUFFER_OFFSET_NONE;
	}

	// Read and copy full DMAs buffers
	for (line = CIS_ADC_OUT_LINES; --line >= 0;)
	{
		/* 2nd full DMA buffer Data represent last 1/2 of G region + Full B region */
		while(cisFullBufferState[line] != CIS_BUFFER_OFFSET_FULL);

		/* Invalidate Data Cache */
		SCB_InvalidateDCache_by_Addr((uint32_t *)&cisData[(CIS_ADC_BUFF_SIZE * line) + (CIS_ADC_BUFF_SIZE / 2)], (CIS_ADC_BUFF_SIZE * sizeof(uint32_t)) / 2);
		arm_copy_q31(&cisData[(CIS_ADC_BUFF_SIZE * line) + (CIS_ADC_BUFF_SIZE / 2)],&cisDataCpy[(CIS_ADC_BUFF_SIZE * line) + (CIS_ADC_BUFF_SIZE / 2)], CIS_ADC_BUFF_SIZE / 2);
		cisFullBufferState[line] = CIS_BUFFER_OFFSET_NONE;
	}

#ifdef CIS_DESACTIVATE_CALIBRATION
	for (line = CIS_ADC_OUT_LINES; --line >= 0;)
	{
		dataOffset_Rx = (CIS_ADC_BUFF_SIZE * line) + CIS_RED_LINE_OFFSET;									//Rx
		dataOffset_Gx = (CIS_ADC_BUFF_SIZE * line) + CIS_GREEN_LINE_OFFSET;									//Gx
		dataOffset_Bx = (CIS_ADC_BUFF_SIZE * line) + CIS_BLUE_LINE_OFFSET;									//Bx

		arm_sub_q31(&cisDataCpy[dataOffset_Rx], &cisCals.whiteCal.data[dataOffset_Rx], &cisDataCpy[dataOffset_Rx], CIS_PIXELS_PER_LINE);
		arm_sub_q31(&cisDataCpy[dataOffset_Gx], &cisCals.whiteCal.data[dataOffset_Gx], &cisDataCpy[dataOffset_Gx], CIS_PIXELS_PER_LINE);
		arm_sub_q31(&cisDataCpy[dataOffset_Bx], &cisCals.whiteCal.data[dataOffset_Bx], &cisDataCpy[dataOffset_Bx], CIS_PIXELS_PER_LINE);

		arm_mult_f32((float32_t *)(&cisDataCpy[dataOffset_Rx]), &cisCals.gainsData[dataOffset_Rx], (float32_t *)(&cisDataCpy[dataOffset_Rx]), CIS_PIXELS_PER_LINE);
		arm_mult_f32((float32_t *)(&cisDataCpy[dataOffset_Gx]), &cisCals.gainsData[dataOffset_Gx], (float32_t *)(&cisDataCpy[dataOffset_Gx]), CIS_PIXELS_PER_LINE);
		arm_mult_f32((float32_t *)(&cisDataCpy[dataOffset_Bx]), &cisCals.gainsData[dataOffset_Bx], (float32_t *)(&cisDataCpy[dataOffset_Bx]), CIS_PIXELS_PER_LINE);

		arm_offset_q31(&cisDataCpy[dataOffset_Rx], cisCals.whiteCal.red.maxPix / 2, &cisDataCpy[dataOffset_Rx], CIS_PIXELS_PER_LINE);
		arm_offset_q31(&cisDataCpy[dataOffset_Gx], cisCals.whiteCal.green.maxPix / 2, &cisDataCpy[dataOffset_Gx], CIS_PIXELS_PER_LINE);
		arm_offset_q31(&cisDataCpy[dataOffset_Bx], cisCals.whiteCal.blue.maxPix / 2, &cisDataCpy[dataOffset_Bx], CIS_PIXELS_PER_LINE);
	}
#endif

	for (line = CIS_ADC_OUT_LINES; --line >= 0;)
	{
		imageOffset = (CIS_PIXELS_PER_LINE * line);	//Mx

		dataOffset_Rx = (CIS_ADC_BUFF_SIZE * line) + CIS_RED_LINE_OFFSET;									//Rx
		dataOffset_Gx = (CIS_ADC_BUFF_SIZE * line) + CIS_GREEN_LINE_OFFSET;									//Gx

		arm_shift_q31(&cisDataCpy[dataOffset_Rx], -8, &cisDataCpy[dataOffset_Rx], (CIS_ADC_BUFF_SIZE / 2) - CIS_START_OFFSET); 	//R + Ghalf 8 bit conversion
		arm_shift_q31(&cisDataCpy[dataOffset_Gx], 8, &cisDataCpy[dataOffset_Gx], CIS_GREEN_HALF_SIZE); 	//Ghalf 8 bit shift

		arm_add_q31(&cisDataCpy[dataOffset_Gx], &cisDataCpy[dataOffset_Rx], &cisDataCpy[dataOffset_Rx], CIS_GREEN_HALF_SIZE); //Add Green

		dataOffset_Cx = (CIS_ADC_BUFF_SIZE * line) + (CIS_ADC_BUFF_SIZE / 2);								//Cx
		dataOffset_Bx = (CIS_ADC_BUFF_SIZE * line) + CIS_BLUE_LINE_OFFSET;									//Bx

		arm_shift_q31(&cisDataCpy[dataOffset_Cx], -8, &cisDataCpy[dataOffset_Cx], CIS_ADC_BUFF_SIZE / 2); 	//Gfull + B 8 bit conversion
		arm_shift_q31(&cisDataCpy[dataOffset_Cx], 8, &cisDataCpy[dataOffset_Cx], CIS_GREEN_FULL_SIZE); 		//Gfull 8 bit shift
		arm_shift_q31(&cisDataCpy[dataOffset_Bx], 16, &cisDataCpy[dataOffset_Bx], CIS_PIXELS_PER_LINE); 	//B 16 bit shift

		arm_add_q31(&cisDataCpy[dataOffset_Cx], &cisDataCpy[dataOffset_Bx + CIS_GREEN_HALF_SIZE], &cisDataCpy[dataOffset_Bx + CIS_GREEN_HALF_SIZE], CIS_GREEN_FULL_SIZE); //Add Green

		arm_add_q31(&cisDataCpy[dataOffset_Rx], &cisDataCpy[dataOffset_Bx], &cis_buff[imageOffset], CIS_PIXELS_PER_LINE); //Half + Full Data

		//		arm_fill_q31(0xFF0000 >> (line * 8), &cis_buff[imageOffset], CIS_PIXELS_PER_LINE); //RGB debug
	}
}

/**
 * @brief  cis_ImageProcessRGB_Calibration
 * @param  cis calibration buffer ptr
 * @retval None
 */
void cis_ImageProcessRGB_Calibration(int32_t *cisCalData, uint16_t iterationNb)
{
	static int32_t line, iteration, pix, currState;
	memset(cisCalData, 0, CIS_ADC_BUFF_SIZE * 3 * sizeof(uint32_t));

	for (iteration = iterationNb; --iteration;)
	{
		// Read and copy half DMAs buffers
		for (line = CIS_ADC_OUT_LINES; --line >= 0;)
		{
			/* 1st half DMA buffer Data represent Full R region + 1/2 of G region */
			while(cisHalfBufferState[line] != CIS_BUFFER_OFFSET_HALF);

			/* Invalidate Data Cache */
			SCB_InvalidateDCache_by_Addr((uint32_t *)&cisData[CIS_ADC_BUFF_SIZE * line], (CIS_ADC_BUFF_SIZE * sizeof(uint32_t)) / 2);
			//			arm_copy_q31(&cisData[CIS_ADC_BUFF_SIZE * line], &cisCalData[CIS_ADC_BUFF_SIZE * line], CIS_ADC_BUFF_SIZE / 2);
			arm_add_q31(&cisData[CIS_ADC_BUFF_SIZE * line], &cisCalData[CIS_ADC_BUFF_SIZE * line], &cisCalData[CIS_ADC_BUFF_SIZE * line], CIS_ADC_BUFF_SIZE / 2);
			cisHalfBufferState[line] = CIS_BUFFER_OFFSET_NONE;
		}

		// Read and copy full DMAs buffers
		for (line = CIS_ADC_OUT_LINES; --line >= 0;)
		{
			/* 2nd full DMA buffer Data represent last 1/2 of G region + Full B region */
			while(cisFullBufferState[line] != CIS_BUFFER_OFFSET_FULL);

			/* Invalidate Data Cache */
			SCB_InvalidateDCache_by_Addr((uint32_t *)&cisData[(CIS_ADC_BUFF_SIZE * line) + (CIS_ADC_BUFF_SIZE / 2)], (CIS_ADC_BUFF_SIZE * sizeof(uint32_t)) / 2);
			//			arm_copy_q31(&cisData[(CIS_ADC_BUFF_SIZE * line) + (CIS_ADC_BUFF_SIZE / 2)], &cisCalData[(CIS_ADC_BUFF_SIZE * line) + (CIS_ADC_BUFF_SIZE / 2)], CIS_ADC_BUFF_SIZE / 2);
			arm_add_q31(&cisData[(CIS_ADC_BUFF_SIZE * line) + (CIS_ADC_BUFF_SIZE / 2)], &cisCalData[(CIS_ADC_BUFF_SIZE * line) + (CIS_ADC_BUFF_SIZE / 2)], &cisCalData[(CIS_ADC_BUFF_SIZE * line) + (CIS_ADC_BUFF_SIZE / 2)], CIS_ADC_BUFF_SIZE / 2);			cisFullBufferState[line] = CIS_BUFFER_OFFSET_NONE;
		}
		currState = iteration * 100 / iterationNb;
		ssd1362_progressBar(30, 30, 100 - currState + 1, 0xF);
	}

	for (pix = (CIS_ADC_BUFF_SIZE * 3); --pix >= 0;)
	{
		cisCalData[pix] /= iterationNb;

#ifdef PRINT_CIS_CALIBRATION
		printf("Pix = %d, Val = %d\n", (int)pix, (int)cisCalData[pix]);
#endif
	}
}

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
	__HAL_TIM_SET_COUNTER(&htim4, (CIS_LINE_SIZE * 1) - CIS_LED_RED_ON);	//R
	__HAL_TIM_SET_COUNTER(&htim5, (CIS_LINE_SIZE * 3) - CIS_LED_GREEN_ON);	//G
	__HAL_TIM_SET_COUNTER(&htim3, (CIS_LINE_SIZE * 2) - CIS_LED_BLUE_ON);	//B

	/* Start LEDs ############################################*/
	cis_LedsOn();

	/* Start SP generation ##################################*/
	if(HAL_TIM_PWM_Start(&htim8, TIM_CHANNEL_3) != HAL_OK)
	{
		Error_Handler();
	}

	/* Start DMA #############################################*/
	if (HAL_ADC_Start_DMA(&hadc1, (uint32_t *)&cisData[0], CIS_ADC_BUFF_SIZE) != HAL_OK)
	{
		Error_Handler();
	}

	if (HAL_ADC_Start_DMA(&hadc2, (uint32_t *)&cisData[CIS_ADC_BUFF_SIZE], CIS_ADC_BUFF_SIZE) != HAL_OK)
	{
		Error_Handler();
	}

	if (HAL_ADC_Start_DMA(&hadc3, (uint32_t *)&cisData[CIS_ADC_BUFF_SIZE * 2], CIS_ADC_BUFF_SIZE) != HAL_OK)
	{
		Error_Handler();
	}

	/* Start CLK generation ##################################*/
	if(HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2) != HAL_OK)
	{
		Error_Handler();
	}

	/* Start ADC Timer #######################################*/
	if(HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1) != HAL_OK)
	{
		Error_Handler();
	}
}

/**
 * @brief  CIS stop captures
 * @param  None
 * @retval None
 */
void cis_Stop_capture()
{
	/* Start ADC Timer #######################################*/
	if(HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_1) != HAL_OK)
	{
		Error_Handler();
	}

	/* Start CLK generation ##################################*/
	if(HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_2) != HAL_OK)
	{
		Error_Handler();
	}

	/* Start SP generation ##################################*/
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

	/* Start LEDs ###########################################*/
	cis_LedsOff();

	cisHalfBufferState[0] = CIS_BUFFER_OFFSET_NONE;
	cisHalfBufferState[1] = CIS_BUFFER_OFFSET_NONE;
	cisHalfBufferState[2] = CIS_BUFFER_OFFSET_NONE;
	cisFullBufferState[0] = CIS_BUFFER_OFFSET_NONE;
	cisFullBufferState[1] = CIS_BUFFER_OFFSET_NONE;
	cisFullBufferState[2] = CIS_BUFFER_OFFSET_NONE;
}

/**
 * @brief  Init CIS clock Frequency
 * @param  sampling_frequency
 * @retval None
 */
void cis_TIM_CLK_Init()
{
	MX_TIM1_Init();
}

/**
 * @brief  CIS start pulse timer init
 * @param  Void
 * @retval None
 */
void cis_TIM_SP_Init()
{
	MX_TIM8_Init();
}

/**
 * @brief  CIS red led timer init
 * @param  Void
 * @retval None
 */
void cis_TIM_LED_B_Init()
{
	MX_TIM3_Init();
}

/**
 * @brief  CIS green led timer init
 * @param  Void
 * @retval None
 */
void cis_TIM_LED_R_Init()
{
	MX_TIM4_Init();
}

/**
 * @brief  CIS blue led timer init
 * @param  Void
 * @retval None
 */
void cis_TIM_LED_G_Init()
{
	MX_TIM5_Init();
}

/**
 * @brief  CIS leds off
 * @param  Void
 * @retval None
 */
void cis_LedsOff()
{
	/* Start LED R generation ###############################*/
	if(HAL_TIM_PWM_Stop(&htim4, TIM_CHANNEL_2) != HAL_OK)
	{
		Error_Handler();
	}
	/* Start LED G generation ###############################*/
	if(HAL_TIM_PWM_Stop(&htim5, TIM_CHANNEL_3) != HAL_OK)
	{
		Error_Handler();
	}
	/* Start LED B generation ###############################*/
	if(HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_1) != HAL_OK)
	{
		Error_Handler();
	}
}

/**
 * @brief  CIS leds on
 * @param  Void
 * @retval None
 */
void cis_LedsOn()
{
	/* Start LED R generation ###############################*/
	if(HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_2) != HAL_OK)
	{
		Error_Handler();
	}
	/* Start LED G generation ###############################*/
	if(HAL_TIM_PWM_Start(&htim5, TIM_CHANNEL_3) != HAL_OK)
	{
		Error_Handler();
	}
	/* Start LED B generation ###############################*/
	if(HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1) != HAL_OK)
	{
		Error_Handler();
	}
}

/**
 * @brief  CIS adc init
 * @param  Void
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

/**
 * @brief  Gets the sector of a given address
 * @param  Address Address of the FLASH Memory
 * @retval The sector of a given address
 */
uint32_t GetSector(uint32_t Address)
{
	uint32_t sector = 0;

	if(((Address < ADDR_FLASH_SECTOR_1_BANK1) && (Address >= ADDR_FLASH_SECTOR_0_BANK1)) || \
			((Address < ADDR_FLASH_SECTOR_1_BANK2) && (Address >= ADDR_FLASH_SECTOR_0_BANK2)))
	{
		sector = FLASH_SECTOR_0;
	}
	else if(((Address < ADDR_FLASH_SECTOR_2_BANK1) && (Address >= ADDR_FLASH_SECTOR_1_BANK1)) || \
			((Address < ADDR_FLASH_SECTOR_2_BANK2) && (Address >= ADDR_FLASH_SECTOR_1_BANK2)))
	{
		sector = FLASH_SECTOR_1;
	}
	else if(((Address < ADDR_FLASH_SECTOR_3_BANK1) && (Address >= ADDR_FLASH_SECTOR_2_BANK1)) || \
			((Address < ADDR_FLASH_SECTOR_3_BANK2) && (Address >= ADDR_FLASH_SECTOR_2_BANK2)))
	{
		sector = FLASH_SECTOR_2;
	}
	else if(((Address < ADDR_FLASH_SECTOR_4_BANK1) && (Address >= ADDR_FLASH_SECTOR_3_BANK1)) || \
			((Address < ADDR_FLASH_SECTOR_4_BANK2) && (Address >= ADDR_FLASH_SECTOR_3_BANK2)))
	{
		sector = FLASH_SECTOR_3;
	}
	else if(((Address < ADDR_FLASH_SECTOR_5_BANK1) && (Address >= ADDR_FLASH_SECTOR_4_BANK1)) || \
			((Address < ADDR_FLASH_SECTOR_5_BANK2) && (Address >= ADDR_FLASH_SECTOR_4_BANK2)))
	{
		sector = FLASH_SECTOR_4;
	}
	else if(((Address < ADDR_FLASH_SECTOR_6_BANK1) && (Address >= ADDR_FLASH_SECTOR_5_BANK1)) || \
			((Address < ADDR_FLASH_SECTOR_6_BANK2) && (Address >= ADDR_FLASH_SECTOR_5_BANK2)))
	{
		sector = FLASH_SECTOR_5;
	}
	else if(((Address < ADDR_FLASH_SECTOR_7_BANK1) && (Address >= ADDR_FLASH_SECTOR_6_BANK1)) || \
			((Address < ADDR_FLASH_SECTOR_7_BANK2) && (Address >= ADDR_FLASH_SECTOR_6_BANK2)))
	{
		sector = FLASH_SECTOR_6;
	}
	else if(((Address < ADDR_FLASH_SECTOR_0_BANK2) && (Address >= ADDR_FLASH_SECTOR_7_BANK1)) || \
			((Address < FLASH_END_ADDR) && (Address >= ADDR_FLASH_SECTOR_7_BANK2)))
	{
		sector = FLASH_SECTOR_7;
	}
	else
	{
		sector = FLASH_SECTOR_7;
	}

	return sector;
}

/**
 * @brief  CIS calibration
 * @param  cisData ptr
 * @param  calibration iteration
 * @retval None
 */
void cis_RW_FlashCalibration(CIS_FlashRW_TypeDef RW)
{
	static FLASH_EraseInitTypeDef eraseInitStruct = {0};
	uint32_t firstSector = 0, nbOfSectors = 0, sectorError = 0, address = 0, idx = 0;
	__IO uint32_t memoryProgramStatus = 0;
	__IO uint32_t data32 = 0;

	switch (RW)
	{
	case CIS_READ_CAL :
		address = ADDR_CIS_FLASH_CALIBRATION;
		for(uint32_t idx = 0; idx < (sizeof(cisCals) / sizeof(uint32_t)); idx++)
		{
			*((uint32_t *)&cisCals+idx) = *(uint32_t *)address;
			__DSB();

			address+=4;
		}
		break;
	case CIS_WRITE_CAL :
		/* -1- Unlock the Flash to enable the flash control register access ***************/
		HAL_FLASH_Unlock();
		/* -2- Erase the user Flash area
			    (area defined by FLASH_USER_START_ADDR and FLASH_USER_END_ADDR) ***********/

		/* Get the 1st sector to erase */
		firstSector = GetSector(ADDR_CIS_FLASH_CALIBRATION);
		nbOfSectors = GetSector(FLASH_END_ADDR) - firstSector + 1;

		/* Fill EraseInit structure*/
		eraseInitStruct.TypeErase     = FLASH_TYPEERASE_SECTORS;
		eraseInitStruct.VoltageRange  = FLASH_VOLTAGE_RANGE_4;
		eraseInitStruct.Banks         = FLASH_BANK_1;
		eraseInitStruct.Sector        = firstSector;
		eraseInitStruct.NbSectors     = nbOfSectors;

		if (HAL_FLASHEx_Erase(&eraseInitStruct, &sectorError) != HAL_OK)
		{
			printf("Flash write fail\n");
			Error_Handler();
		}
		/* -3- Program the user Flash area word by word
			    (area defined by FLASH_USER_START_ADDR and FLASH_USER_END_ADDR) ***********/
		address = ADDR_CIS_FLASH_CALIBRATION;

		while (address < (ADDR_CIS_FLASH_CALIBRATION + (sizeof(cisCals) / sizeof(uint32_t))))
		{
			if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD, address, *(uint32_t *)&cisCals+idx) == HAL_OK)
			{
				address = address + 32; //increment for the next Flash word
				idx+=8;
			}
			else
			{
				printf("Flash write fail\n");
				Error_Handler();
			}
		}
		/* -4- Lock the Flash to disable the flash control register access (recommended
			     to protect the FLASH memory against possible unwanted operation) *********/
		HAL_FLASH_Lock();
		/* -5- Check if the programmed data is OK
		      MemoryProgramStatus = 0: data programmed correctly
		      MemoryProgramStatus != 0: number of words not programmed correctly **********/
		address = ADDR_CIS_FLASH_CALIBRATION;

		//		for(idx = 0; idx < (sizeof(cisCals) / sizeof(uint32_t)); idx++)
		//		{
		//			data32 = *(uint32_t*)address;
		//			__DSB();
		//			if(data32 != *(uint32_t *)&cisCals+idx)
		//			{
		//				memoryProgramStatus++;
		//			}
		//			address+=4;
		//		}

		/* -6- Check if there is an issue to program data*/
		if (memoryProgramStatus != 0)
		{
			printf("Flash write fail\n");
			Error_Handler();
		}
		break;
	default :
		Error_Handler();
	}
}

/**
 * @brief  CIS get Min Max and Delta
 * @param  current calibration type : white or black
 * @param  current color calibration
 * @retval None
 */
void cis_ComputeCalsExtremums(struct cisCalsTypes *currCisCals, CIS_Color_TypeDef color)
{
	int32_t tmpMaxpix = 0, tmpMinpix = 0, lineOffset = 0, offset = 0;
	struct cisColorsParams *currColor;

	switch (color)
	{
	case CIS_RED :
		currColor = &currCisCals->red;
		offset = CIS_RED_LINE_OFFSET;
		break;
	case CIS_GREEN :
		currColor = &currCisCals->green;
		offset = CIS_GREEN_LINE_OFFSET;
		break;
	case CIS_BLUE :
		currColor = &currCisCals->blue;
		offset = CIS_BLUE_LINE_OFFSET;
		break;
	default :
		Error_Handler();
		return;
	}

	currColor->maxPix = 0;
	currColor->minPix = 0xFFFF;

	for (int32_t line = CIS_ADC_OUT_LINES; --line >= 0;)
	{
		lineOffset = (CIS_ADC_BUFF_SIZE * line) + offset;
		// Extrat Min Max and delta
		arm_max_q31(&currCisCals->data[lineOffset], CIS_PIXELS_PER_LINE, &tmpMaxpix, NULL);
		arm_min_q31(&currCisCals->data[lineOffset], CIS_PIXELS_PER_LINE, &tmpMinpix, NULL);

		if (tmpMaxpix > currColor->maxPix)
			currColor->maxPix = tmpMaxpix;

		if (tmpMinpix < currColor->minPix)
			currColor->minPix = tmpMinpix;

		currColor->deltaPix = currColor->maxPix - currColor->minPix;
	}

#ifdef PRINT_CIS_CALIBRATION
	printf("Max     Pix = %d\n", (int)currColor->maxPix);
	printf("Min     Pix = %d\n", (int)currColor->minPix);
	printf("Delta   Pix = %d\n", (int)currColor->deltaPix);
	printf("-------------------------------\n");
#endif
}

/**
 * @brief  CIS compute differential offsets
 * @param  current color calibration
 * @retval None
 */
void cis_ComputeCalsOffsets(CIS_Color_TypeDef color)
{
	uint32_t lineOffset = 0, offset = 0;
	struct cisColorsParams *currColor;

	switch (color)
	{
	case CIS_RED :
		currColor = &cisCals.whiteCal.red;
		offset = CIS_RED_LINE_OFFSET;
		break;
	case CIS_GREEN :
		currColor = &cisCals.whiteCal.green;
		offset = CIS_GREEN_LINE_OFFSET;
		break;
	case CIS_BLUE :
		currColor = &cisCals.whiteCal.blue;
		offset = CIS_BLUE_LINE_OFFSET;
		break;
	default :
		Error_Handler();
		return;
	}

	for (int32_t line = CIS_ADC_OUT_LINES; --line >= 0;)
	{
		lineOffset = (CIS_ADC_BUFF_SIZE * line) + offset;

		// Extract differential offsets
		for (int32_t i = CIS_PIXELS_NB; --i >= 0;)
		{
			cisCals.offsetData[lineOffset + i] = currColor->maxPix - cisCals.whiteCal.data[lineOffset + i];
		}
	}
}

/**
 * @brief  CIS compute gains
 * @param  current color calibration
 * @retval None
 */
void cis_ComputeCalsGains(CIS_Color_TypeDef color)
{
	uint32_t lineOffset = 0, offset;
	struct cisColorsParams *currColor;

	switch (color)
	{
	case CIS_RED :
		currColor = &cisCals.whiteCal.red;
		offset = CIS_RED_LINE_OFFSET;
		break;
	case CIS_GREEN :
		currColor = &cisCals.whiteCal.green;
		offset = CIS_GREEN_LINE_OFFSET;
		break;
	case CIS_BLUE :
		currColor = &cisCals.whiteCal.blue;
		offset = CIS_BLUE_LINE_OFFSET;
		break;
	default :
		Error_Handler();
		return;
	}

	for (int32_t line = CIS_ADC_OUT_LINES; --line >= 0;)
	{
		lineOffset = (CIS_ADC_BUFF_SIZE * line) + offset;

		// Extract differential offsets
		for (int32_t i = CIS_PIXELS_NB; --i >= 0;)
		{
			cisCals.gainsData[lineOffset + i] = (float32_t)(65535 - currColor->maxPix) / (float32_t)(cisCals.blackCal.data[lineOffset + i] - cisCals.whiteCal.data[lineOffset + i]);
		}
	}
}

/**
 * @brief  CIS start calibration
 * @param  calibration iteration
 * @retval None
 */
void cis_StartCalibration(uint16_t iterationNb)
{
	//	uint32_t dataOffset_Rx, dataOffset_Gx, dataOffset_Bx;

	//	int32_t tmpCalibrationData[CIS_ADC_BUFF_SIZE * 3] = {0};

	/* Set header description */
	buttonState[SW1] = SWITCH_RELEASED;

	printf("------ START CALIBRATION ------\n");
	/*-------- 1 --------*/
	// Read black and white level
	ssd1362_drawRect(0, DISPLAY_HEAD_Y1POS, DISPLAY_WIDTH, DISPLAY_HEAD_Y2POS, 4, true);
	ssd1362_drawString(10, DISPLAY_HEAD_Y1POS + 1, (int8_t *)"CIS BW QUALIBRATION", 0xF, 8);
	ssd1362_writeFullBuffer();
	HAL_Delay(1000);
	ssd1362_drawRect(0, DISPLAY_AERA1_Y1POS, DISPLAY_WIDTH, DISPLAY_HEIGHT, 0, true);
	ssd1362_drawRect(0, DISPLAY_HEAD_Y1POS, DISPLAY_WIDTH, DISPLAY_HEAD_Y2POS, 4, true);
	ssd1362_drawString(10, DISPLAY_HEAD_Y1POS + 1, (int8_t *)"PLACE CIS ON BLACK SURFACE", 0xF, 8);
	ssd1362_writeFullBuffer();
	HAL_Delay(1000);
	cis_ImageProcessRGB_Calibration(cisCals.blackCal.data, iterationNb);

	// Print curve
	ssd1362_drawRect(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, 0, true);
	ssd1362_drawString(0, 8, (int8_t *)"  R1 G1 B1  R2 G2 B2  R3 G3 B3", 10, 8);
	for (uint32_t i = 1; i < 9; i++)
	{
		ssd1362_drawVLine(DISPLAY_WIDTH / 9 * i, 0, DISPLAY_HEIGHT - DISPLAY_HEAD_Y2POS, 4, false);
	}
	for (uint32_t i = 0; i < (DISPLAY_WIDTH); i++)
	{
		int32_t cis_color = cisCals.blackCal.data[(uint32_t)(i * ((float)(CIS_ADC_BUFF_SIZE * 3) / (float)DISPLAY_WIDTH))] >> 10;
		ssd1362_drawPixel(DISPLAY_WIDTH - 1 - i, DISPLAY_HEIGHT - cis_color - 1, 15, false);
	}
	ssd1362_writeFullBuffer();
	HAL_Delay(2000);

	ssd1362_drawRect(0, DISPLAY_HEAD_Y2POS, DISPLAY_WIDTH, DISPLAY_HEIGHT, 0, true);
	ssd1362_drawRect(0, DISPLAY_HEAD_Y1POS, DISPLAY_WIDTH, DISPLAY_HEAD_Y2POS, 4, true);
	ssd1362_drawString(10, DISPLAY_HEAD_Y1POS + 1, (int8_t *)"PLACE CIS ON WHITE SURFACE", 0xF, 8);
	ssd1362_writeFullBuffer();
	HAL_Delay(1000);
	cis_ImageProcessRGB_Calibration(cisCals.whiteCal.data, iterationNb);

	// Print curve
	ssd1362_drawRect(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, 0, true);
	ssd1362_drawString(0, DISPLAY_HEIGHT - 8, (int8_t *)"  R1 G1 B1  R2 G2 B2  R3 G3 B3", 10, 8);
	for (uint32_t i = 1; i < 9; i++)
	{
		ssd1362_drawVLine(DISPLAY_WIDTH / 9 * i, 0, DISPLAY_HEIGHT - DISPLAY_HEAD_Y2POS, 4, false);
	}
	for (uint32_t i = 0; i < (DISPLAY_WIDTH); i++)
	{
		int32_t cis_color = cisCals.whiteCal.data[(uint32_t)(i * ((float)(CIS_ADC_BUFF_SIZE * 3) / (float)DISPLAY_WIDTH))] >> 10;
		ssd1362_drawPixel(DISPLAY_WIDTH - 1 - i, DISPLAY_HEIGHT - cis_color -1, 15, false);
	}
	ssd1362_writeFullBuffer();
	HAL_Delay(1000);

	printf("------- LOAD CALIBRATION ------\n");
	/*-------- 2 --------*/
	ssd1362_drawRect(0, DISPLAY_HEAD_Y2POS, DISPLAY_WIDTH, DISPLAY_HEIGHT, 0, true);
	ssd1362_drawRect(0, DISPLAY_HEAD_Y1POS, DISPLAY_WIDTH, DISPLAY_HEAD_Y2POS, 4, true);
	ssd1362_drawString(10, DISPLAY_HEAD_Y1POS + 1, (int8_t *)"EXTRACT EXTREMUMS AND DELTAS", 0xF, 8);
	ssd1362_writeFullBuffer();

	// Extrat Min Max and delta
	cis_ComputeCalsExtremums(&cisCals.blackCal, CIS_RED);
	cis_ComputeCalsExtremums(&cisCals.whiteCal, CIS_RED);

	cis_ComputeCalsExtremums(&cisCals.blackCal, CIS_GREEN);
	cis_ComputeCalsExtremums(&cisCals.whiteCal, CIS_GREEN);

	cis_ComputeCalsExtremums(&cisCals.blackCal, CIS_BLUE);
	cis_ComputeCalsExtremums(&cisCals.whiteCal, CIS_BLUE);

	uint8_t textData[256] = {0};
	sprintf((char *)textData, "WH MIN R%d G%d B%d", (int)cisCals.whiteCal.red.minPix, (int)cisCals.whiteCal.green.minPix, (int)cisCals.whiteCal.blue.minPix);
	ssd1362_drawString(0, 10, (int8_t*)textData, 15, 8);
	sprintf((char *)textData, "WH MAX R%d G%d B%d", (int)cisCals.whiteCal.red.maxPix, (int)cisCals.whiteCal.green.maxPix, (int)cisCals.whiteCal.blue.maxPix);
	ssd1362_drawString(0, 19, (int8_t*)textData, 15, 8);
	sprintf((char *)textData, "WH DLT R%d G%d B%d", (int)cisCals.whiteCal.red.deltaPix, (int)cisCals.whiteCal.green.deltaPix, (int)cisCals.whiteCal.blue.deltaPix);
	ssd1362_drawString(0, 28, (int8_t*)textData, 15, 8);

	sprintf((char *)textData, "BL MIN R%d G%d B%d", (int)cisCals.blackCal.red.minPix, (int)cisCals.blackCal.green.minPix, (int)cisCals.blackCal.blue.minPix);
	ssd1362_drawString(0, 37, (int8_t*)textData, 15, 8);
	sprintf((char *)textData, "BL MAX R%d G%d B%d", (int)cisCals.blackCal.red.maxPix, (int)cisCals.blackCal.green.maxPix, (int)cisCals.blackCal.blue.maxPix);
	ssd1362_drawString(0, 46, (int8_t*)textData, 15, 8);
	sprintf((char *)textData, "BL DLT R%d G%d B%d", (int)cisCals.blackCal.red.deltaPix, (int)cisCals.blackCal.green.deltaPix, (int)cisCals.blackCal.blue.deltaPix);
	ssd1362_drawString(0, 55, (int8_t*)textData, 15, 8);

	ssd1362_writeFullBuffer();
	HAL_Delay(1000);

	/*-------- 3 --------*/
	ssd1362_drawRect(0, DISPLAY_HEAD_Y2POS, DISPLAY_WIDTH, DISPLAY_HEIGHT, 0, true);
	ssd1362_drawRect(0, DISPLAY_HEAD_Y1POS, DISPLAY_WIDTH, DISPLAY_HEAD_Y2POS, 4, true);
	ssd1362_drawString(10, DISPLAY_HEAD_Y1POS + 1, (int8_t *)"EXTRACT DIFFERENTIAL OFFSETS", 0xF, 8);
	ssd1362_drawString(0, 18, (int8_t *)"  R1 G1 B1  R2 G2 B2  R3 G3 B3", 10, 8);
	for (uint32_t i = 1; i < 9; i++)
	{
		ssd1362_drawVLine(DISPLAY_WIDTH / 9 * i, 28, DISPLAY_HEIGHT, 4, false);
	}
	ssd1362_writeFullBuffer();

	// Extract differential offsets
	cis_ComputeCalsOffsets(CIS_RED);
	cis_ComputeCalsOffsets(CIS_GREEN);
	cis_ComputeCalsOffsets(CIS_BLUE);

	// Print curve
	for (uint32_t i = 0; i < (DISPLAY_WIDTH); i++)
	{
		uint32_t index = i * ((float)(CIS_ADC_BUFF_SIZE * 3) / (float)DISPLAY_WIDTH);
		int32_t cis_color = cisCals.offsetData[index] >> 11;
		ssd1362_drawPixel(DISPLAY_WIDTH - 1 - i, DISPLAY_HEIGHT - cis_color, 15, false);
	}
	ssd1362_writeFullBuffer();
	HAL_Delay(1000);

	/*-------- 4 --------*/
	ssd1362_drawRect(0, DISPLAY_HEAD_Y1POS, DISPLAY_WIDTH, DISPLAY_HEAD_Y2POS, 4, true);
	ssd1362_drawString(10, DISPLAY_HEAD_Y1POS + 1, (int8_t *)"COMPUTE COMPENSATIION GAINS", 0xF, 8);
	ssd1362_writeFullBuffer();

	// Compute gains
	cis_ComputeCalsGains(CIS_RED);
	cis_ComputeCalsGains(CIS_GREEN);
	cis_ComputeCalsGains(CIS_BLUE);

	printf("-------- COMPUTE GAINS --------\n");
#ifdef PRINT_CIS_CALIBRATION
	for (uint32_t pix = 0; pix < CIS_EFFECTIVE_PIXELS; pix++)
	{
		printf("Pix = %d, Val = %0.3f\n", (int)pix, (float)cisGainsCalData[pix]);
	}
#endif
	HAL_Delay(1000);

	cis_RW_FlashCalibration(CIS_WRITE_CAL);

	printf("-------------------------------\n");
}
