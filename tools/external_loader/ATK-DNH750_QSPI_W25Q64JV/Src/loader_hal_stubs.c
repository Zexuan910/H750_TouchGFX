#include "loader_api.h"
#include "loader_debug.h"
#include "main.h"

static uint32_t last_cycle;
static uint64_t accumulated_cycles;
static uint32_t fallback_tick;
static uint32_t fallback_subtick;
static int dwt_tick_ready;

void loader_time_init(void)
{
  uint32_t first_cycle;
  uint32_t second_cycle;

  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  DWT->LAR = 0xC5ACCE55U;
  DWT->CYCCNT = 0U;
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

  first_cycle = DWT->CYCCNT;
  for (volatile uint32_t spin = 0U; spin < 1024U; ++spin)
  {
  }
  second_cycle = DWT->CYCCNT;

  last_cycle = DWT->CYCCNT;
  accumulated_cycles = 0U;
  fallback_tick = 0U;
  fallback_subtick = 0U;
  dwt_tick_ready = (((DWT->CTRL & DWT_CTRL_CYCCNTENA_Msk) != 0U) &&
                    ((DWT->CTRL & DWT_CTRL_NOCYCCNT_Msk) == 0U) &&
                    (second_cycle != first_cycle)) ? 1 : 0;

  LOADER_DEBUG_MARK(dwt_tick_ready ? LOADER_DBG_TIME_DWT_READY : LOADER_DBG_TIME_FALLBACK,
                    DWT->CTRL,
                    first_cycle,
                    second_cycle);
}

uint32_t HAL_GetTick(void)
{
  uint32_t cycles_per_ms;
  uint32_t current_cycle;
  uint32_t delta;

  if (!dwt_tick_ready || (SystemCoreClock == 0U))
  {
    fallback_subtick++;
    if (fallback_subtick >= 4096U)
    {
      fallback_subtick = 0U;
      fallback_tick++;
    }
    return fallback_tick;
  }

  cycles_per_ms = SystemCoreClock / 1000U;
  if (cycles_per_ms == 0U)
  {
    return fallback_tick;
  }

  current_cycle = DWT->CYCCNT;
  delta = current_cycle - last_cycle;
  last_cycle = current_cycle;
  accumulated_cycles += delta;

  return (uint32_t)(accumulated_cycles / cycles_per_ms);
}

void HAL_Delay(uint32_t Delay)
{
  uint32_t start = HAL_GetTick();
  while ((HAL_GetTick() - start) < Delay)
  {
  }
}

void HAL_IncTick(void)
{
  fallback_tick++;
}

HAL_StatusTypeDef HAL_InitTick(uint32_t TickPriority)
{
  (void)TickPriority;
  return HAL_OK;
}

void HAL_MspInit(void)
{
}

void Error_Handler(void)
{
  while (1)
  {
  }
}
