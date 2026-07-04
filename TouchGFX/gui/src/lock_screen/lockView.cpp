#include <gui/lock_screen/lockView.hpp>

lockView::lockView()
    : lockHomeState()
{
}

void lockView::setupScreen()
{
    lockViewBase::setupScreen();
}

void lockView::tearDownScreen()
{
    lockViewBase::tearDownScreen();
}

void lockView::handleClickEvent(const touchgfx::ClickEvent& event)
{
    lockViewBase::handleClickEvent(event);

    if (event.getType() == touchgfx::ClickEvent::PRESSED)
    {
        lockHomeState.beginTouch();
    }
    else if (event.getType() == touchgfx::ClickEvent::RELEASED)
    {
        if (lockHomeState.releaseIsDoubleTap())
        {
            application().gotohomeScreenNoTransition();
        }
    }
}

void lockView::handleTickEvent()
{
}
