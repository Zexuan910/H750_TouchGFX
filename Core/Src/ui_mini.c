#include "ui.h"

#include "lcd_port.h"

void UI_Init(void)
{
  LCD_Port_Init();
  LCD_ClearRGB565(LCD_COLOR_BLACK);
  LCD_DrawCenteredText("Hello TouchGFX", LCD_COLOR_WHITE, LCD_COLOR_BLACK, 2U);
}

void UI_Process(void)
{
}
