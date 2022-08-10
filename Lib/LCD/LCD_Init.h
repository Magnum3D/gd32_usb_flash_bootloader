#ifndef _LCD_INIT_H_
#define _LCD_INIT_H_

#ifdef __cplusplus
extern "C" {
#endif

//#include "variants.h"
//#include "menu.h"
#include <stdint.h>

#include "lcd_ui_fonts.h"

#ifdef LCD_LED_PWM_CHANNEL
  #define LCD_0_PERCENT   0
  #define LCD_5_PERCENT   5
  #define LCD_10_PERCENT  10
  #define LCD_20_PERCENT  20
  #define LCD_30_PERCENT  30
  #define LCD_40_PERCENT  40
  #define LCD_50_PERCENT  50
  #define LCD_60_PERCENT  60
  #define LCD_70_PERCENT  70
  #define LCD_80_PERCENT  80
  #define LCD_90_PERCENT  90
  #define LCD_100_PERCENT 100

  #define LCD_DIM_OFF         0    // Off
  #define LCD_DIM_5_SECONDS   5    // Seconds
  #define LCD_DIM_10_SECONDS  10   // Seconds
  #define LCD_DIM_30_SECONDS  30   // Seconds
  #define LCD_DIM_60_SECONDS  60   // Seconds
  #define LCD_DIM_120_SECONDS 120  // Seconds
  #define LCD_DIM_300_SECONDS 300  // Seconds
  /*
   // Custom value, will be predefined in configuration.h
   #define LCD_DIM_CUSTOM_SECONDS LCD_DIM_5_SECONDS
  */

  typedef struct
  {
    uint32_t idle_ms;
    bool dimmed;
  } LCD_AUTO_DIM;
  extern LCD_AUTO_DIM lcd_dim;

  #define ITEM_SECONDS_NUM 8
  #define ITEM_BRIGHTNESS_NUM 12

  extern const uint32_t LCD_DIM_IDLE_TIME[ITEM_SECONDS_NUM];
  extern const LABEL itemDimTime[ITEM_SECONDS_NUM];

  extern const  uint32_t LCD_BRIGHTNESS[ITEM_BRIGHTNESS_NUM];
  extern const LABEL itemBrightness[ITEM_BRIGHTNESS_NUM];

  void LCD_LED_PWM_Init(void);
  void loopDimTimer(void);
  void _wakeLCD(void);

  #define Set_LCD_Brightness(percentage) TIM_PWM_SetDutyCycle(LCD_LED_PWM_CHANNEL, percentage)
  #define wakeLCD() _wakeLCD()
#else
  #define wakeLCD()

#endif  // LCD_LED_PWM_CHANNEL

  typedef enum
  {
  	LCDUI_FONT_H11 = 0,
  	LCDUI_FONT_H11BOLD,
  	LCDUI_FONT_H12,
  	LCDUI_FONT_H12BOLD,
  	LCDUI_FONT_H14,
  	LCDUI_FONT_H14BOLD,
	LCDUI_FONT_H18,
  	LCDUI_FONT_H18BOLD,
  	LCDUI_FONT_H24,
  	LCDUI_FONT_H24BOLD,
  	LCDUI_FONT_H36,
  	LCDUI_FONT_H36NUM,
  	LCDUI_FONT_H170NUM_LCD,
  } LCDUI_FONT_TYPE;

  typedef struct
  {
  	uint16_t	x_size;
  	uint16_t	y_size;
  } TSIZE;


  #define LCD_WR_16BITS_DATA(c) do{ LCD_WR_DATA(c); }while(0)

  //#define LCD_WR_16BITS_DATA(c) do{ LCD_WR_DATA(((c)>>8)&0xFF); LCD_WR_DATA((c)&0xFF); }while(0)
  /* RGB 24-bits color table definition (RGB888). */
  #define LCDUI_RGB(color) ((((color) >> 19) & 0x1f) << 11) | ((((color) >> 10) & 0x3f) << 5) | (((color) >> 3) & 0x1f)
  #define LCDUI_R_G_B(r, g, b) ((((r) >> 3) & 0x1f) << 11) | ((((g) >> 2) & 0x3f) << 5) | (((b) >> 3) & 0x1f)

// keyboard
#define KEYS	  LCDUI_RGB(0x935c29)
#define KEYSA	  LCDUI_RGB(0x478ccc)	// 

#define WHITE         0xFFFF
#define BLACK         0x0000
#define RED           0xF800
#define GREEN         0x0760
#define BLUE          0x00DF
#define CYAN          0x07FF
#define MAGENTA       0xF81F
#define YELLOW        0xFFE0
#define ORANGE        0xFB23
#define PURPLE        0x7817
#define LIME          0xBFE0
#define BROWN         0X9240
#define DARKBLUE      0X0030
#define DARKGREEN     0x0340
#define GRAY          0X8430
#define DARKGRAY      0x2124
#define TURQUOISE	  LCDUI_RGB(0x15A5A9)	// ���������
#define ORANGE_TEXT   0xFB23 	//0xf9b24d	// ��������� ���� ��� ������
#define TURN		  LCDUI_RGB(0xe64c66)	// ���� +/-

#define COLOR_LINE	  LCDUI_RGB(0x333333)	// ���� �������

#define MAT_RED       0xE124
#define MAT_YELLOW    0xED80
#define MAT_GREEN     0x254B
#define MAT_BLUE      0x24BD
#define MAT_ORANGE    0xF3A0
#define MAT_DARKGRAY  0x52AA
#define MAT_LOWWHITE  0xCE79
#define MAT_PURPLE    0x9135
#define MAT_SLATE     0x4B0D
#define MAT_DARKSLATE 0x2187
#define MAT_ASBESTOS  0x7C51
#define MAT_SILVER    0xB618
#define MAT_CONCRETE  0x9514
#define Col_DADA      0xDADA

extern unsigned char cp866_8x18_psf[256][8];
extern unsigned char cp866_8x14_psf[256][14];
extern unsigned char cp866_8x16_psf[][16];

uint32_t LCD_ReadPixel_24Bit(int16_t x, int16_t y);
void LCD_RefreshDirection(void);
void LCD_SetWindow(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey);
void LCD_Init(void);
void GUI_Clear(uint16_t color);
void LCD_Put_pixel(uint16_t x, uint16_t y, uint16_t color);
void Lcd_Line(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t c);
void Lcd_Put_Text(uint16_t x, uint16_t y, uint8_t height, const char *text, uint16_t color);
void Lcd_Fill_Rect(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
void Lcd_Render_Bitmap_8xN(uint16_t x, uint16_t y, uint8_t height, uint8_t *bitmap, uint16_t color);

void putChar(uint16_t x, uint16_t y, char c, uint16_t color, uint16_t bgcolor, uint8_t m);
void putStr(uint16_t x, uint16_t y, char *str, uint16_t color, uint16_t bgcolor, uint8_t m);

void Lcd_Render_Bitmap_16xX_Y(uint16_t x, uint16_t y, uint16_t height_X, uint16_t height_Y, uint16_t *bitmap);

void LCD_Clear(uint16_t color);

//-----------------------------------------------------------------------------------------------------------

#define	LCDUI_TEXT_ALIGN_RIGHT		(uint16_t)1 << 0
#define	LCDUI_TEXT_ALIGN_CENTER		(uint16_t)1 << 1
#define	LCDUI_TEXT_UNDERLINE		(uint16_t)1 << 2
#define	LCDUI_TEXT_OVERLINE			(uint16_t)1 << 3
#define	LCDUI_TEXT_TRANSBACK		(uint16_t)1 << 4
#define	LCDUI_TEXT_GETSIZE			(uint16_t)1 << 5
#define	LCDUI_TEXT_ALIGN_LEFT		(uint16_t)1 << 6

void LCDUI_DrawFastVLine(int16_t x, int16_t y, int16_t h);
void LCDUI_DrawFastHLine(int16_t x, int16_t y, int16_t w);
uint16_t	LCDUI_SetColor(uint16_t color);
uint16_t	LCDUI_SetBackColor(uint16_t color);
uint16_t	_lcdui_GetCharWidth(uint8_t c);
uint8_t*	_lcdui_GetCharData(uint8_t c);
LCDUI_FONT_TYPE		LCDUI_SetFont(LCDUI_FONT_TYPE newfont);
LCDUI_FONT_TYPE		LCDUI_GetCurrentFont();
uint32_t	LCDUI_GetTextWidth(char *str);
uint32_t	LCDUI_GetCurrentFontHeight();
void	LCDUI_DrawChar(char c,  uint16_t opt, int16_t x, int16_t y);
void	LCDUI_DrawText(char *str, uint16_t opt, int16_t x1, int16_t y1, int16_t x2, int16_t y2, TSIZE *tsize);

#ifdef __cplusplus
}
#endif

#endif
