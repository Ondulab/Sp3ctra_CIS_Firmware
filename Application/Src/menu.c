/*
 * menu.c
 *
 *  Created on: May 31, 2020
 *      Author: zhonx
 */

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"
#include "main.h"
#include "config.h"
#include "basetypes.h"
#include "menu.h"
//#include "color.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

static uint32_t Radius = 2;
static uint16_t x = 0, y = 0;
static uint32_t x_size, y_size;
static TS_State_t  TS_State;
TS_Init_t *hTS;

//const uint32_t menu_aBMPHeader[14]=
//{0x13A64D42, 0x00000004, 0x00360000, 0x00280000, 0x01A40000, 0x00D40000, 0x00010000,
// 0x00000018, 0xF5400000, 0x00000006, 0x00000000, 0x00000000, 0x00000000, 0x0000};

/* Private function prototypes -----------------------------------------------*/
static void menu_DrawMenu(void);
static void menu_GetPosition(void);
static void menu_UpdateColor(void);
static void menu_UpdateSize(uint8_t size);

/* Private functions ---------------------------------------------------------*/

/**
 * @brief  Menu program
 * @param  None
 * @retval Menu selection
 */
uint32_t menu_StartSelection(void)
{
	/* Configure LED1 */
	BSP_LED_Init(LED1);

	/*##-1- LCD Initialization #################################################*/
	/* Initialize the LCD */
	BSP_LCD_Init(0, LCD_ORIENTATION_LANDSCAPE);
	GUI_SetFuncDriver(&LCD_Driver);

	/* Set Foreground Layer */
	GUI_SetLayer(0);

	/* Clear the LCD Background layer */
	GUI_Clear(GUI_COLOR_WHITE);
	BSP_LCD_GetXSize(0, &x_size);
	BSP_LCD_GetYSize(0, &y_size);

	hTS->Width = x_size;
	hTS->Height = y_size;
	hTS->Orientation =TS_SWAP_XY ;
	hTS->Accuracy = 5;
	/* Touchscreen initialization */
	BSP_TS_Init(0, hTS);

	/*##-6- Draw the menu ######################################################*/
	menu_DrawMenu();

	/* Infinite loop */
	while (1)
	{
		/*##-7- Configure the touch screen and Get the position ##################*/
		menu_GetPosition();

	}
	return 0;
}

/**
 * @brief  Configures and gets Touch screen position.
 * @param  None
 * @retval None
 */
static void menu_GetPosition(void)
{
	static uint32_t color_width;
	static uint32_t color ;

	if (x_size == 640)
	{
		color_width = 36;
	}
	else
	{
		color_width = 19;
	}

	/* Get Touch screen position */
	BSP_TS_GetState(0,&TS_State);

	/* Read the coordinate */
	x = TS_State.TouchX;
	y = TS_State.TouchY;

	if ((TS_State.TouchDetected) & (x > (67 + Radius)) & (y > (7 + Radius) ) & ( x < (x_size-(7  + Radius )) ) & (y < (y_size-(67 + Radius )) ))
	{
		GUI_FillCircle((x), (y), Radius,GUI_GetTextColor());
	}
	else if ((TS_State.TouchDetected) & (x > 0 ) & ( x < 50 ))
	{
		if ((TS_State.TouchDetected) & ( y > 0 ) & ( y < color_width ))
		{
			GUI_SetTextColor(GUI_COLOR_WHITE);
			menu_UpdateSize(Radius);
		}
		else if ((TS_State.TouchDetected) & ( y > color_width ) & (y < (2 * color_width)))
		{
			GUI_SetTextColor(GUI_COLOR_YELLOW);
			menu_UpdateSize(Radius);
		}
		else if ((TS_State.TouchDetected) & (y > (2 * color_width)) & (y < (3 * color_width)))
		{
			GUI_SetTextColor(GUI_COLOR_ORANGE);
			menu_UpdateSize(Radius);
		}
		else if ((TS_State.TouchDetected) & (y > (3 * color_width)) & (y < (4 * color_width)))
		{
			GUI_SetTextColor(GUI_COLOR_LIGHTMAGENTA);
			menu_UpdateSize(Radius);
		}
		else if ((TS_State.TouchDetected) & (y > (4 * color_width)) & (y < (5 * color_width)))
		{
			GUI_SetTextColor(GUI_COLOR_DARKGREEN);
			menu_UpdateSize(Radius);
		}
		else if ((TS_State.TouchDetected) & (y > (5 * color_width)) &(y < (6 * color_width)))
		{
			GUI_SetTextColor(GUI_COLOR_GREEN);
			menu_UpdateSize(Radius);
		}
		else if ((TS_State.TouchDetected) & (y > (6 * color_width)) &(y < (7 * color_width)))
		{
			GUI_SetTextColor(GUI_COLOR_BROWN);
			menu_UpdateSize(Radius);
		}
		else if ((TS_State.TouchDetected) & (y > (7 * color_width)) & (y < (8 * color_width)))
		{
			GUI_SetTextColor(GUI_COLOR_RED);
			menu_UpdateSize(Radius);
		}
		else if ((TS_State.TouchDetected) & (y > (8 * color_width)) & (y < (9 * color_width)))
		{
			GUI_SetTextColor(GUI_COLOR_DARKMAGENTA);
			menu_UpdateSize(Radius);
		}
		else if ((TS_State.TouchDetected) & (y > (9 * color_width)) & (y < (10 * color_width)))
		{
			GUI_SetTextColor(GUI_COLOR_CYAN);
			menu_UpdateSize(Radius);
		}
		else if ((TS_State.TouchDetected) & (y > (10 * color_width)) & (y < (11 * color_width)))
		{
			GUI_SetTextColor(GUI_COLOR_DARKBLUE);
			menu_UpdateSize(Radius);
		}
		else if ((TS_State.TouchDetected) & (y > (11 * color_width)) & (y < (12 * color_width)))
		{
			GUI_SetTextColor(GUI_COLOR_BLACK);
			menu_UpdateSize(Radius);
		}
		else if ((TS_State.TouchDetected) &  (y > (12 * color_width)) & (y < (13 * color_width)))
		{
			/* Get the current text color */
			color = GUI_GetTextColor();
			GUI_SetTextColor(GUI_COLOR_WHITE);
			/* Clear the working window */
			GUI_FillRect(68, 8, (x_size - 75), (y_size - 75), GUI_COLOR_WHITE);
			GUI_SetTextColor(color);
		}
		else
		{
			x = 0;
			y = 0;
		}
		menu_UpdateColor();
	}
	else if ((TS_State.TouchDetected) & (x > 70 ) & (y > (12 * color_width)) & (y < (13 * color_width)) & ( x < 120 ))
	{
		Radius = 20;
		menu_UpdateSize(Radius);
	}
	else if ((TS_State.TouchDetected) & (x > 120 ) & (y > (12 * color_width)) & (y < (13 * color_width)) & ( x < 170 ))
	{
		Radius = 15;
		menu_UpdateSize(Radius);
	}
	else if ((TS_State.TouchDetected) & (x > 170 ) & (y > (12 * color_width)) & (y < (13 * color_width)) & ( x < 220 ))
	{
		Radius = 10;
		menu_UpdateSize(Radius);
	}
	else if ((TS_State.TouchDetected) & (x > 220 ) & (y > (12 * color_width)) & (y < (13 * color_width)) & ( x < 270 ))
	{
		Radius = 5;
		menu_UpdateSize(Radius);
	}
	else if ((TS_State.TouchDetected) & (x > 270 ) & (y > (12 * color_width)) & (y < (13 * color_width)) & ( x < 320 ))
	{
		Radius = 2;
		menu_UpdateSize(Radius);
	}
	else if ((TS_State.TouchDetected) & (((x > (x_size-5) ) & (y > (12 * color_width)) & (y < (13 * color_width))) | (( x < 55 ) & ( y < 5 ))))
	{
		TS_State.TouchX = 0;
		TS_State.TouchY = 0;
	}
}

/**
 * @brief  Draws the menu.
 * @param  None
 * @retval None
 */
static void menu_DrawMenu(void)
{
	/* Set background Layer */
	GUI_SetLayer(0);

	/* Clear the LCD */
	GUI_Clear(GUI_COLOR_WHITE);

	/* Draw color image */
	//  GUI_DrawBitmap(0, 0, (uint8_t *)color);

	/* Set Black as text color */
	GUI_SetTextColor(GUI_COLOR_BLACK);

	/* Draw working window */
	GUI_DrawRect(61, 0, (x_size - 61), (y_size - 60),GUI_COLOR_BLACK);
	GUI_DrawRect(63, 3, (x_size - 66), (y_size - 66),GUI_COLOR_BLACK);
	GUI_DrawRect(65, 5, (x_size - 70), (y_size - 70),GUI_COLOR_BLACK);
	GUI_DrawRect(67, 7, (x_size - 74), (y_size - 74),GUI_COLOR_BLACK);

	/* Draw size icons */
	GUI_FillRect(60, (y_size - 48), 250, 48,GUI_COLOR_BLACK);
	GUI_SetTextColor(GUI_COLOR_WHITE);
	GUI_FillCircle(95, (y_size - 24), 20,GUI_COLOR_WHITE);
	GUI_FillCircle(145, (y_size - 24), 15,GUI_COLOR_WHITE);
	GUI_FillCircle(195, (y_size - 24), 10,GUI_COLOR_WHITE);
	GUI_FillCircle(245, (y_size - 24), 5,GUI_COLOR_WHITE);
	GUI_FillCircle(295, (y_size - 24), 2,GUI_COLOR_WHITE);

	GUI_SetTextColor(GUI_COLOR_DARKRED);
	GUI_SetFont(&Font8);
	GUI_DisplayStringAt(360, (y_size - 55), (uint8_t *)"Selected Color  Size", LEFT_MODE);
	GUI_SetTextColor(GUI_COLOR_BLACK);
	GUI_FillRect(380, (y_size - 40), 30, 30, GUI_COLOR_BLACK);
	GUI_FillCircle(450, (y_size- 24), Radius, GUI_COLOR_BLACK);
}

/**
 * @brief  Update the selected color
 * @param  None
 * @retval None
 */
static void menu_UpdateColor(void)
{
	static uint32_t color;

	/* Get the current text color */
	color = GUI_GetTextColor();
	/* Update the selected color icon */
	GUI_FillRect(380, (y_size-40), 30, 30, color);
	GUI_SetTextColor(GUI_COLOR_BLACK);
	GUI_DrawRect(380, (y_size-40), 30, 30, color);
	GUI_SetTextColor(color);
}

/**
 * @brief  Updates the selected size
 * @param  size: Size to be updated
 * @retval None
 */
static void menu_UpdateSize(uint8_t size)
{
	static uint32_t color;

	/* Get the current text color */
	color = GUI_GetTextColor();
	/* Update the selected size icon */
	GUI_SetTextColor(GUI_COLOR_WHITE);
	GUI_FillCircle(450, (y_size-24), 20, GUI_COLOR_WHITE);
	GUI_SetTextColor(color);
	GUI_FillCircle(450, (y_size-24), size, color );
	GUI_SetTextColor(GUI_COLOR_BLACK);
	GUI_DrawCircle(450, (y_size-24), size, GUI_COLOR_BLACK);
	GUI_SetTextColor(color);
}
