/**
 ******************************************************************************
 * @file           : ssd1362.c
 * @brief          : Oled display driver
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include <math.h>
#include <stdlib.h>
#include <string.h>  // For memcpy
#include "basetypes.h"

/* Private includes ----------------------------------------------------------*/
#include "ssd1362.h"
#include "font16x16.h"
#include "font16x32.h"
#include "font8x8_basic.h"
#include "config.h"
#include "shared.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
uint8_t frameBuffer[SSD1362_HEIGHT * SSD1362_WIDTH];   // Should mirror the display's own frameBuffer.
uint8_t changedPixels[2048]; // Each bit of this array represets whether a given byte of frameBuffer (e.g. a pair of pixels) is not up to date.

/* Private function prototypes -----------------------------------------------*/
void ssd1362_writeCmd(uint8_t reg);
void ssd1362_writeData(uint8_t data);

/* Private user code ---------------------------------------------------------*/

void ssd1362_Reset(void) {
	// Reset the OLED
	HAL_GPIO_WritePin(SSD1362_Reset_Port, SSD1362_Reset_Pin, GPIO_PIN_RESET);
	HAL_Delay(10);
	HAL_GPIO_WritePin(SSD1362_Reset_Port, SSD1362_Reset_Pin, GPIO_PIN_SET);
	HAL_Delay(10);
}

//Writes a command byte to the driver
void ssd1362_writeCmd(uint8_t reg)
{
	HAL_SRAM_Write_8b(&hsram1, (uint32_t *)LCD_REG, (uint8_t *)&reg, 1);
}

//Writes 1 byte to the display's memory
void ssd1362_writeData(uint8_t data)
{
	HAL_SRAM_Write_8b(&hsram1, (uint32_t *)LCD_RAM, (uint8_t *)&data, 1);
}

void bitWrite(uint8_t *x, uint8_t n, uint8_t value) {
	if (value)
		*x |= (1 << n);
	else
		*x &= ~(1 << n);
}

char bitRead(uint8_t *x, uint8_t n) {
	return (*x & (1 << n)) ? 1 : 0;
}

//defines a rectangular area of memory which the driver will itterate through. This function takes memory locations, meaning a 64x256 space
void ssd1362_setWriteZone(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
	ssd1362_writeCmd(0x15); //Set Column Address
	ssd1362_writeCmd(x1); //Beginning. Note that you must divide the column by 2, since 1 byte in memory is 2 pixels
	ssd1362_writeCmd(x2); //End

	ssd1362_writeCmd(0x75); //Set Row Address
	ssd1362_writeCmd(y1); //Beginning
	ssd1362_writeCmd(y2); //End
}

//Converts a pixel location to a linear memory address
uint16_t ssd1362_coordsToAddress(uint16_t x, uint16_t y)
{
	return (x / 2)+(y * 128);
}

void ssd1362_setPixelChanged(uint16_t x, uint16_t y, bool changed)
{
	uint16_t targetByte = ssd1362_coordsToAddress(x, y)/8;
	bitWrite(&changedPixels[targetByte], ssd1362_coordsToAddress(x, y) % 8, changed);
}

//pixel xy coordinates 0-255, color 0-15, and whether to immediately output it to the display or buffer it
void ssd1362_drawPixel(uint16_t x, uint16_t y, uint8_t color, bool display)
{
	uint32_t address = ssd1362_coordsToAddress(x,y);
	if((x%2) == 0)
	{//If this is an even pixel, and therefore needs shifting to the more significant nibble
		frameBuffer[address] = (frameBuffer[address] & 0x0f) | (color<<4);
	} else {
		frameBuffer[address] = (frameBuffer[address] & 0xf0) | (color);
	}

	if(display)
	{
		ssd1362_setWriteZone(x/2,y,x/2,y);
		ssd1362_writeData(frameBuffer[address]);
		ssd1362_setPixelChanged(x, y, false); // We've now synced the display with this byte of the buffer, no need to write it again
	} else {
		ssd1362_setPixelChanged(x, y, true); // This pixel is due for an update next refresh
	}
}

void ssd1362_drawRect(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t color, bool display)
{
    // Ensure x1 is always less than or equal to x2
    if (x1 > x2)
    {
        uint16_t temp = x1;
        x1 = x2;
        x2 = temp;
    }
    // Ensure y1 is always less than or equal to y2
    if (y1 > y2)
    {
        uint16_t temp = y1;
        y1 = y2;
        y2 = temp;
    }

    // Draw the top and bottom horizontal lines
    for (uint16_t x = x1; x <= x2; x++)
    {
        ssd1362_drawPixel(x, y1, color, display);
        ssd1362_drawPixel(x, y2, color, display);
    }
    // Avoid unnecessary drawing if y1 equals y2
    if (y2 > y1)
    {
        // Draw the left and right vertical lines
        for (uint16_t y = y1 + 1; y < y2; y++)
        {
            ssd1362_drawPixel(x1, y, color, display);
            ssd1362_drawPixel(x2, y, color, display);
        }
    }
}

void ssd1362_fillRect(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t color, bool display)
{
    // Ensure x1 is always less than or equal to x2
    if (x1 > x2)
    {
        uint16_t temp = x1;
        x1 = x2;
        x2 = temp;
    }
    // Ensure y1 is always less than or equal to y2
    if (y1 > y2)
    {
        uint16_t temp = y1;
        y1 = y2;
        y2 = temp;
    }

    // Fill the rectangle area with the specified color
    for (uint16_t y = y1; y <= y2; y++)
    {
        for (uint16_t x = x1; x <= x2; x++)
        {
            ssd1362_drawPixel(x, y, color, display);
        }
    }
}

void ssd1362_drawCircle(uint16_t x0, uint16_t y0, uint16_t r, uint8_t color, bool display)
{
    int32_t x = r;
    int32_t y = 0;
    int32_t dx = 1 - (2 * r);
    int32_t dy = 1;
    int32_t err = 0;

    while (x >= y)
    {
        ssd1362_drawPixel(x0 + x, y0 + y, color, display);
        ssd1362_drawPixel(x0 - x, y0 + y, color, display);
        ssd1362_drawPixel(x0 + x, y0 - y, color, display);
        ssd1362_drawPixel(x0 - x, y0 - y, color, display);
        ssd1362_drawPixel(x0 + y, y0 + x, color, display);
        ssd1362_drawPixel(x0 - y, y0 + x, color, display);
        ssd1362_drawPixel(x0 + y, y0 - x, color, display);
        ssd1362_drawPixel(x0 - y, y0 - x, color, display);

        y++;
        err += dy;
        dy += 2;
        if (2 * err + dx > 0)
        {
            x--;
            err += dx;
            dx += 2;
        }
    }
}

void ssd1362_fillCircle(uint16_t x0, uint16_t y0, uint16_t r, uint8_t color, bool display)
{
    // Correct initialization for filled circles drawing
    int32_t x = r;
    int32_t y = 0;
    int32_t xChange = 1 - (2 * r);
    int32_t yChange = 1;
    int32_t radiusError = 0;

    // Draw the vertical middle line from the start
    while (x >= y)
    {
        for (int32_t i = x0 - x; i <= x0 + x; i++)
        {
            ssd1362_drawPixel(i, y0 + y, color, display);
            ssd1362_drawPixel(i, y0 - y, color, display);
        }
        for (int32_t i = x0 - y; i <= x0 + y; i++)
        {
            ssd1362_drawPixel(i, y0 + x, color, display);
            ssd1362_drawPixel(i, y0 - x, color, display);
        }

        y++;
        radiusError += yChange;
        yChange += 2;
        if (2 * radiusError + xChange > 0)
        {
            x--;
            radiusError += xChange;
            xChange += 2;
        }
    }
}

void ssd1362_drawHLine(uint16_t x, uint16_t y, uint16_t length, uint8_t color, bool display)
{
	for (uint32_t i = x; i < x+length; i++)
	{
		ssd1362_drawPixel(i, y, color, display);
	}
}

void ssd1362_drawVLine(uint16_t x, uint16_t y, int16_t length, uint8_t color, bool display)
{
    if (length >= 0)
    {
        for (uint16_t i = 0; i < length; i++)
        {
            ssd1362_drawPixel(x, y + i, color, display);
        }
    } else
    {
        for (uint16_t i = 0; i < -length; i++)
        {
            ssd1362_drawPixel(x, y - i, color, display);
        }
    }
}



void ssd1362_drawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint8_t color, bool display)
{ //Bresenham's line algorithm
	uint32_t deltaX = abs(x1-x0);
	uint32_t deltaY = abs(y1-y0);
	uint32_t signX = x0<x1 ? 1 : -1;
	uint32_t signY = y0<y1 ? 1 : -1;
	uint32_t error = (deltaX>deltaY ? deltaX : -deltaY)/2, error2;

	while (true)
	{
		ssd1362_drawPixel(x0, y0, color, display);
		if (x0==x1 && y0==y1) break;
		error2 = error;
		if (error2 >-deltaX)
		{
			error -= deltaY;
			x0 += signX;
		}
		if (error2 < deltaY)
		{
			error += deltaX;
			y0 += signY;
		}
	}
}

//Draws a byte as an 8 pixel row
void ssd1362_drawByteAsRow(uint16_t x, uint16_t y, uint8_t byte, uint8_t color)
{
	for (uint32_t i = 0; i < 8; i++)
	{
		if(bitRead(&byte, i))
		{
			ssd1362_drawPixel(x+i, y, color, false);
		}
	}
}

void ssd1362_drawChar(uint16_t x, uint16_t y, uint8_t thisChar, uint8_t color)
{
	for (size_t i = 0; i < 8; i++)
	{
		ssd1362_drawByteAsRow(x, y+i, font8x8_basic[(unsigned char)thisChar][i], color);
	}
}

void ssd1362_drawCharArray(uint16_t x, uint16_t y, int8_t text[], uint8_t color, uint32_t size)
{
	const int8_t* thisChar;
	uint16_t xOffset = 0;
	if(size==16)
	{
		for (thisChar = text; *thisChar != '\0'; thisChar++)
		{
			ssd1362_drawChar16(x+xOffset, y, *thisChar, color);
			xOffset += 8;
		}
	} else if(size==32)
	{
		for (thisChar = text; *thisChar != '\0'; thisChar++)
		{
			ssd1362_drawChar32(x+xOffset, y, *thisChar, color);
			xOffset += 16;
		}
	}
	else {
		for (thisChar = text; *thisChar != '\0'; thisChar++)
		{
			ssd1362_drawChar(x+xOffset, y, *thisChar, color);
			xOffset += 8;
		}
	}
}

void ssd1362_drawString(uint16_t x, uint16_t y, int8_t textString[], uint8_t color, uint32_t size)
{
	//	uint8_t text[64];
	//	textString.toCharArray(text, 64);
	ssd1362_drawCharArray(x,y, textString, color, size);
}

void ssd1362_drawChar16(uint16_t x, uint16_t y, uint8_t thisChar, uint8_t color)
{
	for (size_t row = 0; row < 16; row++) {
		ssd1362_drawByteAsRow(x, y+row, font16x16[(unsigned char)thisChar][row*2], color);
		ssd1362_drawByteAsRow(x+8, y+row, font16x16[(unsigned char)thisChar][(row*2)+1], color);
	}
}

void ssd1362_drawChar32(uint16_t x, uint16_t y, uint8_t thisChar, uint8_t color)
{
	for (size_t row = 0; row < 32; row++) {
		ssd1362_drawByteAsRow(x, y+row, font16x32[(unsigned char)thisChar][row*2], color);
		ssd1362_drawByteAsRow(x+8, y+row, font16x32[(unsigned char)thisChar][(row*2)+1], color);
	}
}

/**************************************************************************/
/*
 Procedure to use ssd1306DrawBmp function :

 - make your picture with gimp
 - save to Bitmap file
 - open Image2LCD
 - chose into Scan Mode box => Data hor, Bite ver
 - chose into BitsPixel box => monochrome
 - check => Antitone pixel in byte
 - uncheck the rest
 - select => chose normal
 - chose into Output file type box => C array (*C)
 - save
 - open the file with SublimeText => select all => copy
 - paste in pictures_bmp.h
 */
/**************************************************************************/
void ssd1362_drawBmp(const uint8_t *bitmap, uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint8_t color, bool display)
{
	uint16_t i, j;

	for (j = 0; j < h; j++)
	{
		for (i = 0; i < w; i++)
		{
			if (bitmap[i + (j / 8) * w] & 1 << (j % 8))
			{
				ssd1362_drawPixel(x + i, y + j, color, display);
			}
		}
	}
}

void ssd1362_progressBar(uint16_t x, uint16_t y, uint8_t state, uint8_t color)
{
	//sanity check
	if (state > 100)
		state = 100;
	ssd1362_fillRect(x, y, 202 + x, 12 + y, 4, false);
	if ((state > 0) && (state < 100))
		ssd1362_fillRect(x + 2, y + 2, state * 2 + x + 2, 8 + y + 2, color, false);
	ssd1362_writeUpdates();
}

//gradient test pattern
void ssd1362_fillStripes(uint8_t offset)
{
	for (uint32_t i = 0; i < (SSD1362_HEIGHT * SSD1362_WIDTH); i++)
	{
		uint8_t color = ((i+offset) & 0xF) | (((i+offset) & 0xF)<<4);
		frameBuffer[i] = color;
	}
	for (uint16_t i = 0; i < 2048; i++) {
		changedPixels[i] = 0xFF; // Set all pixels to be updated next frame. fillStripes should not be used without a full write anyways, but just in case
	}
}

void ssd1362_setupScrolling(uint8_t startRow, uint8_t endRow, uint8_t startCol, uint8_t endCol, uint8_t scrollSpeed, bool right)
{
	uint8_t swap;
	if (startRow > endRow) { // Ensure start row is before end
		swap = startRow;
		startRow = endRow;
		endRow = swap;
	}
	if (startCol > endCol) { // Ditto for columns
		swap = startCol;
		startCol = endCol;
		endCol = swap;
	}
	ssd1362_writeCmd(0x2E);   // Deactivate scrolling before changing anything
	if (right) {
		ssd1362_writeCmd(0x26); // Scroll right
	} else {
		ssd1362_writeCmd(0x27); // Scroll left
	}
	ssd1362_writeCmd(0); // Dummy byte
	ssd1362_writeCmd(startRow);
	ssd1362_writeCmd(scrollSpeed);
	ssd1362_writeCmd(endRow);
	ssd1362_writeCmd(startCol);
	ssd1362_writeCmd(endCol);
	ssd1362_writeCmd(0); // Dummy byte
};

void ssd1362_startScrolling()
{
	ssd1362_writeCmd(0x2F);
}

void ssd1362_stopScrolling()
{
	ssd1362_writeCmd(0x2E);
}

void ssd1362_scrollStep(uint8_t startRow, uint8_t endRow, uint8_t startCol, uint8_t endCol, bool right)
{
	ssd1362_setupScrolling(startRow, endRow, startCol, endCol, SSD1327_SCROLL_2, right);
	ssd1362_startScrolling();
	HAL_Delay(15);
	ssd1362_stopScrolling();
}

void ssd1362_clearBuffer()
{
	for (uint32_t i = 0; i < ((SSD1362_HEIGHT * SSD1362_WIDTH) / 2); i++)
	{
		// If there is a non-zero (non-black) byte here, make sure it gets updated
		if (frameBuffer[i])
		{
			frameBuffer[i] = 0;
			bitWrite(&changedPixels[i/8], i%8, 1); // Mark this pixel as needing an update
		}
	}
}

//Outputs the full framebuffer to the display
void ssd1362_writeFullBuffer()
{
	ssd1362_setWriteZone(0, 0, (SSD1362_WIDTH / 2) - 1, SSD1362_HEIGHT - 1); //Full display
	for(uint32_t i = 0; i < ((SSD1362_HEIGHT * SSD1362_WIDTH) / 2); i++)
	{
		ssd1362_writeData(frameBuffer[i]);
	}
	for (uint32_t i = 0; i < 1024; i++)
	{
		changedPixels[i] = 0; // Set all pixels as up to date.
	}
}

// Writes only the pixels that have changed to the display
void ssd1362_writeUpdates()
{
	for (size_t y = 0; y < SSD1362_HEIGHT; y++) {
		bool continued = false; // If we can continue with the write zone we're using
		for (size_t x = 0; x < SSD1362_WIDTH; x++) {
			uint16_t address = ssd1362_coordsToAddress(x, y);
			if ( bitRead(&changedPixels[address/8], address % 8) ) { // If we need an update here
				if (!continued) { // Just write the byte, no new write zone needed
					continued = true;
					ssd1362_setWriteZone(x/2, y, SSD1362_WIDTH - 1, SSD1362_HEIGHT - 1); // Set the write zone for this new byte and any subsequent ones
				}
				ssd1362_writeData(frameBuffer[address]);
				bitWrite(&changedPixels[address/8], address % 8, 0);
			} else {
				continued = false; // The chain of pixels is broken
			}
		}
	}
}

// Set screen rotation
void ssd1362_screenRotation(uint32_t val)
{
	if (val == 0)
	{
		ssd1362_writeCmd(0XA0); //Set Remap
		ssd1362_writeCmd(0XC3);
	}
	if (val > 0)
	{
		ssd1362_writeCmd(0XA0); //Set Remap
		ssd1362_writeCmd(0X52);
	}
}

void ssd1362_setContrast(uint8_t contrast)
{
	ssd1362_writeCmd(0x81);  //set contrast control
	ssd1362_writeCmd(contrast);  //Contrast byte
}

//Sends all the boilerplate startup and config commands to the driver
void ssd1362_init()
{
	// Enable 12V power DC/DC for CIS
	HAL_GPIO_WritePin(EN_12V_GPIO_Port, EN_12V_Pin, GPIO_PIN_SET);
	HAL_Delay(100);

	// Reset OLED
	ssd1362_Reset();

	// Wait for the screen to boot
	HAL_Delay(100);

	// Init OLED
	ssd1362_writeCmd(0XFD); //Set Command Lock
	ssd1362_writeCmd(0X12); //(12H=Unlock,16H=Lock)

	ssd1362_writeCmd(0XAE); //Display OFF(Sleep Mode)

	ssd1362_writeCmd(0X15); //Set column Address
	ssd1362_writeCmd(0X00); //Start column Address
	ssd1362_writeCmd(0X7F); //End column Address

	ssd1362_writeCmd(0X75); //Set Row Address
	ssd1362_writeCmd(0X00); //Start Row Address
	ssd1362_writeCmd(0X3F); //End Row Address

	ssd1362_writeCmd(0X81); //Set contrast
	ssd1362_writeCmd(0x2f);

	ssd1362_screenRotation(shared_var.cis_scanDir);

	ssd1362_writeCmd(0XA1); //Set Display Start Line
	ssd1362_writeCmd(0X00);

	ssd1362_writeCmd(0XA2); //Set Display Offset
	ssd1362_writeCmd(0X00);

	ssd1362_writeCmd(0XA1); //Set Display Start Line
	ssd1362_writeCmd(0X00); // Set to the maximum row number for flipping vertically

	ssd1362_writeCmd(0XA2); //Set Display Offset
	ssd1362_writeCmd(0X00); // Set to the maximum column number for flipping horizontally




	ssd1362_writeCmd(0XA4); //Normal Display

	ssd1362_writeCmd(0XA8); //Set Multiplex Ratio
	ssd1362_writeCmd(0X3F);

	ssd1362_writeCmd(0XAB); //Set VDD regulator
	ssd1362_writeCmd(0X01); //Regulator Enable

	ssd1362_writeCmd(0XAD); //External /Internal IREF Selection
	ssd1362_writeCmd(0X8E);

	ssd1362_writeCmd(0XB1); //Set Phase Length
	ssd1362_writeCmd(0X22);

	ssd1362_writeCmd(0XB3); //Display clock Divider
	ssd1362_writeCmd(0XA0);

	ssd1362_writeCmd(0XB6); //Set Second pre-charge Period
	ssd1362_writeCmd(0X04);

	ssd1362_writeCmd(0XB9); //Set Linear LUT

	ssd1362_writeCmd(0XBc); //Set pre-charge voltage level
	ssd1362_writeCmd(0X10); //0.5*Vcc

	ssd1362_writeCmd(0XBD); //Pre-charge voltage capacitor Selection
	ssd1362_writeCmd(0X01);

	ssd1362_writeCmd(0XBE); //Set COM deselect voltage level
	ssd1362_writeCmd(0X07); //0.82*Vcc

	ssd1362_writeCmd(0XAF); //Display ON
}

