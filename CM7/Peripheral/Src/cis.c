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
#include "tim.h"
#include "adc.h"
#include "arm_math.h"

#include "stdlib.h"
#include "stdio.h"
#include "stdbool.h"

#include "synth.h"
#include "cis.h"

/* Private includes ----------------------------------------------------------*/

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
#ifdef CIS_BW
/* Definition of ADCx conversions data table size this buffer contains BW conversion */
static uint16_t *cisData = NULL;
//static uint16_t *cisBuffSommation = NULL;

#else
/* Definition of ADCx conversions data table size this buffer contains RGB conversion */
#define ADC_CONVERTED_DATA_BUFFER_SIZE (CIS_END_CAPTURE * 3) /* Size of array cisData[] */
ALIGN_32BYTES (static uint8_t cisData[ADC_CONVERTED_DATA_BUFFER_SIZE]);
#endif

static uint16_t CIS_EFFECTIVE_PIXELS	 		= 	CIS_ACTIVE_PIXELS_PER_LINE / CIS_IFFT_OVERSAMPLING_RATIO * CIS_ADC_OUT_LINES;
static uint16_t CIS_EFFECTIVE_PIXELS_PER_LINE	= 	CIS_ACTIVE_PIXELS_PER_LINE / CIS_IFFT_OVERSAMPLING_RATIO;
static uint16_t CIS_ADC_BUFF_START_OFFSET	 	= 	CIS_PIXEL_AERA_START / CIS_IFFT_OVERSAMPLING_RATIO;
static uint16_t CIS_ADC_BUFF_STOP_OFFSET	 	= 	CIS_PIXEL_AERA_STOP / CIS_IFFT_OVERSAMPLING_RATIO;
static uint16_t CIS_ADC_BUFF_END_CAPTURE 		= 	CIS_END_CAPTURE / CIS_IFFT_OVERSAMPLING_RATIO;
static uint16_t CIS_ADC_BUFF_SIZE 	 	 		= 	(CIS_END_CAPTURE * CIS_ADC_OUT_LINES) / CIS_IFFT_OVERSAMPLING_RATIO;

CIS_BUFF_StateTypeDef  cisBufferState[3] = {0};

/* Variable containing ADC conversions data */

/* Private function prototypes -----------------------------------------------*/
void cis_TIM_CLK_Init(void);
void cis_TIM_SP_Init(void);
void cis_TIM_LED_R_Init(void);
void cis_TIM_LED_G_Init(void);
void cis_TIM_LED_B_Init(void);

void cis_ADC_Init(synthModeTypeDef mode);
void cis_DisplayLine(void);
void cis_ImageFilterBW(uint16_t *cis_buff);

/* Private user code ---------------------------------------------------------*/

/**
 * @brief  CIS init
 * @param  Void
 * @retval None
 */
void cis_Init(synthModeTypeDef mode)
{
	if (mode == IFFT_MODE)
	{
		CIS_EFFECTIVE_PIXELS_PER_LINE	=	CIS_ACTIVE_PIXELS_PER_LINE / CIS_IFFT_OVERSAMPLING_RATIO;	//5530 / CIS_OVERSAMPLING_RATIO active pixels
		CIS_ADC_BUFF_START_OFFSET		=	CIS_PIXEL_AERA_START / (CIS_IFFT_OVERSAMPLING_RATIO);
		CIS_ADC_BUFF_STOP_OFFSET		=	CIS_PIXEL_AERA_STOP / (CIS_IFFT_OVERSAMPLING_RATIO);
		CIS_ADC_BUFF_END_CAPTURE 		=	CIS_END_CAPTURE / (CIS_IFFT_OVERSAMPLING_RATIO);
	}
	else
	{
		CIS_EFFECTIVE_PIXELS_PER_LINE	=	CIS_ACTIVE_PIXELS_PER_LINE / CIS_IMGPLY_OVERSAMPLING_RATIO;	//5530 / CIS_OVERSAMPLING_RATIO active pixels
		CIS_ADC_BUFF_START_OFFSET		=	CIS_PIXEL_AERA_START / CIS_IMGPLY_OVERSAMPLING_RATIO;
		CIS_ADC_BUFF_STOP_OFFSET		=	CIS_PIXEL_AERA_STOP / CIS_IMGPLY_OVERSAMPLING_RATIO;
		CIS_ADC_BUFF_END_CAPTURE 		=	CIS_END_CAPTURE / CIS_IMGPLY_OVERSAMPLING_RATIO;
	}

	//allocate the contiguous memory area for storage cis data
	cisData = malloc(CIS_ADC_BUFF_SIZE * sizeof(uint16_t*));
	if (cisData == NULL)
	{
		Error_Handler();
	}

	memset(cisData, 0, CIS_ADC_BUFF_SIZE * sizeof(uint16_t*)); //clear image

	HAL_GPIO_WritePin(CIS_RS_GPIO_Port, CIS_RS_Pin, GPIO_PIN_SET); //SET : 200DPI   RESET : 400DPI

	cis_ADC_Init(mode);
	cis_TIM_SP_Init();
	cis_TIM_LED_R_Init();
	cis_TIM_LED_G_Init();
	cis_TIM_LED_B_Init();
	cis_TIM_CLK_Init();
	if (HAL_ADC_Start_DMA(&hadc1, (uint32_t *)cisData, (CIS_ADC_BUFF_END_CAPTURE)) != HAL_OK)
	{
		Error_Handler();
	}

	if (HAL_ADC_Start_DMA(&hadc2, (uint32_t *)&cisData[CIS_ADC_BUFF_END_CAPTURE], (CIS_ADC_BUFF_END_CAPTURE)) != HAL_OK)
	{
		Error_Handler();
	}

	if (HAL_ADC_Start_DMA(&hadc3, (uint32_t *)&cisData[CIS_ADC_BUFF_END_CAPTURE * 2], (CIS_ADC_BUFF_END_CAPTURE)) != HAL_OK)
	{
		Error_Handler();
	}

	//Reset CLK counter
	__HAL_TIM_SET_COUNTER(&htim1, 0);

	//Reset SP counter
	__HAL_TIM_SET_COUNTER(&htim8, 0);

#ifdef CIS_BW
	//Set BW phase shift
	__HAL_TIM_SET_COUNTER(&htim5, (CIS_END_CAPTURE) - CIS_LED_ON);			//B
	__HAL_TIM_SET_COUNTER(&htim4, (CIS_END_CAPTURE) - CIS_LED_ON);			//G
	__HAL_TIM_SET_COUNTER(&htim3, (CIS_END_CAPTURE) - CIS_LED_ON);			//R
#else
	//Set RGB phase shift
	__HAL_TIM_SET_COUNTER(&htim5, (CIS_END_CAPTURE * 2) - CIS_LED_ON);		//B
	__HAL_TIM_SET_COUNTER(&htim4, (CIS_END_CAPTURE * 3) - CIS_LED_ON);		//G
	__HAL_TIM_SET_COUNTER(&htim3, (CIS_END_CAPTURE) - CIS_LED_ON);			//R
#endif
}

/**
 * @brief  GetEffectivePixelNb
 * @param  Void
 * @retval Nuber of effective pixels
 */
__inline uint16_t cis_GetEffectivePixelNb(void)
{
	return CIS_EFFECTIVE_PIXELS;
}

/**
 * @brief  CIS test
 * @param  Void
 * @retval None
 */
void cis_Test(void)
{
	//	uint32_t j = 0;
	//	uint32_t x_size = 0;
	//	uint32_t y_size = 0;
	//	uint32_t color = 0;
	//
	//	//	BSP_LCD_GetXSize(0, &x_size);
	//	//	BSP_LCD_GetYSize(0, &y_size);
	//
	//	while(1)
	//	{
	//		/* 1st half buffer played; so fill it and continue playing from bottom*/
	//		if(cisBufferState[] == CIS_BUFFER_OFFSET_HALF)
	//		{
	//			cisBufferState[] = CIS_BUFFER_OFFSET_NONE;
	//			/* Invalidate Data Cache to get the updated content of the SRAM on the first half of the ADC converted data buffer */
	//			SCB_InvalidateDCache_by_Addr((uint32_t *) &cisData[0], ADC_CONVERTED_DATA_BUFFER_SIZE/2);
	//		}
	//
	//		/* 2nd half buffer played; so fill it and continue playing from top */
	//		if(cisBufferState[] == CIS_BUFFER_OFFSET_FULL)
	//		{
	//			cisBufferState[] = CIS_BUFFER_OFFSET_NONE;
	//			/* Invalidate Data Cache to get the updated content of the SRAM on the second half of the ADC converted data buffer */
	//			SCB_InvalidateDCache_by_Addr((uint32_t *) &cisData[ADC_CONVERTED_DATA_BUFFER_SIZE/2], ADC_CONVERTED_DATA_BUFFER_SIZE/2);
	//			for (uint32_t i = 0; i < x_size; i++)
	//			{
	//#ifdef CIS_BW
	//				color = color >> 8;
	//				color |= 0xFF000000;
	//				color |= color << 8;
	//				color |= color << 16;
	//				//				GUI_SetPixel(i, j + 24, color);
	//#else
	//				color = 0xFF000000;
	//				color |= cis_GetBuffData((i * (CIS_EFFECTIVE_PIXELS_NB/x_size)) + CIS_ADC_BUFF_PIXEL_AERA_START) << 16;
	//				color |= cis_GetBuffData((i * (CIS_EFFECTIVE_PIXELS_NB/x_size)) + CIS_ADC_BUFF_END_CAPTURE + CIS_ADC_BUFF_PIXEL_AERA_START) << 8;
	//				color |= cis_GetBuffData((i * (CIS_EFFECTIVE_PIXELS_NB/x_size)) + (CIS_ADC_BUFF_END_CAPTURE * 2) + CIS_ADC_BUFF_PIXEL_AERA_START);
	//				GUI_SetPixel(i, j + 24, color);
	//#endif
	//			}
	//			j++;
	//			if (j >= (y_size - 24))
	//			{
	//				j = 0;
	//			}
	//		}
	//	}
}

/**
 * @brief  Return buffer data
 * @param  index
 * @retval value
 */
uint16_t cis_GetBuffData(uint32_t index)
{
	//	if (index >= ADC_CONVERTED_DATA_BUFFER_SIZE)
	//		Error_Handler();
	if (index < CIS_EFFECTIVE_PIXELS_PER_LINE)
		return cisData[index + CIS_ADC_BUFF_START_OFFSET];
//	if (index < CIS_EFFECTIVE_PIXELS_PER_LINE * 2)
//		return cisData[index + CIS_ADC_BUFF_START_OFFSET + CIS_EFFECTIVE_PIXELS_PER_LINE];
//	if (index < CIS_EFFECTIVE_PIXELS_PER_LINE * 3)
//		return cisData[index + CIS_ADC_BUFF_START_OFFSET + (CIS_EFFECTIVE_PIXELS_PER_LINE * 2)];
	return 0;
}

/**
 * @brief  Manages Image process.
 * @param  None
 * @retval Image error
 */
void cis_ImageProcessBW(uint16_t *cis_buff)
{
//	for (int32_t line = (CIS_ADC_OUT_LINES); --line >= 0;)
//	{
//		uint32_t offset = (CIS_ADC_BUFF_END_CAPTURE * line) / 2;
//
//		/* 1st half buffer played; so fill it and continue playing from bottom*/
//		if(cisBufferState[line] == CIS_BUFFER_OFFSET_HALF)
//		{
//			cisBufferState[line] = CIS_BUFFER_OFFSET_NONE;
//			/* Invalidate Data Cache to get the updated content of the SRAM on the first half of the ADC converted data buffer */
//			SCB_InvalidateDCache_by_Addr((uint32_t *) &cisData[offset] , CIS_ADC_BUFF_END_CAPTURE);
//			arm_copy_q15((int16_t*)&cisData[offset + CIS_ADC_BUFF_START_OFFSET], (int16_t*)cis_buff, CIS_EFFECTIVE_PIXELS_PER_LINE / 2);
//
//			cis_ImageFilterBW(cis_buff);
//		}
//
//		/* 2nd half buffer played; so fill it and continue playing from top */
//		if(cisBufferState[line] == CIS_BUFFER_OFFSET_FULL)
//		{
//			cisBufferState[line] = CIS_BUFFER_OFFSET_NONE;
//			/* Invalidate Data Cache to get the updated content of the SRAM on the second half of the ADC converted data buffer */
//			SCB_InvalidateDCache_by_Addr((uint32_t *) &cisData[offset + (CIS_ADC_BUFF_END_CAPTURE / 2)], CIS_ADC_BUFF_END_CAPTURE);
//			arm_copy_q15((int16_t*)&cisData[offset + CIS_ADC_BUFF_START_OFFSET + (CIS_ADC_BUFF_END_CAPTURE / 2)], (int16_t*)cis_buff, CIS_EFFECTIVE_PIXELS_PER_LINE / 2);
//
//			cis_ImageFilterBW(cis_buff);
//		}
//	}
}

/**
 * @brief  Image filtering
 * @param  Audio buffer
 * @retval None
 */
void cis_ImageFilterBW(uint16_t *cis_buff)
{
	for (uint32_t i = 0; i < CIS_EFFECTIVE_PIXELS; i++)
	{
#ifdef CIS_INVERT_COLOR
		cis_buff[i] = (double)(65535 - cis_buff[i]) * (pow(10.00, ((double)(65535 - cis_buff[i]) / 65535.00)) / 10.00); //sensibility filer generate some glitchs

#else
		cis_buff[i] = (double)(cis_buff[i]) * (pow(10.00, ((double)(cis_buff[i]) / 65535.00)) / 10.00);
#endif
	}
}

/**
 * @brief  Init CIS clock Frequency
 * @param  sampling_frequency
 * @retval None
 */
void cis_TIM_CLK_Init()
{
	MX_TIM1_Init();

	/* Start ADC Timer #######################################*/
	if(HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1) != HAL_OK)
	{
		Error_Handler();
	}

	/* Start CLK generation ##################################*/
	if(HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2) != HAL_OK)
	{
		Error_Handler();
	}
}

/**
 * @brief  CIS start pulse timer init
 * @param  Void
 * @retval None
 */
void cis_TIM_SP_Init()
{
	MX_TIM8_Init();

	/* Start SP generation ##################################*/
	if(HAL_TIM_PWM_Start(&htim8, TIM_CHANNEL_3) != HAL_OK)
	{
		Error_Handler();
	}
}

/**
 * @brief  CIS red led timer init
 * @param  Void
 * @retval None
 */
void cis_TIM_LED_B_Init()
{
	MX_TIM3_Init();

	/* Start LED R generation ###############################*/
	if(HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1) != HAL_OK)
	{
		Error_Handler();
	}
}

/**
 * @brief  CIS green led timer init
 * @param  Void
 * @retval None
 */
void cis_TIM_LED_R_Init()
{
	MX_TIM4_Init();

	/* Start LED G generation ###############################*/
	if(HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_2) != HAL_OK)
	{
		Error_Handler();
	}
}

/**
 * @brief  CIS blue led timer init
 * @param  Void
 * @retval None
 */
void cis_TIM_LED_G_Init()
{
	MX_TIM5_Init();

	/* Start LED B generation ###############################*/
	if(HAL_TIM_PWM_Start(&htim5, TIM_CHANNEL_3) != HAL_OK)
	{
		Error_Handler();
	}
}

/**
 * @brief  CIS adc init
 * @param  Void
 * @retval None
 */
void cis_ADC_Init(synthModeTypeDef mode)
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
		cisBufferState[0] = CIS_BUFFER_OFFSET_HALF;
		return;
	}
//	if (hadc->Instance == ADC2)
//	{
//		cisBufferState[1] = CIS_BUFFER_OFFSET_HALF;
//		return;
//	}
//	if (hadc->Instance == ADC3)
//	{
//		cisBufferState[2] = CIS_BUFFER_OFFSET_HALF;
//	}
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
		cisBufferState[0] = CIS_BUFFER_OFFSET_FULL;
		return;
	}
//	if (hadc->Instance == ADC2)
//	{
//		cisBufferState[1] = CIS_BUFFER_OFFSET_FULL;
//		return;
//	}
//	if (hadc->Instance == ADC3)
//	{
//		cisBufferState[2] = CIS_BUFFER_OFFSET_FULL;
//	}
}



