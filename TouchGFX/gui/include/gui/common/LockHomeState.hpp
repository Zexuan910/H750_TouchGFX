#ifndef LOCKHOMESTATE_HPP
#define LOCKHOMESTATE_HPP

#include <stdint.h>

class LockHomeState
{
public:
    static const int16_t SwipeThreshold = 36;
    static const uint32_t DoubleTapWindowMs = 350;

    LockHomeState();

    void beginTouch();
    void addDrag(int16_t deltaX, int16_t deltaY);
    bool isDownSwipe() const;
    bool isLeftSwipe() const;
    bool acceptsVerticalSwipeGesture(int16_t velocity) const;
    bool acceptsHorizontalSwipeGesture(int16_t velocity) const;
    bool releaseIsDoubleTap();

private:
    int16_t dragX;
    int16_t dragY;
    uint32_t lastTapMs;
    bool hasTap;
};

#endif // LOCKHOMESTATE_HPP
