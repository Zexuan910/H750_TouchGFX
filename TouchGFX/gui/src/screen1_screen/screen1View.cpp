#include <gui/screen1_screen/screen1View.hpp>
#include <touchgfx/Color.hpp>
#include <texts/TextKeysAndLanguages.hpp>
#include <images/BitmapDatabase.hpp>

screen1View::screen1View()
    : demoState(0), pressX(0), pressY(0)
{

}

void screen1View::setupScreen()
{
    screen1ViewBase::setupScreen();
    demoState = 0;
    updateDisplay();
}

void screen1View::tearDownScreen()
{
    screen1ViewBase::tearDownScreen();
}

bool screen1View::isInTouchBox(int x, int y) const
{
    return (x >= 30 && x <= 210 && y >= 25 && y <= 85);
}

void screen1View::handleClickEvent(const touchgfx::ClickEvent& evt)
{
    screen1ViewBase::handleClickEvent(evt);

    if (evt.getType() == touchgfx::ClickEvent::PRESSED)
    {
        pressX = evt.getX();
        pressY = evt.getY();
    }
    else if (evt.getType() == touchgfx::ClickEvent::RELEASED)
    {
        const int dx = evt.getX() - pressX;
        const int dy = evt.getY() - pressY;
        handleSwipe(dx, dy);

        if (dx < 20 && dx > -20 && dy < 20 && dy > -20 && isInTouchBox(evt.getX(), evt.getY()))
        {
            nextDemoState();
        }
    }
}

void screen1View::handleSwipe(int dx, int dy)
{
    const int SWIPE_THRESHOLD = 40;

    if (dx > SWIPE_THRESHOLD && (dy < SWIPE_THRESHOLD && dy > -SWIPE_THRESHOLD))
    {
        application().gotoScreen2ScreenNoTransition();
    }
}

void screen1View::nextDemoState()
{
    demoState++;
    if (demoState >= 11)
    {
        demoState = 0;
    }

    updateDisplay();
}

void screen1View::updateDisplay()
{
    displayText.setVisible(true);
    displayImage.setVisible(false);

    switch (demoState)
    {
    case 0:
        displayBox.setColor(touchgfx::Color::getColorFromRGB(255, 0, 0));
        displayText.setTypedText(touchgfx::TypedText(T_T_HELLO_WORD));
        break;

    case 1:
        displayBox.setColor(touchgfx::Color::getColorFromRGB(0, 255, 0));
        displayText.setTypedText(touchgfx::TypedText(T_T_HELLO_WORD));
        break;

    case 2:
        displayBox.setColor(touchgfx::Color::getColorFromRGB(0, 0, 255));
        displayText.setTypedText(touchgfx::TypedText(T_T_HELLO_WORD));
        break;

    case 3:
        displayBox.setColor(touchgfx::Color::getColorFromRGB(32, 32, 32));
        displayText.setTypedText(touchgfx::TypedText(T_T_HELLO_WORD));
        break;

    case 4:
        displayBox.setColor(touchgfx::Color::getColorFromRGB(32, 32, 32));
        displayText.setTypedText(touchgfx::TypedText(T_T_HELLO_BUAA));
        break;

    case 5:
        displayBox.setColor(touchgfx::Color::getColorFromRGB(32, 32, 32));
        displayText.setTypedText(touchgfx::TypedText(T_T_HELLO_UBAA));
        break;

    case 6:
        displayBox.setColor(touchgfx::Color::getColorFromRGB(20, 80, 120));
        displayText.setTypedText(touchgfx::TypedText(T_T_CN_1));
        break;

    case 7:
        displayBox.setColor(touchgfx::Color::getColorFromRGB(20, 80, 120));
        displayText.setTypedText(touchgfx::TypedText(T_T_CN_2));
        break;

    case 8:
        displayBox.setColor(touchgfx::Color::getColorFromRGB(20, 80, 120));
        displayText.setTypedText(touchgfx::TypedText(T_T_CN_3));
        break;

    case 9:
        displayBox.setColor(touchgfx::Color::getColorFromRGB(255, 255, 255));
        displayText.setVisible(false);
        displayImage.setVisible(true);
        displayImage.setBitmap(touchgfx::Bitmap(BITMAP_A_ID));
        break;

    case 10:
        displayBox.setColor(touchgfx::Color::getColorFromRGB(255, 255, 255));
        displayText.setVisible(false);
        displayImage.setVisible(true);
        displayImage.setBitmap(touchgfx::Bitmap(BITMAP_B_ID));
        break;

    default:
        break;
    }

    displayBox.invalidate();
    displayText.invalidate();
    displayImage.invalidate();
}
