/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : STM32TouchController.cpp
  * @brief             : CST816T touch bridge for TouchGFX.
  ******************************************************************************
  */
/* USER CODE END Header */

/* USER CODE BEGIN STM32TouchController */

#include <STM32TouchController.hpp>

extern "C"
{
#include "cst816t.h"
}

void STM32TouchController::init()
{
#if ENABLE_CST816T_TOUCH
    CST816T_Init();
#endif
}

bool STM32TouchController::sampleTouch(int32_t& x, int32_t& y)
{
#if ENABLE_CST816T_TOUCH
    uint16_t touchX = 0;
    uint16_t touchY = 0;

    if (CST816T_ReadTouch(&touchX, &touchY))
    {
        x = touchX;
        y = touchY;
        return true;
    }
#else
    (void)x;
    (void)y;
#endif

    return false;
}

/* USER CODE END STM32TouchController */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
