/**
 * Private configuration file for the SSD1362 library.
 * This example is configured for STM32F0, I2C and including all fonts.
 */

#ifndef __SSD1362_CONF_H__
#define __SSD1362_CONF_H__

// Choose a microcontroller family
//#define STM32F0
//#define STM32F1
//#define STM32F4
//#define STM32L0
//#define STM32L4
//#define STM32F3
#define STM32H7
//#define STM32F7

// Choose a bus
//#define SSD1362_USE_I2C
//#define SSD1362_USE_SPI
#define SSD1362_USE_8080

// I2C Configuration
//#define SSD1362_I2C_PORT        hi2c1
//#define SSD1362_I2C_ADDR        (0x3C << 1)

// SPI Configuration
//#define SSD1362_SPI_PORT        hspi1
//#define SSD1362_CS_Port         OLED_CS_GPIO_Port
//#define SSD1362_CS_Pin          OLED_CS_Pin
//#define SSD1362_DC_Port         OLED_DC_GPIO_Port
//#define SSD1362_DC_Pin          OLED_DC_Pin
//#define SSD1362_Reset_Port      OLED_Res_GPIO_Port
//#define SSD1362_Reset_Pin       OLED_Res_Pin

// Mirror the screen if needed
// #define SSD1362_MIRROR_VERT
// #define SSD1362_MIRROR_HORIZ

// Set inverse color if needed
// # define SSD1362_INVERSE_COLOR

// Include only needed fonts
#define SSD1362_INCLUDE_FONT_6x8
#define SSD1362_INCLUDE_FONT_7x10
#define SSD1362_INCLUDE_FONT_11x18
#define SSD1362_INCLUDE_FONT_16x26

// Some OLEDs don't display anything in first two columns.
// In this case change the following macro to 130.
// The default value is 128.
 #define SSD1362_WIDTH           256

// The height can be changed as well if necessary.
// It can be 32, 64 or 128. The default value is 64.
 #define SSD1362_HEIGHT          64

#endif /* __SSD1362_CONF_H__ */
