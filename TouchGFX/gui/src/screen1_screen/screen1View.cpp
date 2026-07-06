#include <gui/screen1_screen/screen1View.hpp>
#include <images/BitmapDatabase.hpp>
#include <touchgfx/Color.hpp>

screen1View::screen1View()
    : pressX(0), pressY(0), navigationBuilt(false)
{

}

void screen1View::setupScreen()
{
    screen1ViewBase::setupScreen();
    setupNavigation();
    updateNavigation();
}

void screen1View::tearDownScreen()
{
    screen1ViewBase::tearDownScreen();
}

bool screen1View::isInCard(int index, int x, int y) const
{
    const int cardY[3] = { 70, 128, 186 };
    return index >= 0 &&
           index < 3 &&
           x >= 18 &&
           x <= 222 &&
           y >= cardY[index] &&
           y <= (cardY[index] + 44);
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

        if (dx < 20 && dx > -20 && dy < 20 && dy > -20)
        {
            for (int i = 0; i < 3; i++)
            {
                if (isInCard(i, evt.getX(), evt.getY()))
                {
                    openCard(i);
                    break;
                }
            }
        }
    }
}

void screen1View::handleSwipe(int dx, int dy)
{
    const int SWIPE_THRESHOLD = 40;

    if (dx > SWIPE_THRESHOLD && (dy < SWIPE_THRESHOLD && dy > -SWIPE_THRESHOLD))
    {
        application().gotohomeScreenNoTransition();
    }
}

void screen1View::setupNavigation()
{
    if (navigationBuilt)
    {
        return;
    }

    touchBox.setVisible(false);
    touchText.setVisible(false);
    displayBox.setVisible(false);
    displayText.setVisible(false);
    displayImage.setVisible(false);
    nextButton.setVisible(false);
    nextButton.setTouchable(false);

    backgroundImage.setXY(0, 0);
    backgroundImage.setBitmap(touchgfx::Bitmap(BITMAP_WATCH_BG_ID));
    add(backgroundImage);

    dimOverlay.setPosition(0, 0, 240, 280);
    dimOverlay.setColor(touchgfx::Color::getColorFromRGB(8, 18, 28));
    dimOverlay.setAlpha(132);
    add(dimOverlay);

    topAccent.setPosition(0, 0, 240, 5);
    topAccent.setColor(touchgfx::Color::getColorFromRGB(63, 212, 122));
    add(topAccent);

    titleText.setScale(2);
    titleText.setPosition(18, 18, 0, 0);
    titleText.setColor(touchgfx::Color::getColorFromRGB(245, 248, 255));
    add(titleText);

    hintText.setScale(1);
    hintText.setPosition(18, 44, 0, 0);
    hintText.setColor(touchgfx::Color::getColorFromRGB(151, 169, 190));
    add(hintText);

    const int cardY[3] = { 70, 128, 186 };
    const uint8_t cardColor[3][3] = {
        { 24, 78, 66 },
        { 106, 51, 38 },
        { 48, 58, 125 }
    };

    for (int i = 0; i < 3; i++)
    {
        cardBox[i].setPosition(18, cardY[i], 204, 44);
        cardBox[i].setColor(touchgfx::Color::getColorFromRGB(cardColor[i][0], cardColor[i][1], cardColor[i][2]));
        cardBox[i].setAlpha(226);
        add(cardBox[i]);

        cardTitleText[i].setScale(2);
        cardTitleText[i].setPosition(30, cardY[i] + 8, 0, 0);
        cardTitleText[i].setColor(touchgfx::Color::getColorFromRGB(245, 248, 255));
        add(cardTitleText[i]);

        cardHintText[i].setScale(1);
        cardHintText[i].setPosition(142, cardY[i] + 18, 0, 0);
        cardHintText[i].setColor(touchgfx::Color::getColorFromRGB(190, 206, 222));
        add(cardHintText[i]);
    }

    navigationBuilt = true;
}

void screen1View::updateNavigation()
{
    titleText.setText("SPORT");
    hintText.setText("TAP MODE");

    cardTitleText[0].setText("WALK");
    cardTitleText[1].setText("RUN");
    cardTitleText[2].setText("ROPE");

    cardHintText[0].setText("START");
    cardHintText[1].setText("START");
    cardHintText[2].setText("START");
}

void screen1View::openCard(int index)
{
    switch (index)
    {
    case 0:
        application().gotowalkScreenNoTransition();
        break;

    case 1:
        application().gotorunScreenNoTransition();
        break;

    case 2:
        application().gotoropeScreenNoTransition();
        break;

    default:
        break;
    }
}
