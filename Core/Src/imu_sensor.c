#include "imu_sensor.h"

#include "i2c.h"
#include "usart.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define IMU_SENSOR_ADDR_7BIT    0x23U
#define IMU_SENSOR_ADDR         (IMU_SENSOR_ADDR_7BIT << 1)
#define IMU_SENSOR_REG_VERSION  0x01U
#define IMU_SENSOR_REG_ACCEL_X  0x04U
#define IMU_SENSOR_FRAME_SIZE   12U
#define IMU_SENSOR_I2C_TIMEOUT  20U
#define IMU_WIRELESS_LINE_SIZE  96U
#define IMU_WIRELESS_TIMEOUT_MS 3000U
#define IMU_WIRELESS_TX_PERIOD_MS 50U
#define IMU_WIRELESS_TX_TIMEOUT_MS 20U
#define IMU_ENABLE_WIRELESS_TX  0U

static uint8_t imu_sensor_ready;
static char imu_uart_line[IMU_WIRELESS_LINE_SIZE];
static char imu_uart_pending_line[IMU_WIRELESS_LINE_SIZE];
static volatile uint8_t imu_uart_line_index;
static volatile uint8_t imu_uart_line_ready;
static volatile uint32_t imu_uart_rx_byte_count;
static volatile uint32_t imu_uart_rx_line_count;
static uint32_t imu_wireless_parse_ok_count;
static uint32_t imu_wireless_parse_error_count;
static uint32_t imu_wireless_last_tick_ms;
static uint32_t imu_wireless_last_tx_tick_ms;
static IMU_SensorSample imu_wireless_last_sample;

static int16_t bytes_to_i16(uint8_t low_byte, uint8_t high_byte)
{
  return (int16_t)(((uint16_t)high_byte << 8) | low_byte);
}

static int16_t clamp_i16(int32_t value)
{
  if (value > 32767)
  {
    return 32767;
  }
  if (value < -32768)
  {
    return -32768;
  }
  return (int16_t)value;
}

static bool parse_next_i32(const char** cursor, int32_t* value)
{
  char* end_ptr;
  long parsed;

  if ((cursor == NULL) || (*cursor == NULL) || (value == NULL))
  {
    return false;
  }

  parsed = strtol(*cursor, &end_ptr, 10);
  if (end_ptr == *cursor)
  {
    return false;
  }

  *value = (int32_t)parsed;
  *cursor = end_ptr;
  return true;
}

static bool parse_wireless_sample(const char* line, IMU_SensorSample* sample)
{
  const char* cursor;
  int32_t fields[7];

  if ((line == NULL) || (sample == NULL) || (strncmp(line, "IMU,", 4U) != 0))
  {
    return false;
  }

  cursor = line + 4U;
  for (uint8_t i = 0U; i < 7U; i++)
  {
    if (!parse_next_i32(&cursor, &fields[i]))
    {
      return false;
    }
    if (i < 6U)
    {
      if (*cursor != ',')
      {
        return false;
      }
      cursor++;
    }
  }

  for (uint8_t i = 0U; i < 3U; i++)
  {
    sample->raw_accel[i] = clamp_i16(fields[1U + i]);
    sample->raw_gyro[i] = clamp_i16(fields[4U + i]);
    sample->accel_g[i] = (float)fields[1U + i] / 1000.0f;
    sample->gyro_rad_s[i] = (float)fields[4U + i] / 1000.0f;
  }

  (void)fields[0];
  return true;
}

static bool read_wireless_sample(IMU_SensorSample* sample)
{
  char line[IMU_WIRELESS_LINE_SIZE];
  uint8_t has_line = 0U;

  __disable_irq();
  if (imu_uart_line_ready != 0U)
  {
    memcpy(line, imu_uart_pending_line, sizeof(line));
    imu_uart_line_ready = 0U;
    has_line = 1U;
  }
  __enable_irq();

  if (has_line == 0U)
  {
    return false;
  }

  if (!parse_wireless_sample(line, sample))
  {
    imu_wireless_parse_error_count++;
    return false;
  }

  imu_wireless_last_sample = *sample;
  imu_wireless_last_tick_ms = HAL_GetTick();
  imu_wireless_parse_ok_count++;
  imu_sensor_ready = 1U;
  return true;
}

static bool read_i2c_sample(IMU_SensorSample* sample)
{
  uint8_t buffer[IMU_SENSOR_FRAME_SIZE];
  const float accel_ratio = 16.0f / 32767.0f;
  const float gyro_ratio = (1.0f / 131.0f) * (3.1415926f / 180.0f);

  if (HAL_I2C_Mem_Read(&hi2c4,
                       IMU_SENSOR_ADDR,
                       IMU_SENSOR_REG_ACCEL_X,
                       I2C_MEMADD_SIZE_8BIT,
                       buffer,
                       sizeof(buffer),
                       IMU_SENSOR_I2C_TIMEOUT) != HAL_OK)
  {
    imu_sensor_ready = 0U;
    return false;
  }

  sample->raw_accel[0] = bytes_to_i16(buffer[0], buffer[1]);
  sample->raw_accel[1] = bytes_to_i16(buffer[2], buffer[3]);
  sample->raw_accel[2] = bytes_to_i16(buffer[4], buffer[5]);
  sample->raw_gyro[0] = bytes_to_i16(buffer[6], buffer[7]);
  sample->raw_gyro[1] = bytes_to_i16(buffer[8], buffer[9]);
  sample->raw_gyro[2] = bytes_to_i16(buffer[10], buffer[11]);

  for (uint8_t i = 0U; i < 3U; i++)
  {
    sample->accel_g[i] = (float)sample->raw_accel[i] * accel_ratio;
    sample->gyro_rad_s[i] = (float)sample->raw_gyro[i] * gyro_ratio;
  }

  imu_sensor_ready = 1U;
  return true;
}

#if IMU_ENABLE_WIRELESS_TX
static int32_t scale_float_to_i32(float value, float scale)
{
  const float scaled = value * scale;

  if (scaled >= 0.0f)
  {
    return (int32_t)(scaled + 0.5f);
  }
  return (int32_t)(scaled - 0.5f);
}

static void write_wireless_sample(const IMU_SensorSample* sample, uint32_t tick_ms)
{
  char line[IMU_WIRELESS_LINE_SIZE];
  const int32_t accel_mg[3] = {
    scale_float_to_i32(sample->accel_g[0], 1000.0f),
    scale_float_to_i32(sample->accel_g[1], 1000.0f),
    scale_float_to_i32(sample->accel_g[2], 1000.0f),
  };
  const int32_t gyro_mrad_s[3] = {
    scale_float_to_i32(sample->gyro_rad_s[0], 1000.0f),
    scale_float_to_i32(sample->gyro_rad_s[1], 1000.0f),
    scale_float_to_i32(sample->gyro_rad_s[2], 1000.0f),
  };
  const int length = snprintf(line,
                              sizeof(line),
                              "IMU,%lu,%ld,%ld,%ld,%ld,%ld,%ld\r\n",
                              (unsigned long)tick_ms,
                              (long)accel_mg[0],
                              (long)accel_mg[1],
                              (long)accel_mg[2],
                              (long)gyro_mrad_s[0],
                              (long)gyro_mrad_s[1],
                              (long)gyro_mrad_s[2]);

  if ((length > 0) && ((size_t)length < sizeof(line)))
  {
    (void)USART1_Wireless_Write((const uint8_t*)line, (uint16_t)length, IMU_WIRELESS_TX_TIMEOUT_MS);
  }
}
#endif

void IMU_Sensor_Init(void)
{
  uint8_t version[3] = {0U, 0U, 0U};
  imu_uart_line_index = 0U;
  imu_uart_line_ready = 0U;
  imu_uart_rx_byte_count = 0U;
  imu_uart_rx_line_count = 0U;
  imu_wireless_parse_ok_count = 0U;
  imu_wireless_parse_error_count = 0U;
  imu_wireless_last_tick_ms = 0U;
  imu_wireless_last_tx_tick_ms = 0U;

  imu_sensor_ready = (HAL_I2C_Mem_Read(&hi2c4,
                                       IMU_SENSOR_ADDR,
                                       IMU_SENSOR_REG_VERSION,
                                       I2C_MEMADD_SIZE_8BIT,
                                       version,
                                       sizeof(version),
                                       IMU_SENSOR_I2C_TIMEOUT) == HAL_OK) ? 1U : 0U;
}

bool IMU_Sensor_IsReady(void)
{
  if ((imu_wireless_last_tick_ms != 0U) && ((HAL_GetTick() - imu_wireless_last_tick_ms) <= IMU_WIRELESS_TIMEOUT_MS))
  {
    return true;
  }
  return imu_sensor_ready != 0U;
}

bool IMU_Sensor_Read(IMU_SensorSample* sample)
{
  if (sample == NULL)
  {
    return false;
  }

  if (read_wireless_sample(sample))
  {
    return true;
  }

  if ((imu_wireless_last_tick_ms != 0U) && ((HAL_GetTick() - imu_wireless_last_tick_ms) <= IMU_WIRELESS_TIMEOUT_MS))
  {
    *sample = imu_wireless_last_sample;
    return true;
  }

  return read_i2c_sample(sample);
}

void IMU_Sensor_Process(void)
{
  IMU_SensorSample sample;
  IMU_SensorSample wirelessSample;
  const uint32_t now = HAL_GetTick();

  (void)read_wireless_sample(&wirelessSample);

#if IMU_ENABLE_WIRELESS_TX
  if ((now - imu_wireless_last_tx_tick_ms) < IMU_WIRELESS_TX_PERIOD_MS)
  {
    return;
  }

  imu_wireless_last_tx_tick_ms = now;
  if (read_i2c_sample(&sample))
  {
    write_wireless_sample(&sample, now);
  }
#else
  (void)sample;
  (void)now;
#endif
}

void IMU_Sensor_OnWirelessByte(uint8_t byte)
{
  imu_uart_rx_byte_count++;

  if ((byte == '\n') || (byte == '\r'))
  {
    if (imu_uart_line_index > 0U)
    {
      imu_uart_line[imu_uart_line_index] = '\0';
      memcpy(imu_uart_pending_line, imu_uart_line, sizeof(imu_uart_pending_line));
      imu_uart_line_ready = 1U;
      imu_uart_rx_line_count++;
      imu_uart_line_index = 0U;
    }
  }
  else if (imu_uart_line_index < (IMU_WIRELESS_LINE_SIZE - 1U))
  {
    imu_uart_line[imu_uart_line_index] = (char)byte;
    imu_uart_line_index++;
  }
  else
  {
    imu_uart_line_index = 0U;
  }
}

void IMU_Sensor_GetDebugSnapshot(IMU_SensorDebugSnapshot* snapshot)
{
  uint32_t rxBytes;
  uint32_t rxLines;

  if (snapshot == NULL)
  {
    return;
  }

  (void)read_wireless_sample(&snapshot->last_sample);

  __disable_irq();
  rxBytes = imu_uart_rx_byte_count;
  rxLines = imu_uart_rx_line_count;
  __enable_irq();

  memset(snapshot, 0, sizeof(*snapshot));
  snapshot->rx_byte_count = rxBytes;
  snapshot->rx_line_count = rxLines;
  snapshot->parse_ok_count = imu_wireless_parse_ok_count;
  snapshot->parse_error_count = imu_wireless_parse_error_count;
  snapshot->has_sample = (imu_wireless_last_tick_ms != 0U) ? 1U : 0U;
  if (snapshot->has_sample != 0U)
  {
    snapshot->last_sample_age_ms = HAL_GetTick() - imu_wireless_last_tick_ms;
    snapshot->last_sample = imu_wireless_last_sample;
  }
}
