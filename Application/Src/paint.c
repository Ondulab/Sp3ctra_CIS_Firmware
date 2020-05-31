/**
  ******************************************************************************
  * @file    Display/LTDC_Paint/Src/paint.c
  * @author  MCD Application Team
  * @brief   Paint program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "paint.h"
#include "color.h"

/** @addtogroup STM32H7xx_HAL_Applications
  * @{
  */

/** @addtogroup LTDC_Paint
  * @{
  */ 

/* Private typedef -----------------------------------------------------------*/
typedef enum {
  APPLICATION_IDLE = 0,  
  APPLICATION_RUNNIG    
}MSC_ApplicationTypeDef;

/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

 DMA2D_HandleTypeDef hdma2d_discovery;

static uint32_t Radius = 2;
uint16_t x = 0, y = 0;
 uint32_t x_size, y_size;
static TS_State_t  TS_State;
TS_Init_t *hTS;

const uint32_t aBMPHeader[14]=         
{0x13A64D42, 0x00000004, 0x00360000, 0x00280000, 0x01A40000, 0x00D40000, 0x00010000, 
 0x00000018, 0xF5400000, 0x00000006, 0x00000000, 0x00000000, 0x00000000, 0x0000};

/* Variable to save the state of USB */
MSC_ApplicationTypeDef Appli_state = APPLICATION_IDLE;

/* Private function prototypes -----------------------------------------------*/
static void Draw_Menu(void);
static void GetPosition(void);
static void Update_Color(void);
static void Update_Size(uint8_t size);

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Paint program
  * @param  None
  * @retval None
  */
int paint(void)
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
  Draw_Menu();  
  
  /* Infinite loop */  
  while (1)
  { 
    /*##-7- Configure the touch screen and Get the position ##################*/    
    GetPosition();

  }
}

/**
  * @brief  Configures and gets Touch screen position.
  * @param  None
  * @retval None
  */
static void GetPosition(void)
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
      Update_Size(Radius);
    }
    else if ((TS_State.TouchDetected) & ( y > color_width ) & (y < (2 * color_width)))
    {
      GUI_SetTextColor(GUI_COLOR_YELLOW);
      Update_Size(Radius);
    }
    else if ((TS_State.TouchDetected) & (y > (2 * color_width)) & (y < (3 * color_width)))
    {
      GUI_SetTextColor(GUI_COLOR_ORANGE);
      Update_Size(Radius);
    }
    else if ((TS_State.TouchDetected) & (y > (3 * color_width)) & (y < (4 * color_width)))
    {
      GUI_SetTextColor(GUI_COLOR_LIGHTMAGENTA);
      Update_Size(Radius);
    }
    else if ((TS_State.TouchDetected) & (y > (4 * color_width)) & (y < (5 * color_width)))
    {
      GUI_SetTextColor(GUI_COLOR_DARKGREEN);
      Update_Size(Radius);
    }
    else if ((TS_State.TouchDetected) & (y > (5 * color_width)) &(y < (6 * color_width)))
    {
      GUI_SetTextColor(GUI_COLOR_GREEN);
      Update_Size(Radius);
    }
    else if ((TS_State.TouchDetected) & (y > (6 * color_width)) &(y < (7 * color_width)))
    {
      GUI_SetTextColor(GUI_COLOR_BROWN);
      Update_Size(Radius);
    }
    else if ((TS_State.TouchDetected) & (y > (7 * color_width)) & (y < (8 * color_width)))
    {
      GUI_SetTextColor(GUI_COLOR_RED);
      Update_Size(Radius);
    }
    else if ((TS_State.TouchDetected) & (y > (8 * color_width)) & (y < (9 * color_width)))
    {
      GUI_SetTextColor(GUI_COLOR_DARKMAGENTA);
      Update_Size(Radius);
    }
    else if ((TS_State.TouchDetected) & (y > (9 * color_width)) & (y < (10 * color_width)))
    {
      GUI_SetTextColor(GUI_COLOR_CYAN);
      Update_Size(Radius);
    }
    else if ((TS_State.TouchDetected) & (y > (10 * color_width)) & (y < (11 * color_width)))
    {
      GUI_SetTextColor(GUI_COLOR_DARKBLUE);
      Update_Size(Radius);
    }
    else if ((TS_State.TouchDetected) & (y > (11 * color_width)) & (y < (12 * color_width)))
    {
      GUI_SetTextColor(GUI_COLOR_BLACK);
      Update_Size(Radius);
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
    Update_Color();    
  }
  else if ((TS_State.TouchDetected) & (x > 70 ) & (y > (12 * color_width)) & (y < (13 * color_width)) & ( x < 120 ))
  {    
    Radius = 20;
    Update_Size(Radius);
  }
  else if ((TS_State.TouchDetected) & (x > 120 ) & (y > (12 * color_width)) & (y < (13 * color_width)) & ( x < 170 ))
  {    
    Radius = 15;
    Update_Size(Radius);
  }
  else if ((TS_State.TouchDetected) & (x > 170 ) & (y > (12 * color_width)) & (y < (13 * color_width)) & ( x < 220 ))
  {    
    Radius = 10;
    Update_Size(Radius);
  }
  else if ((TS_State.TouchDetected) & (x > 220 ) & (y > (12 * color_width)) & (y < (13 * color_width)) & ( x < 270 ))
  {    
    Radius = 5;
    Update_Size(Radius);
  }
  else if ((TS_State.TouchDetected) & (x > 270 ) & (y > (12 * color_width)) & (y < (13 * color_width)) & ( x < 320 ))
  {    
    Radius = 2;
    Update_Size(Radius);
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
static void Draw_Menu(void)
{ 
  /* Set background Layer */
  GUI_SetLayer(0);
  
  /* Clear the LCD */
  GUI_Clear(GUI_COLOR_WHITE);  
  
  /* Draw color image */
  GUI_DrawBitmap(0, 0, (uint8_t *)color);
  
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
static void Update_Color(void)
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
static void Update_Size(uint8_t size)
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

/**
  * @brief  Clock Config.
  * @param  hltdc: LTDC handle
  * @param  Params: Pointer to void
  * @note   This API is called by BSP_LCD_Init()
  *         Being __weak it can be overwritten by the application
  * @retval None
  */
HAL_StatusTypeDef MX_LTDC_ClockConfig(LTDC_HandleTypeDef *hltdc)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hltdc);

  static RCC_PeriphCLKInitTypeDef  periph_clk_init_struct;

  /* RK043FN48H LCD clock configuration */
  /* LCD clock configuration */
  /* PLL3_VCO Input = HSE_VALUE/PLL3M = 5 Mhz */
  /* PLL3_VCO Output = PLL3_VCO Input * PLL3N = 800 Mhz */
  /* PLLLCDCLK = PLL3_VCO Output/PLL3R = 800/83 = 9.63 Mhz */
  /* LTDC clock frequency = PLLLCDCLK = 9.63 Mhz */
  periph_clk_init_struct.PeriphClockSelection = RCC_PERIPHCLK_LTDC;
  periph_clk_init_struct.PLL3.PLL3M = 5;
  periph_clk_init_struct.PLL3.PLL3N = 192;
  periph_clk_init_struct.PLL3.PLL3P = 2;
  periph_clk_init_struct.PLL3.PLL3Q = 20;
  periph_clk_init_struct.PLL3.PLL3R = 99;

  return HAL_RCCEx_PeriphCLKConfig(&periph_clk_init_struct);
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{ 
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
}
#endif

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
