#include <gui/home_screen/homeView.hpp>

homeView::homeView()
    : lockHomeState()
{
}

void homeView::setupScreen()
{
    homeViewBase::setupScreen();
}

void homeView::tearDownScreen()
{
    homeViewBase::tearDownScreen();
}

void homeView::handleClickEvent(const touchgfx::ClickEvent& event)
{
    homeViewBase::handleClickEvent(event);

    if (event.getType() == touchgfx::ClickEvent::PRESSED)
    {
        lockHomeState.beginTouch();
    }
}

void homeView::handleDragEvent(const touchgfx::DragEvent& event)
{
    homeViewBase::handleDragEvent(event);

    lockHomeState.addDrag(event.getDeltaX(), event.getDeltaY());
    if (lockHomeState.isDownSwipe())
    {
        application().gotolockScreenNoTransition();
    }
    else if (lockHomeState.isLeftSwipe())
    {
        application().gotohomeScreenNoTransition();
    }
}

void homeView::handleGestureEvent(const touchgfx::GestureEvent& event)
{
    homeViewBase::handleGestureEvent(event);

    if (event.getType() == touchgfx::GestureEvent::SWIPE_VERTICAL &&
        lockHomeState.acceptsVerticalSwipeGesture(event.getVelocity()))
    {
        application().gotolockScreenNoTransition();
    }
    else if (event.getType() == touchgfx::GestureEvent::SWIPE_HORIZONTAL &&
             lockHomeState.acceptsHorizontalSwipeGesture(event.getVelocity()))
    {
        application().gotohomeScreenNoTransition();
    }
}

void homeView::handleTickEvent()
{
}
