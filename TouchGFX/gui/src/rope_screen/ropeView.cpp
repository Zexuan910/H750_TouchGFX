#include <gui/rope_screen/ropeView.hpp>
#include <cstdio>
#include <images/BitmapDatabase.hpp>
#include <touchgfx/Color.hpp>

ropeView::ropeView()
    : pressX(0),
      pressY(0),
      detailBuilt(false),
      lastSnapshot(WatchUi::sampleSnapshot(0U)),
      heartBuffer{0},
      spo2Buffer{0},
      rawRedBuffer{0},
      rawIrBuffer{0},
      fifoBuffer{0},
      regBuffer{0},
      ledBuffer{0},
      addrBuffer{0},
      failBuffer{0},
      counterBuffer{0}
{
}

void ropeView::setupScreen()
{
    ropeViewBase::setupScreen();
    setupSportDetail();
    applyStaticText();
    applyWatchSnapshot();
}

void ropeView::tearDownScreen()
{
    ropeViewBase::tearDownScreen();
}

void ropeView::handleClickEvent(const touchgfx::ClickEvent& evt)
{
    ropeViewBase::handleClickEvent(evt);

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
    }
}

void ropeView::updateWatchSnapshot(const WatchUi::WatchSnapshot& snapshot)
{
    lastSnapshot = snapshot;
    applyWatchSnapshot();
}

void ropeView::setupSportDetail()
{
    if (detailBuilt)
    {
        return;
    }

    __background.setVisible(false);

    backgroundImage.setXY(0, 0);
    backgroundImage.setBitmap(touchgfx::Bitmap(BITMAP_SPORT_BG_ID));
    add(backgroundImage);

    dimOverlay.setPosition(0, 0, 240, 280);
    dimOverlay.setColor(touchgfx::Color::getColorFromRGB(5, 13, 26));
    dimOverlay.setAlpha(96);
    add(dimOverlay);

    topAccent.setPosition(0, 0, 240, 5);
    topAccent.setColor(touchgfx::Color::getColorFromRGB(108, 140, 255));
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

    counterText.setVisible(false);

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

void ropeView::applyStaticText()
{
    titleText.setText("ROPE");
    heartText.setText("HR 132");
    mainValueText.setText("860");
    mainLabelText.setText("COUNT");

    const char* value[5] = { "00:12", "98%", "72", "68", "95" };
    const char* label[5] = { "TIME", "SPO2", "NOW", "AVG", "KCAL" };

    for (int i = 0; i < 5; i++)
    {
        statValueText[i].setText(value[i]);
        statLabelText[i].setText(label[i]);
    }
}

void ropeView::applyWatchSnapshot()
{
    if (!detailBuilt)
    {
        return;
    }

    if (!lastSnapshot.sensorReady)
    {
        heartText.setText("SENSOR ERR");
        statValueText[1].setText("ERR");
    }
    else if (!lastSnapshot.fingerDetected)
    {
        heartText.setText("PLACE FINGER");
        statValueText[1].setText("--");
    }
    else if (!lastSnapshot.sensorValid)
    {
        heartText.setText("MEASURING");
        statValueText[1].setText("--");
    }
    else
    {
        (void)std::snprintf(heartBuffer, sizeof(heartBuffer), "HR %u", lastSnapshot.heartRate);
        (void)std::snprintf(spo2Buffer, sizeof(spo2Buffer), "%u%%", lastSnapshot.spo2Percent);
        heartText.setText(heartBuffer);
        statValueText[1].setText(spo2Buffer);
    }
}

void ropeView::handleSwipe(int dx, int dy)
{
    const int SWIPE_THRESHOLD = 40;

    if (dx > SWIPE_THRESHOLD && dy < SWIPE_THRESHOLD && dy > -SWIPE_THRESHOLD)
    {
        application().gotoscreen1ScreenNoTransition();
    }
}
