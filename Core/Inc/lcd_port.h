#ifndef __LCD_PORT_H__
#define __LCD_PORT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define LCD_PORT_WIDTH   240U
#define LCD_PORT_HEIGHT  280U
#define LCD_PORT_X_OFFSET 0U
#define LCD_PORT_Y_OFFSET 20U

#define LCD_COLOR_BLACK   0x0000U
#define LCD_COLOR_WHITE   0xFFFFU
#define LCD_COLOR_RED     0xF800U
#define LCD_COLOR_GREEN   0x07E0U
#define LCD_COLOR_BLUE    0x001FU
#define LCD_COLOR_YELLOW  0xFFE0U

void LCD_Port_Init(void);
void LCD_Port_SetBacklight(uint8_t on);
void LCD_ClearRGB565(uint16_t color);
void LCD_FillRectRGB565(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color);
void LCD_DrawPixelRGB565(uint16_t x, uint16_t y, uint16_t color);
void LCD_WriteRectRGB565(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const uint16_t* pixels);
void LCD_WriteRectRGB565Strided(uint16_t x,
                                uint16_t y,
                                uint16_t width,
                                uint16_t height,
                                const uint16_t* pixels,
                                uint16_t stridePixels);
void LCD_WriteLandscapeRGB565StridedClockwise(int16_t x,
                                              int16_t y,
                                              int16_t width,
                                              int16_t height,
                                              const uint16_t* framebuffer,
                                              uint16_t stridePixels);
void LCD_DrawCenteredText(const char* text, uint16_t color, uint16_t bg_color, uint8_t scale);

#ifdef __cplusplus
}
#endif

#endif /* __LCD_PORT_H__ */
