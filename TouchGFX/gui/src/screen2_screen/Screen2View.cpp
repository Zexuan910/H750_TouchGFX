#include <gui/screen2_screen/Screen2View.hpp>
#include <touchgfx/Color.hpp>
#include <texts/TextKeysAndLanguages.hpp>

Screen2View::Screen2View()
    : pageIndex(0), pressX(0), pressY(0)
{

}

void Screen2View::setupScreen()
{
    Screen2ViewBase::setupScreen();
    pageIndex = 0;
    showPage();
}

void Screen2View::tearDownScreen()
{
    Screen2ViewBase::tearDownScreen();
}

void Screen2View::handleClickEvent(const touchgfx::ClickEvent& evt)
{
    Screen2ViewBase::handleClickEvent(evt);

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

void Screen2View::handleSwipe(int dx, int dy)
{
    const int SWIPE_THRESHOLD = 40;

    if (dx < -SWIPE_THRESHOLD)
    {
        application().gotoScreen3ScreenNoTransition();
        return;
    }

    if (dy < -SWIPE_THRESHOLD)
    {
        nextPage();
    }
    else if (dy > SWIPE_THRESHOLD)
    {
        prevPage();
    }
}

void Screen2View::nextPage()
{
    if (pageIndex < 4)
    {
        pageIndex++;
    }
    showPage();
}

void Screen2View::prevPage()
{
    if (pageIndex > 0)
    {
        pageIndex--;
    }
    showPage();
}

void Screen2View::showPage()
{
    switch (pageIndex)
    {
    case 0:
        rectText1.setTypedText(touchgfx::TypedText(T_T_RECT_1));
        rectText2.setTypedText(touchgfx::TypedText(T_T_RECT_2));
        pageText.setTypedText(touchgfx::TypedText(T_T_PAGE_1));
        rectBox1.setColor(touchgfx::Color::getColorFromRGB(255, 80, 80));
        rectBox2.setColor(touchgfx::Color::getColorFromRGB(80, 255, 80));
        break;

    case 1:
        rectText1.setTypedText(touchgfx::TypedText(T_T_RECT_3));
        rectText2.setTypedText(touchgfx::TypedText(T_T_RECT_4));
        pageText.setTypedText(touchgfx::TypedText(T_T_PAGE_2));
        rectBox1.setColor(touchgfx::Color::getColorFromRGB(80, 80, 255));
        rectBox2.setColor(touchgfx::Color::getColorFromRGB(255, 255, 80));
        break;

    case 2:
        rectText1.setTypedText(touchgfx::TypedText(T_T_RECT_5));
        rectText2.setTypedText(touchgfx::TypedText(T_T_RECT_6));
        pageText.setTypedText(touchgfx::TypedText(T_T_PAGE_3));
        rectBox1.setColor(touchgfx::Color::getColorFromRGB(255, 80, 255));
        rectBox2.setColor(touchgfx::Color::getColorFromRGB(80, 255, 255));
        break;

    case 3:
        rectText1.setTypedText(touchgfx::TypedText(T_T_RECT_7));
        rectText2.setTypedText(touchgfx::TypedText(T_T_RECT_8));
        pageText.setTypedText(touchgfx::TypedText(T_T_PAGE_4));
        rectBox1.setColor(touchgfx::Color::getColorFromRGB(180, 120, 60));
        rectBox2.setColor(touchgfx::Color::getColorFromRGB(60, 120, 180));
        break;

    case 4:
        rectText1.setTypedText(touchgfx::TypedText(T_T_RECT_9));
        rectText2.setTypedText(touchgfx::TypedText(T_T_RECT_10));
        pageText.setTypedText(touchgfx::TypedText(T_T_PAGE_5));
        rectBox1.setColor(touchgfx::Color::getColorFromRGB(120, 120, 120));
        rectBox2.setColor(touchgfx::Color::getColorFromRGB(220, 160, 60));
        break;

    default:
        break;
    }

    rectBox1.invalidate();
    rectBox2.invalidate();
    rectText1.invalidate();
    rectText2.invalidate();
    pageText.invalidate();
}
