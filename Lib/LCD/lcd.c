#include "lcd.h"

uint16_t LCD_RD_DATA(void)
{
  volatile uint16_t ram;
  ram = LCD->LCD_RAM;
  return ram;
}

void LCD_WriteReg(uint8_t LCD_Reg, uint16_t LCD_RegValue)
{
  LCD->LCD_REG = LCD_Reg;
  LCD->LCD_RAM = LCD_RegValue;
}

// uint16_t LCD_ReadReg(uint8_t LCD_Reg)
// {
//   LCD_WR_REG(LCD_Reg);
//   methods.delay_1ms(5);
//   return LCD_RD_DATA();
// }

