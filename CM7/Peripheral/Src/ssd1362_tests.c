#include <string.h>
#include <stdio.h>
#include "ssd1362.h"
#include "ssd1362_tests.h"

void ssd1362_TestBorder() {
    ssd1362_Fill(Black);
   
    uint32_t start = HAL_GetTick();
    uint32_t end = start;
    uint8_t x = 0;
    uint8_t y = 0;
    do {
        ssd1362_DrawPixel(x, y, Black);

        if((y == 0) && (x < 127))
            x++;
        else if((x == 127) && (y < (SSD1362_HEIGHT-1)))
            y++;
        else if((y == (SSD1362_HEIGHT-1)) && (x > 0))
            x--;
        else
            y--;

        ssd1362_DrawPixel(x, y, White);
        ssd1362_UpdateScreen();
    
        HAL_Delay(5);
        end = HAL_GetTick();
    } while((end - start) < 8000);
   
    HAL_Delay(1000);
}

void ssd1362_TestFonts() {
    ssd1362_Fill(Black);
    ssd1362_SetCursor(2, 0);
    ssd1362_WriteString("Font 16x26", Font_16x26, White);
    ssd1362_SetCursor(2, 26);
    ssd1362_WriteString("Font 11x18", Font_11x18, White);
    ssd1362_SetCursor(2, 26+18);
    ssd1362_WriteString("Font 7x10", Font_7x10, White);
    ssd1362_SetCursor(2, 26+18+10);
    ssd1362_WriteString("Font 6x8", Font_6x8, White);
    ssd1362_UpdateScreen();
}

void ssd1362_TestFPS() {
    ssd1362_Fill(White);
   
    uint32_t start = HAL_GetTick();
    uint32_t end = start;
    int fps = 0;
    char message[] = "ABCDEFGHIJK";
   
    ssd1362_SetCursor(2,0);
    ssd1362_WriteString("Testing...", Font_11x18, Black);
   
    do {
        ssd1362_SetCursor(2, 18);
        ssd1362_WriteString(message, Font_11x18, Black);
        ssd1362_UpdateScreen();
       
        char ch = message[0];
        memmove(message, message+1, sizeof(message)-2);
        message[sizeof(message)-2] = ch;

        fps++;
        end = HAL_GetTick();
    } while((end - start) < 5000);
   
    HAL_Delay(1000);

    char buff[64];
    fps = (float)fps / ((end - start) / 1000.0);
    snprintf(buff, sizeof(buff), "~%d FPS", fps);
   
    ssd1362_Fill(White);
    ssd1362_SetCursor(2, 2);
    ssd1362_WriteString(buff, Font_11x18, Black);
    ssd1362_UpdateScreen();
}

void ssd1362_TestLine() {

  ssd1362_Line(1,1,SSD1362_WIDTH - 1,SSD1362_HEIGHT - 1,White);
  ssd1362_Line(SSD1362_WIDTH - 1,1,1,SSD1362_HEIGHT - 1,White);
  ssd1362_UpdateScreen();
  return;
}

void ssd1362_TestRectangle() {
  uint32_t delta;

  for(delta = 0; delta < 5; delta ++) {
    ssd1362_DrawRectangle(1 + (5*delta),1 + (5*delta) ,SSD1362_WIDTH-1 - (5*delta),SSD1362_HEIGHT-1 - (5*delta),White);
  }
  ssd1362_UpdateScreen();
  return;
}

void ssd1362_TestCircle() {
  uint32_t delta;

  for(delta = 0; delta < 5; delta ++) {
    ssd1362_DrawCircle(20* delta+30, 15, 10, White);
  }
  ssd1362_UpdateScreen();
  return;
}

void ssd1362_TestArc() {
  ssd1362_DrawArc(30, 30, 30, 20, 270, White);
  ssd1362_UpdateScreen();
  return;
}

void ssd1362_TestPolyline() {
  SSD1362_VERTEX loc_vertex[] =
  {
      {35,40},
      {40,20},
      {45,28},
      {50,10},
      {45,16},
      {50,10},
      {53,16}
  };

  ssd1362_Polyline(loc_vertex,sizeof(loc_vertex)/sizeof(loc_vertex[0]),White);
  ssd1362_UpdateScreen();
  return;
}

void ssd1362_TestAll() {
    ssd1362_Init();
    ssd1362_TestFPS();
    HAL_Delay(3000);
    ssd1362_TestBorder();
    ssd1362_TestFonts();
    HAL_Delay(3000);
    ssd1362_Fill(Black);
    ssd1362_TestRectangle();
    ssd1362_TestLine();
    HAL_Delay(3000);
    ssd1362_Fill(Black);
    ssd1362_TestPolyline();
    HAL_Delay(3000);
    ssd1362_Fill(Black);
    ssd1362_TestArc();
    HAL_Delay(3000);
    ssd1362_Fill(Black);
    ssd1362_TestCircle();
    HAL_Delay(3000);
}

