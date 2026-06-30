#include <gui/screen3_screen/Screen3View.hpp>

Screen3View::Screen3View()
    : pressX(0), pressY(0)
{

}

void Screen3View::setupScreen()
{
    Screen3ViewBase::setupScreen();
}

void Screen3View::tearDownScreen()
{
    Screen3ViewBase::tearDownScreen();
}

void Screen3View::handleClickEvent(const touchgfx::ClickEvent& evt)
{
    Screen3ViewBase::handleClickEvent(evt);

    if (evt.getType() == touchgfx::ClickEvent::PRESSED)
    {
        pressX = evt.getX();
        pressY = evt.getY();
    }
    else if (evt.getType() == touchgfx::ClickEvent::RELEASED)
    {
        int dx = evt.getX() - pressX;
        int dy = evt.getY() - pressY;
        handleSwipe(dx, dy);
    }
}

void Screen3View::handleSwipe(int dx, int dy)
{
    const int SWIPE_THRESHOLD = 40;

    if (dx > SWIPE_THRESHOLD)
    {
        application().gotoScreen2ScreenNoTransition();
    }
}
