#include "qspi.h"

QSPI_HandleTypeDef hqspi;

HAL_StatusTypeDef qspi_init(void)
{
  hqspi.Instance = QUADSPI;
  hqspi.Init.ClockPrescaler = 1;
  hqspi.Init.FifoThreshold = 32;
  hqspi.Init.SampleShifting = QSPI_SAMPLE_SHIFTING_HALFCYCLE;
  hqspi.Init.FlashSize = 22;
  hqspi.Init.ChipSelectHighTime = QSPI_CS_HIGH_TIME_6_CYCLE;
  hqspi.Init.ClockMode = QSPI_CLOCK_MODE_0;
  hqspi.Init.FlashID = QSPI_FLASH_ID_1;
  hqspi.Init.DualFlash = QSPI_DUALFLASH_DISABLE;

  return HAL_QSPI_Init(&hqspi);
}

HAL_StatusTypeDef qspi_deinit(void)
{
  return HAL_QSPI_DeInit(&hqspi);
}

void HAL_QSPI_MspInit(QSPI_HandleTypeDef* qspiHandle)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

  if (qspiHandle->Instance == QUADSPI)
  {
    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_QSPI;
    PeriphClkInitStruct.QspiClockSelection = RCC_QSPICLKSOURCE_D1HCLK;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_RCC_QSPI_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOF_CLK_ENABLE();

    GPIO_InitStruct.Pin = QSPI_CLK_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF9_QUADSPI;
    HAL_GPIO_Init(QSPI_CLK_GPIO_Port, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = QSPI_NCS_Pin;
    GPIO_InitStruct.Alternate = GPIO_AF10_QUADSPI;
    HAL_GPIO_Init(QSPI_NCS_GPIO_Port, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = QSPI_BK1_IO0_Pin | QSPI_BK1_IO1_Pin;
    GPIO_InitStruct.Alternate = GPIO_AF10_QUADSPI;
    HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = QSPI_BK1_IO2_Pin | QSPI_BK1_IO3_Pin;
    GPIO_InitStruct.Alternate = GPIO_AF9_QUADSPI;
    HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);
  }
}

void HAL_QSPI_MspDeInit(QSPI_HandleTypeDef* qspiHandle)
{
  if (qspiHandle->Instance == QUADSPI)
  {
    __HAL_RCC_QSPI_CLK_DISABLE();

    HAL_GPIO_DeInit(QSPI_CLK_GPIO_Port, QSPI_CLK_Pin);
    HAL_GPIO_DeInit(QSPI_NCS_GPIO_Port, QSPI_NCS_Pin);
    HAL_GPIO_DeInit(GPIOF, QSPI_BK1_IO0_Pin | QSPI_BK1_IO1_Pin | QSPI_BK1_IO2_Pin | QSPI_BK1_IO3_Pin);
  }
}
