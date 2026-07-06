#include "max30102_service.h"
#include "max30102.h"

#include <stdint.h>
#include <string.h>

#define MAX30102_SERVICE_SAMPLE_THRESHOLD       100000U
#define MAX30102_SERVICE_FIFO_DEPTH                32U
#define MAX30102_SERVICE_REQUIRED_SAMPLES          150U
#define MAX30102_SERVICE_SAMPLE_PERIOD_MS           20U
#define MAX30102_SERVICE_EFFECTIVE_SAMPLE_HZ        50U
#define MAX30102_SERVICE_TEMP_UPDATE_MS           1000U
#define MAX30102_SERVICE_RETRY_MS                 1000U
#define MAX30102_SERVICE_SAMPLE_ENGINE_SETTLE_MS   150U

#define MAX30102_SERVICE_ENGINE_TEST_NOT_RUN         0U
#define MAX30102_SERVICE_ENGINE_TEST_INITIAL_FIFO    1U
#define MAX30102_SERVICE_ENGINE_TEST_KICK_FIFO       2U
#define MAX30102_SERVICE_ENGINE_TEST_STILL_EMPTY     3U
#define MAX30102_SERVICE_ENGINE_TEST_INITIAL_READ_FAIL 4U
#define MAX30102_SERVICE_ENGINE_TEST_KICK_WRITE_FAIL   5U
#define MAX30102_SERVICE_ENGINE_TEST_KICK_READ_FAIL    6U

#define MAX30102_SERVICE_ENGINE_READ_NOT_RUN        0U
#define MAX30102_SERVICE_ENGINE_READ_OK             1U
#define MAX30102_SERVICE_ENGINE_READ_FAIL           2U

#define MAX30102_SERVICE_BPM_WINDOW_SIZE             5U
#define MAX30102_SERVICE_BPM_FILTER_SHIFT            2U
#define MAX30102_SERVICE_MIN_BPM                    40U
#define MAX30102_SERVICE_MAX_BPM                   220U
#define MAX30102_SERVICE_BPM_MAX_JUMP_BPM           40U

#define MAX30102_SERVICE_SPO2_WINDOW_SIZE            5U
#define MAX30102_SERVICE_SPO2_MIN_X10              700U
#define MAX30102_SERVICE_SPO2_MAX_X10             1000U
#define MAX30102_SERVICE_SPO2_RATIO_MIN            350U
#define MAX30102_SERVICE_SPO2_RATIO_MAX           1500U
#define MAX30102_SERVICE_SPO2_JUMP_LIMIT_X10       300U
#define MAX30102_SERVICE_SPO2_MAX_STEP_PER_500MS    10U

#define MAX30102_SERVICE_FINGER_ENTER_STREAK         2U
#define MAX30102_SERVICE_FINGER_EXIT_STREAK          2U
#define MAX30102_SERVICE_FINGER_TIMEOUT_MS        1500U
#define MAX30102_SERVICE_RESULT_KEEP_MS           5000U
#define MAX30102_SERVICE_BEAT_PULSE_MS              60U

#define MAX30102_SERVICE_FIR_TAP_COUNT              29U

static Max30102ServiceSnapshot g_snapshot = {
  MAX30102_SERVICE_STATUS_NOT_INITIALIZED
};

static bool g_initialized = false;
static uint16_t g_validSamples = 0U;
static uint32_t g_lastPollTickMs = 0U;
static uint32_t g_lastTempTickMs = 0U;
static uint32_t g_lastFingerSeenMs = 0U;
static uint32_t g_lastConfidenceTickMs = 0U;
static uint32_t g_lastRetryTickMs = 0U;
static uint32_t g_sampleCounter = 0U;
static uint32_t g_lastHeartBeatSample = 0U;
static uint32_t g_lastBeatTickMs = 0U;
static uint16_t g_lastBeatIntervalMs = 0U;
static uint16_t g_lastStableHeartBpm = 0U;
static uint16_t g_lastStableSpo2X10 = 0U;
static uint16_t g_bpmHistory[MAX30102_SERVICE_BPM_WINDOW_SIZE] = {0U};
static uint8_t g_bpmHistoryCount = 0U;
static uint8_t g_bpmHistoryIndex = 0U;
static uint16_t g_spo2History[MAX30102_SERVICE_SPO2_WINDOW_SIZE] = {0U};
static uint8_t g_spo2HistoryCount = 0U;
static uint8_t g_spo2HistoryIndex = 0U;
static uint16_t g_fingerPresentStreak = 0U;
static uint16_t g_fingerMissStreak = 0U;
static uint16_t g_signalQuality = 0U;
static uint16_t g_beatMissCount = 0U;
static bool g_overflowDrainPendingReset = false;

static int32_t g_irDcEstimate = 0;
static int32_t g_redDcEstimate = 0;
static int32_t g_irAc = 0;
static int32_t g_redAc = 0;
static uint32_t g_irAcEnvelope = 0U;
static uint32_t g_redAcEnvelope = 0U;

static const float g_firCoeffs[MAX30102_SERVICE_FIR_TAP_COUNT] = {
  -0.001542701735f, -0.002211477375f, -0.003286228748f, -0.004426511470f, -0.004758632276f,
  -0.003007677384f,  0.002192312852f,  0.011883096770f,  0.026376428080f,  0.044981528070f,
   0.065962076190f,  0.086760722100f,  0.104456014900f,  0.116349831200f,  0.120542444300f,
   0.116349831200f,  0.104456014900f,  0.086760722100f,  0.065962076190f,  0.044981528070f,
   0.026376428080f,  0.011883096770f,  0.002192312852f, -0.003007677384f, -0.004758632276f,
  -0.004426511470f, -0.003286228748f, -0.002211477375f, -0.001542701735f
};

static float g_redFirHistory[MAX30102_SERVICE_FIR_TAP_COUNT] = {0.0f};
static float g_irFirHistory[MAX30102_SERVICE_FIR_TAP_COUNT] = {0.0f};
static uint8_t g_redFirIndex = 0U;
static uint8_t g_irFirIndex = 0U;
static float g_redFilteredCache[MAX30102_SERVICE_REQUIRED_SAMPLES] = {0.0f};
static float g_irFilteredCache[MAX30102_SERVICE_REQUIRED_SAMPLES] = {0.0f};
static uint16_t g_filterCacheCount = 0U;
static float g_irWindowSum = 0.0f;
static float g_irFilteredPrev = 0.0f;
static bool g_irFilteredPrevValid = false;

static uint32_t Max30102Service_Abs32(int32_t value);
static uint16_t Max30102Service_AbsDiffU16(uint16_t lhs, uint16_t rhs);
static uint16_t Max30102Service_ClampU16(uint16_t value, uint16_t min_value, uint16_t max_value);
static uint16_t Max30102Service_CalcMedianU16(const uint16_t *values, uint8_t count);
static bool Max30102Service_IsBeatPulseActive(uint32_t now_ms);
static void Max30102Service_ClearFirHistory(void);
static void Max30102Service_ClearFilteredWindow(void);
static void Max30102Service_ClearBeatDetector(void);
static float Max30102Service_ProcessFir(float input, float *history, uint8_t *index);
static void Max30102Service_ResetFingerState(void);
static uint16_t Max30102Service_FilterBpm(uint16_t bpm);
static uint16_t Max30102Service_FilterSpO2(uint16_t spo2_x10);
static uint16_t Max30102Service_ApplySpo2SlopeLimit(uint16_t spo2_x10, uint32_t now_ms);
static uint16_t Max30102Service_CalcHeartBpm(void);
static uint16_t Max30102Service_CalcSpO2(void);
static bool Max30102Service_ProcessSampleForHeartRate(uint32_t sample_tick, float filtered_ir, float threshold);
static uint8_t Max30102Service_UpdateSignalQuality(Max30102Sample sample, bool raw_has_finger);
static void Max30102Service_UpdateBusProbe(void);
static bool Max30102Service_UpdateDiagnostics(void);
static uint8_t Max30102Service_EffectiveFifoAvailable(uint8_t available);
static void Max30102Service_ResetAfterOverflowDrain(void);
static void Max30102Service_UpdateTemperature(uint32_t now_ms);
static void Max30102Service_SetNoFinger(void);
static void Max30102Service_SetNoDevice(void);
static void Max30102Service_RecordSampleEngineRead(void);
static void Max30102Service_RunSampleEngineSelfTest(void);
static void Max30102Service_UpdateResultState(uint32_t now_ms);
static void Max30102Service_ProcessFullWindow(uint32_t now_ms);

static uint32_t Max30102Service_Abs32(int32_t value)
{
  return (uint32_t)((value < 0) ? -value : value);
}

static uint16_t Max30102Service_AbsDiffU16(uint16_t lhs, uint16_t rhs)
{
  return (uint16_t)((lhs >= rhs) ? (lhs - rhs) : (rhs - lhs));
}

static uint16_t Max30102Service_ClampU16(uint16_t value, uint16_t min_value, uint16_t max_value)
{
  if (value < min_value)
  {
    return min_value;
  }

  if (value > max_value)
  {
    return max_value;
  }

  return value;
}

static uint16_t Max30102Service_CalcMedianU16(const uint16_t *values, uint8_t count)
{
  uint16_t sorted[MAX30102_SERVICE_SPO2_WINDOW_SIZE] = {0U};

  if ((values == NULL) || (count == 0U))
  {
    return 0U;
  }

  if (count > MAX30102_SERVICE_SPO2_WINDOW_SIZE)
  {
    count = MAX30102_SERVICE_SPO2_WINDOW_SIZE;
  }

  for (uint8_t i = 0U; i < count; ++i)
  {
    sorted[i] = values[i];
  }

  for (uint8_t i = 0U; i < count; ++i)
  {
    for (uint8_t j = (uint8_t)(i + 1U); j < count; ++j)
    {
      if (sorted[j] < sorted[i])
      {
        uint16_t temp = sorted[i];
        sorted[i] = sorted[j];
        sorted[j] = temp;
      }
    }
  }

  return sorted[(uint8_t)(count >> 1U)];
}

static bool Max30102Service_IsBeatPulseActive(uint32_t now_ms)
{
  return (g_lastBeatTickMs != 0U) &&
         ((uint32_t)(now_ms - g_lastBeatTickMs) <= MAX30102_SERVICE_BEAT_PULSE_MS);
}

static void Max30102Service_ClearFirHistory(void)
{
  (void)memset(g_redFirHistory, 0, sizeof(g_redFirHistory));
  (void)memset(g_irFirHistory, 0, sizeof(g_irFirHistory));
  g_redFirIndex = 0U;
  g_irFirIndex = 0U;
}

static void Max30102Service_ClearFilteredWindow(void)
{
  (void)memset(g_redFilteredCache, 0, sizeof(g_redFilteredCache));
  (void)memset(g_irFilteredCache, 0, sizeof(g_irFilteredCache));
  g_filterCacheCount = 0U;
  g_irWindowSum = 0.0f;
  g_validSamples = 0U;
  g_snapshot.validSamples = 0U;
}

static void Max30102Service_ClearBeatDetector(void)
{
  g_lastHeartBeatSample = 0U;
  g_lastBeatIntervalMs = 0U;
  g_lastBeatTickMs = 0U;
  g_irFilteredPrev = 0.0f;
  g_irFilteredPrevValid = false;
  g_beatMissCount = 0U;
}

static float Max30102Service_ProcessFir(float input, float *history, uint8_t *index)
{
  float output = 0.0f;
  uint8_t history_index = 0U;

  if ((history == NULL) || (index == NULL))
  {
    return input;
  }

  history[*index] = input;
  history_index = *index;

  for (uint8_t i = 0U; i < MAX30102_SERVICE_FIR_TAP_COUNT; ++i)
  {
    output += g_firCoeffs[i] * history[history_index];
    history_index = (history_index == 0U) ? (MAX30102_SERVICE_FIR_TAP_COUNT - 1U) : (uint8_t)(history_index - 1U);
  }

  ++(*index);
  if (*index >= MAX30102_SERVICE_FIR_TAP_COUNT)
  {
    *index = 0U;
  }

  return output;
}

static void Max30102Service_ResetFingerState(void)
{
  Max30102Service_ClearFirHistory();
  Max30102Service_ClearFilteredWindow();
  Max30102Service_ClearBeatDetector();

  g_irDcEstimate = 0;
  g_redDcEstimate = 0;
  g_irAc = 0;
  g_redAc = 0;
  g_irAcEnvelope = 0U;
  g_redAcEnvelope = 0U;
  g_lastStableHeartBpm = 0U;
  g_lastStableSpo2X10 = 0U;
  g_lastConfidenceTickMs = 0U;
  g_signalQuality = 0U;
  g_snapshot.heartBpm = 0U;
  g_snapshot.spo2X10 = 0U;
  g_snapshot.heartValid = false;
  g_snapshot.spo2Valid = false;
  g_snapshot.beatPulse = false;
  g_snapshot.signalQuality = 0U;

  (void)memset(g_bpmHistory, 0, sizeof(g_bpmHistory));
  (void)memset(g_spo2History, 0, sizeof(g_spo2History));
  g_bpmHistoryCount = 0U;
  g_bpmHistoryIndex = 0U;
  g_spo2HistoryCount = 0U;
  g_spo2HistoryIndex = 0U;
}

static uint16_t Max30102Service_FilterBpm(uint16_t bpm)
{
  if (bpm == 0U)
  {
    return 0U;
  }

  if (g_lastStableHeartBpm == 0U)
  {
    return bpm;
  }

  if (Max30102Service_AbsDiffU16(bpm, g_lastStableHeartBpm) > MAX30102_SERVICE_BPM_MAX_JUMP_BPM)
  {
    return g_lastStableHeartBpm;
  }

  return (uint16_t)(((g_lastStableHeartBpm * (1U << MAX30102_SERVICE_BPM_FILTER_SHIFT)) + bpm) /
                    ((1U << MAX30102_SERVICE_BPM_FILTER_SHIFT) + 1U));
}

static uint16_t Max30102Service_FilterSpO2(uint16_t spo2_x10)
{
  uint16_t filtered_spo2 = 0U;

  if (spo2_x10 == 0U)
  {
    return 0U;
  }

  if ((g_lastStableSpo2X10 != 0U) &&
      (Max30102Service_AbsDiffU16(spo2_x10, g_lastStableSpo2X10) > MAX30102_SERVICE_SPO2_JUMP_LIMIT_X10))
  {
    return g_lastStableSpo2X10;
  }

  g_spo2History[g_spo2HistoryIndex] = spo2_x10;
  g_spo2HistoryIndex = (uint8_t)((g_spo2HistoryIndex + 1U) % MAX30102_SERVICE_SPO2_WINDOW_SIZE);
  if (g_spo2HistoryCount < MAX30102_SERVICE_SPO2_WINDOW_SIZE)
  {
    ++g_spo2HistoryCount;
  }

  filtered_spo2 = Max30102Service_CalcMedianU16(g_spo2History, g_spo2HistoryCount);
  return Max30102Service_ClampU16(filtered_spo2,
                                  MAX30102_SERVICE_SPO2_MIN_X10,
                                  MAX30102_SERVICE_SPO2_MAX_X10);
}

static uint16_t Max30102Service_ApplySpo2SlopeLimit(uint16_t spo2_x10, uint32_t now_ms)
{
  uint32_t elapsed_ms = 0U;
  uint32_t max_step = 0U;

  if ((spo2_x10 == 0U) || (g_lastStableSpo2X10 == 0U))
  {
    return spo2_x10;
  }

  if (g_lastConfidenceTickMs == 0U)
  {
    return spo2_x10;
  }

  elapsed_ms = now_ms - g_lastConfidenceTickMs;
  if (elapsed_ms >= 5000U)
  {
    elapsed_ms = 5000U;
  }

  max_step = (elapsed_ms * MAX30102_SERVICE_SPO2_MAX_STEP_PER_500MS) / 500U;
  if (max_step == 0U)
  {
    return g_lastStableSpo2X10;
  }

  if ((spo2_x10 > g_lastStableSpo2X10) &&
      (spo2_x10 > (uint16_t)(g_lastStableSpo2X10 + max_step)))
  {
    return (uint16_t)(g_lastStableSpo2X10 + max_step);
  }

  if ((spo2_x10 < g_lastStableSpo2X10) &&
      (g_lastStableSpo2X10 > max_step) &&
      (spo2_x10 < (uint16_t)(g_lastStableSpo2X10 - max_step)))
  {
    return (uint16_t)(g_lastStableSpo2X10 - max_step);
  }

  return spo2_x10;
}

static uint16_t Max30102Service_CalcHeartBpm(void)
{
  float average = 0.0f;
  int16_t last_peak = -1;
  uint16_t interval_sum = 0U;
  uint8_t interval_count = 0U;
  uint16_t min_interval = 0U;
  uint16_t max_interval = 0U;
  uint16_t avg_interval = 0U;
  uint16_t bpm = 0U;

  if (g_filterCacheCount < MAX30102_SERVICE_REQUIRED_SAMPLES)
  {
    return 0U;
  }

  for (uint16_t i = 0U; i < MAX30102_SERVICE_REQUIRED_SAMPLES; ++i)
  {
    average += g_irFilteredCache[i];
  }
  average /= (float)MAX30102_SERVICE_REQUIRED_SAMPLES;

  min_interval = (uint16_t)((MAX30102_SERVICE_EFFECTIVE_SAMPLE_HZ * 60U) / MAX30102_SERVICE_MAX_BPM);
  max_interval = (uint16_t)((MAX30102_SERVICE_EFFECTIVE_SAMPLE_HZ * 60U) / MAX30102_SERVICE_MIN_BPM);

  for (uint16_t i = 0U; i < (MAX30102_SERVICE_REQUIRED_SAMPLES - 1U); ++i)
  {
    if ((g_irFilteredCache[i] > average) && (g_irFilteredCache[(uint16_t)(i + 1U)] < average))
    {
      if (last_peak >= 0)
      {
        uint16_t interval = (uint16_t)(i - (uint16_t)last_peak);
        if ((interval >= min_interval) && (interval <= max_interval))
        {
          interval_sum = (uint16_t)(interval_sum + interval);
          ++interval_count;
        }
      }
      last_peak = (int16_t)i;
    }
  }

  if (interval_count == 0U)
  {
    return 0U;
  }

  avg_interval = (uint16_t)(interval_sum / interval_count);
  if (avg_interval == 0U)
  {
    return 0U;
  }

  bpm = (uint16_t)((MAX30102_SERVICE_EFFECTIVE_SAMPLE_HZ * 60U) / avg_interval);
  if ((bpm < MAX30102_SERVICE_MIN_BPM) || (bpm > MAX30102_SERVICE_MAX_BPM))
  {
    return 0U;
  }

  return bpm;
}

static uint16_t Max30102Service_CalcSpO2(void)
{
  float ir_max = 0.0f;
  float ir_min = 0.0f;
  float red_max = 0.0f;
  float red_min = 0.0f;
  float denominator = 0.0f;
  float ratio = 0.0f;
  float spo2 = 0.0f;
  int32_t spo2_x10 = 0;

  if (g_filterCacheCount < MAX30102_SERVICE_REQUIRED_SAMPLES)
  {
    return 0U;
  }

  ir_max = g_irFilteredCache[0];
  ir_min = g_irFilteredCache[0];
  red_max = g_redFilteredCache[0];
  red_min = g_redFilteredCache[0];

  for (uint16_t i = 1U; i < MAX30102_SERVICE_REQUIRED_SAMPLES; ++i)
  {
    if (g_irFilteredCache[i] > ir_max) { ir_max = g_irFilteredCache[i]; }
    if (g_irFilteredCache[i] < ir_min) { ir_min = g_irFilteredCache[i]; }
    if (g_redFilteredCache[i] > red_max) { red_max = g_redFilteredCache[i]; }
    if (g_redFilteredCache[i] < red_min) { red_min = g_redFilteredCache[i]; }
  }

  denominator = (red_max + red_min) * (ir_max - ir_min);
  if ((denominator == 0.0f) || ((ir_max - ir_min) <= 0.0f) || ((red_max - red_min) <= 0.0f))
  {
    return 0U;
  }

  ratio = ((ir_max + ir_min) * (red_max - red_min)) / denominator;
  if ((ratio < ((float)MAX30102_SERVICE_SPO2_RATIO_MIN / 1000.0f)) ||
      (ratio > ((float)MAX30102_SERVICE_SPO2_RATIO_MAX / 1000.0f)))
  {
    return 0U;
  }

  spo2 = (-45.060f * ratio * ratio) + (30.354f * ratio) + 94.845f;
  spo2_x10 = (int32_t)((spo2 * 10.0f) + 0.5f);

  if (spo2_x10 < (int32_t)MAX30102_SERVICE_SPO2_MIN_X10)
  {
    spo2_x10 = (int32_t)MAX30102_SERVICE_SPO2_MIN_X10;
  }
  else if (spo2_x10 > (int32_t)MAX30102_SERVICE_SPO2_MAX_X10)
  {
    spo2_x10 = (int32_t)MAX30102_SERVICE_SPO2_MAX_X10;
  }

  return (uint16_t)spo2_x10;
}

static bool Max30102Service_ProcessSampleForHeartRate(uint32_t sample_tick,
                                                      float filtered_ir,
                                                      float threshold)
{
  uint32_t min_interval_samples = 0U;
  uint32_t max_interval_samples = 0U;
  uint32_t elapsed_samples = 0U;
  bool beat_detected = false;

  if (sample_tick == 0U)
  {
    return false;
  }

  if (g_irFilteredPrevValid &&
      (g_irFilteredPrev > threshold) &&
      (filtered_ir < threshold))
  {
    if (g_lastHeartBeatSample == 0U)
    {
      g_lastHeartBeatSample = sample_tick;
    }
    else
    {
      min_interval_samples = (MAX30102_SERVICE_EFFECTIVE_SAMPLE_HZ * 60U) / MAX30102_SERVICE_MAX_BPM;
      max_interval_samples = (MAX30102_SERVICE_EFFECTIVE_SAMPLE_HZ * 60U) / MAX30102_SERVICE_MIN_BPM;
      elapsed_samples = sample_tick - g_lastHeartBeatSample;

      if ((elapsed_samples >= min_interval_samples) && (elapsed_samples <= max_interval_samples))
      {
        g_lastBeatIntervalMs = (uint16_t)((elapsed_samples * 1000U) /
                                          MAX30102_SERVICE_EFFECTIVE_SAMPLE_HZ);
        g_lastHeartBeatSample = sample_tick;
        g_beatMissCount = 0U;
        beat_detected = true;
      }
      else if (elapsed_samples > max_interval_samples)
      {
        g_lastHeartBeatSample = sample_tick;
      }
    }
  }
  else if (g_beatMissCount < UINT16_MAX)
  {
    ++g_beatMissCount;
  }

  g_irFilteredPrev = filtered_ir;
  g_irFilteredPrevValid = true;

  return beat_detected;
}

static uint8_t Max30102Service_UpdateSignalQuality(Max30102Sample sample, bool raw_has_finger)
{
  uint32_t ir_ac_abs = 0U;
  uint32_t red_ac_abs = 0U;
  uint32_t ir_quality_x1000 = 0U;
  uint32_t red_quality_x1000 = 0U;
  uint32_t quality_x1000 = 0U;
  uint8_t quality_percent = 0U;

  g_irDcEstimate += ((int32_t)sample.ir - g_irDcEstimate) >> 4;
  if (g_irDcEstimate < 0)
  {
    g_irDcEstimate = 0;
  }

  g_redDcEstimate += ((int32_t)sample.red - g_redDcEstimate) >> 4;
  if (g_redDcEstimate < 0)
  {
    g_redDcEstimate = 0;
  }

  g_irAc = (int32_t)sample.ir - g_irDcEstimate;
  g_redAc = (int32_t)sample.red - g_redDcEstimate;
  ir_ac_abs = Max30102Service_Abs32(g_irAc);
  red_ac_abs = Max30102Service_Abs32(g_redAc);

  g_irAcEnvelope = (ir_ac_abs > g_irAcEnvelope)
                     ? ((g_irAcEnvelope + ir_ac_abs) / 2U)
                     : ((g_irAcEnvelope * 7U + ir_ac_abs) / 8U);
  g_redAcEnvelope = (red_ac_abs > g_redAcEnvelope)
                      ? ((g_redAcEnvelope + red_ac_abs) / 2U)
                      : ((g_redAcEnvelope * 7U + red_ac_abs) / 8U);

  if (raw_has_finger && (g_irDcEstimate > 0) && (g_redDcEstimate > 0))
  {
    ir_quality_x1000 = (uint32_t)(((uint64_t)ir_ac_abs * 1000U) / (uint32_t)g_irDcEstimate);
    red_quality_x1000 = (uint32_t)(((uint64_t)red_ac_abs * 1000U) / (uint32_t)g_redDcEstimate);
    quality_x1000 = (ir_quality_x1000 < red_quality_x1000) ? ir_quality_x1000 : red_quality_x1000;
  }

  if (quality_x1000 > 100U)
  {
    quality_x1000 = 100U;
  }
  quality_percent = (uint8_t)quality_x1000;

  g_signalQuality = (uint16_t)((g_signalQuality * 3U + quality_percent) / 4U);
  if (g_signalQuality > 100U)
  {
    g_signalQuality = 100U;
  }

  return (uint8_t)g_signalQuality;
}

static void Max30102Service_UpdateBusProbe(void)
{
  Max30102BusProbe probe = {0};

  if (!MAX30102_ProbeBus(&probe))
  {
    g_snapshot.i2cReady = false;
    g_snapshot.max30102Ack = false;
    g_snapshot.i2cReadyStatus = 0xFFU;
    g_snapshot.i2cError = 0xFFFFFFFFUL;
    return;
  }

  g_snapshot.i2cReady = probe.max30102Ack;
  g_snapshot.max30102Ack = probe.max30102Ack;
  g_snapshot.busDeviceCount = probe.deviceCount;
  g_snapshot.busFirstAddress = probe.firstAddress;
  g_snapshot.i2cReadyStatus = probe.readyStatus;
  g_snapshot.i2cError = probe.i2cError;
}

static bool Max30102Service_UpdateDiagnostics(void)
{
  Max30102Diagnostics diagnostics = {0U};

  if (!MAX30102_ReadDiagnostics(&diagnostics))
  {
    if (g_snapshot.diagnosticReadFailures < UINT16_MAX)
    {
      ++g_snapshot.diagnosticReadFailures;
    }
    return false;
  }

  g_snapshot.fifoWritePtr = diagnostics.fifoWritePtr;
  g_snapshot.fifoReadPtr = diagnostics.fifoReadPtr;
  g_snapshot.fifoAvailable = diagnostics.fifoAvailable;
  g_snapshot.fifoConfig = diagnostics.fifoConfig;
  g_snapshot.modeConfig = diagnostics.modeConfig;
  g_snapshot.spo2Config = diagnostics.spo2Config;
  g_snapshot.led1Pa = diagnostics.led1Pa;
  g_snapshot.led2Pa = diagnostics.led2Pa;
  g_snapshot.interruptEnable1 = diagnostics.interruptEnable1;
  g_snapshot.interruptEnable2 = diagnostics.interruptEnable2;
  g_snapshot.overflowCounter = diagnostics.overflowCounter;

  return true;
}

static uint8_t Max30102Service_EffectiveFifoAvailable(uint8_t available)
{
  if ((available == 0U) &&
      (g_snapshot.overflowCounter != 0U) &&
      (g_snapshot.fifoWritePtr == g_snapshot.fifoReadPtr))
  {
    g_overflowDrainPendingReset = true;
    available = MAX30102_SERVICE_FIFO_DEPTH;
  }

  return available;
}

static void Max30102Service_ResetAfterOverflowDrain(void)
{
  if (!g_overflowDrainPendingReset)
  {
    return;
  }

  g_overflowDrainPendingReset = false;
  if (!MAX30102_ClearFifo() || !MAX30102_KickSampleEngine())
  {
    if (g_snapshot.diagnosticReadFailures < UINT16_MAX)
    {
      ++g_snapshot.diagnosticReadFailures;
    }
    return;
  }

  g_snapshot.fifoWritePtr = 0U;
  g_snapshot.fifoReadPtr = 0U;
  g_snapshot.fifoAvailable = 0U;
  g_snapshot.overflowCounter = 0U;
}

static void Max30102Service_UpdateTemperature(uint32_t now_ms)
{
  int16_t temperature_x10 = 0;

  if ((uint32_t)(now_ms - g_lastTempTickMs) < MAX30102_SERVICE_TEMP_UPDATE_MS)
  {
    return;
  }

  if (MAX30102_ReadTemperatureX10(&temperature_x10))
  {
    g_snapshot.temperatureX10 = temperature_x10;
  }
  g_lastTempTickMs = now_ms;
}

static void Max30102Service_SetNoFinger(void)
{
  g_snapshot.status = MAX30102_SERVICE_STATUS_NO_FINGER;
  g_snapshot.fingerDetected = false;
  g_snapshot.heartBpm = 0U;
  g_snapshot.spo2X10 = 0U;
  g_snapshot.heartValid = false;
  g_snapshot.spo2Valid = false;
  g_snapshot.beatPulse = false;
  g_snapshot.signalQuality = 0U;
  g_snapshot.validSamples = 0U;
  g_validSamples = 0U;
  g_fingerPresentStreak = 0U;
  g_fingerMissStreak = 0U;
  Max30102Service_ResetFingerState();
}

static void Max30102Service_SetNoDevice(void)
{
  Max30102Service_SetNoFinger();
  g_snapshot.status = MAX30102_SERVICE_STATUS_NO_DEVICE;
  g_snapshot.sensorReady = false;
}

static void Max30102Service_RecordSampleEngineRead(void)
{
  Max30102Sample sample = {0U, 0U};

  if (MAX30102_ReadFifoSample(&sample))
  {
    g_snapshot.sampleEngineReadCode = MAX30102_SERVICE_ENGINE_READ_OK;
    g_snapshot.sampleEngineRawRed = sample.red;
    g_snapshot.sampleEngineRawIr = sample.ir;
    return;
  }

  g_snapshot.sampleEngineReadCode = MAX30102_SERVICE_ENGINE_READ_FAIL;
  if (g_snapshot.fifoReadFailures < UINT16_MAX)
  {
    ++g_snapshot.fifoReadFailures;
  }
}

static void Max30102Service_RunSampleEngineSelfTest(void)
{
  uint8_t write_ptr = 0U;
  uint8_t read_ptr = 0U;
  uint8_t available = 0U;

  g_snapshot.sampleEngineTestCode = MAX30102_SERVICE_ENGINE_TEST_NOT_RUN;
  g_snapshot.sampleEngineFifoAfterKick = 0U;
  g_snapshot.sampleEngineReadCode = MAX30102_SERVICE_ENGINE_READ_NOT_RUN;
  g_snapshot.sampleEngineRawRed = 0U;
  g_snapshot.sampleEngineRawIr = 0U;

  HAL_Delay(MAX30102_SERVICE_SAMPLE_ENGINE_SETTLE_MS);
  if (!MAX30102_ReadFifoPointers(&write_ptr, &read_ptr, &available))
  {
    g_snapshot.sampleEngineTestCode = MAX30102_SERVICE_ENGINE_TEST_INITIAL_READ_FAIL;
    return;
  }

  if (available > g_snapshot.maxFifoAvailable)
  {
    g_snapshot.maxFifoAvailable = available;
  }

  if (available > 0U)
  {
    g_snapshot.sampleEngineTestCode = MAX30102_SERVICE_ENGINE_TEST_INITIAL_FIFO;
    g_snapshot.sampleEngineFifoAfterKick = available;
    Max30102Service_RecordSampleEngineRead();
    return;
  }

  if (!MAX30102_ClearFifo() || !MAX30102_KickSampleEngine())
  {
    g_snapshot.sampleEngineTestCode = MAX30102_SERVICE_ENGINE_TEST_KICK_WRITE_FAIL;
    return;
  }

  HAL_Delay(MAX30102_SERVICE_SAMPLE_ENGINE_SETTLE_MS);
  if (!MAX30102_ReadFifoPointers(&write_ptr, &read_ptr, &available))
  {
    g_snapshot.sampleEngineTestCode = MAX30102_SERVICE_ENGINE_TEST_KICK_READ_FAIL;
    return;
  }

  if (available > g_snapshot.maxFifoAvailable)
  {
    g_snapshot.maxFifoAvailable = available;
  }

  g_snapshot.sampleEngineFifoAfterKick = available;
  g_snapshot.sampleEngineTestCode = (available > 0U)
                                      ? MAX30102_SERVICE_ENGINE_TEST_KICK_FIFO
                                      : MAX30102_SERVICE_ENGINE_TEST_STILL_EMPTY;
  if (available > 0U)
  {
    Max30102Service_RecordSampleEngineRead();
  }
}

static void Max30102Service_UpdateResultState(uint32_t now_ms)
{
  bool result_recent = (g_lastConfidenceTickMs != 0U) &&
                       ((uint32_t)(now_ms - g_lastConfidenceTickMs) <= MAX30102_SERVICE_RESULT_KEEP_MS);

  g_snapshot.fingerDetected = (g_fingerPresentStreak >= MAX30102_SERVICE_FINGER_ENTER_STREAK);
  g_snapshot.validSamples = g_validSamples;
  g_snapshot.signalQuality = (uint8_t)g_signalQuality;
  g_snapshot.beatPulse = Max30102Service_IsBeatPulseActive(now_ms) && g_snapshot.fingerDetected;

  g_snapshot.heartValid = g_snapshot.fingerDetected && result_recent && (g_lastStableHeartBpm != 0U);
  g_snapshot.spo2Valid = g_snapshot.fingerDetected && result_recent && (g_lastStableSpo2X10 != 0U);
  g_snapshot.heartBpm = g_snapshot.heartValid ? g_lastStableHeartBpm : 0U;
  g_snapshot.spo2X10 = g_snapshot.spo2Valid ? g_lastStableSpo2X10 : 0U;

  if (!g_snapshot.fingerDetected)
  {
    g_snapshot.status = MAX30102_SERVICE_STATUS_NO_FINGER;
  }
  else if (g_snapshot.heartValid && g_snapshot.spo2Valid)
  {
    g_snapshot.status = MAX30102_SERVICE_STATUS_VALID;
  }
  else
  {
    g_snapshot.status = MAX30102_SERVICE_STATUS_COLLECTING;
  }
}

static void Max30102Service_ProcessFullWindow(uint32_t now_ms)
{
  uint16_t raw_bpm = Max30102Service_CalcHeartBpm();
  uint16_t raw_spo2 = Max30102Service_CalcSpO2();
  bool updated = false;

  if (raw_bpm != 0U)
  {
    g_lastStableHeartBpm = Max30102Service_FilterBpm(raw_bpm);
    g_snapshot.heartBpm = g_lastStableHeartBpm;
    g_snapshot.heartValid = true;
    updated = true;

    g_bpmHistory[g_bpmHistoryIndex] = raw_bpm;
    g_bpmHistoryIndex = (uint8_t)((g_bpmHistoryIndex + 1U) % MAX30102_SERVICE_BPM_WINDOW_SIZE);
    if (g_bpmHistoryCount < MAX30102_SERVICE_BPM_WINDOW_SIZE)
    {
      ++g_bpmHistoryCount;
    }
  }
  else
  {
    g_lastStableHeartBpm = 0U;
    g_snapshot.heartBpm = 0U;
    g_snapshot.heartValid = false;
  }

  if (raw_spo2 != 0U)
  {
    uint16_t filtered_spo2 = Max30102Service_FilterSpO2(raw_spo2);
    g_lastStableSpo2X10 = Max30102Service_ApplySpo2SlopeLimit(filtered_spo2, now_ms);
    g_snapshot.spo2X10 = g_lastStableSpo2X10;
    g_snapshot.spo2Valid = true;
    updated = true;
  }
  else
  {
    g_lastStableSpo2X10 = 0U;
    g_snapshot.spo2X10 = 0U;
    g_snapshot.spo2Valid = false;
  }

  if (updated)
  {
    g_lastConfidenceTickMs = now_ms;
  }

  Max30102Service_ClearFilteredWindow();
}

bool Max30102Service_Init(void)
{
  uint8_t part_id = 0U;
  int16_t temperature_x10 = 0;

  g_snapshot.status = MAX30102_SERVICE_STATUS_NOT_INITIALIZED;
  g_snapshot.sensorReady = false;
  g_snapshot.fingerDetected = false;
  g_snapshot.heartBpm = 0U;
  g_snapshot.spo2X10 = 0U;
  g_snapshot.heartValid = false;
  g_snapshot.spo2Valid = false;
  g_snapshot.beatPulse = false;
  g_snapshot.signalQuality = 0U;
  g_snapshot.rawRed = 0U;
  g_snapshot.rawIr = 0U;
  g_snapshot.temperatureX10 = 0;
  g_snapshot.partId = 0U;
  g_snapshot.validSamples = 0U;
  g_snapshot.lastUpdateMs = 0U;
  g_snapshot.fifoWritePtr = 0U;
  g_snapshot.fifoReadPtr = 0U;
  g_snapshot.fifoAvailable = 0U;
  g_snapshot.modeConfig = 0U;
  g_snapshot.spo2Config = 0U;
  g_snapshot.led1Pa = 0U;
  g_snapshot.led2Pa = 0U;
  g_snapshot.fifoConfig = 0U;
  g_snapshot.interruptEnable1 = 0U;
  g_snapshot.interruptEnable2 = 0U;
  g_snapshot.overflowCounter = 0U;
  g_snapshot.maxFifoAvailable = 0U;
  g_snapshot.samplesReadTotal = 0U;
  g_snapshot.emptyFifoPolls = 0U;
  g_snapshot.sampleEngineTestCode = MAX30102_SERVICE_ENGINE_TEST_NOT_RUN;
  g_snapshot.sampleEngineFifoAfterKick = 0U;
  g_snapshot.sampleEngineReadCode = MAX30102_SERVICE_ENGINE_READ_NOT_RUN;
  g_snapshot.sampleEngineRawRed = 0U;
  g_snapshot.sampleEngineRawIr = 0U;
  g_snapshot.fifoReadAttempts = 0U;
  g_snapshot.i2cReady = false;
  g_snapshot.max30102Ack = false;
  g_snapshot.busDeviceCount = 0U;
  g_snapshot.busFirstAddress = 0U;
  g_snapshot.i2cReadyStatus = 0U;
  g_snapshot.i2cError = 0U;
  g_snapshot.fifoReadFailures = 0U;
  g_snapshot.diagnosticReadFailures = 0U;

  g_initialized = false;
  g_lastPollTickMs = 0U;
  g_lastTempTickMs = 0U;
  g_lastFingerSeenMs = 0U;
  g_lastConfidenceTickMs = 0U;
  g_sampleCounter = 0U;
  g_overflowDrainPendingReset = false;
  Max30102Service_ResetFingerState();

  Max30102Service_UpdateBusProbe();

  if (!g_snapshot.max30102Ack || !MAX30102_ReadPartId(&part_id) || !MAX30102_Init())
  {
    g_snapshot.partId = part_id;
    Max30102Service_SetNoDevice();
    return false;
  }

  (void)MAX30102_ReadTemperatureX10(&temperature_x10);

  g_snapshot.partId = part_id;
  g_snapshot.temperatureX10 = temperature_x10;
  g_snapshot.status = MAX30102_SERVICE_STATUS_READY;
  g_snapshot.sensorReady = true;
  (void)Max30102Service_UpdateDiagnostics();
  Max30102Service_RunSampleEngineSelfTest();
  g_initialized = true;

  return true;
}

void Max30102Service_Poll(uint32_t now_ms)
{
  Max30102Sample sample = {0U, 0U};
  uint8_t available = 0U;
  uint8_t sampled_count = 0U;
  bool saw_finger_sample = false;
  bool beat_detected = false;

  g_snapshot.lastUpdateMs = now_ms;

  if (!g_initialized)
  {
    if ((g_lastRetryTickMs == 0U) || ((uint32_t)(now_ms - g_lastRetryTickMs) >= MAX30102_SERVICE_RETRY_MS))
    {
      g_lastRetryTickMs = now_ms;
      (void)Max30102Service_Init();
    }
    return;
  }

  if ((g_lastPollTickMs != 0U) &&
      ((uint32_t)(now_ms - g_lastPollTickMs) < MAX30102_SERVICE_SAMPLE_PERIOD_MS))
  {
    g_snapshot.beatPulse = Max30102Service_IsBeatPulseActive(now_ms) && g_snapshot.fingerDetected;
    return;
  }
  g_lastPollTickMs = now_ms;

  Max30102Service_UpdateBusProbe();
  if (!g_snapshot.max30102Ack)
  {
    Max30102Service_SetNoDevice();
    g_initialized = false;
    return;
  }

  g_snapshot.sensorReady = true;
  if (g_snapshot.status == MAX30102_SERVICE_STATUS_NO_DEVICE)
  {
    g_snapshot.status = MAX30102_SERVICE_STATUS_READY;
  }

  if (!Max30102Service_UpdateDiagnostics())
  {
    return;
  }
  available = Max30102Service_EffectiveFifoAvailable(g_snapshot.fifoAvailable);
  g_snapshot.fifoAvailable = available;
  if (available > g_snapshot.maxFifoAvailable)
  {
    g_snapshot.maxFifoAvailable = available;
  }
  if ((available == 0U) && (g_snapshot.emptyFifoPolls < UINT32_MAX))
  {
    ++g_snapshot.emptyFifoPolls;
  }

  for (uint8_t i = 0U; i < available; ++i)
  {
    bool raw_has_finger = false;
    float filtered_red = 0.0f;
    float filtered_ir = 0.0f;
    float threshold = 0.0f;

    if (g_snapshot.fifoReadAttempts < UINT32_MAX)
    {
      ++g_snapshot.fifoReadAttempts;
    }

    if (!MAX30102_ReadFifoSample(&sample))
    {
      if (g_snapshot.fifoReadFailures < UINT16_MAX)
      {
        ++g_snapshot.fifoReadFailures;
      }
      break;
    }

    ++sampled_count;
    if (g_snapshot.samplesReadTotal < UINT32_MAX)
    {
      ++g_snapshot.samplesReadTotal;
    }
    ++g_sampleCounter;
    if (g_sampleCounter == 0U)
    {
      g_sampleCounter = 1U;
    }

    g_snapshot.rawRed = sample.red;
    g_snapshot.rawIr = sample.ir;
    raw_has_finger = (sample.red > MAX30102_SERVICE_SAMPLE_THRESHOLD) &&
                     (sample.ir > MAX30102_SERVICE_SAMPLE_THRESHOLD);

    filtered_red = Max30102Service_ProcessFir((float)sample.red, g_redFirHistory, &g_redFirIndex);
    filtered_ir = Max30102Service_ProcessFir((float)sample.ir, g_irFirHistory, &g_irFirIndex);
    g_snapshot.signalQuality = Max30102Service_UpdateSignalQuality(sample, raw_has_finger);

    if (!raw_has_finger)
    {
      if (g_fingerMissStreak < UINT16_MAX)
      {
        ++g_fingerMissStreak;
      }
      Max30102Service_SetNoFinger();
      continue;
    }

    saw_finger_sample = true;
    g_fingerMissStreak = 0U;
    if (g_fingerPresentStreak < MAX30102_SERVICE_FINGER_ENTER_STREAK)
    {
      ++g_fingerPresentStreak;
    }
    g_lastFingerSeenMs = now_ms;

    if (g_filterCacheCount < MAX30102_SERVICE_REQUIRED_SAMPLES)
    {
      g_redFilteredCache[g_filterCacheCount] = filtered_red;
      g_irFilteredCache[g_filterCacheCount] = filtered_ir;
      g_irWindowSum += filtered_ir;
      ++g_filterCacheCount;
      g_validSamples = g_filterCacheCount;
    }

    threshold = (g_filterCacheCount != 0U) ? (g_irWindowSum / (float)g_filterCacheCount) : filtered_ir;
    beat_detected = Max30102Service_ProcessSampleForHeartRate(g_sampleCounter, filtered_ir, threshold) || beat_detected;

    if (g_filterCacheCount >= MAX30102_SERVICE_REQUIRED_SAMPLES)
    {
      Max30102Service_ProcessFullWindow(now_ms);
    }
  }

  Max30102Service_ResetAfterOverflowDrain();

  if (beat_detected)
  {
    g_lastBeatTickMs = now_ms;
  }

  if (sampled_count == 0U)
  {
    if (g_snapshot.fingerDetected &&
        ((uint32_t)(now_ms - g_lastFingerSeenMs) > MAX30102_SERVICE_FINGER_TIMEOUT_MS))
    {
      Max30102Service_SetNoFinger();
    }

    Max30102Service_UpdateResultState(now_ms);
    Max30102Service_UpdateTemperature(now_ms);
    return;
  }

  if (!saw_finger_sample && (g_fingerMissStreak >= MAX30102_SERVICE_FINGER_EXIT_STREAK))
  {
    Max30102Service_SetNoFinger();
  }
  else
  {
    Max30102Service_UpdateResultState(now_ms);
  }

  Max30102Service_UpdateTemperature(now_ms);
}

Max30102ServiceSnapshot Max30102Service_GetSnapshot(void)
{
  if (!g_initialized)
  {
    g_snapshot.status = MAX30102_SERVICE_STATUS_NOT_INITIALIZED;
    g_snapshot.sensorReady = false;
    g_snapshot.heartValid = false;
    g_snapshot.spo2Valid = false;
    g_snapshot.beatPulse = false;
  }

  return g_snapshot;
}
