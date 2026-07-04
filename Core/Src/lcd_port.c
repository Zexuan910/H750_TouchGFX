#include "lcd_port.h"

#include "main.h"
#include "spi.h"
#include "tim.h"

#include <stddef.h>
#include <string.h>

#define LCD_SPI_TIMEOUT_MS 1000U
#define LCD_TX_PIXELS      240U
#define LCD_TX_BYTES       (LCD_TX_PIXELS * 2U)
#define LCD_USE_SPI_DMA    0U
#define LCD_DMA_MIN_BYTES  32U

__attribute__((section("LCD_DMA_Buffer"), aligned(32))) static uint8_t lcd_dma_tx_buffer[LCD_TX_BYTES];
static volatile uint8_t lcd_spi_dma_done = 1U;
static volatile uint8_t lcd_spi_dma_error = 0U;

static void lcd_select(void)
{
  HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_RESET);
}

static void lcd_deselect(void)
{
  HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_SET);
}

static void lcd_cmd_mode(void)
{
  HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_RESET);
}

static void lcd_data_mode(void)
{
  HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_SET);
}

static void lcd_write_bytes(const uint8_t* data, uint16_t size)
{
  if ((data == NULL) || (size == 0U))
  {
    return;
  }

#if LCD_USE_SPI_DMA
  if ((size < LCD_DMA_MIN_BYTES) || (size > LCD_TX_BYTES))
  {
    (void)HAL_SPI_Transmit(&hspi1, (uint8_t*)data, size, LCD_SPI_TIMEOUT_MS);
    return;
  }

  if (data != lcd_dma_tx_buffer)
  {
    memcpy(lcd_dma_tx_buffer, data, size);
  }

  SCB_CleanDCache_by_Addr((uint32_t*)lcd_dma_tx_buffer, (int32_t)((size + 31U) & ~31U));

  lcd_spi_dma_done = 0U;
  lcd_spi_dma_error = 0U;
  if (HAL_SPI_Transmit_DMA(&hspi1, lcd_dma_tx_buffer, size) != HAL_OK)
  {
    (void)HAL_SPI_Transmit(&hspi1, lcd_dma_tx_buffer, size, LCD_SPI_TIMEOUT_MS);
    return;
  }

  const uint32_t startTick = HAL_GetTick();
  while ((lcd_spi_dma_done == 0U) && (lcd_spi_dma_error == 0U))
  {
    if ((HAL_GetTick() - startTick) > LCD_SPI_TIMEOUT_MS)
    {
      (void)HAL_SPI_Abort(&hspi1);
      lcd_spi_dma_error = 1U;
      break;
    }
  }
#else
  (void)HAL_SPI_Transmit(&hspi1, (uint8_t*)data, size, LCD_SPI_TIMEOUT_MS);
#endif
}

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef* hspi)
{
  if (hspi->Instance == SPI1)
  {
    lcd_spi_dma_done = 1U;
  }
}

void HAL_SPI_ErrorCallback(SPI_HandleTypeDef* hspi)
{
  if (hspi->Instance == SPI1)
  {
    lcd_spi_dma_error = 1U;
    lcd_spi_dma_done = 1U;
  }
}

static void lcd_write_command(uint8_t command)
{
  lcd_cmd_mode();
  lcd_write_bytes(&command, 1U);
}

static void lcd_write_data8(uint8_t data)
{
  lcd_data_mode();
  lcd_write_bytes(&data, 1U);
}

static void lcd_write_data16(uint16_t data)
{
  uint8_t bytes[2];

  bytes[0] = (uint8_t)(data >> 8);
  bytes[1] = (uint8_t)(data & 0xFFU);
  lcd_data_mode();
  lcd_write_bytes(bytes, sizeof(bytes));
}

static void lcd_reset(void)
{
  HAL_GPIO_WritePin(LCD_RST_GPIO_Port, LCD_RST_Pin, GPIO_PIN_SET);
  HAL_Delay(100U);
  HAL_GPIO_WritePin(LCD_RST_GPIO_Port, LCD_RST_Pin, GPIO_PIN_RESET);
  HAL_Delay(100U);
  HAL_GPIO_WritePin(LCD_RST_GPIO_Port, LCD_RST_Pin, GPIO_PIN_SET);
  HAL_Delay(100U);
}

static void lcd_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
  x0 = (uint16_t)(x0 + LCD_PORT_X_OFFSET);
  x1 = (uint16_t)(x1 + LCD_PORT_X_OFFSET);
  y0 = (uint16_t)(y0 + LCD_PORT_Y_OFFSET);
  y1 = (uint16_t)(y1 + LCD_PORT_Y_OFFSET);

  lcd_write_command(0x2AU);
  lcd_write_data16(x0);
  lcd_write_data16(x1);

  lcd_write_command(0x2BU);
  lcd_write_data16(y0);
  lcd_write_data16(y1);

  lcd_write_command(0x2CU);
}

static void lcd_write_init_sequence(void)
{
  /* Initialization sequence aligned with SCREEN_COLOR/Core/Src/LCD_1in69.c. */
  lcd_write_command(0x36U);
  lcd_write_data8(0x00U);

  lcd_write_command(0x3AU);
  lcd_write_data8(0x05U);

  lcd_write_command(0xB2U);
  lcd_write_data8(0x0BU);
  lcd_write_data8(0x0BU);
  lcd_write_data8(0x00U);
  lcd_write_data8(0x33U);
  lcd_write_data8(0x35U);

  lcd_write_command(0xB7U);
  lcd_write_data8(0x11U);

  lcd_write_command(0xBBU);
  lcd_write_data8(0x35U);

  lcd_write_command(0xC0U);
  lcd_write_data8(0x2CU);

  lcd_write_command(0xC2U);
  lcd_write_data8(0x01U);

  lcd_write_command(0xC3U);
  lcd_write_data8(0x0DU);

  lcd_write_command(0xC4U);
  lcd_write_data8(0x20U);

  lcd_write_command(0xC6U);
  lcd_write_data8(0x13U);

  lcd_write_command(0xD0U);
  lcd_write_data8(0xA4U);
  lcd_write_data8(0xA1U);

  lcd_write_command(0xD6U);
  lcd_write_data8(0xA1U);

  lcd_write_command(0xE0U);
  lcd_write_data8(0xF0U);
  lcd_write_data8(0x06U);
  lcd_write_data8(0x0BU);
  lcd_write_data8(0x0AU);
  lcd_write_data8(0x09U);
  lcd_write_data8(0x26U);
  lcd_write_data8(0x29U);
  lcd_write_data8(0x33U);
  lcd_write_data8(0x41U);
  lcd_write_data8(0x18U);
  lcd_write_data8(0x16U);
  lcd_write_data8(0x15U);
  lcd_write_data8(0x29U);
  lcd_write_data8(0x2DU);

  lcd_write_command(0xE1U);
  lcd_write_data8(0xF0U);
  lcd_write_data8(0x04U);
  lcd_write_data8(0x08U);
  lcd_write_data8(0x08U);
  lcd_write_data8(0x07U);
  lcd_write_data8(0x03U);
  lcd_write_data8(0x28U);
  lcd_write_data8(0x32U);
  lcd_write_data8(0x40U);
  lcd_write_data8(0x3BU);
  lcd_write_data8(0x19U);
  lcd_write_data8(0x18U);
  lcd_write_data8(0x2AU);
  lcd_write_data8(0x2EU);

  lcd_write_command(0xE4U);
  lcd_write_data8(0x25U);
  lcd_write_data8(0x00U);
  lcd_write_data8(0x00U);

  lcd_write_command(0x21U);

  lcd_write_command(0x11U);
  HAL_Delay(120U);

  lcd_write_command(0x29U);
  HAL_Delay(20U);
}

static const uint8_t* lcd_glyph_for(char ch)
{
  static const uint8_t glyph_space[7] = {0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U};
  static const uint8_t glyph_c[7] = {0x00U, 0x00U, 0x0EU, 0x10U, 0x10U, 0x0EU, 0x00U};
  static const uint8_t glyph_e[7] = {0x00U, 0x0EU, 0x11U, 0x1FU, 0x10U, 0x0EU, 0x00U};
  static const uint8_t glyph_h[7] = {0x10U, 0x10U, 0x16U, 0x19U, 0x11U, 0x11U, 0x00U};
  static const uint8_t glyph_l[7] = {0x0CU, 0x04U, 0x04U, 0x04U, 0x04U, 0x0EU, 0x00U};
  static const uint8_t glyph_o[7] = {0x00U, 0x0EU, 0x11U, 0x11U, 0x11U, 0x0EU, 0x00U};
  static const uint8_t glyph_u[7] = {0x00U, 0x11U, 0x11U, 0x11U, 0x13U, 0x0DU, 0x00U};
  static const uint8_t glyph_F[7] = {0x1FU, 0x10U, 0x10U, 0x1EU, 0x10U, 0x10U, 0x00U};
  static const uint8_t glyph_G[7] = {0x0EU, 0x11U, 0x10U, 0x17U, 0x11U, 0x0EU, 0x00U};
  static const uint8_t glyph_H[7] = {0x11U, 0x11U, 0x11U, 0x1FU, 0x11U, 0x11U, 0x00U};
  static const uint8_t glyph_T[7] = {0x1FU, 0x04U, 0x04U, 0x04U, 0x04U, 0x04U, 0x00U};
  static const uint8_t glyph_X[7] = {0x11U, 0x0AU, 0x04U, 0x04U, 0x0AU, 0x11U, 0x00U};

  switch (ch)
  {
  case 'c':
    return glyph_c;
  case 'e':
    return glyph_e;
  case 'h':
    return glyph_h;
  case 'l':
    return glyph_l;
  case 'o':
    return glyph_o;
  case 'u':
    return glyph_u;
  case 'F':
    return glyph_F;
  case 'G':
    return glyph_G;
  case 'H':
    return glyph_H;
  case 'T':
    return glyph_T;
  case 'X':
    return glyph_X;
  case ' ':
  default:
    return glyph_space;
  }
}

static void lcd_draw_char(uint16_t x, uint16_t y, char ch, uint16_t color, uint16_t bg_color, uint8_t scale)
{
  const uint8_t* glyph = lcd_glyph_for(ch);
  uint8_t row;
  uint8_t col;

  if (scale == 0U)
  {
    scale = 1U;
  }

  for (row = 0U; row < 7U; row++)
  {
    for (col = 0U; col < 5U; col++)
    {
      uint16_t pixel_color = (glyph[row] & (uint8_t)(1U << (4U - col))) ? color : bg_color;
      LCD_FillRectRGB565((uint16_t)(x + (uint16_t)col * scale),
                         (uint16_t)(y + (uint16_t)row * scale),
                         scale,
                         scale,
                         pixel_color);
    }
  }
}

void LCD_Port_Init(void)
{
  LCD_Port_SetBacklight(1U);

  HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(LCD_RST_GPIO_Port, LCD_RST_Pin, GPIO_PIN_SET);

  lcd_reset();
  lcd_select();
  lcd_write_init_sequence();
  lcd_deselect();
}

void LCD_Port_SetBacklight(uint8_t on)
{
  static uint8_t pwm_started = 0U;
  uint32_t pulse;

  if (pwm_started == 0U)
  {
    if (HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1) != HAL_OK)
    {
      Error_Handler();
    }
    pwm_started = 1U;
  }

  pulse = on ? __HAL_TIM_GET_AUTORELOAD(&htim1) : 0U;
  __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, pulse);
}

void LCD_ClearRGB565(uint16_t color)
{
  LCD_FillRectRGB565(0U, 0U, LCD_PORT_WIDTH, LCD_PORT_HEIGHT, color);
}

void LCD_FillRectRGB565(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color)
{
  uint32_t remaining;
  uint8_t* buffer = lcd_dma_tx_buffer;
  uint16_t i;

  if ((x >= LCD_PORT_WIDTH) || (y >= LCD_PORT_HEIGHT) || (width == 0U) || (height == 0U))
  {
    return;
  }

  if ((uint32_t)x + width > LCD_PORT_WIDTH)
  {
    width = (uint16_t)(LCD_PORT_WIDTH - x);
  }
  if ((uint32_t)y + height > LCD_PORT_HEIGHT)
  {
    height = (uint16_t)(LCD_PORT_HEIGHT - y);
  }

  for (i = 0U; i < LCD_TX_PIXELS; i++)
  {
    buffer[(uint16_t)i * 2U] = (uint8_t)(color >> 8);
    buffer[(uint16_t)i * 2U + 1U] = (uint8_t)(color & 0xFFU);
  }

  remaining = (uint32_t)width * height;
  lcd_select();
  lcd_set_window(x, y, (uint16_t)(x + width - 1U), (uint16_t)(y + height - 1U));
  lcd_data_mode();

  while (remaining > 0U)
  {
    uint16_t chunk = (remaining > LCD_TX_PIXELS) ? LCD_TX_PIXELS : (uint16_t)remaining;
    lcd_write_bytes(buffer, (uint16_t)(chunk * 2U));
    remaining -= chunk;
  }

  lcd_deselect();
}

void LCD_DrawPixelRGB565(uint16_t x, uint16_t y, uint16_t color)
{
  LCD_FillRectRGB565(x, y, 1U, 1U, color);
}

void LCD_WriteRectRGB565(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const uint16_t* pixels)
{
  LCD_WriteRectRGB565Strided(x, y, width, height, pixels, width);
}

void LCD_WriteRectRGB565Strided(uint16_t x,
                                uint16_t y,
                                uint16_t width,
                                uint16_t height,
                                const uint16_t* pixels,
                                uint16_t stridePixels)
{
  uint8_t* buffer = lcd_dma_tx_buffer;
  uint16_t row;

  if ((pixels == NULL) || (stridePixels < width) || (x >= LCD_PORT_WIDTH) || (y >= LCD_PORT_HEIGHT) || (width == 0U) || (height == 0U))
  {
    return;
  }

  if ((uint32_t)x + width > LCD_PORT_WIDTH)
  {
    width = (uint16_t)(LCD_PORT_WIDTH - x);
  }
  if ((uint32_t)y + height > LCD_PORT_HEIGHT)
  {
    height = (uint16_t)(LCD_PORT_HEIGHT - y);
  }

  lcd_select();
  lcd_set_window(x, y, (uint16_t)(x + width - 1U), (uint16_t)(y + height - 1U));
  lcd_data_mode();

  for (row = 0U; row < height; row++)
  {
    const uint16_t* rowPixels = pixels + (uint32_t)row * stridePixels;
    uint16_t sent = 0U;

    while (sent < width)
    {
      uint16_t chunk = ((uint16_t)(width - sent) > LCD_TX_PIXELS) ? LCD_TX_PIXELS : (uint16_t)(width - sent);
      uint16_t i;

      for (i = 0U; i < chunk; i++)
      {
        uint16_t pixel = rowPixels[sent + i];
        buffer[(uint16_t)i * 2U] = (uint8_t)(pixel >> 8);
        buffer[(uint16_t)i * 2U + 1U] = (uint8_t)(pixel & 0xFFU);
      }

      lcd_write_bytes(buffer, (uint16_t)(chunk * 2U));
      sent = (uint16_t)(sent + chunk);
    }
  }

  lcd_deselect();
}

void LCD_WriteLandscapeRGB565StridedClockwise(int16_t x,
                                              int16_t y,
                                              int16_t width,
                                              int16_t height,
                                              const uint16_t* framebuffer,
                                              uint16_t stridePixels)
{
  uint8_t* buffer = lcd_dma_tx_buffer;
  int16_t clippedX = x;
  int16_t clippedY = y;
  int16_t clippedWidth = width;
  int16_t clippedHeight = height;
  uint16_t physicalX;
  uint16_t physicalY;
  uint16_t physicalWidth;
  uint16_t physicalHeight;
  uint16_t row;
  uint8_t framebufferIsLogicalLandscape;

  if ((framebuffer == NULL) || (width <= 0) || (height <= 0) || (stridePixels < LCD_PORT_WIDTH))
  {
    return;
  }

  framebufferIsLogicalLandscape = (stridePixels >= LCD_PORT_HEIGHT) ? 1U : 0U;

  if (clippedX < 0)
  {
    clippedWidth = (int16_t)(clippedWidth + clippedX);
    clippedX = 0;
  }
  if (clippedY < 0)
  {
    clippedHeight = (int16_t)(clippedHeight + clippedY);
    clippedY = 0;
  }
  if ((clippedX >= (int16_t)LCD_PORT_HEIGHT) || (clippedY >= (int16_t)LCD_PORT_WIDTH))
  {
    return;
  }
  if ((clippedX + clippedWidth) > (int16_t)LCD_PORT_HEIGHT)
  {
    clippedWidth = (int16_t)((int16_t)LCD_PORT_HEIGHT - clippedX);
  }
  if ((clippedY + clippedHeight) > (int16_t)LCD_PORT_WIDTH)
  {
    clippedHeight = (int16_t)((int16_t)LCD_PORT_WIDTH - clippedY);
  }
  if ((clippedWidth <= 0) || (clippedHeight <= 0))
  {
    return;
  }

  physicalX = (uint16_t)(LCD_PORT_WIDTH - (uint16_t)(clippedY + clippedHeight));
  physicalY = (uint16_t)clippedX;
  physicalWidth = (uint16_t)clippedHeight;
  physicalHeight = (uint16_t)clippedWidth;

  lcd_select();
  lcd_set_window(physicalX,
                 physicalY,
                 (uint16_t)(physicalX + physicalWidth - 1U),
                 (uint16_t)(physicalY + physicalHeight - 1U));
  lcd_data_mode();

  for (row = 0U; row < physicalHeight; row++)
  {
    uint16_t sent = 0U;
    uint16_t logicalX = (uint16_t)(clippedX + (int16_t)row);

    while (sent < physicalWidth)
    {
      uint16_t chunk = ((uint16_t)(physicalWidth - sent) > LCD_TX_PIXELS) ? LCD_TX_PIXELS : (uint16_t)(physicalWidth - sent);
      uint16_t i;

      for (i = 0U; i < chunk; i++)
      {
        uint16_t physicalPixelX = (uint16_t)(physicalX + sent + i);
        uint16_t logicalY = (uint16_t)(LCD_PORT_WIDTH - 1U - physicalPixelX);
        uint16_t pixel;

        if (framebufferIsLogicalLandscape != 0U)
        {
          pixel = framebuffer[(uint32_t)logicalY * stridePixels + logicalX];
        }
        else
        {
          uint16_t sourceX = logicalY;
          uint16_t sourceY = (uint16_t)(LCD_PORT_HEIGHT - 1U - logicalX);
          pixel = framebuffer[(uint32_t)sourceY * stridePixels + sourceX];
        }

        buffer[(uint16_t)i * 2U] = (uint8_t)(pixel >> 8);
        buffer[(uint16_t)i * 2U + 1U] = (uint8_t)(pixel & 0xFFU);
      }

      lcd_write_bytes(buffer, (uint16_t)(chunk * 2U));
      sent = (uint16_t)(sent + chunk);
    }
  }

  lcd_deselect();
}

void LCD_DrawCenteredText(const char* text, uint16_t color, uint16_t bg_color, uint8_t scale)
{
  size_t len = 0U;
  uint16_t text_width;
  uint16_t text_height;
  uint16_t x;
  uint16_t y;
  size_t i;

  if (text == NULL)
  {
    return;
  }

  if (scale == 0U)
  {
    scale = 1U;
  }

  while (text[len] != '\0')
  {
    len++;
  }

  if (len == 0U)
  {
    return;
  }

  text_width = (uint16_t)(((uint32_t)len * 6U - 1U) * scale);
  text_height = (uint16_t)(7U * scale);
  x = (text_width >= LCD_PORT_WIDTH) ? 0U : (uint16_t)((LCD_PORT_WIDTH - text_width) / 2U);
  y = (uint16_t)((LCD_PORT_HEIGHT - text_height) / 2U);

  for (i = 0U; i < len; i++)
  {
    lcd_draw_char((uint16_t)(x + (uint16_t)i * 6U * scale), y, text[i], color, bg_color, scale);
  }
}
