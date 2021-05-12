/*
 * menu.c
 *
 *  Created on: May 31, 2020
 *      Author: zhonx
 */

/* Includes ------------------------------------------------------------------*/
#include <cisynth_eth.h>
#include "stm32h7xx_hal.h"
#include "main.h"
#include "config.h"
#include "basetypes.h"
#include "menu.h"
#include "cisynth_ifft.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

//static uint16_t x = 0, y = 0;
//static uint32_t x_size = 0, y_size = 0;
//static TS_State_t  TS_State;
//TS_Init_t *hTS;

/* Private function prototypes -----------------------------------------------*/
//static void menu_SetHint(void);

/* Private functions ---------------------------------------------------------*/

/**
 * @brief  Menu program
 * @param  None
 * @retval Menu selection
 */
uint32_t menu_StartSelection(void)
{

	/*##-1- LCD Initialization #################################################*/
//	/* Initialize the LCD */
//	BSP_LCD_DeInit(0);
//	BSP_LCD_Init(0, LCD_ORIENTATION_LANDSCAPE);
//	GUI_SetFuncDriver(&LCD_Driver);
//
//	/* Set Foreground Layer */
//	GUI_SetLayer(0);
//
//	BSP_LCD_SetBrightness(0, 50); //Conflit with TIM8 in CIS
//
//	BSP_LCD_DisplayOff(0);
//	BSP_LCD_DisplayOn(0);
//
//	uint8_t coordinates[256] = {0};
//
//	/* Clear the LCD Background layer */
//	GUI_Clear(GUI_COLOR_LIGHTGRAY);
//	BSP_LCD_GetXSize(0, &x_size);
//	BSP_LCD_GetYSize(0, &y_size);
//
//	hTS->Width = x_size;
//	hTS->Height = y_size;
//	hTS->Orientation =TS_SWAP_XY ;
//	hTS->Accuracy = 5;
//	/* Touchscreen initialization */
//	BSP_TS_Init(0, hTS);
//
//	/*##-6- Draw the menu ######################################################*/
//	menu_SetHint();
//
//	TS_State.TouchX = 0;
//	TS_State.TouchY = 0;
//
//	GUI_SetFont(&Font16);
//	GUI_SetTextColor(GUI_COLOR_WHITE);
//	GUI_SetBackColor(GUI_COLOR_DARKGRAY);
//	GUI_FillRect((FT5336_MAX_X_LENGTH / 2) - 120, DISPLAY_AERA3_YPOS, 240, DISPLAY_AERAS_HEIGHT, GUI_COLOR_DARKGRAY);
//	GUI_DisplayStringAt(0, DISPLAY_AERA3_YPOS + 10, (uint8_t *)"Start  iFFT  mode", CENTER_MODE);
//	GUI_FillRect((FT5336_MAX_X_LENGTH / 2) - 120, DISPLAY_AERA4_YPOS, 240, DISPLAY_AERAS_HEIGHT, GUI_COLOR_DARKGRAY);
//	GUI_DisplayStringAt(0, DISPLAY_AERA4_YPOS + 10, (uint8_t *)"Start direct mode", CENTER_MODE);
//
//	while (1)
//	{
//		GUI_SetBackColor(GUI_COLOR_DARKRED);
//		GUI_SetFont(&Font12);
//		sprintf((char *)coordinates, " x = %d y = %d", (int)x, (int) y);
//		GUI_DisplayStringAt(0, 5, (uint8_t*)coordinates, RIGHT_MODE);
//		GUI_SetBackColor(GUI_COLOR_DARKGRAY);
//
//		BSP_TS_GetState(0,&TS_State);
//		/* Read the coordinate */
//		x = TS_State.TouchX;
//		y = TS_State.TouchY;
//		if ((TS_State.TouchDetected) & (x > ((FT5336_MAX_X_LENGTH / 2) - 120)) & (y > DISPLAY_AERA3_YPOS ) & ( x < ((FT5336_MAX_X_LENGTH / 2) + 120) ) & (y < DISPLAY_AERA4_YPOS))
//		{
//			GUI_SetFont(&Font16);
//			GUI_SetTextColor(GUI_COLOR_YELLOW);
//			GUI_SetBackColor(GUI_COLOR_DARKRED);
//			GUI_FillRect((FT5336_MAX_X_LENGTH / 2) - 120, DISPLAY_AERA3_YPOS, 240, DISPLAY_AERAS_HEIGHT, GUI_COLOR_DARKRED);
//			GUI_DisplayStringAt(0, DISPLAY_AERA3_YPOS + 10, (uint8_t *)"Start  iFFT  mode", CENTER_MODE);
//
//			while(TS_State.TouchDetected)
//			{
//				BSP_TS_GetState(0,&TS_State);
//			}
//			HAL_Delay(100);
//			cisynth_ifft();
//
//			GUI_SetFont(&Font16);
//			GUI_SetTextColor(GUI_COLOR_WHITE);
//			GUI_SetBackColor(GUI_COLOR_DARKGRAY);
//			GUI_FillRect((FT5336_MAX_X_LENGTH / 2) - 120, DISPLAY_AERA3_YPOS, 240, DISPLAY_AERAS_HEIGHT, GUI_COLOR_DARKGRAY);
//			GUI_DisplayStringAt(0, DISPLAY_AERA3_YPOS + 10, (uint8_t *)"Start  iFFT  mode", CENTER_MODE);
//		}
//		else if ((TS_State.TouchDetected) & (x > ((FT5336_MAX_X_LENGTH / 2) - 120)) & (y > DISPLAY_AERA4_YPOS ) & ( x < ((FT5336_MAX_X_LENGTH / 2) + 120) ) & (y < DISPLAY_AERA5_YPOS))
//		{
//			GUI_SetFont(&Font16);
//			GUI_SetTextColor(GUI_COLOR_YELLOW);
//			GUI_SetBackColor(GUI_COLOR_DARKRED);
//			GUI_FillRect((FT5336_MAX_X_LENGTH / 2) - 120, DISPLAY_AERA4_YPOS, 240, DISPLAY_AERAS_HEIGHT, GUI_COLOR_DARKRED);
//			GUI_DisplayStringAt(0, DISPLAY_AERA4_YPOS + 10, (uint8_t *)"Start direct mode", CENTER_MODE);
//
//			while(TS_State.TouchDetected)
//			{
//				BSP_TS_GetState(0,&TS_State);
//			}
//			HAL_Delay(100);
//			cisynth_imagePlay();
//
//			GUI_SetFont(&Font16);
//			GUI_SetTextColor(GUI_COLOR_WHITE);
//			GUI_SetBackColor(GUI_COLOR_DARKGRAY);
//			GUI_FillRect((FT5336_MAX_X_LENGTH / 2) - 120, DISPLAY_AERA4_YPOS, 240, DISPLAY_AERAS_HEIGHT, GUI_COLOR_DARKGRAY);
//			GUI_DisplayStringAt(0, DISPLAY_AERA4_YPOS + 10, (uint8_t *)"Start direct mode", CENTER_MODE);
//		}
//	}

	return 0;
}

/**
 * @brief  Display menu hint
 * @param  None
 * @retval None
 */
//static void menu_SetHint(void)
//{
	/* Set menu description */
//	GUI_FillRect(0, 0, FT5336_MAX_X_LENGTH, DISPLAY_HEAD_HEIGHT, GUI_COLOR_DARKRED);
//	GUI_SetTextColor(GUI_COLOR_WHITE);
//	GUI_SetBackColor(GUI_COLOR_DARKRED);
//	GUI_SetFont(&Font20);
//	GUI_DisplayStringAt(0, 2, (uint8_t *)"CISYNTH 3", CENTER_MODE);
//	GUI_SetFont(&Font12);
//	GUI_DisplayStringAt(0, 5, (uint8_t *)"Menu selection", LEFT_MODE);
//}
