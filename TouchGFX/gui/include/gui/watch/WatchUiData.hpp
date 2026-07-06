#ifndef WATCHUIDATA_HPP
#define WATCHUIDATA_HPP

#include <stdint.h>

namespace WatchUi
{
static const uint16_t kScreenWidth = 240;
static const uint16_t kScreenHeight = 280;
static const uint8_t kMetricCount = 4;

enum class ScreenMode : uint8_t
{
    WatchFace,
    Menu
};

struct WatchSnapshot
{
    uint8_t hour;
    uint8_t minute;
    uint8_t batteryPercent;
    uint32_t steps;
    uint8_t heartRate;
    uint8_t spo2Percent;
    uint32_t rawRed;
    uint32_t rawIr;
    uint16_t validSamples;
    uint8_t partId;
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
    int8_t temperatureC;
    uint8_t humidityPercent;
    uint16_t altitudeM;
    bool sensorReady;
    bool fingerDetected;
    bool sensorValid;
};

constexpr WatchSnapshot sampleSnapshot(uint32_t tick)
{
    return WatchSnapshot{
        static_cast<uint8_t>(10U + ((tick / 60U) % 4U)),
        static_cast<uint8_t>((tick / 25U) % 60U),
        static_cast<uint8_t>(87U - ((tick / 900U) % 8U)),
        static_cast<uint32_t>(3205U + (tick * 3U)),
        static_cast<uint8_t>(72U + ((tick / 35U) % 9U)),
        static_cast<uint8_t>(97U + ((tick / 300U) % 3U)),
        static_cast<uint32_t>(128000U + (tick % 4000U)),
        static_cast<uint32_t>(132000U + (tick % 5000U)),
        static_cast<uint16_t>(tick % 150U),
        static_cast<uint8_t>(0x15U),
        static_cast<uint8_t>((tick / 2U) % 32U),
        static_cast<uint8_t>((tick / 3U) % 32U),
        static_cast<uint8_t>(tick % 4U),
        static_cast<uint8_t>(0x03U),
        static_cast<uint8_t>(0x2AU),
        static_cast<uint8_t>(0x2FU),
        static_cast<uint8_t>(0x2FU),
        static_cast<uint8_t>(0x4FU),
        static_cast<uint8_t>(0xC0U),
        static_cast<uint8_t>(0x00U),
        static_cast<uint8_t>((tick / 200U) % 4U),
        static_cast<uint8_t>(3U + (tick % 6U)),
        static_cast<uint32_t>(tick * 2U),
        static_cast<uint32_t>(tick / 5U),
        static_cast<uint8_t>(1U),
        static_cast<uint8_t>(3U),
        static_cast<uint8_t>(1U),
        static_cast<uint32_t>(127500U + (tick % 4000U)),
        static_cast<uint32_t>(133500U + (tick % 5000U)),
        static_cast<uint32_t>(tick * 2U + 5U),
        true,
        true,
        static_cast<uint8_t>(1U),
        static_cast<uint8_t>(0x57U),
        static_cast<uint8_t>(0U),
        static_cast<uint32_t>(0U),
        static_cast<uint16_t>(0U),
        static_cast<uint16_t>(0U),
        static_cast<int8_t>(24 + static_cast<int8_t>((tick / 80U) % 5U)),
        static_cast<uint8_t>(48U + ((tick / 55U) % 12U)),
        static_cast<uint16_t>(126U + ((tick / 75U) % 18U)),
        true,
        true,
        true};
}

constexpr ScreenMode nextMode(ScreenMode current)
{
    return current == ScreenMode::WatchFace ? ScreenMode::Menu : ScreenMode::WatchFace;
}
} // namespace WatchUi

#endif // WATCHUIDATA_HPP
