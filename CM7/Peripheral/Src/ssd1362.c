#include "ssd1362.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>  // For memcpy
#include "main.h"

#if defined(SSD1362_USE_I2C)

void ssd1362_Reset(void) {
    /* for I2C - do nothing */
}

// Send a byte to the command register
void ssd1362_WriteCommand(uint8_t byte) {
    HAL_I2C_Mem_Write(&SSD1362_I2C_PORT, SSD1362_I2C_ADDR, 0x00, 1, &byte, 1, HAL_MAX_DELAY);
}

// Send data
void ssd1362_WriteData(uint8_t* buffer, size_t buff_size) {
    HAL_I2C_Mem_Write(&SSD1362_I2C_PORT, SSD1362_I2C_ADDR, 0x40, 1, buffer, buff_size, HAL_MAX_DELAY);
}

#elif defined(SSD1362_USE_SPI)

void ssd1362_Reset(void) {
    // CS = High (not selected)
    HAL_GPIO_WritePin(SSD1362_CS_Port, SSD1362_CS_Pin, GPIO_PIN_SET);

    // Reset the OLED
    HAL_GPIO_WritePin(SSD1362_Reset_Port, SSD1362_Reset_Pin, GPIO_PIN_RESET);
    HAL_Delay(10);
    HAL_GPIO_WritePin(SSD1362_Reset_Port, SSD1362_Reset_Pin, GPIO_PIN_SET);
    HAL_Delay(10);
}

// Send a byte to the command register
void ssd1362_WriteCommand(uint8_t byte) {
    HAL_GPIO_WritePin(SSD1362_CS_Port, SSD1362_CS_Pin, GPIO_PIN_RESET); // select OLED
    HAL_GPIO_WritePin(SSD1362_DC_Port, SSD1362_DC_Pin, GPIO_PIN_RESET); // command
    HAL_SPI_Transmit(&SSD1362_SPI_PORT, (uint8_t *) &byte, 1, HAL_MAX_DELAY);
    HAL_GPIO_WritePin(SSD1362_CS_Port, SSD1362_CS_Pin, GPIO_PIN_SET); // un-select OLED
}

// Send data
void ssd1362_WriteData(uint8_t* buffer, size_t buff_size) {
    HAL_GPIO_WritePin(SSD1362_CS_Port, SSD1362_CS_Pin, GPIO_PIN_RESET); // select OLED
    HAL_GPIO_WritePin(SSD1362_DC_Port, SSD1362_DC_Pin, GPIO_PIN_SET); // data
    HAL_SPI_Transmit(&SSD1362_SPI_PORT, buffer, buff_size, HAL_MAX_DELAY);
    HAL_GPIO_WritePin(SSD1362_CS_Port, SSD1362_CS_Pin, GPIO_PIN_SET); // un-select OLED
}

#elif defined(SSD1362_USE_8080)  //todo add correct peripheral calls

void ssd1362_Reset(void) {
    // Reset the OLED
    HAL_GPIO_WritePin(SSD1362_Reset_Port, SSD1362_Reset_Pin, GPIO_PIN_RESET);
    HAL_Delay(10);
    HAL_GPIO_WritePin(SSD1362_Reset_Port, SSD1362_Reset_Pin, GPIO_PIN_SET);
    HAL_Delay(10);
}

// Send a byte to the command register
void ssd1362_WriteCommand(uint8_t byte) {
    HAL_SRAM_Write_8b(&hsram1, (uint32_t *)LCD_REG, (uint8_t *)&byte, 1);
}

// Send data
//void ssd1362_WriteData(uint8_t byte) {
//	HAL_SRAM_Write_8b(&hsram1, (uint32_t *)LCD_RAM, (uint8_t *)&byte, 1);
//}

void ssd1362_WriteData(uint8_t* buffer, size_t buff_size) {
    HAL_SRAM_Write_8b(&hsram1, (uint32_t *)LCD_RAM, (uint8_t *)&buffer, buff_size);
}

#else
#error "You should define SSD1362_USE_SPI or SSD1362_USE_I2C macro"
#endif


// Screenbuffer
static uint8_t SSD1362_Buffer[SSD1362_BUFFER_SIZE];

// Screen object
static SSD1362_t SSD1362;

/* Fills the Screenbuffer with values from a given buffer of a fixed length */
SSD1362_Error_t ssd1362_FillBuffer(uint8_t* buf, uint32_t len) {
    SSD1362_Error_t ret = SSD1362_ERR;
    if (len <= SSD1362_BUFFER_SIZE) {
        memcpy(SSD1362_Buffer,buf,len);
        ret = SSD1362_OK;
    }
    return ret;
}

// Initialize the oled screen
void ssd1362_Init(void) {
    // Reset OLED
    ssd1362_Reset();

    // Wait for the screen to boot
    HAL_Delay(100);

    // Init OLED
    ssd1362_WriteCommand(0XFD); //Set Command Lock
    ssd1362_WriteData((uint8_t *)0X12, 1); //(12H=Unlock,16H=Lock)

    ssd1362_WriteCommand(0XAE); //Display OFF(Sleep Mode)

    ssd1362_WriteCommand(0X15); //Set column Address
    ssd1362_WriteData((uint8_t *)0X00, 1); //Start column Address
    ssd1362_WriteData((uint8_t *)0X7F, 1); //End column Address

    ssd1362_WriteCommand(0X75); //Set Row Address
    ssd1362_WriteData((uint8_t *)0X00, 1); //Start Row Address
    ssd1362_WriteData((uint8_t *)0X3F, 1); //End Row Address

    ssd1362_WriteCommand(0X81); //Set contrast
    ssd1362_WriteData((uint8_t *)0x2f, 1);

    ssd1362_WriteCommand(0XA0); //Set Remap
    ssd1362_WriteData((uint8_t *)0XC3, 1);

    ssd1362_WriteCommand(0XA1); //Set Display Start Line
    ssd1362_WriteData((uint8_t *)0X00, 1);

    ssd1362_WriteCommand(0XA2); //Set Display Offset
    ssd1362_WriteData((uint8_t *)0X00, 1);

    ssd1362_WriteCommand(0XA4); //Normal Display

    ssd1362_WriteCommand(0XA8); //Set Multiplex Ratio
    ssd1362_WriteData((uint8_t *)0X3F, 1);

    ssd1362_WriteCommand(0XAB); //Set VDD regulator
    ssd1362_WriteData((uint8_t *)0X01, 1); //Regulator Enable

    ssd1362_WriteCommand(0XAD); //External /Internal IREF Selection
    ssd1362_WriteData((uint8_t *)0X8E, 1);

    ssd1362_WriteCommand(0XB1); //Set Phase Length
    ssd1362_WriteData((uint8_t *)0X22, 1);

    ssd1362_WriteCommand(0XB3); //Display clock Divider
    ssd1362_WriteData((uint8_t *)0XA0, 1);

    ssd1362_WriteCommand(0XB6); //Set Second pre-charge Period
    ssd1362_WriteData((uint8_t *)0X04, 1);

    ssd1362_WriteCommand(0XB9); //Set Linear LUT

    ssd1362_WriteCommand(0XBc); //Set pre-charge voltage level
    ssd1362_WriteData((uint8_t *)0X10, 1); //0.5*Vcc

    ssd1362_WriteCommand(0XBD); //Pre-charge voltage capacitor Selection
    ssd1362_WriteData((uint8_t *)0X01, 1);

    ssd1362_WriteCommand(0XBE); //Set COM deselect voltage level
    ssd1362_WriteData((uint8_t *)0X07, 1); //0.82*Vcc

    ssd1362_WriteCommand(0XAF); //Display ON

    // Clear screen
    ssd1362_Fill(White);
    
    // Flush buffer to screen
    ssd1362_UpdateScreen();
    
    // Set default values for screen object
    SSD1362.CurrentX = 0;
    SSD1362.CurrentY = 0;
    
    SSD1362.Initialized = 1;
}

// Fill the whole screen with the given color
void ssd1362_Fill(SSD1362_COLOR color) {
    /* Set memory */
    uint32_t i;

    for(i = 0; i < sizeof(SSD1362_Buffer); i++) {
        SSD1362_Buffer[i] = (color == Black) ? 0x00 : 0xFF;
    }
}

// Write the screenbuffer with changed to the screen
void ssd1362_UpdateScreen(void) {
    // Write data to each page of RAM. Number of pages
    // depends on the screen height:
    //
    //  * 32px   ==  4 pages
    //  * 64px   ==  8 pages
    //  * 128px  ==  16 pages
//    for(uint8_t i = 0; i < SSD1362_HEIGHT/8; i++) {
//        ssd1362_WriteCommand(0xB0 + i); // Set the current RAM page address.
//        ssd1362_WriteCommand(0x00);
//        ssd1362_WriteCommand(0x10);
//        ssd1362_WriteData(&SSD1362_Buffer[SSD1362_WIDTH*i],SSD1362_WIDTH);
//    }

	    for(uint8_t i = 0; i < SSD1362_HEIGHT; i++) {
//	        ssd1362_WriteCommand(0xA1 + i); // Set the current RAM page address.
//	        ssd1362_WriteCommand(0x00 + i);
//	        ssd1362_WriteCommand(0x10);
	        ssd1362_WriteData(&SSD1362_Buffer[SSD1362_WIDTH*i],SSD1362_WIDTH * 4);
	    }

}

//    Draw one pixel in the screenbuffer
//    X => X Coordinate
//    Y => Y Coordinate
//    color => Pixel color
void ssd1362_DrawPixel(uint8_t x, uint8_t y, SSD1362_COLOR color) {
    if(x >= SSD1362_WIDTH || y >= SSD1362_HEIGHT) {
        // Don't write outside the buffer
        return;
    }
    
    // Check if pixel should be inverted
    if(SSD1362.Inverted) {
        color = (SSD1362_COLOR)!color;
    }
    
    // Draw in the right color
    if(color == White) {
        SSD1362_Buffer[x + (y / 8) * SSD1362_WIDTH] |= 1 << (y % 8);
    } else { 
        SSD1362_Buffer[x + (y / 8) * SSD1362_WIDTH] &= ~(1 << (y % 8));
    }
}

// Draw 1 char to the screen buffer
// ch       => char om weg te schrijven
// Font     => Font waarmee we gaan schrijven
// color    => Black or White
char ssd1362_WriteChar(char ch, FontDef Font, SSD1362_COLOR color) {
    uint32_t i, b, j;
    
    // Check if character is valid
    if (ch < 32 || ch > 126)
        return 0;
    
    // Check remaining space on current line
    if (SSD1362_WIDTH < (SSD1362.CurrentX + Font.FontWidth) ||
        SSD1362_HEIGHT < (SSD1362.CurrentY + Font.FontHeight))
    {
        // Not enough space on current line
        return 0;
    }
    
    // Use the font to write
    for(i = 0; i < Font.FontHeight; i++) {
        b = Font.data[(ch - 32) * Font.FontHeight + i];
        for(j = 0; j < Font.FontWidth; j++) {
            if((b << j) & 0x8000)  {
                ssd1362_DrawPixel(SSD1362.CurrentX + j, (SSD1362.CurrentY + i), (SSD1362_COLOR) color);
            } else {
                ssd1362_DrawPixel(SSD1362.CurrentX + j, (SSD1362.CurrentY + i), (SSD1362_COLOR)!color);
            }
        }
    }
    
    // The current space is now taken
    SSD1362.CurrentX += Font.FontWidth;
    
    // Return written char for validation
    return ch;
}

// Write full string to screenbuffer
char ssd1362_WriteString(char* str, FontDef Font, SSD1362_COLOR color) {
    // Write until null-byte
    while (*str) {
        if (ssd1362_WriteChar(*str, Font, color) != *str) {
            // Char could not be written
            return *str;
        }
        
        // Next char
        str++;
    }
    
    // Everything ok
    return *str;
}

// Position the cursor
void ssd1362_SetCursor(uint8_t x, uint8_t y) {
    SSD1362.CurrentX = x;
    SSD1362.CurrentY = y;
}

// Draw line by Bresenhem's algorithm
void ssd1362_Line(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, SSD1362_COLOR color) {
  int32_t deltaX = abs(x2 - x1);
  int32_t deltaY = abs(y2 - y1);
  int32_t signX = ((x1 < x2) ? 1 : -1);
  int32_t signY = ((y1 < y2) ? 1 : -1);
  int32_t error = deltaX - deltaY;
  int32_t error2;
    
  ssd1362_DrawPixel(x2, y2, color);
    while((x1 != x2) || (y1 != y2))
    {
    ssd1362_DrawPixel(x1, y1, color);
    error2 = error * 2;
    if(error2 > -deltaY)
    {
      error -= deltaY;
      x1 += signX;
    }
    else
    {
    /*nothing to do*/
    }
        
    if(error2 < deltaX)
    {
      error += deltaX;
      y1 += signY;
    }
    else
    {
    /*nothing to do*/
    }
  }
  return;
}
//Draw polyline
void ssd1362_Polyline(const SSD1362_VERTEX *par_vertex, uint16_t par_size, SSD1362_COLOR color) {
  uint16_t i;
  if(par_vertex != 0){
    for(i = 1; i < par_size; i++){
      ssd1362_Line(par_vertex[i - 1].x, par_vertex[i - 1].y, par_vertex[i].x, par_vertex[i].y, color);
    }
  }
  else
  {
    /*nothing to do*/
  }
  return;
}
/*Convert Degrees to Radians*/
static float ssd1362_DegToRad(float par_deg) {
    return par_deg * 3.14 / 180.0;
}
/*Normalize degree to [0;360]*/
static uint16_t ssd1362_NormalizeTo0_360(uint16_t par_deg) {
  uint16_t loc_angle;
  if(par_deg <= 360)
  {
    loc_angle = par_deg;
  }
  else
  {
    loc_angle = par_deg % 360;
    loc_angle = ((par_deg != 0)?par_deg:360);
  }
  return loc_angle;
}
/*DrawArc. Draw angle is beginning from 4 quart of trigonometric circle (3pi/2)
 * start_angle in degree
 * sweep in degree
 */
void ssd1362_DrawArc(uint8_t x, uint8_t y, uint8_t radius, uint16_t start_angle, uint16_t sweep, SSD1362_COLOR color) {
    #define CIRCLE_APPROXIMATION_SEGMENTS 36
    float approx_degree;
    uint32_t approx_segments;
    uint8_t xp1,xp2;
    uint8_t yp1,yp2;
    uint32_t count = 0;
    uint32_t loc_sweep = 0;
    float rad;
    
    loc_sweep = ssd1362_NormalizeTo0_360(sweep);
    
    count = (ssd1362_NormalizeTo0_360(start_angle) * CIRCLE_APPROXIMATION_SEGMENTS) / 360;
    approx_segments = (loc_sweep * CIRCLE_APPROXIMATION_SEGMENTS) / 360;
    approx_degree = loc_sweep / (float)approx_segments;
    while(count < approx_segments)
    {
        rad = ssd1362_DegToRad(count*approx_degree);
        xp1 = x + (int8_t)(sin(rad)*radius);
        yp1 = y + (int8_t)(cos(rad)*radius);    
        count++;
        if(count != approx_segments)
        {
            rad = ssd1362_DegToRad(count*approx_degree);
        }
        else
        {            
            rad = ssd1362_DegToRad(loc_sweep);
        }
        xp2 = x + (int8_t)(sin(rad)*radius);
        yp2 = y + (int8_t)(cos(rad)*radius);    
        ssd1362_Line(xp1,yp1,xp2,yp2,color);
    }
    
    return;
}
//Draw circle by Bresenhem's algorithm
void ssd1362_DrawCircle(uint8_t par_x,uint8_t par_y,uint8_t par_r,SSD1362_COLOR par_color) {
  int32_t x = -par_r;
  int32_t y = 0;
  int32_t err = 2 - 2 * par_r;
  int32_t e2;

  if (par_x >= SSD1362_WIDTH || par_y >= SSD1362_HEIGHT) {
    return;
  }

    do {
      ssd1362_DrawPixel(par_x - x, par_y + y, par_color);
      ssd1362_DrawPixel(par_x + x, par_y + y, par_color);
      ssd1362_DrawPixel(par_x + x, par_y - y, par_color);
      ssd1362_DrawPixel(par_x - x, par_y - y, par_color);
        e2 = err;
        if (e2 <= y) {
            y++;
            err = err + (y * 2 + 1);
            if(-x == y && e2 <= x) {
              e2 = 0;
            }
            else
            {
              /*nothing to do*/
            }
        }
        else
        {
          /*nothing to do*/
        }
        if(e2 > x) {
          x++;
          err = err + (x * 2 + 1);
        }
        else
        {
          /*nothing to do*/
        }
    } while(x <= 0);

    return;
}

//Draw rectangle
void ssd1362_DrawRectangle(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, SSD1362_COLOR color) {
  ssd1362_Line(x1,y1,x2,y1,color);
  ssd1362_Line(x2,y1,x2,y2,color);
  ssd1362_Line(x2,y2,x1,y2,color);
  ssd1362_Line(x1,y2,x1,y1,color);

  return;
}

void ssd1362_SetContrast(const uint8_t value) {
    const uint8_t kSetContrastControlRegister = 0x81;
    ssd1362_WriteCommand(kSetContrastControlRegister);
    ssd1362_WriteData((uint8_t * )&value, 1);
}

void ssd1362_SetDisplayOn(const uint8_t on) {
    uint8_t value;
    if (on) {
        value = 0xAF;   // Display on
        SSD1362.DisplayOn = 1;
    } else {
        value = 0xAE;   // Display off
        SSD1362.DisplayOn = 0;
    }
    ssd1362_WriteCommand(value);
}

uint8_t ssd1362_GetDisplayOn() {
    return SSD1362.DisplayOn;
}
