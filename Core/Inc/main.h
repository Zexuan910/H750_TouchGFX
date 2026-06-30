/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#ifndef ENABLE_CST816T_TOUCH
#define ENABLE_CST816T_TOUCH 0
#endif

#define LCD_SDA_Pin GPIO_PIN_5
#define LCD_SDA_GPIO_Port GPIOB
#define LCD_SCK_Pin GPIO_PIN_11
#define LCD_SCK_GPIO_Port GPIOG
#define LCD_DC_Pin GPIO_PIN_8
#define LCD_DC_GPIO_Port GPIOJ
#define LCD_RST_Pin GPIO_PIN_7
#define LCD_RST_GPIO_Port GPIOJ
#define LCD_CS_Pin GPIO_PIN_6
#define LCD_CS_GPIO_Port GPIOJ
#define LCD_BL_Pin GPIO_PIN_8
#define LCD_BL_GPIO_Port GPIOA

#define QSPI_CLK_Pin GPIO_PIN_2
#define QSPI_CLK_GPIO_Port GPIOB
#define QSPI_NCS_Pin GPIO_PIN_6
#define QSPI_NCS_GPIO_Port GPIOB
#define QSPI_BK1_IO0_Pin GPIO_PIN_8
#define QSPI_BK1_IO0_GPIO_Port GPIOF
#define QSPI_BK1_IO1_Pin GPIO_PIN_9
#define QSPI_BK1_IO1_GPIO_Port GPIOF
#define QSPI_BK1_IO2_Pin GPIO_PIN_7
#define QSPI_BK1_IO2_GPIO_Port GPIOF
#define QSPI_BK1_IO3_Pin GPIO_PIN_6
#define QSPI_BK1_IO3_GPIO_Port GPIOF

#if ENABLE_CST816T_TOUCH
#define CTP_SCL_Pin GPIO_PIN_6
#define CTP_SCL_GPIO_Port GPIOB
#define CTP_SDA_Pin GPIO_PIN_7
#define CTP_SDA_GPIO_Port GPIOB
#endif

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
