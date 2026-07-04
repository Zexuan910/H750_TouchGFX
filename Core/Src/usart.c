#include "usart.h"

#include "imu_sensor.h"

#include <stddef.h>
#include <string.h>

#define WIRELESS_UART_BAUDRATE 115200U

void MX_USART1_UART_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  uint32_t pclk;

  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_USART1_CLK_ENABLE();

  GPIO_InitStruct.Pin = GPIO_PIN_9 | GPIO_PIN_10;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  USART1->CR1 = 0U;
  USART1->CR2 = 0U;
  USART1->CR3 = 0U;
  USART1->PRESC = 0U;

  pclk = HAL_RCC_GetPCLK2Freq();
  USART1->BRR = (pclk + (WIRELESS_UART_BAUDRATE / 2U)) / WIRELESS_UART_BAUDRATE;
  USART1->ICR = USART_ICR_FECF | USART_ICR_NECF | USART_ICR_ORECF;
  USART1->CR1 = USART_CR1_RE | USART_CR1_TE | USART_CR1_RXNEIE_RXFNEIE | USART_CR1_UE;

  HAL_NVIC_SetPriority(USART1_IRQn, 6, 0);
  HAL_NVIC_EnableIRQ(USART1_IRQn);
}

bool USART1_Wireless_Write(const uint8_t* data, uint16_t length, uint32_t timeout_ms)
{
  const uint32_t start_tick = HAL_GetTick();

  if ((data == NULL) || (length == 0U))
  {
    return false;
  }

  for (uint16_t i = 0U; i < length; i++)
  {
    while ((USART1->ISR & USART_ISR_TXE_TXFNF) == 0U)
    {
      if ((HAL_GetTick() - start_tick) > timeout_ms)
      {
        return false;
      }
    }
    USART1->TDR = data[i];
  }

  while ((USART1->ISR & USART_ISR_TC) == 0U)
  {
    if ((HAL_GetTick() - start_tick) > timeout_ms)
    {
      return false;
    }
  }

  return true;
}

bool USART1_Wireless_WriteString(const char* text, uint32_t timeout_ms)
{
  if (text == NULL)
  {
    return false;
  }

  return USART1_Wireless_Write((const uint8_t*)text, (uint16_t)strlen(text), timeout_ms);
}

void USART1_Wireless_IRQHandler(void)
{
  const uint32_t isr = USART1->ISR;

  if ((isr & (USART_ISR_FE | USART_ISR_NE | USART_ISR_ORE)) != 0U)
  {
    USART1->ICR = USART_ICR_FECF | USART_ICR_NECF | USART_ICR_ORECF;
  }

  while ((USART1->ISR & USART_ISR_RXNE_RXFNE) != 0U)
  {
    IMU_Sensor_OnWirelessByte((uint8_t)(USART1->RDR & 0xFFU));
  }
}
