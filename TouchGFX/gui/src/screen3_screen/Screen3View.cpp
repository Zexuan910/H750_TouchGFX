#include <gui/screen3_screen/Screen3View.hpp>
#include <images/BitmapDatabase.hpp>
#include <touchgfx/Color.hpp>

SportMode Screen3View::pendingSportMode = SportMode::Walk;

Screen3View::Screen3View()
    : pressX(0), pressY(0), currentMode(SportMode::Walk), detailBuilt(false)
{

}

void Screen3View::setupScreen()
{
    Screen3ViewBase::setupScreen();
    currentMode = pendingSportMode;
    setupSportDetail();
    applySportMode();
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
    else if (dx < -SWIPE_THRESHOLD)
    {
        advanceCarousel();
    }
}

void Screen3View::handleTickEvent()
{
    Screen3ViewBase::handleTickEvent();
}

void Screen3View::setPendingSportMode(SportMode mode)
{
    pendingSportMode = mode;
}

void Screen3View::setupSportDetail()
{
    if (detailBuilt)
    {
        circle1.setVisible(false);
        circleText.setVisible(false);
        image1.setVisible(false);
        return;
    }

    circle1.setVisible(false);
    circleText.setVisible(false);
    image1.setVisible(false);

    backgroundBox.setVisible(false);

    backgroundImage.setXY(0, 0);
    backgroundImage.setBitmap(touchgfx::Bitmap(BITMAP_SPORT_BG_ID));
    add(backgroundImage);

    dimOverlay.setPosition(0, 0, 240, 280);
    dimOverlay.setColor(touchgfx::Color::getColorFromRGB(5, 13, 26));
    dimOverlay.setAlpha(96);
    add(dimOverlay);

    topAccent.setPosition(0, 0, 240, 5);
    topAccent.setColor(touchgfx::Color::getColorFromRGB(63, 212, 122));
    add(topAccent);

    titleText.setScale(2);
    titleText.setPosition(12, 14, 0, 0);
    titleText.setColor(touchgfx::Color::getColorFromRGB(245, 248, 255));
    add(titleText);

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

    detailBuilt = true;
}

void Screen3View::applySportMode()
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
    titleText.setText(detail.title);
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

void Screen3View::advanceCarousel()
{
    switch (currentMode)
    {
    case SportMode::Walk:
        currentMode = SportMode::Run;
        setPendingSportMode(currentMode);
        applySportMode();
        break;

    case SportMode::Run:
        currentMode = SportMode::Rope;
        setPendingSportMode(currentMode);
        applySportMode();
        break;

    case SportMode::Rope:
    default:
        currentMode = SportMode::Walk;
        setPendingSportMode(currentMode);
        applySportMode();
        break;
    }
}
