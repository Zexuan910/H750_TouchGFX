#ifndef IMU_SENSOR_H
#define IMU_SENSOR_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
  int16_t raw_accel[3];
  int16_t raw_gyro[3];
  float accel_g[3];
  float gyro_rad_s[3];
} IMU_SensorSample;

typedef struct
{
  uint32_t rx_byte_count;
  uint32_t rx_line_count;
  uint32_t parse_ok_count;
  uint32_t parse_error_count;
  uint32_t last_sample_age_ms;
  uint8_t has_sample;
  IMU_SensorSample last_sample;
} IMU_SensorDebugSnapshot;

void IMU_Sensor_Init(void);
bool IMU_Sensor_IsReady(void);
bool IMU_Sensor_Read(IMU_SensorSample* sample);
void IMU_Sensor_Process(void);
void IMU_Sensor_OnWirelessByte(uint8_t byte);
void IMU_Sensor_GetDebugSnapshot(IMU_SensorDebugSnapshot* snapshot);

#ifdef __cplusplus
}
#endif

#endif
