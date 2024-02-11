/**
 ******************************************************************************
 * @file           : ssd1362.h
 * @brief          : Oled display driver
 ******************************************************************************
 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __SSD1362_H__
#define __SSD1362_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

/* Private includes ----------------------------------------------------------*/
#include <stdbool.h>
#include "main.h"

/* Exported types ------------------------------------------------------------*/
extern SRAM_HandleTypeDef hsram1;

/* Exported constants --------------------------------------------------------*/
#define SSD1362_HEIGHT          64		// SSD1362 OLED height in pixels
#define SSD1362_WIDTH           256 	// SSD1362 width in pixels
#define SSD1362_Reset_Port      OLED_RESET_GPIO_Port
#define SSD1362_Reset_Pin       OLED_RESET_Pin

/* LCD is connected to the FSMC_Bank1_NOR/SRAM1 and NE1 is used as ship select signal */
/* RS <==> A0 */
#define LCD_REG   ((uint32_t) 0xC0000000)
#define LCD_RAM   ((uint32_t) 0xC0000001)

// Scroll rate constants. See datasheet page 40.
#define SSD1327_SCROLL_2   0b111
#define SSD1327_SCROLL_3   0b100
#define SSD1327_SCROLL_4   0b101
#define SSD1327_SCROLL_5   0b110
#define SSD1327_SCROLL_6   0b000
#define SSD1327_SCROLL_32  0b001
#define SSD1327_SCROLL_64  0b010
#define SSD1327_SCROLL_256 0b011
/* Exported macro ------------------------------------------------------------*/

/* Exported functions prototypes ---------------------------------------------*/
void ssd1362_setWriteZone(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);
uint16_t ssd1362_coordsToAddress(uint16_t x, uint16_t y);
void ssd1362_setPixelChanged(uint16_t x, uint16_t y, bool changed);
void ssd1362_drawPixel(uint16_t x, uint16_t y, uint8_t color, bool display);
void ssd1362_drawRect(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t color, bool display);
void ssd1362_fillRect(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t color, bool display);
void ssd1362_drawCircle(uint16_t x0, uint16_t y0, uint16_t r, uint8_t color, bool display);
void ssd1362_fillCircle(uint16_t x0, uint16_t y0, uint16_t r, uint8_t color, bool display);
void ssd1362_drawHLine(uint16_t x, uint16_t y, uint16_t length, uint8_t color, bool display);
void ssd1362_drawVLine(uint16_t x, uint16_t y, int16_t length, uint8_t color, bool display);
void ssd1362_drawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t color, bool display);
void ssd1362_drawByteAsRow(uint16_t x, uint16_t y, uint8_t byte, uint8_t color);
void ssd1362_drawChar(uint16_t x, uint16_t y, uint8_t thisChar, uint8_t color);
void ssd1362_drawChar16(uint16_t x, uint16_t y, uint8_t thisChar, uint8_t color);
void ssd1362_drawChar32(uint16_t x, uint16_t y, uint8_t thisChar, uint8_t color);
void ssd1362_drawCharArray(uint16_t x, uint16_t y, int8_t text[], uint8_t color, uint32_t size);
void ssd1362_drawString(uint16_t x, uint16_t y, int8_t textString[], uint8_t color, uint32_t size);
void ssd1362_drawBmp(const uint8_t *bitmap, uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint8_t color, bool display);
void ssd1362_progressBar(uint16_t x, uint16_t y, uint8_t state, uint8_t color);
void ssd1362_setupScrolling(uint8_t startRow, uint8_t endRow, uint8_t startCol, uint8_t endCol, uint8_t scrollSpeed, bool right);
void ssd1362_startScrolling();
void ssd1362_stopScrolling();
void ssd1362_scrollStep(uint8_t startRow, uint8_t endRow, uint8_t startCol, uint8_t endCol, bool right);
void ssd1362_fillStripes(uint8_t offset);
void ssd1362_clearBuffer();
void ssd1362_writeFullBuffer();
void ssd1362_writeUpdates();
void ssd1362_screenRotation(uint32_t val);
void ssd1362_setContrast(uint8_t contrast);
void ssd1362_init();

/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /*__SSD1362_H__*/
