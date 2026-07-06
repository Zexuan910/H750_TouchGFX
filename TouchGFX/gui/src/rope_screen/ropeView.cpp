#include <gui/rope_screen/ropeView.hpp>
#include <cstdio>
#include <images/BitmapDatabase.hpp>
#include <stm32h7xx_hal.h>
#include <touchgfx/Color.hpp>

ropeView::ropeView()
    : pressX(0),
      pressY(0),
      detailBuilt(false),
      sportRunning(false),
      pressInSportButton(false),
      sportStartMs(0),
      displayedSportSeconds(0),
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
      counterBuffer{0},
      timeBuffer{0}
{
}

void ropeView::setupScreen()
{
    ropeViewBase::setupScreen();
    setupSportDetail();
    applyStaticText();
    setSportButtonLabel();
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
        pressInSportButton = isInSportButton(pressX, pressY);
    }
    else if (evt.getType() == touchgfx::ClickEvent::RELEASED)
    {
        const int dx = evt.getX() - pressX;
        const int dy = evt.getY() - pressY;
        if (pressInSportButton && isInSportButton(evt.getX(), evt.getY()) && dx < 20 && dx > -20 && dy < 20 && dy > -20)
        {
            toggleSportMode();
            pressInSportButton = false;
            return;
        }
        pressInSportButton = false;
        handleSwipe(dx, dy);
    }
}

void ropeView::handleTickEvent()
{
    ropeViewBase::handleTickEvent();
    updateSportDuration(HAL_GetTick());
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

    sportButtonBox.setPosition(52, 248, 136, 28);
    sportButtonBox.setAlpha(230);
    add(sportButtonBox);

    sportButtonText.setScale(2);
    sportButtonText.setColor(touchgfx::Color::getColorFromRGB(5, 12, 18));
    add(sportButtonText);

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

void ropeView::startSportMode(uint32_t now)
{
    sportRunning = true;
    sportStartMs = now;
    displayedSportSeconds = 0U;
    statValueText[0].setText("00:00");
    setSportButtonLabel();
}

void ropeView::stopSportMode()
{
    if (!sportRunning)
    {
        return;
    }

    sportRunning = false;
    setSportButtonLabel();
}

void ropeView::updateSportDuration(uint32_t now)
{
    if (!sportRunning)
    {
        return;
    }

    const uint32_t elapsedSeconds = (now - sportStartMs) / 1000U;
    if (elapsedSeconds == displayedSportSeconds)
    {
        return;
    }

    displayedSportSeconds = elapsedSeconds;
    const unsigned int minutes = static_cast<unsigned int>((elapsedSeconds / 60U) % 100U);
    const unsigned int seconds = static_cast<unsigned int>(elapsedSeconds % 60U);
    (void)std::snprintf(timeBuffer, sizeof(timeBuffer), "%02u:%02u", minutes, seconds);
    statValueText[0].setText(timeBuffer);
}

bool ropeView::isInSportButton(int x, int y) const
{
    return x >= 52 && x < 188 && y >= 248 && y < 276;
}

void ropeView::toggleSportMode()
{
    if (sportRunning)
    {
        stopSportMode();
    }
    else
    {
        startSportMode(HAL_GetTick());
    }
}

void ropeView::setSportButtonLabel()
{
    if (!detailBuilt)
    {
        return;
    }

    if (sportRunning)
    {
        sportButtonBox.setColor(touchgfx::Color::getColorFromRGB(255, 106, 61));
        sportButtonText.setPosition(103, 255, 0, 0);
        sportButtonText.setText("END");
    }
    else
    {
        sportButtonBox.setColor(touchgfx::Color::getColorFromRGB(63, 212, 122));
        sportButtonText.setPosition(91, 255, 0, 0);
        sportButtonText.setText("START");
    }

    sportButtonBox.invalidate();
    sportButtonText.invalidate();
}

void ropeView::handleSwipe(int dx, int dy)
{
    const int SWIPE_THRESHOLD = 40;

    if (dx > SWIPE_THRESHOLD && dy < SWIPE_THRESHOLD && dy > -SWIPE_THRESHOLD)
    {
        application().gotoscreen1ScreenNoTransition();
    }
}
