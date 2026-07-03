#include <gui/screen2_screen/Screen2View.hpp>
#include <cstdlib>
#include <images/BitmapDatabase.hpp>
#include <touchgfx/Color.hpp>

Screen2View::Screen2View()
    : pressX(0), pressY(0), menuBuilt(false), detailBuilt(false), pageState(PageState::Nav), currentMode(SportMode::Walk)
{

}

void Screen2View::setupScreen()
{
    Screen2ViewBase::setupScreen();
    setupSportMenu();
    setupSportDetail();
    showNav();
    updateSportMenu();
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

        if (pageState == PageState::Nav && abs(dx) < 20 && abs(dy) < 20)
        {
            for (int i = 0; i < 3; i++)
            {
                if (isInCard(i, evt.getX(), evt.getY()))
                {
                    openSport(static_cast<SportMode>(i));
                    return;
                }
            }
        }

        handleSwipe(dx, dy);
    }
}

void Screen2View::handleTickEvent()
{
    Screen2ViewBase::handleTickEvent();
}

void Screen2View::setupSportMenu()
{
    if (menuBuilt)
    {
        return;
    }

    rectBox1.setVisible(false);
    rectBox2.setVisible(false);
    rectText1.setVisible(false);
    rectText2.setVisible(false);
    pageText.setVisible(false);
    image1.setVisible(false);
    image2.setVisible(false);

    __background.setVisible(false);

    navBackgroundImage.setXY(0, 0);
    navBackgroundImage.setBitmap(touchgfx::Bitmap(BITMAP_WATCH_BG_ID));
    add(navBackgroundImage);

    detailBackgroundImage.setXY(0, 0);
    detailBackgroundImage.setBitmap(touchgfx::Bitmap(BITMAP_SPORT_BG_ID));
    detailBackgroundImage.setVisible(false);
    add(detailBackgroundImage);

    dimOverlay.setPosition(0, 0, 240, 280);
    dimOverlay.setColor(touchgfx::Color::getColorFromRGB(9, 18, 10));
    dimOverlay.setAlpha(58);
    add(dimOverlay);

    headerBand.setPosition(0, 0, 240, 66);
    headerBand.setColor(touchgfx::Color::getColorFromRGB(18, 35, 17));
    headerBand.setAlpha(178);
    add(headerBand);

    titleText.setScale(2);
    titleText.setPosition(77, 13, 0, 0);
    titleText.setColor(touchgfx::Color::getColorFromRGB(245, 248, 255));
    titleText.setText("SPORT");
    add(titleText);

    subtitleText.setScale(1);
    subtitleText.setPosition(84, 42, 0, 0);
    subtitleText.setColor(touchgfx::Color::getColorFromRGB(222, 238, 210));
    subtitleText.setText("CHOOSE MODE");
    add(subtitleText);

    const int cardY[3] = { 76, 138, 200 };
    const char* title[3] = { "WALK", "RUN", "ROPE" };
    const char* hint[3] = { "DISTANCE", "SPEED", "COUNT" };
    const uint8_t color[3][3] = {
        { 63, 212, 122 },
        { 255, 106, 61 },
        { 108, 140, 255 }
    };

    for (int i = 0; i < 3; i++)
    {
        cardBox[i].setPosition(18, cardY[i], 204, 48);
        cardBox[i].setColor(touchgfx::Color::getColorFromRGB(color[i][0], color[i][1], color[i][2]));
        cardBox[i].setAlpha(220);
        add(cardBox[i]);

        cardTitleText[i].setScale(2);
        cardTitleText[i].setPosition(32, cardY[i] + 8, 0, 0);
        cardTitleText[i].setColor(touchgfx::Color::getColorFromRGB(6, 10, 16));
        cardTitleText[i].setText(title[i]);
        add(cardTitleText[i]);

        cardHintText[i].setScale(1);
        cardHintText[i].setPosition(34, cardY[i] + 31, 0, 0);
        cardHintText[i].setColor(touchgfx::Color::getColorFromRGB(20, 26, 34));
        cardHintText[i].setText(hint[i]);
        add(cardHintText[i]);
    }

    menuBuilt = true;
}

void Screen2View::setupSportDetail()
{
    if (detailBuilt)
    {
        return;
    }

    topAccent.setPosition(0, 0, 240, 5);
    topAccent.setColor(touchgfx::Color::getColorFromRGB(63, 212, 122));
    add(topAccent);

    detailTitleText.setScale(2);
    detailTitleText.setPosition(12, 14, 0, 0);
    detailTitleText.setColor(touchgfx::Color::getColorFromRGB(245, 248, 255));
    add(detailTitleText);

    heartText.setScale(1);
    heartText.setPosition(157, 18, 0, 0);
    heartText.setColor(touchgfx::Color::getColorFromRGB(255, 112, 112));
    add(heartText);

    mainValueText.setScale(3);
    mainValueText.setPosition(58, 48, 0, 0);
    mainValueText.setColor(touchgfx::Color::getColorFromRGB(255, 255, 255));
    add(mainValueText);

    mainLabelText.setScale(1);
    mainLabelText.setPosition(96, 84, 0, 0);
    mainLabelText.setColor(touchgfx::Color::getColorFromRGB(144, 160, 180));
    add(mainLabelText);

    const int boxX[5] = { 12, 124, 12, 85, 158 };
    const int boxY[5] = { 110, 110, 186, 186, 186 };
    const int boxW[5] = { 104, 104, 62, 70, 70 };
    const int boxH[5] = { 58, 58, 58, 58, 58 };
    const uint8_t boxColor[5][3] = {
        { 18, 44, 82 },
        { 16, 91, 118 },
        { 22, 104, 63 },
        { 128, 72, 29 },
        { 70, 70, 143 }
    };

    for (int i = 0; i < 5; i++)
    {
        statBox[i].setPosition(boxX[i], boxY[i], boxW[i], boxH[i]);
        statBox[i].setColor(touchgfx::Color::getColorFromRGB(boxColor[i][0], boxColor[i][1], boxColor[i][2]));
        statBox[i].setAlpha(214);
        add(statBox[i]);

        statValueText[i].setScale(i < 2 ? 2 : 1);
        statValueText[i].setPosition(boxX[i] + 8, boxY[i] + 9, 0, 0);
        statValueText[i].setColor(touchgfx::Color::getColorFromRGB(238, 244, 255));
        add(statValueText[i]);

        statLabelText[i].setScale(1);
        statLabelText[i].setPosition(boxX[i] + 8, boxY[i] + 39, 0, 0);
        statLabelText[i].setColor(touchgfx::Color::getColorFromRGB(142, 158, 178));
        add(statLabelText[i]);
    }

    setDetailVisible(false);
    detailBuilt = true;
}

void Screen2View::updateSportMenu()
{
    rectBox1.setVisible(false);
    rectBox2.setVisible(false);
    rectText1.setVisible(false);
    rectText2.setVisible(false);
    pageText.setVisible(false);
    image1.setVisible(false);
    image2.setVisible(false);

    invalidate();
}

void Screen2View::openSport(SportMode mode)
{
    showDetail(mode);
}

void Screen2View::showNav()
{
    pageState = PageState::Nav;
    detailBackgroundImage.setVisible(false);
    navBackgroundImage.setVisible(true);
    dimOverlay.setColor(touchgfx::Color::getColorFromRGB(9, 18, 10));
    dimOverlay.setAlpha(58);
    setDetailVisible(false);
    setNavVisible(true);
    invalidate();
}

void Screen2View::showDetail(SportMode mode)
{
    pageState = PageState::Detail;
    currentMode = mode;
    navBackgroundImage.setVisible(false);
    detailBackgroundImage.setVisible(true);
    dimOverlay.setColor(touchgfx::Color::getColorFromRGB(5, 13, 26));
    dimOverlay.setAlpha(96);
    setNavVisible(false);
    setDetailVisible(true);
    applySportMode();
    invalidate();
}

void Screen2View::setNavVisible(bool visible)
{
    headerBand.setVisible(visible);
    titleText.setVisible(visible);
    subtitleText.setVisible(visible);

    for (int i = 0; i < 3; i++)
    {
        cardBox[i].setVisible(visible);
        cardTitleText[i].setVisible(visible);
        cardHintText[i].setVisible(visible);
    }
}

void Screen2View::setDetailVisible(bool visible)
{
    topAccent.setVisible(visible);
    detailTitleText.setVisible(visible);
    heartText.setVisible(visible);
    mainValueText.setVisible(visible);
    mainLabelText.setVisible(visible);

    for (int i = 0; i < 5; i++)
    {
        statBox[i].setVisible(visible);
        statValueText[i].setVisible(visible);
        statLabelText[i].setVisible(visible);
    }
}

void Screen2View::applySportMode()
{
    struct SportDetail
    {
        const char* title;
        const char* heart;
        const char* mainValue;
        const char* mainLabel;
        const char* value[5];
        const char* label[5];
        uint8_t accentR;
        uint8_t accentG;
        uint8_t accentB;
    };

    const SportDetail details[3] = {
        {
            "WALK",
            "HR 82",
            "1.26",
            "KM",
            { "00:18", "98%", "1.2", "1.0", "62" },
            { "TIME", "SPO2", "NOW", "AVG", "STEP" },
            63, 212, 122
        },
        {
            "RUN",
            "HR 146",
            "4.32",
            "KM",
            { "00:26", "97%", "3.6", "2.9", "84" },
            { "TIME", "SPO2", "NOW", "AVG", "CAD" },
            255, 106, 61
        },
        {
            "ROPE",
            "HR 132",
            "860",
            "COUNT",
            { "00:12", "98%", "72", "68", "95" },
            { "TIME", "SPO2", "NOW", "AVG", "KCAL" },
            108, 140, 255
        }
    };

    const int index = static_cast<int>(currentMode);
    const SportDetail& detail = details[index];

    topAccent.setColor(touchgfx::Color::getColorFromRGB(detail.accentR, detail.accentG, detail.accentB));
    detailTitleText.setText(detail.title);
    heartText.setText(detail.heart);
    mainValueText.setText(detail.mainValue);
    mainLabelText.setText(detail.mainLabel);

    for (int i = 0; i < 5; i++)
    {
        statValueText[i].setText(detail.value[i]);
        statLabelText[i].setText(detail.label[i]);
    }

    topAccent.invalidate();
}

void Screen2View::advanceSportMode()
{
    switch (currentMode)
    {
    case SportMode::Walk:
        currentMode = SportMode::Run;
        break;

    case SportMode::Run:
        currentMode = SportMode::Rope;
        break;

    case SportMode::Rope:
    default:
        currentMode = SportMode::Walk;
        break;
    }

    applySportMode();
}

bool Screen2View::isInCard(int index, int x, int y) const
{
    const int cardY[3] = { 76, 138, 200 };
    return index >= 0 && index < 3 && x >= 18 && x <= 222 && y >= cardY[index] && y <= cardY[index] + 48;
}

void Screen2View::handleSwipe(int dx, int dy)
{
    const int SWIPE_THRESHOLD = 40;

    if (dy >= SWIPE_THRESHOLD || dy <= -SWIPE_THRESHOLD)
    {
        return;
    }

    if (pageState == PageState::Detail && dx > SWIPE_THRESHOLD)
    {
        showNav();
    }
    else if (pageState == PageState::Detail && dx < -SWIPE_THRESHOLD)
    {
        advanceSportMode();
    }
}
