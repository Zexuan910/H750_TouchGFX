#include "ui.h"

#include "app_touchgfx.h"
#include "lcd_port.h"

void UI_TouchGFX_SignalVSync(void);

void UI_Init(void)
{
  LCD_Port_Init();
  MX_TouchGFX_Init();
}

void UI_Process(void)
{
  UI_TouchGFX_SignalVSync();
  MX_TouchGFX_Process();
}
