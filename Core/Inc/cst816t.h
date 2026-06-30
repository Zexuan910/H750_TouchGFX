#ifndef __CST816T_H__
#define __CST816T_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include <stdbool.h>
#include <stdint.h>

#define CST816T_I2C_ADDR_7BIT    0x15U
#define CST816T_I2C_ADDR         (CST816T_I2C_ADDR_7BIT << 1)

#define CST816T_SCREEN_WIDTH     240U
#define CST816T_SCREEN_HEIGHT    280U

/* 坐标方向调整区：真机如果左右/上下反了，只改这里。 */
#define CST816T_SWAP_XY          0U
#define CST816T_MIRROR_X         0U
#define CST816T_MIRROR_Y         0U

void CST816T_Init(void);
bool CST816T_IsConnected(void);
bool CST816T_ReadTouch(uint16_t* x, uint16_t* y);

#ifdef __cplusplus
}
#endif

#endif /* __CST816T_H__ */
