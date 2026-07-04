#include <gui/common/LockHomeState.hpp>

#include "stm32h7xx_hal.h"

LockHomeState::LockHomeState()
    : dragX(0),
      dragY(0),
      lastTapMs(0),
      hasTap(false)
{
}

void LockHomeState::beginTouch()
{
    dragX = 0;
    dragY = 0;
}

void LockHomeState::addDrag(int16_t deltaX, int16_t deltaY)
{
    dragX = static_cast<int16_t>(dragX + deltaX);
    dragY = static_cast<int16_t>(dragY + deltaY);
}

bool LockHomeState::isDownSwipe() const
{
    return dragY >= SwipeThreshold && dragY > -dragX && dragY > dragX;
}

bool LockHomeState::isLeftSwipe() const
{
    return dragX <= -SwipeThreshold && -dragX > dragY && -dragX > -dragY;
}

bool LockHomeState::acceptsVerticalSwipeGesture(int16_t velocity) const
{
    return isDownSwipe() || velocity > 0;
}

bool LockHomeState::acceptsHorizontalSwipeGesture(int16_t velocity) const
{
    return isLeftSwipe() || velocity < 0;
}

bool LockHomeState::releaseIsDoubleTap()
{
    const uint32_t nowMs = HAL_GetTick();
    const bool isDoubleTap = hasTap && ((nowMs - lastTapMs) <= DoubleTapWindowMs);
    lastTapMs = nowMs;
    hasTap = !isDoubleTap;
    return isDoubleTap;
}
