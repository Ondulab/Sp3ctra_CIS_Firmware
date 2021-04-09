/**
 * This Library was originally written by Olivier Van den Eede (4ilo) in 2016.
 * Some refactoring was done and SPI support was added by Aleksander Alekseev (afiskon) in 2018.
 *
 * https://github.com/afiskon/stm32-ssd1362
 */

#ifndef __SSD1362_H__
#define __SSD1362_H__

#include <stddef.h>
#include <_ansi.h>

_BEGIN_STD_C

#include "ssd1362_conf.h"
#include "main.h"

#if defined(STM32F0)
#include "stm32f0xx_hal.h"
#elif defined(STM32F1)
#include "stm32f1xx_hal.h"
#elif defined(STM32F4)
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_gpio.h"
#elif defined(STM32L0)
#include "stm32l0xx_hal.h"
#elif defined(STM32L4)
#include "stm32l4xx_hal.h"
#elif defined(STM32F3)
#include "stm32f3xx_hal.h"
#elif defined(STM32H7)
#include "stm32h7xx_hal.h"
#elif defined(STM32F7)
#include "stm32f7xx_hal.h"
#else
#error "SSD1362 library was tested only on  STM32F0, STM32F1, STM32F3, STM32F4, STM32F7, STM32L0, STM32L4, STM32H7 MCU families. Please modify ssd1362.h if you know what you are doing. Also please send a pull request if it turns out the library works on other MCU's as well!"
#endif

#include "ssd1362_fonts.h"

#ifndef SSD1362_Reset_Port
#define SSD1362_Reset_Port      OLED_RESET_GPIO_Port
#endif
#ifndef SSD1362_Reset_Pin
#define SSD1362_Reset_Pin       OLED_RESET_Pin
#endif

/* ^^^ SPI config ^^^ */

#if defined(SSD1362_USE_I2C)
extern I2C_HandleTypeDef SSD1362_I2C_PORT;
#ifndef SSD1362_I2C_PORT
#define SSD1362_I2C_PORT        hi2c1
#endif

#ifndef SSD1362_I2C_ADDR
#define SSD1362_I2C_ADDR        (0x3C << 1)
#endif
#elif defined(SSD1362_USE_SPI)
#ifndef SSD1362_SPI_PORT
#define SSD1362_SPI_PORT        hspi2
#endif

#ifndef SSD1362_CS_Port
#define SSD1362_CS_Port         GPIOB
#endif
#ifndef SSD1362_CS_Pin
#define SSD1362_CS_Pin          GPIO_PIN_12
#endif

#ifndef SSD1362_DC_Port
#define SSD1362_DC_Port         GPIOB
#endif
#ifndef SSD1362_DC_Pin
#define SSD1362_DC_Pin          GPIO_PIN_14
#endif
extern SPI_HandleTypeDef SSD1362_SPI_PORT;
#elif defined(SSD1362_USE_8080)
/* LCD is connected to the FSMC_Bank1_NOR/SRAM1 and NE1 is used as ship select signal */
/* RS <==> A0 */
#define LCD_REG   ((uint32_t) 0xC0000000)
#define LCD_RAM   ((uint32_t) 0xC0000000)

extern SRAM_HandleTypeDef hsram1;
#else
#error "You should define SSD1362_USE_SPI or SSD1362_USE_I2C macro!"
#endif

// SSD1362 OLED height in pixels
#ifndef SSD1362_HEIGHT
#define SSD1362_HEIGHT          64
#endif

// SSD1362 width in pixels
#ifndef SSD1362_WIDTH
#define SSD1362_WIDTH           128
#endif

#ifndef SSD1362_BUFFER_SIZE
#define SSD1362_BUFFER_SIZE   SSD1362_WIDTH * SSD1362_HEIGHT * 4
#endif

// Enumeration for screen colors
typedef enum {
    Black = 0x00, // Black color, no pixel
    White = 0x01  // Pixel is set. Color depends on OLED
} SSD1362_COLOR;

typedef enum {
    SSD1362_OK = 0x00,
    SSD1362_ERR = 0x01  // Generic error.
} SSD1362_Error_t;

// Struct to store transformations
typedef struct {
    uint16_t CurrentX;
    uint16_t CurrentY;
    uint8_t Inverted;
    uint8_t Initialized;
    uint8_t DisplayOn;
} SSD1362_t;
typedef struct {
    uint8_t x;
    uint8_t y;
} SSD1362_VERTEX;

// Procedure definitions
void ssd1362_Init(void);
void ssd1362_Fill(SSD1362_COLOR color);
void ssd1362_UpdateScreen(void);
void ssd1362_DrawPixel(uint8_t x, uint8_t y, SSD1362_COLOR color);
char ssd1362_WriteChar(char ch, FontDef Font, SSD1362_COLOR color);
char ssd1362_WriteString(char* str, FontDef Font, SSD1362_COLOR color);
void ssd1362_SetCursor(uint8_t x, uint8_t y);
void ssd1362_Line(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, SSD1362_COLOR color);
void ssd1362_DrawArc(uint8_t x, uint8_t y, uint8_t radius, uint16_t start_angle, uint16_t sweep, SSD1362_COLOR color);
void ssd1362_DrawCircle(uint8_t par_x, uint8_t par_y, uint8_t par_r, SSD1362_COLOR color);
void ssd1362_Polyline(const SSD1362_VERTEX *par_vertex, uint16_t par_size, SSD1362_COLOR color);
void ssd1362_DrawRectangle(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, SSD1362_COLOR color);
/**
 * @brief Sets the contrast of the display.
 * @param[in] value contrast to set.
 * @note Contrast increases as the value increases.
 * @note RESET = 7Fh.
 */
void ssd1362_SetContrast(const uint8_t value);
/**
 * @brief Set Display ON/OFF.
 * @param[in] on 0 for OFF, any for ON.
 */
void ssd1362_SetDisplayOn(const uint8_t on);
/**
 * @brief Reads DisplayOn state.
 * @return  0: OFF.
 *          1: ON.
 */
uint8_t ssd1362_GetDisplayOn();

// Low-level procedures
void ssd1362_Reset(void);
void ssd1362_WriteCommand(uint8_t byte);
//void ssd1362_WriteData(uint8_t byte);
void ssd1362_WriteData(uint8_t* buffer, size_t buff_size);
SSD1362_Error_t ssd1362_FillBuffer(uint8_t* buf, uint32_t len);

_END_STD_C

#endif // __SSD1362_H__
