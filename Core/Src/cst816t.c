#include "cst816t.h"
#include "i2c.h"

#define CST816T_REG_GESTURE       0x01U
#define CST816T_REG_FINGER_NUM    0x02U
#define CST816T_REG_XPOS_H        0x03U
#define CST816T_REG_XPOS_L        0x04U
#define CST816T_REG_YPOS_H        0x05U
#define CST816T_REG_YPOS_L        0x06U
#define CST816T_REG_CHIP_ID       0xA7U
#define CST816T_REG_PROJ_ID       0xA8U
#define CST816T_REG_FW_VERSION    0xA9U
#define CST816T_REG_SLEEP_MODE    0xE5U
#define CST816T_REG_LONG_PRESS    0xEBU
#define CST816T_REG_AUTO_SLEEP    0xFEU

static bool cst816t_ready = false;

static HAL_StatusTypeDef CST816T_ReadReg(uint8_t reg, uint8_t* data, uint16_t len)
{
  return HAL_I2C_Mem_Read(&hi2c3,
                          CST816T_I2C_ADDR,
                          reg,
                          I2C_MEMADD_SIZE_8BIT,
                          data,
                          len,
                          50U);
}

static HAL_StatusTypeDef CST816T_WriteReg(uint8_t reg, uint8_t data)
{
  return HAL_I2C_Mem_Write(&hi2c3,
                           CST816T_I2C_ADDR,
                           reg,
                           I2C_MEMADD_SIZE_8BIT,
                           &data,
                           1U,
                           50U);
}

static void CST816T_Transform(uint16_t* x, uint16_t* y)
{
  uint16_t tx = *x;
  uint16_t ty = *y;

#if CST816T_SWAP_XY
  uint16_t tmp = tx;
  tx = ty;
  ty = tmp;
#endif

#if CST816T_MIRROR_X
  if (tx < CST816T_SCREEN_WIDTH)
  {
    tx = (uint16_t)(CST816T_SCREEN_WIDTH - 1U - tx);
  }
#endif

#if CST816T_MIRROR_Y
  if (ty < CST816T_SCREEN_HEIGHT)
  {
    ty = (uint16_t)(CST816T_SCREEN_HEIGHT - 1U - ty);
  }
#endif

  *x = tx;
  *y = ty;
}

void CST816T_Init(void)
{
  uint8_t dummy = 0U;
  cst816t_ready = false;

  HAL_GPIO_WritePin(TRST_GPIO_Port, TRST_Pin, GPIO_PIN_RESET);
  HAL_Delay(10U);
  HAL_GPIO_WritePin(TRST_GPIO_Port, TRST_Pin, GPIO_PIN_SET);
  HAL_Delay(80U);

  /* 先探测芯片。部分 CST816T 的 chip id 可能读不出固定值，
     这里能 ACK 就认为硬件通信基本正常。 */
  if (CST816T_ReadReg(CST816T_REG_CHIP_ID, &dummy, 1U) == HAL_OK)
  {
    cst816t_ready = true;

    /* 关闭自动休眠，避免长时间不摸后第一次触摸不灵敏。若你的模块不支持，失败也没关系。 */
    (void)CST816T_WriteReg(CST816T_REG_AUTO_SLEEP, 0xFFU);
    (void)CST816T_WriteReg(CST816T_REG_LONG_PRESS, 0x01U);
  }
}

bool CST816T_IsConnected(void)
{
  return cst816t_ready;
}

bool CST816T_ReadTouch(uint16_t* x, uint16_t* y)
{
  uint8_t buf[5];
  uint8_t finger_num;
  uint16_t raw_x;
  uint16_t raw_y;

  if ((x == 0) || (y == 0))
  {
    return false;
  }

  if (!cst816t_ready)
  {
    /* 允许运行过程中重新插好/复位触摸后自动恢复。 */
    uint8_t dummy = 0U;
    if (CST816T_ReadReg(CST816T_REG_CHIP_ID, &dummy, 1U) != HAL_OK)
    {
      return false;
    }
    cst816t_ready = true;
  }

  /* 从 0x02 连续读取：finger_num, xh, xl, yh, yl */
  if (CST816T_ReadReg(CST816T_REG_FINGER_NUM, buf, sizeof(buf)) != HAL_OK)
  {
    cst816t_ready = false;
    return false;
  }

  finger_num = buf[0] & 0x0FU;
  if (finger_num == 0U)
  {
    return false;
  }

  raw_x = (uint16_t)(((uint16_t)(buf[1] & 0x0FU) << 8) | buf[2]);
  raw_y = (uint16_t)(((uint16_t)(buf[3] & 0x0FU) << 8) | buf[4]);

  *x = raw_x;
  *y = raw_y;
  CST816T_Transform(x, y);

  if ((*x >= CST816T_SCREEN_WIDTH) || (*y >= CST816T_SCREEN_HEIGHT))
  {
    return false;
  }

  return true;
}
