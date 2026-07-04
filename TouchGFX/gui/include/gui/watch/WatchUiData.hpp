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
    int8_t temperatureC;
    uint8_t humidityPercent;
    uint16_t altitudeM;
};

constexpr WatchSnapshot sampleSnapshot(uint32_t tick)
{
    return WatchSnapshot{
        static_cast<uint8_t>(10U + ((tick / 60U) % 4U)),
        static_cast<uint8_t>((tick / 25U) % 60U),
        static_cast<uint8_t>(87U - ((tick / 900U) % 8U)),
        static_cast<uint32_t>(3205U + (tick * 3U)),
        static_cast<uint8_t>(72U + ((tick / 35U) % 9U)),
        static_cast<int8_t>(24 + static_cast<int8_t>((tick / 80U) % 5U)),
        static_cast<uint8_t>(48U + ((tick / 55U) % 12U)),
        static_cast<uint16_t>(126U + ((tick / 75U) % 18U))};
}

constexpr ScreenMode nextMode(ScreenMode current)
{
    return current == ScreenMode::WatchFace ? ScreenMode::Menu : ScreenMode::WatchFace;
}
} // namespace WatchUi

#endif // WATCHUIDATA_HPP
