#ifndef __MAX30102_H__
#define __MAX30102_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include <stdbool.h>
#include <stdint.h>

#define MAX30102_I2C_ADDR_7BIT       0x57U
#define MAX30102_I2C_ADDR_HAL        (MAX30102_I2C_ADDR_7BIT << 1)
#define MAX30102_EXPECTED_PART_ID    0x15U

typedef struct
{
  uint32_t red;
  uint32_t ir;
} Max30102Sample;

typedef struct
{
  uint8_t fifoWritePtr;
  uint8_t fifoReadPtr;
  uint8_t fifoAvailable;
  uint8_t fifoConfig;
  uint8_t modeConfig;
  uint8_t spo2Config;
  uint8_t led1Pa;
  uint8_t led2Pa;
  uint8_t interruptEnable1;
  uint8_t interruptEnable2;
  uint8_t overflowCounter;
} Max30102Diagnostics;

typedef struct
{
  bool max30102Ack;
  uint8_t deviceCount;
  uint8_t firstAddress;
  uint8_t readyStatus;
  uint32_t i2cError;
} Max30102BusProbe;

bool MAX30102_IsReady(void);
bool MAX30102_ProbeBus(Max30102BusProbe *probe);
bool MAX30102_ReadReg(uint8_t reg, uint8_t *value);
bool MAX30102_WriteReg(uint8_t reg, uint8_t value);
bool MAX30102_ReadBlock(uint8_t reg, uint8_t *data, uint16_t length);
bool MAX30102_ReadPartId(uint8_t *part_id);
bool MAX30102_Reset(void);
bool MAX30102_Init(void);
bool MAX30102_ClearFifo(void);
bool MAX30102_KickSampleEngine(void);
bool MAX30102_ReadFifoPointers(uint8_t *write_ptr, uint8_t *read_ptr, uint8_t *available);
bool MAX30102_ReadDiagnostics(Max30102Diagnostics *diagnostics);
bool MAX30102_GetFifoSampleCount(uint8_t *sample_count);
bool MAX30102_ReadFifoSample(Max30102Sample *sample);
bool MAX30102_ReadTemperatureX10(int16_t *temperature_x10);

#ifdef __cplusplus
}
#endif

#endif /* __MAX30102_H__ */
