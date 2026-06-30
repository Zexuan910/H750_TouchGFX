/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : STM32TouchController.cpp
  * @brief             : CST816T touch bridge for TouchGFX.
  ******************************************************************************
  */
/* USER CODE END Header */

#include <STM32TouchController.hpp>

#if ENABLE_CST816T_TOUCH
extern "C"
{
#include "cst816t.h"
}
#endif

void STM32TouchController::init()
{
#if ENABLE_CST816T_TOUCH
    CST816T_Init();
#endif
}

bool STM32TouchController::sampleTouch(int32_t& x, int32_t& y)
{
#if ENABLE_CST816T_TOUCH
    uint16_t tx = 0;
    uint16_t ty = 0;

    if (CST816T_ReadTouch(&tx, &ty))
    {
        x = static_cast<int32_t>(tx);
        y = static_cast<int32_t>(ty);
        return true;
    }

    return false;
#else
    (void)x;
    (void)y;
    return false;
#endif
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
