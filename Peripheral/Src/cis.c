
/**
 ******************************************************************************
 * @file           : cis.c
 * @brief          :
 ******************************************************************************
 */
/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"
#include "main.h"
#include "config.h"

#include "stdlib.h"
#include "stdio.h"

#include "tim.h"
#include "cis.h"

/* Extern variables ---------------------------------------------------------*/

extern ADC_HandleTypeDef hadc1;
extern DAC_HandleTypeDef hdac1;

/* Private variables ---------------------------------------------------------*/
__IO static int32_t aADCxConvertedDataDMA = 0;
__IO uint32_t cis_adc_data[CIS_PIXELS_NB] = {0};

//__IO static int16_t redData[CIS_PIXELS_NB + 1000] = {0};
//__IO static int16_t greenData[CIS_PIXELS_NB + 1000] = {0};
//__IO static int16_t blueData[CIS_PIXELS_NB + 1000] = {0};

//__IO static int16_t aADCxConvertedDataOffset[CIS_PIXELS_NB] = {0};

//__IO static int16_t redDataOffset[CIS_PIXELS_NB + 1000] = {0};
//__IO static int16_t greenDataOffset[CIS_PIXELS_NB + 1000] = {0};
//__IO static int16_t blueDataOffset[CIS_PIXELS_NB + 1000] = {0};

__IO static uint16_t deadZones[TOTAL_DEADZONE + 1000] = {0};

const uint32_t aEscalator16bit[32] = {0x111, 0x222, 0x333, 0x444, 0x555, 0x666, 0x777, 0x888,
		0x999, 0xAAA, 0xBBB, 0xCCC, 0xDDD, 0xEEE, 0xFFF, 0xEEE,
		0xDDD, 0xCCC, 0xBBB, 0xAAA, 0x999, 0x888, 0x777, 0x666,
		0x555, 0x444, 0x333, 0x222, 0x111};
static __IO uint32_t pixel_cnt = 0;
static __IO uint16_t color_selector = 0;
static __IO uint32_t calib_cnt = 1;

/* Private function prototypes -----------------------------------------------*/

void cisInit(void)
{
	int k = 0;
	for (int i = 0; i < SEGMENT_NB; i++)
	{
		for (int j = 0; j < DEADZONE_WIDTH; j++)
		{
			deadZones[k++] = (i * SEGMENT_WIDTH) + (SEGMENT_WIDTH / 2) - ((DEADZONE_WIDTH / 2) - j);
		}
	}

	/* ### - 4 - Start conversion in DMA mode ################################# */
	if (HAL_ADC_Start_DMA(&hadc1, (uint32_t *)&aADCxConvertedDataDMA , 1) != HAL_OK)
	{
		Error_Handler();
	}
}

/**
 * @brief  End of conversion callback in non blocking mode
 * @param  hadc : hadc handle
 * @retval None
 */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
	if (hadc != &hadc1)
		return;


	if (pixel_cnt < 1)
	{
		HAL_GPIO_WritePin(CIS_SP_GPIO_Port, CIS_SP_Pin, GPIO_PIN_SET);

	}
	if (pixel_cnt == 1)
	{
		HAL_GPIO_WritePin(CIS_SP_GPIO_Port, CIS_SP_Pin, GPIO_PIN_RESET);
	}

	HAL_GPIO_WritePin(CIS_LED_B_GPIO_Port, CIS_LED_B_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(CIS_LED_B_GPIO_Port, CIS_LED_B_Pin, GPIO_PIN_RESET);

	pixel_cnt++;

	if (pixel_cnt > 2700)
	{
		pixel_cnt = 0;
	}

		//	static uint8_t colorIsSet = 0;
		//	static uint16_t deadZone_cnt = 0;
		//
		//	if (!colorIsSet)
		//	{
		//		if (color_selector == RED)
		//		{
		//			HAL_GPIO_WritePin(CIS_LED_B_GPIO_Port, CIS_LED_B_Pin, GPIO_PIN_RESET);
		//			HAL_GPIO_WritePin(CIS_LED_R_GPIO_Port, CIS_LED_R_Pin, GPIO_PIN_SET);
		//			colorIsSet = 1;
		//		}
		//		else if (color_selector == GREEN)
		//		{
		//			HAL_GPIO_WritePin(CIS_LED_R_GPIO_Port, CIS_LED_R_Pin, GPIO_PIN_RESET);
		//			HAL_GPIO_WritePin(CIS_LED_G_GPIO_Port, CIS_LED_G_Pin, GPIO_PIN_SET);
		//			colorIsSet = 1;
		//		}
		//		else if (color_selector == BLUE)
		//		{
		//			HAL_GPIO_WritePin(CIS_LED_G_GPIO_Port, CIS_LED_G_Pin, GPIO_PIN_RESET);
		//			HAL_GPIO_WritePin(CIS_LED_B_GPIO_Port, CIS_LED_B_Pin, GPIO_PIN_SET);
		//			colorIsSet = 1;
		//		}
		//		else if (color_selector == 3)
		//		{
		//			HAL_GPIO_WritePin(CIS_LED_R_GPIO_Port, CIS_LED_R_Pin, GPIO_PIN_RESET);
		//			HAL_GPIO_WritePin(CIS_LED_G_GPIO_Port, CIS_LED_G_Pin, GPIO_PIN_RESET);
		//			HAL_GPIO_WritePin(CIS_LED_B_GPIO_Port, CIS_LED_B_Pin, GPIO_PIN_RESET);
		//			colorIsSet = 1;
		//		}
		//		else if (color_selector == 4)
		//		{
		//			HAL_GPIO_WritePin(CIS_LED_R_GPIO_Port, CIS_LED_R_Pin, GPIO_PIN_SET);
		//			HAL_GPIO_WritePin(CIS_LED_G_GPIO_Port, CIS_LED_G_Pin, GPIO_PIN_SET);
		//			HAL_GPIO_WritePin(CIS_LED_B_GPIO_Port, CIS_LED_B_Pin, GPIO_PIN_SET);
		//			colorIsSet = 1;
		//		}
		//	}
		//
		//	if (pixel_cnt == 0)
		//	{
		//		for (int i = 0; i < 25; i++)
		//		{
		//			HAL_GPIO_WritePin(CIS_SP_GPIO_Port, CIS_SP_Pin, GPIO_PIN_RESET);
		//		}
		//		HAL_GPIO_WritePin(CIS_SP_GPIO_Port, CIS_SP_Pin, GPIO_PIN_SET);
		//	}
		//	else if (pixel_cnt >= PIXEL_CNT_OFFSET)
		//	{
		//		if ((pixel_cnt - PIXEL_CNT_OFFSET) != deadZones[deadZone_cnt])
		//		{
		//			if (calib_cnt < 100)
		//			{
		//				switch (color_selector)
		//				{
		//				case RED :
		//					redDataOffset[pixel_cnt - PIXEL_CNT_OFFSET - deadZone_cnt] += (4096 - (aADCxConvertedDataDMA >> 1));
		//					redDataOffset[pixel_cnt - PIXEL_CNT_OFFSET - deadZone_cnt] /= 2;
		//					break;
		//				case GREEN :
		//					greenDataOffset[pixel_cnt - PIXEL_CNT_OFFSET - deadZone_cnt] += (4096 - (aADCxConvertedDataDMA >> 1));
		//					greenDataOffset[pixel_cnt - PIXEL_CNT_OFFSET - deadZone_cnt] /= 2;
		//					break;
		//				case BLUE :
		//					blueDataOffset[pixel_cnt - PIXEL_CNT_OFFSET - deadZone_cnt] += (4096 - (aADCxConvertedDataDMA >> 1));
		//					blueDataOffset[pixel_cnt - PIXEL_CNT_OFFSET - deadZone_cnt] /= 2;
		//					break;
		//				default :
		//					aADCxConvertedDataOffset[pixel_cnt - PIXEL_CNT_OFFSET - deadZone_cnt] += (4096 - (aADCxConvertedDataDMA >> 1));
		//					aADCxConvertedDataOffset[pixel_cnt - PIXEL_CNT_OFFSET - deadZone_cnt] /= 2;
		//					break;
		//				}
		//			}
		//			else
		//			{
		//				switch (color_selector)
		//				{
		//				case RED :
		//					redData[pixel_cnt - PIXEL_CNT_OFFSET - deadZone_cnt] = ((4096 - (aADCxConvertedDataDMA >> 1)) - redDataOffset[pixel_cnt - PIXEL_CNT_OFFSET - deadZone_cnt]);
		//					if (redData[pixel_cnt - PIXEL_CNT_OFFSET - deadZone_cnt] < 0)
		//						redData[pixel_cnt - PIXEL_CNT_OFFSET - deadZone_cnt] = 0;
		//					break;
		//				case GREEN :
		//					greenData[pixel_cnt - PIXEL_CNT_OFFSET - deadZone_cnt] = ((4096 - (aADCxConvertedDataDMA >> 1)) - greenDataOffset[pixel_cnt - PIXEL_CNT_OFFSET - deadZone_cnt]);
		//					if (greenData[pixel_cnt - PIXEL_CNT_OFFSET - deadZone_cnt] < 0)
		//						greenData[pixel_cnt - PIXEL_CNT_OFFSET - deadZone_cnt] = 0;
		//					break;
		//				case BLUE :
		//					blueData[pixel_cnt - PIXEL_CNT_OFFSET - deadZone_cnt] = ((4096 - (aADCxConvertedDataDMA >> 1)) - blueDataOffset[pixel_cnt - PIXEL_CNT_OFFSET - deadZone_cnt]);
		//					if (blueData[pixel_cnt - PIXEL_CNT_OFFSET - deadZone_cnt] < 0)
		//						blueData[pixel_cnt - PIXEL_CNT_OFFSET - deadZone_cnt] = 0;
		//					break;
		//				default :
		//					aADCxConvertedData[pixel_cnt - PIXEL_CNT_OFFSET - deadZone_cnt] = ((4096 - (aADCxConvertedDataDMA >> 1)) - aADCxConvertedDataOffset[pixel_cnt - PIXEL_CNT_OFFSET - deadZone_cnt]);
		//					if (aADCxConvertedData[pixel_cnt - PIXEL_CNT_OFFSET - deadZone_cnt] < 0)
		//						aADCxConvertedData[pixel_cnt - PIXEL_CNT_OFFSET -deadZone_cnt] = 0;
		//					break;
		//				}
		//#ifndef BLACK_AND_WITHE
		//				aADCxConvertedData[pixel_cnt - PIXEL_CNT_OFFSET - deadZone_cnt] = ((redData[pixel_cnt - PIXEL_CNT_OFFSET - deadZone_cnt]) +
		//						(greenData[pixel_cnt - PIXEL_CNT_OFFSET - deadZone_cnt]) +
		//						(blueData[pixel_cnt - PIXEL_CNT_OFFSET - deadZone_cnt] )) / 3;
		//#endif
		//			}
		//		}
		//		else
		//		{
		//			deadZone_cnt++;
		//		}
		//	}
		//
		//	pixel_cnt++;
		//
		//	if (pixel_cnt > CIS_PIXELS_NB*2)
		//	{
		//		if (calib_cnt < 10000)
		//			calib_cnt++;
		//		pixel_cnt = 0;
		//		deadZone_cnt = 0;
		//
		//#ifdef LED_ON
		//#ifdef BLACK_AND_WITHE
		//		color_selector = 4;
		//#else
		//		color_selector++;
		//		if (color_selector > 2)
		//		{
		//			color_selector = 0;
		//		}
		//#endif
		//#else
		//		color_selector = 3;
		//#endif
		//		colorIsSet = 0;
		//	}
		//	/* Invalidate Data Cache to get the updated content of the SRAM on the second half of the ADC converted data buffer: 32 bytes */
		//	//	SCB_InvalidateDCache_by_Addr((uint32_t *)&aADCxConvertedDataDMA, 1);
		//	//	SCB_CleanDCache();
	}

	/**
	 * @brief  Conversion complete callback in non-blocking mode
	 * @param  hadc: ADC handle
	 * @retval None
	 */
	//void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef* hadc)
	//{
	//	/* Invalidate Data Cache to get the updated content of the SRAM on the first half of the ADC converted data buffer: 32 bytes */
	//	SCB_InvalidateDCache_by_Addr((uint32_t *) &aADCxConvertedDataDMA, 1);
	//}
