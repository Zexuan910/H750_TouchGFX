#include "max30102.h"
#include "i2c.h"

#define MAX30102_INTR_STATUS_1       0x00U
#define MAX30102_INTR_STATUS_2       0x01U
#define MAX30102_INTR_ENABLE_1       0x02U
#define MAX30102_INTR_ENABLE_2       0x03U
#define MAX30102_FIFO_WR_PTR         0x04U
#define MAX30102_OVF_COUNTER         0x05U
#define MAX30102_FIFO_RD_PTR         0x06U
#define MAX30102_FIFO_DATA           0x07U
#define MAX30102_FIFO_CONFIG         0x08U
#define MAX30102_MODE_CONFIG         0x09U
#define MAX30102_SPO2_CONFIG         0x0AU
#define MAX30102_LED1_PA             0x0CU
#define MAX30102_LED2_PA             0x0DU
#define MAX30102_TEMP_INTR           0x1FU
#define MAX30102_TEMP_FRAC           0x20U
#define MAX30102_TEMP_CONFIG         0x21U
#define MAX30102_PART_ID             0xFFU

#define MAX30102_I2C_TIMEOUT_MS      100U
#define MAX30102_I2C_SCAN_TIMEOUT_MS   3U
#define MAX30102_FIFO_DEPTH          32U

bool MAX30102_IsReady(void)
{
  return HAL_I2C_IsDeviceReady(&hi2c1,
                               MAX30102_I2C_ADDR_HAL,
                               3U,
                               MAX30102_I2C_TIMEOUT_MS) == HAL_OK;
}

bool MAX30102_ProbeBus(Max30102BusProbe *probe)
{
  Max30102BusProbe snapshot = {0};
  HAL_StatusTypeDef ready_status = HAL_OK;

  if (probe == NULL)
  {
    return false;
  }

  ready_status = HAL_I2C_IsDeviceReady(&hi2c1,
                                       MAX30102_I2C_ADDR_HAL,
                                       1U,
                                       MAX30102_I2C_TIMEOUT_MS);
  snapshot.max30102Ack = (ready_status == HAL_OK);
  snapshot.readyStatus = (uint8_t)ready_status;
  snapshot.i2cError = HAL_I2C_GetError(&hi2c1);

  for (uint8_t address = 0x08U; address <= 0x77U; ++address)
  {
    const uint16_t hal_address = (uint16_t)(address << 1U);
    if (HAL_I2C_IsDeviceReady(&hi2c1, hal_address, 1U, MAX30102_I2C_SCAN_TIMEOUT_MS) == HAL_OK)
    {
      if (snapshot.deviceCount == 0U)
      {
        snapshot.firstAddress = address;
      }
      if (snapshot.deviceCount < UINT8_MAX)
      {
        ++snapshot.deviceCount;
      }
    }
  }

  *probe = snapshot;
  return true;
}

bool MAX30102_ReadReg(uint8_t reg, uint8_t *value)
{
  if (value == NULL)
  {
    return false;
  }

  return HAL_I2C_Mem_Read(&hi2c1,
                          MAX30102_I2C_ADDR_HAL,
                          reg,
                          I2C_MEMADD_SIZE_8BIT,
                          value,
                          1U,
                          MAX30102_I2C_TIMEOUT_MS) == HAL_OK;
}

bool MAX30102_WriteReg(uint8_t reg, uint8_t value)
{
  return HAL_I2C_Mem_Write(&hi2c1,
                           MAX30102_I2C_ADDR_HAL,
                           reg,
                           I2C_MEMADD_SIZE_8BIT,
                           &value,
                           1U,
                           MAX30102_I2C_TIMEOUT_MS) == HAL_OK;
}

bool MAX30102_ReadBlock(uint8_t reg, uint8_t *data, uint16_t length)
{
  if ((data == NULL) || (length == 0U))
  {
    return false;
  }

  return HAL_I2C_Mem_Read(&hi2c1,
                          MAX30102_I2C_ADDR_HAL,
                          reg,
                          I2C_MEMADD_SIZE_8BIT,
                          data,
                          length,
                          MAX30102_I2C_TIMEOUT_MS) == HAL_OK;
}

bool MAX30102_ReadPartId(uint8_t *part_id)
{
  return MAX30102_ReadReg(MAX30102_PART_ID, part_id);
}

bool MAX30102_Reset(void)
{
  if (!MAX30102_WriteReg(MAX30102_MODE_CONFIG, 0x40U))
  {
    return false;
  }

  HAL_Delay(100U);
  return true;
}

bool MAX30102_Init(void)
{
  uint8_t part_id = 0U;
  uint8_t ignored = 0U;

  if (!MAX30102_ReadPartId(&part_id) || (part_id != MAX30102_EXPECTED_PART_ID))
  {
    return false;
  }

  if (!MAX30102_Reset())
  {
    return false;
  }

  (void)MAX30102_ReadReg(MAX30102_INTR_STATUS_1, &ignored);
  (void)MAX30102_ReadReg(MAX30102_INTR_STATUS_2, &ignored);

  if (!MAX30102_WriteReg(MAX30102_INTR_ENABLE_1, 0xC0U) ||
      !MAX30102_WriteReg(MAX30102_INTR_ENABLE_2, 0x00U) ||
      !MAX30102_WriteReg(MAX30102_FIFO_WR_PTR, 0x00U) ||
      !MAX30102_WriteReg(MAX30102_OVF_COUNTER, 0x00U) ||
      !MAX30102_WriteReg(MAX30102_FIFO_RD_PTR, 0x00U) ||
      !MAX30102_WriteReg(MAX30102_FIFO_CONFIG, 0x4FU) ||
      !MAX30102_WriteReg(MAX30102_SPO2_CONFIG, 0x2AU) ||
      !MAX30102_WriteReg(MAX30102_LED1_PA, 0x2FU) ||
      !MAX30102_WriteReg(MAX30102_LED2_PA, 0x2FU) ||
      !MAX30102_WriteReg(MAX30102_MODE_CONFIG, 0x03U))
  {
    return false;
  }

  (void)MAX30102_ReadReg(MAX30102_INTR_STATUS_1, &ignored);
  (void)MAX30102_ReadReg(MAX30102_INTR_STATUS_2, &ignored);

  return true;
}

bool MAX30102_ClearFifo(void)
{
  return MAX30102_WriteReg(MAX30102_FIFO_WR_PTR, 0x00U) &&
         MAX30102_WriteReg(MAX30102_OVF_COUNTER, 0x00U) &&
         MAX30102_WriteReg(MAX30102_FIFO_RD_PTR, 0x00U);
}

bool MAX30102_KickSampleEngine(void)
{
  return MAX30102_WriteReg(MAX30102_MODE_CONFIG, 0x03U);
}

bool MAX30102_ReadFifoPointers(uint8_t *write_ptr, uint8_t *read_ptr, uint8_t *available)
{
  uint8_t fifo_write_ptr = 0U;
  uint8_t fifo_read_ptr = 0U;

  if ((write_ptr == NULL) || (read_ptr == NULL) || (available == NULL))
  {
    return false;
  }

  if (!MAX30102_ReadReg(MAX30102_FIFO_WR_PTR, &fifo_write_ptr) ||
      !MAX30102_ReadReg(MAX30102_FIFO_RD_PTR, &fifo_read_ptr))
  {
    return false;
  }

  fifo_write_ptr &= 0x1FU;
  fifo_read_ptr &= 0x1FU;

  *write_ptr = fifo_write_ptr;
  *read_ptr = fifo_read_ptr;
  *available = (fifo_write_ptr >= fifo_read_ptr)
                 ? (uint8_t)(fifo_write_ptr - fifo_read_ptr)
                 : (uint8_t)(MAX30102_FIFO_DEPTH + fifo_write_ptr - fifo_read_ptr);

  return true;
}

bool MAX30102_ReadDiagnostics(Max30102Diagnostics *diagnostics)
{
  Max30102Diagnostics snapshot = {0U};

  if (diagnostics == NULL)
  {
    return false;
  }

  if (!MAX30102_ReadFifoPointers(&snapshot.fifoWritePtr,
                                 &snapshot.fifoReadPtr,
                                 &snapshot.fifoAvailable) ||
      !MAX30102_ReadReg(MAX30102_FIFO_CONFIG, &snapshot.fifoConfig) ||
      !MAX30102_ReadReg(MAX30102_MODE_CONFIG, &snapshot.modeConfig) ||
      !MAX30102_ReadReg(MAX30102_SPO2_CONFIG, &snapshot.spo2Config) ||
      !MAX30102_ReadReg(MAX30102_LED1_PA, &snapshot.led1Pa) ||
      !MAX30102_ReadReg(MAX30102_LED2_PA, &snapshot.led2Pa) ||
      !MAX30102_ReadReg(MAX30102_INTR_ENABLE_1, &snapshot.interruptEnable1) ||
      !MAX30102_ReadReg(MAX30102_INTR_ENABLE_2, &snapshot.interruptEnable2) ||
      !MAX30102_ReadReg(MAX30102_OVF_COUNTER, &snapshot.overflowCounter))
  {
    return false;
  }

  *diagnostics = snapshot;
  return true;
}

bool MAX30102_GetFifoSampleCount(uint8_t *sample_count)
{
  uint8_t write_ptr = 0U;
  uint8_t read_ptr = 0U;

  if (sample_count == NULL)
  {
    return false;
  }

  return MAX30102_ReadFifoPointers(&write_ptr, &read_ptr, sample_count);
}

bool MAX30102_ReadFifoSample(Max30102Sample *sample)
{
  uint8_t fifo_data[6] = {0};

  if (sample == NULL)
  {
    return false;
  }

  if (!MAX30102_ReadBlock(MAX30102_FIFO_DATA, fifo_data, sizeof(fifo_data)))
  {
    return false;
  }

  sample->red = (((uint32_t)fifo_data[0] << 16) |
                 ((uint32_t)fifo_data[1] << 8) |
                 ((uint32_t)fifo_data[2])) & 0x03FFFFU;
  sample->ir = (((uint32_t)fifo_data[3] << 16) |
                ((uint32_t)fifo_data[4] << 8) |
                ((uint32_t)fifo_data[5])) & 0x03FFFFU;

  return true;
}

bool MAX30102_ReadTemperatureX10(int16_t *temperature_x10)
{
  uint8_t integer_raw = 0U;
  uint8_t fraction_raw = 0U;
  int16_t integer_part = 0;
  int16_t fraction_x10 = 0;

  if (temperature_x10 == NULL)
  {
    return false;
  }

  if (!MAX30102_WriteReg(MAX30102_TEMP_CONFIG, 0x01U))
  {
    return false;
  }

  HAL_Delay(30U);

  if (!MAX30102_ReadReg(MAX30102_TEMP_INTR, &integer_raw) ||
      !MAX30102_ReadReg(MAX30102_TEMP_FRAC, &fraction_raw))
  {
    return false;
  }

  integer_part = (integer_raw >= 0x80U) ? (int16_t)((int16_t)integer_raw - 256) : (int16_t)integer_raw;
  fraction_x10 = (int16_t)(((uint16_t)(fraction_raw & 0x0FU) * 625U + 500U) / 1000U);
  *temperature_x10 = (int16_t)(integer_part * 10 + fraction_x10);

  return true;
}
