#ifndef __USART_H__
#define __USART_H__

#include <stdbool.h>
#include <stdint.h>

#include "main.h"

#ifdef __cplusplus
extern "C" {
#endif

void MX_USART1_UART_Init(void);
bool USART1_Wireless_Write(const uint8_t* data, uint16_t length, uint32_t timeout_ms);
bool USART1_Wireless_WriteString(const char* text, uint32_t timeout_ms);
void USART1_Wireless_IRQHandler(void);

#ifdef __cplusplus
}
#endif

#endif /* __USART_H__ */
