#ifndef _SSD1963_H_
#define _SSD1963_H_

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SSD1963_0_DEGREE_REG_VALUE   0x00
#define SSD1963_180_DEGREE_REG_VALUE 0x03


#define SSD1963_LCD_PARA
#define SSD_DCLK_FREQUENCY  9   // 9Mhz

#define SSD_HOR_PULSE_WIDTH 1
#define SSD_HOR_BACK_PORCH  41
#define SSD_HOR_FRONT_PORCH 2

#define SSD_VER_PULSE_WIDTH 1
#define SSD_VER_BACK_PORCH  10
#define SSD_VER_FRONT_PORCH 2

#define LCD_WIDTH   480
#define LCD_HEIGHT  272


typedef union
{
  uint16_t color;
  struct
  {
    uint16_t b:5;
    uint16_t g:6;
    uint16_t r:5;
  } RGB;
} GUI_COLOR;

uint8_t LCD_DriveIsSSD1963(void);
void SSD1963_Init_Sequential(void);
void SSD1963_SetDirection(uint8_t rotate);
void SSD1963_SetWindow(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey);
void SSD1963_SetWindowWH(uint16_t sx, uint16_t sy, uint16_t w, uint16_t h);
uint32_t SSD1963_ReadPixel_24Bit(int16_t x, int16_t y);

#ifdef __cplusplus
}
#endif

#endif
