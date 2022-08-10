#include <string.h>
#include "LCD_Init.h"
#include "lcd.h"
//#include "GPIO_Init.h"
// #include "includes.h"
// #include "trans_cpu.h"
//#include "stm32f2xx_hal.h"
// #include "unicode_utils.h"

extern LCDUI_FONT		font_fnt11;
extern LCDUI_FONT		font_fnt11bold;
extern LCDUI_FONT		font_fnt12;
extern LCDUI_FONT		font_fnt12bold;
extern LCDUI_FONT		font_fnt14;
//extern LCDUI_FONT		font_fnt14bold;
extern LCDUI_FONT		font_fnt18;
extern LCDUI_FONT		font_fnt18bold;
extern LCDUI_FONT		font_fnt24;
extern LCDUI_FONT		font_fnt24bold;
extern LCDUI_FONT		font_fnt36;
extern LCDUI_FONT		font_fnt36num;
//extern LCDUI_FONT		font_fnt170num_lcd;

int16_t			lcdui_cursor_x = 0, lcdui_cursor_y = 0;
LCDUI_FONT		*lcdui_current_font = &font_fnt36;
LCDUI_FONT_TYPE	lcdui_current_font_type = LCDUI_FONT_H36;
uint16_t		lcdui_bgcolor = LCDUI_RGB(0x000000), lcdui_color = LCDUI_RGB(0xFFFFFF);

#ifdef LCD_LED_PIN
void LCD_LED_On()
{
  #ifdef LCD_LED_PWM_CHANNEL
    Set_LCD_Brightness(100);
  #else
    GPIO_SetLevel(LCD_LED_PIN, 1);
  #endif
}
void LCD_LED_Off()
{
  #ifdef LCD_LED_PWM_CHANNEL
    Set_LCD_Brightness(0);
  #else
    GPIO_SetLevel(LCD_LED_PIN, 0);
  #endif
}

#ifdef LCD_LED_PWM_CHANNEL
LCD_AUTO_DIM lcd_dim = {0, 0};
const uint32_t LCD_BRIGHTNESS[ITEM_BRIGHTNESS_NUM] = {
  LCD_0_PERCENT,
  LCD_5_PERCENT,
  LCD_10_PERCENT,
  LCD_20_PERCENT,
  LCD_30_PERCENT,
  LCD_40_PERCENT,
  LCD_50_PERCENT,
  LCD_60_PERCENT,
  LCD_70_PERCENT,
  LCD_80_PERCENT,
  LCD_90_PERCENT,
  LCD_100_PERCENT
};

const LABEL itemDimTime[ITEM_SECONDS_NUM] = {
  //item value text(only for custom value)
  LABEL_OFF,
  LABEL_5_SECONDS,
  LABEL_10_SECONDS,
  LABEL_30_SECONDS,
  LABEL_60_SECONDS,
  LABEL_120_SECONDS,
  LABEL_300_SECONDS,
  LABEL_CUSTOM
};

const uint32_t LCD_DIM_IDLE_TIME[ITEM_SECONDS_NUM] = {
  LCD_DIM_OFF,
  LCD_DIM_5_SECONDS,
  LCD_DIM_10_SECONDS,
  LCD_DIM_30_SECONDS,
  LCD_DIM_60_SECONDS,
  LCD_DIM_120_SECONDS,
  LCD_DIM_300_SECONDS,
  LCD_DIM_CUSTOM_SECONDS
};

void loopDimTimer(void)
{
  if (infoSettings.lcd_idle_timer == LCD_DIM_OFF)
    return;

  if (isPress()
    #if LCD_ENCODER_SUPPORT
      || encoder_CheckState() || encoder_ReadBtn(LCD_BUTTON_INTERVALS)
    #endif
  )
  {
    if (lcd_dim.dimmed)
    {
      lcd_dim.dimmed = false;
      Set_LCD_Brightness(LCD_BRIGHTNESS[infoSettings.lcd_brightness]);
      #ifdef LED_COLOR_PIN
        if (infoSettings.knob_led_idle)
        {
          WS2812_Send_DAT(led_color[infoSettings.knob_led_color]);
        }
      #endif
    }
    lcd_dim.idle_ms = OS_GetTimeMs();
  }
  else
  {
    if (OS_GetTimeMs() - lcd_dim.idle_ms < (LCD_DIM_IDLE_TIME[infoSettings.lcd_idle_timer] * 1000))
      return;

    if (!lcd_dim.dimmed)
    {
      lcd_dim.dimmed = true;
      Set_LCD_Brightness(LCD_BRIGHTNESS[infoSettings.lcd_idle_brightness]);
      #ifdef LED_COLOR_PIN
        if (infoSettings.knob_led_idle)
        {
          WS2812_Send_DAT(led_color[LED_OFF]);
        }
      #endif
    }
  }
}

void _wakeLCD(void)
{
  if (infoSettings.lcd_idle_timer != LCD_DIM_OFF)
  {
    // The LCD dim function is activated. First check if it's dimmed
    if (lcd_dim.dimmed)
    {
      lcd_dim.dimmed = false;
      Set_LCD_Brightness(LCD_BRIGHTNESS[infoSettings.lcd_brightness]);
    }
    // Set a new idle_ms time
    lcd_dim.idle_ms = OS_GetTimeMs();
  }
}

#endif

void LCD_LED_Init(void)
{
  #ifdef LCD_LED_PWM_CHANNEL
    GPIO_InitSet(LCD_LED_PIN, MGPIO_MODE_AF_PP, LCD_LED_PIN_ALTERNATE);
    TIM_PWM_Init(LCD_LED_PWM_CHANNEL);
  #else
    LCD_LED_Off();
    GPIO_InitSet(LCD_LED_PIN, MGPIO_MODE_OUT_PP, 0);
  #endif
}
#endif

// LCD driver sequential

  #include "SSD1963.h"

void (*pLCD_SetDirection)(uint8_t rotate);
void (*pLCD_SetWindow)(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey);
void (*pLCD_SetWindowWH)(uint16_t sx, uint16_t sy, uint16_t w, uint16_t h);
#ifdef SCREEN_SHOT_TO_SD
  uint32_t (*pLCD_ReadPixel_24Bit)(int16_t x, int16_t y);

  uint32_t LCD_ReadPixel_24Bit(int16_t x, int16_t y)
  {
    return pLCD_ReadPixel_24Bit(x, y);
  }
#endif

void LCD_Init_Sequential(void)
{

    SSD1963_Init_Sequential();
    pLCD_SetDirection = SSD1963_SetDirection;
    pLCD_SetWindow = SSD1963_SetWindow;
    pLCD_SetWindowWH = SSD1963_SetWindowWH;
    #ifdef SCREEN_SHOT_TO_SD
      pLCD_ReadPixel_24Bit = SSD1963_ReadPixel_24Bit;
    #endif

}

void LCD_RefreshDirection(void)
{
  pLCD_SetDirection(0);
}

void LCD_SetWindow(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey)
{
  pLCD_SetWindow(sx, sy, ex, ey);
}

void LCD_SetWindowWH(uint16_t sx, uint16_t sy, uint16_t w, uint16_t h)
{
  pLCD_SetWindowWH(sx, sy, w, h);
}
 
void LCD_Clear(uint16_t color)
{
  uint32_t index=0;
  uint32_t W_H=LCD_WIDTH*LCD_HEIGHT;
  LCD_SetWindow(0, 0, LCD_WIDTH-1, LCD_HEIGHT-1);
  for (index=0; index<W_H; index++)
  {
    LCD_WR_16BITS_DATA(color);
  }
}

void Lcd_Fill_Rect(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color) {
	uint32_t index=0;
	uint32_t W_H=(((x2-x1)+1) * ((y2-y1)+1));
	LCD_SetWindow(x1, y1, x2, y2);
	//fill(color, ((x2+x1)) * (y2+y1));
	for (index=0; index<W_H; index++)
	 {
	   LCD_WR_16BITS_DATA(color);
	 }
}

void LCD_Put_pixel(uint16_t x, uint16_t y, uint16_t color){
	if((x >= LCD_WIDTH) || (y >= LCD_HEIGHT))
			return;
	LCD_SetWindow(x, y, x+1, y+1);
	LCD_WR_16BITS_DATA(color);
}

void	LCDUI_DrawFastVLine(int16_t x, int16_t y, int16_t h)
{
	if((x >= LCD_WIDTH) || (y >= LCD_HEIGHT))
		return;
	if((y+h) >= LCD_HEIGHT)
		h = LCD_HEIGHT-y;
	LCD_SetWindowWH(x, y, 1, h);
	while (h-- > 0)
		LCD_WR_16BITS_DATA(lcdui_color);
//	LCD_SetWindow(0, 0, LCD_WIDTH-1, LCD_HEIGHT-1);
}
//==============================================================================
void	LCDUI_DrawFastHLine(int16_t x, int16_t y, int16_t w)
{
	if((x >= LCD_WIDTH) || (y >= LCD_HEIGHT))
		return;
	if((x+w) >= LCD_WIDTH)
		w = LCD_WIDTH-x;
	LCD_SetWindowWH(x, y, w, 1);
	while (w-- > 0)
		LCD_WR_16BITS_DATA(lcdui_color);
//	LCD_SetWindow(0, 0, LCD_WIDTH-1, LCD_HEIGHT-1);
}
//==============================================================================

void Lcd_Line(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t c)
{
	int x, y, dx, dy, dx1, dy1, px, py, xe, ye, i;

	dx = x2 - x1;
	dy = y2 - y1;
	dx1 = dx > 0 ? dx : -dx;
	dy1 = dy > 0 ? dy : -dy;
	px = 2 * dy1 - dx1;
	py = 2 * dx1 - dy1;
	if (dy1 <= dx1) {
		if (dx >= 0) {
			x = x1;
			y = y1;
			xe = x2;
		} else {
			x = x2;
			y = y2;
			xe = x1;
		}
		LCD_Put_pixel(x, y, c);
		for (i = 0; x < xe; i++) {
			x = x + 1;
			if (px < 0) {
				px = px + 2 * dy1;
			} else {
				if ((dx < 0 && dy < 0) || (dx > 0 && dy > 0)) {
					y = y + 1;
				} else {
					y = y - 1;
				}
				px = px + 2 * (dy1 - dx1);
			}
			LCD_Put_pixel(x, y, c);
		}
	} else {
		if (dy >= 0) {
			x = x1;
			y = y1;
			ye = y2;
		} else {
			x = x2;
			y = y2;
			ye = y1;
		}
		LCD_Put_pixel(x, y, c);
		for (i = 0; y < ye; i++) {
			y = y + 1;
			if (py <= 0) {
				py = py + 2 * dx1;
			} else {
				if ((dx < 0 && dy < 0) || (dx > 0 && dy > 0)) {
					x = x + 1;
				} else {
					x = x - 1;
				}
				py = py + 2 * (dx1 - dy1);
			}
			LCD_Put_pixel(x, y, c);
		}
	}
}

void Lcd_Render_Bitmap_8xN(uint16_t x, uint16_t y, uint8_t height, uint8_t *bitmap, uint16_t color)
{
	for (int y1=0; y1 < height; y1++) {

		if ((y1 + y < LCD_HEIGHT) && bitmap[y1]) {

    	    for (int x1=0; x1<8; x1++) {

    	    	if ((x1 + x < LCD_WIDTH) && (bitmap[y1] & 1 << x1)) {

    	    		LCD_Put_pixel (x + x1, y + y1, color);
				}
			}
    	}
    }
}

void Lcd_Render_Bitmap_16xX_Y(uint16_t x, uint16_t y, uint16_t height_X, uint16_t height_Y, uint16_t *bitmap)
{
	for (int y1=0; y1 < height_Y; y1++)
		for (int x1=0; x1<height_X; x1++)
		{
    	    if(bitmap[y1*height_X+x1]>0x0030)LCD_Put_pixel(x+x1, y+y1,  bitmap[y1*height_X+x1]);
		}

}

static inline char _R( const char c ) {
    if ((uint8_t) c > 239)
            return (char) ((uint8_t) c - 16);
    if ((uint8_t) c > 191)
            return (char) ((uint8_t) c - 64);

    return c;
}
void Lcd_Put_Text(uint16_t x, uint16_t y, uint8_t height, const char *text, uint16_t color) {

	uint8_t *bitmap;

	for(; *text; x += 8, text++) {
		int idx = _R(* (uint8_t *) text);

		switch (height) {
		//case 14: bitmap = cp866_8x14_psf[idx]; break;
		case 16: bitmap = cp866_8x16_psf[idx]; break;
		//case 8:	 bitmap = cp866_8x8_psf[idx];  break;
		default: return;
			break;
		}
		Lcd_Render_Bitmap_8xN(x, y, height, bitmap, color);
	}
}


#define CHAR_H 16
#define CHAR_W 8

void putChar(uint16_t x, uint16_t y, char c, uint16_t color, uint16_t bgcolor, uint8_t m)
{
	uint8_t h,ch,p,mask,tt;

	if ((uint8_t) c == 184) c = (char)(241);
	else if (((uint8_t) c == 168)) c = (char)(240);
	else {
		if ((uint8_t) c > 239)
	       c= (char) ((uint8_t) c - 16);
		else if ((uint8_t) c > 191)
	       c= (char) ((uint8_t) c - 64);
	}

	  LCD_SetWindow(x, y, x+CHAR_W*m-1, y+CHAR_H*m-1);

	  for (h=CHAR_H; h>0; h--) // every column of the character
	  {
	      for(tt=0;tt<m;tt++)
	      {
	        ch=cp866_8x16_psf[(uint8_t)c][CHAR_H-h];
	        mask=0x01;
	        for (p=0; p<CHAR_W; p++)  // write the pixels

	          {
	          if (ch&mask)
	            {
	        	  switch (m) {
					case 1:
						LCD_WR_16BITS_DATA(color);
						break;
					case 2:
						LCD_WR_16BITS_DATA(color);
						LCD_WR_16BITS_DATA(color);
						break;
					case 3:
						LCD_WR_16BITS_DATA(color);
						LCD_WR_16BITS_DATA(color);
						LCD_WR_16BITS_DATA(color);
						break;
					case 4:
						LCD_WR_16BITS_DATA(color);
						LCD_WR_16BITS_DATA(color);
						LCD_WR_16BITS_DATA(color);
						LCD_WR_16BITS_DATA(color);
						break;
					case 5:
						LCD_WR_16BITS_DATA(color);
						LCD_WR_16BITS_DATA(color);
						LCD_WR_16BITS_DATA(color);
						LCD_WR_16BITS_DATA(color);
						LCD_WR_16BITS_DATA(color);
						break;
					default:
						break;
	        	  }

	            }
	           else
	            {
				  switch (m) {
					case 1:
						LCD_WR_16BITS_DATA(bgcolor);
						break;
					case 2:
						LCD_WR_16BITS_DATA(bgcolor);
						LCD_WR_16BITS_DATA(bgcolor);
						break;
					case 3:
						LCD_WR_16BITS_DATA(bgcolor);
						LCD_WR_16BITS_DATA(bgcolor);
						LCD_WR_16BITS_DATA(bgcolor);
						break;
					case 4:
						LCD_WR_16BITS_DATA(bgcolor);
						LCD_WR_16BITS_DATA(bgcolor);
						LCD_WR_16BITS_DATA(bgcolor);
						LCD_WR_16BITS_DATA(bgcolor);
						break;
					case 5:
						LCD_WR_16BITS_DATA(bgcolor);
						LCD_WR_16BITS_DATA(bgcolor);
						LCD_WR_16BITS_DATA(bgcolor);
						LCD_WR_16BITS_DATA(bgcolor);
						LCD_WR_16BITS_DATA(bgcolor);
						break;
					default:
						break;
				  }
	            }
	          mask=mask*2;
	          }
	      }
	  }

}

void putStr(uint16_t x, uint16_t y, char *str, uint16_t color, uint16_t bgcolor, uint8_t m)
{
	unsigned char  j=0;
	while (j<strlen(str))
	{
		putChar(x+j*CHAR_W*m,y,str[j],color, bgcolor,m);   //  0 deg. rotated str[j]
		j++;
	}
}

void LCD_Init(void)
{
  LCD_Init_Sequential();
}

uint16_t	LCDUI_SetColor(uint16_t color)
{
	uint16_t oldcolor = lcdui_color;
	lcdui_color = color;
	return oldcolor;
}
//==============================================================================

uint16_t	LCDUI_SetBackColor(uint16_t color)
{
	uint16_t oldcolor = lcdui_bgcolor;
	lcdui_bgcolor = color;
	return oldcolor;
}
//==============================================================================

uint16_t	_lcdui_GetCharWidth(uint8_t c)
{
	if (c < 32)
		return 0;
	if (c > 126)
		c -= 65;
	c -= 32;
	if (c >= lcdui_current_font->symcount)
		return 0;
	uint16_t res = lcdui_current_font->width[c];
	if (res & 0x8000)
		return lcdui_current_font->width[(res & 0x7FFF)];
	return res;
}
//==============================================================================
uint8_t*	_lcdui_GetCharData(uint8_t c)
{
	if (c < 32)
		return 0;
	if (c > 126)
		c -= 65;
	c -= 32;
	if (c >= lcdui_current_font->symcount)
		return 0;
	uint16_t c1 = lcdui_current_font->width[c];
	if (c1 & 0x8000)
		c = (c1 & 0x7FFF);
	uint16_t ch = lcdui_current_font->height;
	int32_t i = 0, ptr = 0, bits = 0, line_bits = ch;
	for (i = 0; i < c; i++)
	{
		if (lcdui_current_font->width[i] & 0x8000)
			continue;
		bits = lcdui_current_font->width[i] * line_bits;
		ptr += bits >> 3;
		if (bits & 0x07)
			ptr++;
	}

	return &(lcdui_current_font->data[ptr]);
}
//==============================================================================
LCDUI_FONT_TYPE		LCDUI_SetFont(LCDUI_FONT_TYPE newfont)
{
	switch (newfont)
	{
		case LCDUI_FONT_H11:
			lcdui_current_font = &font_fnt11;
			break;

		case LCDUI_FONT_H11BOLD:
			//lcdui_current_font = &font_fnt11bold;
			break;

		case LCDUI_FONT_H12:
			lcdui_current_font = &font_fnt12;
			break;

		case LCDUI_FONT_H12BOLD:
			//lcdui_current_font = &font_fnt12bold;
			break;

		case LCDUI_FONT_H14:
			lcdui_current_font = &font_fnt14;
			break;

		case LCDUI_FONT_H14BOLD:
			//lcdui_current_font = &font_fnt14bold;
			break;

		case LCDUI_FONT_H18:
			lcdui_current_font = &font_fnt18;
			break;

		case LCDUI_FONT_H18BOLD:
			//lcdui_current_font = &font_fnt18bold;
			break;

		case LCDUI_FONT_H24:
			lcdui_current_font = &font_fnt24;
			break;

		case LCDUI_FONT_H24BOLD:
			//lcdui_current_font = &font_fnt24bold;
			break;

		case LCDUI_FONT_H36:
			lcdui_current_font = &font_fnt36;
			break;

		case LCDUI_FONT_H36NUM:
			//lcdui_current_font = &font_fnt36num;
			break;

		case LCDUI_FONT_H170NUM_LCD:
			//lcdui_current_font = &font_fnt170num_lcd;
			break;

		default:
			return lcdui_current_font_type;
	}
	LCDUI_FONT_TYPE oldfont = lcdui_current_font_type;
	lcdui_current_font_type = newfont;
	return oldfont;
}
//==============================================================================

LCDUI_FONT_TYPE		LCDUI_GetCurrentFont()
{
	return lcdui_current_font_type;
}
//==================================
// uint32_t	LCDUI_GetTextWidth(char *str)
// {
// 	uint32_t i = 0, res = 0;
// 	char c;
// 	while(str[i])
// 	{
// 		c =  UTF8toANSI(str+i);
// 		if (c > 31)
// 			res += _lcdui_GetCharWidth(c);
// 		if (str[i] < 0x80)
// 			i++;
// 		else
// 			i += 2;
// 	}
// 	return res;
// }
//==============================================================================
uint32_t	LCDUI_GetCurrentFontHeight()
{
	return lcdui_current_font->height;
}
//==============================================================================
void	LCDUI_DrawChar(char c,  uint16_t opt, int16_t x, int16_t y)
{
	if((x > LCD_WIDTH) || (y > LCD_HEIGHT) || c < 32)
	{
		return;
	}
	if (x >= 0)
		lcdui_cursor_x = x;
	if (y >= 0)
		lcdui_cursor_y = y;
	if((lcdui_cursor_x > LCD_WIDTH) || (lcdui_cursor_y > LCD_HEIGHT))
		return;

	uint16_t	cres[2];

	cres[1] = lcdui_color;
	cres[0] = lcdui_bgcolor;
	uint16_t	cw = 0, ch = lcdui_current_font->height;
	uint16_t	i = 0, ptr = 0;
	cw = _lcdui_GetCharWidth(c);
	if (cw == 0)
		return;
	uint8_t *data = _lcdui_GetCharData(c);
	if (data == 0)
		return;

	c = data[ptr++];
	uint8_t 	sh = 0, csh = 0;
	uint16_t	xc = lcdui_cursor_x, yc = lcdui_cursor_y;

//	LCD_SetCursor(lcdui_cursor_x, lcdui_cursor_y);
	//LCD_SetWindows(lcdui_cursor_x, lcdui_cursor_y, cw, ch);
	//LCD_WriteRAM_Prepare();
	LCD_SetWindowWH(lcdui_cursor_x, lcdui_cursor_y, cw, ch);

	if (opt & LCDUI_TEXT_TRANSBACK)
	{
		for (i = 0; i < cw*ch; i++)
		{
			csh = (c >> sh);
			if (csh&0x01)
			{
				//LCD_SetCursor(xc, yc);
				//LCD_WriteRAM_Prepare();
				//LCD_WriteRAM(lcdui_color);
				LCD_SetWindow(xc, yc, xc, yc);
				LCD_WR_16BITS_DATA(lcdui_color);
			}
			xc++;
			if (xc == lcdui_cursor_x+cw)
			{
				xc = lcdui_cursor_x;
				yc++;
			}

			sh ++;
			if (sh == 8)
			{
				c = data[ptr++];
				sh = 0;
			}
		}
	}
	else
	{
		for (i = 0; i < cw*ch; i++)
		{
			csh = (c >> sh);
			//LCD_WriteRAM(cres[(csh&0x01)]);
			LCD_WR_16BITS_DATA(cres[(csh&0x01)]);
			sh ++;
			if (sh == 8)
			{
				c = data[ptr++];
				sh = 0;
			}
		}
	}
	lcdui_cursor_x += cw;
}
//==============================================================================
void	LCDUI_DrawText(char *str, uint16_t opt, int16_t x1, int16_t y1, int16_t x2, int16_t y2, TSIZE *tsize)
{
	uint16_t i = 0, oldcolor = lcdui_color, oldbgcolor = lcdui_bgcolor, copt = opt;
	char c;
	if (x1 > LCD_WIDTH-1 || y1 > LCD_HEIGHT-1)
		return;
	if (x1 < 0)
		x1 = lcdui_cursor_x;
	if (y1 < 0)
		y1 = lcdui_cursor_y;
	if (x2 < 0)
		x2 = LCD_WIDTH-1;
	if (y2 < 0)
		y2 = LCD_HEIGHT-1;
	if ((opt & LCDUI_TEXT_GETSIZE) == 0)
	{
		lcdui_cursor_x = x1;
		lcdui_cursor_y = y1;
	}



	char *cp = (char*)str;
	int16_t sp = 0, cw = 0, sw = 0;

	while(1)
	{
		c = cp[i];//UTF8toANSI(cp+i);
		if (c == 0 || c == ' ')
		{
			sp = i;
			sw = cw;
		}
		cw += _lcdui_GetCharWidth(c);
		if (c == '\n' || cw > x2-x1 || c == 0)
		{
			if (cw > x2-x1)
				cw = sw;
			if (sp == 0)
				sp = i;
			if (c == '\n')
				sp = i;
			if (tsize != NULL && tsize->x_size < cw)
				tsize->x_size = cw;
			if (tsize != NULL && cw > 0)
				tsize->y_size += lcdui_current_font->height;

			if ((opt & LCDUI_TEXT_GETSIZE) == 0)
			{
				if (opt & LCDUI_TEXT_ALIGN_RIGHT)
					lcdui_cursor_x = x2 - cw;
				if (opt & LCDUI_TEXT_ALIGN_CENTER)
					lcdui_cursor_x = x1 + (x2 - x1 - cw) / 2;

				for (uint16_t j = 0; j < sp; )
				{
					LCDUI_DrawChar(cp[j]/*UTF8toANSI(cp+j)*/, copt, -1, -1);
					j++;
					/*if ((*(cp+j) & 0x80)==0){
						j++;
						//j++;
					}
					else
						j += 2;*/
				}
			}
			cw = 0;
			sw = 0;
			cp += sp;
			i = 0;
			sp = 0;
			while(/*UTF8toANSI(cp)*/cp[0] == ' ')
			{
				cp++;
				/*if ((*cp & 0x80)==0){
					cp++;
				}
				else
					cp += 2;*/
			}
			if (c != 0 && (opt & LCDUI_TEXT_GETSIZE) == 0)
			{
				lcdui_cursor_y += lcdui_current_font->height;
				lcdui_cursor_x = x1;
			}
			if (c == 0 || lcdui_cursor_y+lcdui_current_font->height > y2)
				break;
			if (c == '\n')
				i++;;
			continue;
		}
		i++;
		/*if ((*(cp+i) & 0x80)==0){
			i++;
		}
		else
			i += 2;*/
		if (c == '.' || c == ',' || c == ':' || c == ';' || c == '!' || c == '?' || c == '_' || c == '-' || c == '(' || c == ')' || c == '_')
		{
//			cw += _lcdui_GetCharWidth(c);
			sp = i;
			sw = cw;
		}

	}

	lcdui_bgcolor = oldbgcolor;
	lcdui_color = oldcolor;
}
//==============================================================================
// void	LCDUI_DrawCharUTF(char *c,  uint16_t opt, int16_t x, int16_t y)
// {
// 	char cc = UTF8toANSI(c);
// 	LCDUI_DrawChar(cc, opt, x, y);
// }
//==============================================================================
