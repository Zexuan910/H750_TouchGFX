#ifndef __MAX30102_SERVICE_H__
#define __MAX30102_SERVICE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
  MAX30102_SERVICE_STATUS_NOT_INITIALIZED = 0,
  MAX30102_SERVICE_STATUS_NO_DEVICE,
  MAX30102_SERVICE_STATUS_READY,
  MAX30102_SERVICE_STATUS_NO_FINGER,
  MAX30102_SERVICE_STATUS_COLLECTING,
  MAX30102_SERVICE_STATUS_VALID,
  MAX30102_SERVICE_STATUS_ERROR
} Max30102ServiceStatus;

typedef struct
{
  Max30102ServiceStatus status;
  bool sensorReady;
  bool fingerDetected;
  uint16_t heartBpm;
  uint16_t spo2X10;
  bool heartValid;
  bool spo2Valid;
  bool beatPulse;
  uint8_t signalQuality;
  uint32_t rawRed;
  uint32_t rawIr;
  int16_t temperatureX10;
  uint8_t partId;
  uint16_t validSamples;
  uint32_t lastUpdateMs;
  uint8_t fifoWritePtr;
  uint8_t fifoReadPtr;
  uint8_t fifoAvailable;
  uint8_t modeConfig;
  uint8_t spo2Config;
  uint8_t led1Pa;
  uint8_t led2Pa;
  uint8_t fifoConfig;
  uint8_t interruptEnable1;
  uint8_t interruptEnable2;
  uint8_t overflowCounter;
  uint8_t maxFifoAvailable;
  uint32_t samplesReadTotal;
  uint32_t emptyFifoPolls;
  uint8_t sampleEngineTestCode;
  uint8_t sampleEngineFifoAfterKick;
  uint8_t sampleEngineReadCode;
  uint32_t sampleEngineRawRed;
  uint32_t sampleEngineRawIr;
  uint32_t fifoReadAttempts;
  bool i2cReady;
  bool max30102Ack;
  uint8_t busDeviceCount;
  uint8_t busFirstAddress;
  uint8_t i2cReadyStatus;
  uint32_t i2cError;
  uint16_t fifoReadFailures;
  uint16_t diagnosticReadFailures;
} Max30102ServiceSnapshot;

bool Max30102Service_Init(void);
void Max30102Service_Poll(uint32_t now_ms);
Max30102ServiceSnapshot Max30102Service_GetSnapshot(void);

#ifdef __cplusplus
}
#endif

#endif /* __MAX30102_SERVICE_H__ */
