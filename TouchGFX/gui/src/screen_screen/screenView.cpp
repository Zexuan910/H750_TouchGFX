#include <gui/screen_screen/screenView.hpp>
#include <touchgfx/Color.hpp>

screenView::screenView()
{

}

void screenView::setupScreen()
{
    screenViewBase::setupScreen();
    T_TITLE.setColor(touchgfx::Color::getColorFromRGB(255, 255, 255));
    T_TITLE.invalidate();
}

void screenView::tearDownScreen()
{
    screenViewBase::tearDownScreen();
}
