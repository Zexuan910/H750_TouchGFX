#include <gui/screen2_screen/Screen2View.hpp>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <images/BitmapDatabase.hpp>
#include <stm32h7xx_hal.h>
#include <touchgfx/Color.hpp>

extern "C"
{
#include "imu_sensor.h"
#include "lcd_port.h"
}

template <typename WidgetType>
static void setVisibleWithLocalRefresh(WidgetType& widget, bool visible)
{
    widget.invalidate();
    widget.setVisible(visible);
    widget.invalidate();
}

Screen2View::Screen2View()
    : pressX(0),
      pressY(0),
      menuBuilt(false),
      detailBuilt(false),
      exerciseRunning(false),
      confirmVisible(false),
      startDataResetPending(false),
      screenLocked(false),
      ignoreReleaseAfterWake(false),
      hasStoredMetrics{ false, false, false },
      pageState(PageState::Nav),
      pendingSwipeAction(PendingSwipeAction::None),
      currentMode(SportMode::Walk),
      exerciseStartMs(0),
      lastMetricUpdateMs(0),
      lastTouchMs(0),
      lastWalkSampleMs(0),
      displayedExerciseSeconds(0),
      lastSnapshot(WatchUi::sampleSnapshot(0U)),
      heartBuffer{0},
      spo2Buffer{0}
{
    std::snprintf(durationBuffer, sizeof(durationBuffer), "00:00:00");
    std::snprintf(mainValueBuffer, sizeof(mainValueBuffer), "0.00");
    WalkMetrics_Reset(&walkMetricsState);
    std::memset(&walkMetricsOutput, 0, sizeof(walkMetricsOutput));

    for (int i = 0; i < 4; i++)
    {
        std::snprintf(statValueBuffer[i], sizeof(statValueBuffer[i]), "0");
    }

    for (int mode = 0; mode < 3; mode++)
    {
        std::snprintf(storedDurationBuffer[mode], sizeof(storedDurationBuffer[mode]), "00:00:00");
        std::snprintf(storedMainValueBuffer[mode], sizeof(storedMainValueBuffer[mode]), "0.00");
        for (int i = 0; i < 4; i++)
        {
            std::snprintf(storedStatValueBuffer[mode][i], sizeof(storedStatValueBuffer[mode][i]), "0");
        }
    }
}

void Screen2View::setupScreen()
{
    Screen2ViewBase::setupScreen();
    setupSportMenu();
    setupSportDetail();
    lastTouchMs = HAL_GetTick();
    wakeScreen();
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
        if (screenLocked)
        {
            wakeScreen();
            ignoreReleaseAfterWake = true;
            return;
        }

        recordTouchActivity();
    }
    else if (screenLocked)
    {
        return;
    }
    else if (evt.getType() == touchgfx::ClickEvent::RELEASED && ignoreReleaseAfterWake)
    {
        ignoreReleaseAfterWake = false;
        return;
    }

    if (evt.getType() == touchgfx::ClickEvent::PRESSED)
    {
        pressX = evt.getX();
        pressY = evt.getY();
    }
    else if (evt.getType() == touchgfx::ClickEvent::RELEASED)
    {
        recordTouchActivity();
        int dx = evt.getX() - pressX;
        int dy = evt.getY() - pressY;

        if (confirmVisible && abs(dx) < 20 && abs(dy) < 20)
        {
            if (isInConfirmYes(evt.getX(), evt.getY()))
            {
                confirmEndExercise();
            }
            else if (isInConfirmNo(evt.getX(), evt.getY()))
            {
                hideConfirm();
            }

            return;
        }

        if (confirmVisible)
        {
            return;
        }

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
        else if (pageState == PageState::Detail && abs(dx) < 20 && abs(dy) < 20 && isInStartButton(evt.getX(), evt.getY()))
        {
            toggleExercise();
            return;
        }

        handleSwipe(dx, dy);
    }
}

void Screen2View::handleTickEvent()
{
    Screen2ViewBase::handleTickEvent();

    const uint32_t now = HAL_GetTick();
    lockScreenIfIdle(now);

    if (screenLocked)
    {
        return;
    }

    if (startDataResetPending)
    {
        startDataResetPending = false;
        displayedExerciseSeconds = 0;
        updateExerciseDuration(true);
        setExerciseMetricsZero();
        lastMetricUpdateMs = now;
        return;
    }

    if (exerciseRunning)
    {
        updateExerciseDuration(false);

        if (currentMode == SportMode::Walk && updateWalkMetrics(now))
        {
            return;
        }

        if ((now - lastMetricUpdateMs) >= 500U)
        {
            lastMetricUpdateMs = now;
            updateExerciseMetrics(false);
        }
    }
}

void Screen2View::updateWatchSnapshot(const WatchUi::WatchSnapshot& snapshot)
{
    lastSnapshot = snapshot;
    applyWatchSnapshot();
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

    durationText.setScale(2);
    durationText.setPosition(73, 8, 0, 0);
    durationText.setColor(touchgfx::Color::getColorFromRGB(226, 246, 255));
    durationText.setText(durationBuffer);
    add(durationText);

    detailTitleText.setScale(2);
    detailTitleText.setPosition(12, 31, 0, 0);
    detailTitleText.setColor(touchgfx::Color::getColorFromRGB(245, 248, 255));
    add(detailTitleText);

    heartText.setScale(1);
    heartText.setPosition(157, 34, 0, 0);
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

    const int boxX[4] = { 12, 124, 12, 124 };
    const int boxY[4] = { 112, 112, 180, 180 };
    const int boxW[4] = { 104, 104, 104, 104 };
    const int boxH[4] = { 56, 56, 56, 56 };
    const uint8_t boxColor[4][3] = {
        { 16, 91, 118 },
        { 22, 104, 63 },
        { 128, 72, 29 },
        { 70, 70, 143 }
    };

    for (int i = 0; i < 4; i++)
    {
        statBox[i].setPosition(boxX[i], boxY[i], boxW[i], boxH[i]);
        statBox[i].setColor(touchgfx::Color::getColorFromRGB(boxColor[i][0], boxColor[i][1], boxColor[i][2]));
        statBox[i].setAlpha(214);
        add(statBox[i]);

        statValueText[i].setScale(i < 1 ? 2 : 1);
        statValueText[i].setPosition(boxX[i] + 8, boxY[i] + 9, 0, 0);
        statValueText[i].setColor(touchgfx::Color::getColorFromRGB(238, 244, 255));
        add(statValueText[i]);

        statLabelText[i].setScale(1);
        statLabelText[i].setPosition(boxX[i] + 8, boxY[i] + 39, 0, 0);
        statLabelText[i].setColor(touchgfx::Color::getColorFromRGB(142, 158, 178));
        add(statLabelText[i]);
    }

    startButtonBox.setPosition(52, 246, 136, 30);
    startButtonBox.setColor(touchgfx::Color::getColorFromRGB(63, 212, 122));
    startButtonBox.setAlpha(230);
    add(startButtonBox);

    startButtonText.setScale(2);
    startButtonText.setPosition(91, 253, 0, 0);
    startButtonText.setColor(touchgfx::Color::getColorFromRGB(5, 12, 18));
    startButtonText.setText("START");
    add(startButtonText);

    confirmScrim.setPosition(20, 68, 200, 140);
    confirmScrim.setColor(touchgfx::Color::getColorFromRGB(0, 0, 0));
    confirmScrim.setAlpha(92);
    add(confirmScrim);

    confirmBox.setPosition(24, 72, 192, 132);
    confirmBox.setColor(touchgfx::Color::getColorFromRGB(18, 28, 42));
    confirmBox.setAlpha(255);
    add(confirmBox);

    confirmTitleText.setScale(2);
    confirmTitleText.setPosition(66, 96, 0, 0);
    confirmTitleText.setColor(touchgfx::Color::getColorFromRGB(238, 244, 255));
    confirmTitleText.setText("END SPORT");
    add(confirmTitleText);

    confirmYesBox.setPosition(42, 152, 72, 38);
    confirmYesBox.setColor(touchgfx::Color::getColorFromRGB(63, 212, 122));
    confirmYesBox.setAlpha(255);
    add(confirmYesBox);

    confirmNoBox.setPosition(126, 152, 72, 38);
    confirmNoBox.setColor(touchgfx::Color::getColorFromRGB(255, 106, 61));
    confirmNoBox.setAlpha(255);
    add(confirmNoBox);

    confirmYesText.setScale(2);
    confirmYesText.setPosition(60, 160, 0, 0);
    confirmYesText.setColor(touchgfx::Color::getColorFromRGB(5, 12, 18));
    confirmYesText.setText("YES");
    add(confirmYesText);

    confirmNoText.setScale(2);
    confirmNoText.setPosition(150, 160, 0, 0);
    confirmNoText.setColor(touchgfx::Color::getColorFromRGB(5, 12, 18));
    confirmNoText.setText("NO");
    add(confirmNoText);

    setConfirmVisible(false);

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
    stopExercise();
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
    exerciseRunning = false;
    startDataResetPending = false;
    exerciseStartMs = 0;
    lastMetricUpdateMs = 0;
    lastWalkSampleMs = 0;
    hideConfirm();
    setStartButtonLabel();
    navBackgroundImage.setVisible(false);
    detailBackgroundImage.setVisible(true);
    dimOverlay.setColor(touchgfx::Color::getColorFromRGB(4, 16, 32));
    dimOverlay.setAlpha(70);
    setNavVisible(false);
    setDetailVisible(true);
    applySportMode();
    applyWatchSnapshot();
    invalidate();
}

void Screen2View::setNavVisible(bool visible)
{
    setVisibleWithLocalRefresh(headerBand, visible);
    setVisibleWithLocalRefresh(titleText, visible);
    setVisibleWithLocalRefresh(subtitleText, visible);

    for (int i = 0; i < 3; i++)
    {
        setVisibleWithLocalRefresh(cardBox[i], visible);
        setVisibleWithLocalRefresh(cardTitleText[i], visible);
        setVisibleWithLocalRefresh(cardHintText[i], visible);
    }
}

void Screen2View::setDetailVisible(bool visible)
{
    setVisibleWithLocalRefresh(topAccent, visible);
    setVisibleWithLocalRefresh(durationText, visible);
    setVisibleWithLocalRefresh(detailTitleText, visible);
    setVisibleWithLocalRefresh(heartText, visible);
    setVisibleWithLocalRefresh(mainValueText, visible);
    setVisibleWithLocalRefresh(mainLabelText, visible);

    for (int i = 0; i < 4; i++)
    {
        setVisibleWithLocalRefresh(statBox[i], visible);
        setVisibleWithLocalRefresh(statValueText[i], visible);
        setVisibleWithLocalRefresh(statLabelText[i], visible);
    }

    setVisibleWithLocalRefresh(startButtonBox, visible);
    setVisibleWithLocalRefresh(startButtonText, visible);

    if (!visible)
    {
        setConfirmVisible(false);
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
        const char* value[4];
        const char* label[4];
        uint8_t accentR;
        uint8_t accentG;
        uint8_t accentB;
    };

    const SportDetail details[3] = {
        {
            "WALK",
            "HR 82",
            "0.00",
            "KM",
            { "0.0", "0.0", "0", "0" },
            { "NOW M/S", "AVG M/S", "STR CM", "STEPS" },
            63, 212, 122
        },
        {
            "RUN",
            "HR 146",
            "4.32",
            "KM",
            { "97%", "3.6", "2.9", "84" },
            { "SPO2", "NOW", "AVG", "CAD" },
            255, 106, 61
        },
        {
            "ROPE",
            "HR 132",
            "860",
            "COUNT",
            { "98%", "72", "68", "95" },
            { "SPO2", "NOW", "AVG", "KCAL" },
            108, 140, 255
        }
    };

    const int index = static_cast<int>(currentMode);
    const SportDetail& detail = details[index];

    topAccent.setColor(touchgfx::Color::getColorFromRGB(detail.accentR, detail.accentG, detail.accentB));
    detailTitleText.setText(detail.title);
    heartText.setText(detail.heart);
    mainLabelText.setText(detail.mainLabel);

    for (int i = 0; i < 4; i++)
    {
        statLabelText[i].setText(detail.label[i]);
    }

    applyStoredSportSnapshot(detail.mainValue, detail.value);

    topAccent.invalidate();
}

void Screen2View::applyWatchSnapshot()
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

void Screen2View::advanceSportMode()
{
    resetExercise();

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
    applyWatchSnapshot();
}

bool Screen2View::isInCard(int index, int x, int y) const
{
    const int cardY[3] = { 76, 138, 200 };
    return index >= 0 && index < 3 && x >= 18 && x <= 222 && y >= cardY[index] && y <= cardY[index] + 48;
}

bool Screen2View::isInStartButton(int x, int y) const
{
    return x >= 52 && x <= 188 && y >= 246 && y <= 276;
}

bool Screen2View::isInConfirmYes(int x, int y) const
{
    return x >= 42 && x <= 114 && y >= 152 && y <= 190;
}

bool Screen2View::isInConfirmNo(int x, int y) const
{
    return x >= 126 && x <= 198 && y >= 152 && y <= 190;
}

void Screen2View::resetExercise()
{
    exerciseRunning = false;
    startDataResetPending = false;
    exerciseStartMs = 0;
    lastMetricUpdateMs = 0;
    displayedExerciseSeconds = 0;
    updateExerciseDuration(true);
    setExerciseMetricsZero();
    hideConfirm();
    setStartButtonLabel();
}

void Screen2View::startExercise()
{
    hideConfirm();
    navBackgroundImage.setVisible(false);
    detailBackgroundImage.setVisible(true);
    dimOverlay.setColor(touchgfx::Color::getColorFromRGB(4, 16, 32));
    dimOverlay.setAlpha(70);
    exerciseRunning = true;
    startDataResetPending = true;
    exerciseStartMs = HAL_GetTick();
    lastMetricUpdateMs = exerciseStartMs;
    lastWalkSampleMs = exerciseStartMs;
    if (currentMode == SportMode::Walk)
    {
        WalkMetrics_Start(&walkMetricsState, exerciseStartMs);
        std::memset(&walkMetricsOutput, 0, sizeof(walkMetricsOutput));
    }
    setStartButtonLabel();
    invalidate();
}

void Screen2View::stopExercise()
{
    const bool wasRunning = exerciseRunning;

    if (exerciseRunning)
    {
        updateExerciseDuration(true);
    }

    exerciseRunning = false;
    startDataResetPending = false;
    if (wasRunning)
    {
        if (currentMode == SportMode::Walk)
        {
            WalkMetrics_Stop(&walkMetricsState);
        }
        updateExerciseMetrics(true);
        storeCurrentSportSnapshot();
    }
    setStartButtonLabel();
}

void Screen2View::toggleExercise()
{
    if (exerciseRunning)
    {
        stopExercise();
    }
    else
    {
        startExercise();
    }
}

void Screen2View::setStartButtonLabel()
{
    if (exerciseRunning)
    {
        startButtonBox.setColor(touchgfx::Color::getColorFromRGB(255, 106, 61));
        startButtonText.setPosition(103, 253, 0, 0);
        startButtonText.setText("END");
    }
    else
    {
        startButtonBox.setColor(touchgfx::Color::getColorFromRGB(63, 212, 122));
        startButtonText.setPosition(91, 253, 0, 0);
        startButtonText.setText("START");
    }

    startButtonBox.invalidate();
    startButtonText.invalidate();
}

void Screen2View::updateExerciseDuration(bool force)
{
    const uint32_t elapsedSeconds = exerciseRunning ? ((HAL_GetTick() - exerciseStartMs) / 1000U) : displayedExerciseSeconds;

    if (!force && elapsedSeconds == displayedExerciseSeconds)
    {
        return;
    }

    displayedExerciseSeconds = elapsedSeconds;
    const unsigned int hours = (elapsedSeconds / 3600U) > 99U ? 99U : static_cast<unsigned int>(elapsedSeconds / 3600U);
    const unsigned int minutes = static_cast<unsigned int>((elapsedSeconds / 60U) % 60U);
    const unsigned int seconds = static_cast<unsigned int>(elapsedSeconds % 60U);

    std::snprintf(durationBuffer,
                  sizeof(durationBuffer),
                  "%02u:%02u:%02u",
                  hours,
                  minutes,
                  seconds);
    durationText.setText(durationBuffer);
    durationText.invalidate();
}

void Screen2View::updateExerciseMetrics(bool force)
{
    (void)force;

    if (currentMode == SportMode::Walk)
    {
        if (IMU_Sensor_IsReady())
        {
            applyWalkMetricsOutput();
        }
        else if (exerciseRunning || walkMetricsOutput.step_count > 0U)
        {
            applyWalkMetricsOutput();
        }
        else
        {
            setExerciseMetricsZero();
        }
        return;
    }

    const uint32_t elapsedMs = exerciseRunning ? (HAL_GetTick() - exerciseStartMs) : (displayedExerciseSeconds * 1000U);
    const uint32_t halfSeconds = elapsedMs / 500U;
    const uint32_t seconds = elapsedMs / 1000U;

    if (currentMode == SportMode::Run)
    {
        const uint32_t distanceCm = (elapsedMs * 340UL) / 1000UL;
        const uint32_t nowSpeedTenths = (elapsedMs == 0U) ? 0UL : 34UL + (halfSeconds % 5UL);
        const uint32_t avgSpeedTenths = (elapsedMs == 0U) ? 0UL : 29UL + ((seconds / 6UL) % 3UL);
        std::snprintf(mainValueBuffer, sizeof(mainValueBuffer), "%lu.%02lu", distanceCm / 100000UL, (distanceCm % 100000UL) / 1000UL);
        std::snprintf(statValueBuffer[0], sizeof(statValueBuffer[0]), "%lu%%", elapsedMs == 0U ? 0UL : 97UL);
        std::snprintf(statValueBuffer[1], sizeof(statValueBuffer[1]), "%lu.%lu", nowSpeedTenths / 10UL, nowSpeedTenths % 10UL);
        std::snprintf(statValueBuffer[2], sizeof(statValueBuffer[2]), "%lu.%lu", avgSpeedTenths / 10UL, avgSpeedTenths % 10UL);
        std::snprintf(statValueBuffer[3], sizeof(statValueBuffer[3]), "%lu", elapsedMs == 0U ? 0UL : 82UL + ((halfSeconds / 2UL) % 18UL));
    }
    else
    {
        const uint32_t count = halfSeconds;
        std::snprintf(mainValueBuffer, sizeof(mainValueBuffer), "%lu", count);
        std::snprintf(statValueBuffer[0], sizeof(statValueBuffer[0]), "%lu%%", elapsedMs == 0U ? 0UL : 98UL);
        std::snprintf(statValueBuffer[1], sizeof(statValueBuffer[1]), "%lu", elapsedMs == 0U ? 0UL : 70UL + (halfSeconds % 8UL));
        std::snprintf(statValueBuffer[2], sizeof(statValueBuffer[2]), "%lu", elapsedMs == 0U ? 0UL : 68UL);
        std::snprintf(statValueBuffer[3], sizeof(statValueBuffer[3]), "%lu", elapsedMs == 0U ? 0UL : seconds / 6UL);
    }

    mainValueText.setText(mainValueBuffer);

    for (int i = 0; i < 4; i++)
    {
        statValueText[i].setText(statValueBuffer[i]);
        statValueText[i].invalidate();
    }

    mainValueText.invalidate();
}

bool Screen2View::updateWalkMetrics(uint32_t now)
{
    IMU_SensorSample imuSample;
    WalkMetricsImuSample walkSample;

    if ((now - lastWalkSampleMs) < 20U)
    {
        return false;
    }

    lastWalkSampleMs = now;

    if (!IMU_Sensor_Read(&imuSample))
    {
        return false;
    }

    walkSample.tick_ms = now;
    for (int i = 0; i < 3; i++)
    {
        walkSample.accel_g[i] = imuSample.accel_g[i];
        walkSample.gyro_rad_s[i] = imuSample.gyro_rad_s[i];
    }

    WalkMetrics_Update(&walkMetricsState, &walkSample, &walkMetricsOutput);
    if (walkMetricsOutput.output_updated == 0U)
    {
        return false;
    }

    lastMetricUpdateMs = now;
    applyWalkMetricsOutput();
    return true;
}

void Screen2View::applyWalkMetricsOutput()
{
    const uint32_t distanceCm = static_cast<uint32_t>(walkMetricsOutput.distance_m * 100.0f + 0.5f);
    const uint32_t instantSpeedTenths = static_cast<uint32_t>(walkMetricsOutput.instant_speed_mps * 10.0f + 0.5f);
    const uint32_t averageSpeedTenths = static_cast<uint32_t>(walkMetricsOutput.average_speed_mps * 10.0f + 0.5f);
    const uint32_t strideCm = (walkMetricsOutput.step_count == 0U) ? 0U : static_cast<uint32_t>(walkMetricsOutput.stride_m * 100.0f + 0.5f);

    std::snprintf(mainValueBuffer, sizeof(mainValueBuffer), "%lu.%02lu", distanceCm / 100000UL, (distanceCm % 100000UL) / 1000UL);
    std::snprintf(statValueBuffer[0], sizeof(statValueBuffer[0]), "%lu.%lu", instantSpeedTenths / 10UL, instantSpeedTenths % 10UL);
    std::snprintf(statValueBuffer[1], sizeof(statValueBuffer[1]), "%lu.%lu", averageSpeedTenths / 10UL, averageSpeedTenths % 10UL);
    std::snprintf(statValueBuffer[2], sizeof(statValueBuffer[2]), "%lu", strideCm);
    std::snprintf(statValueBuffer[3], sizeof(statValueBuffer[3]), "%lu", walkMetricsOutput.step_count);

    mainValueText.setText(mainValueBuffer);
    for (int i = 0; i < 4; i++)
    {
        statValueText[i].setText(statValueBuffer[i]);
        statValueText[i].invalidate();
    }
    mainValueText.invalidate();
}

void Screen2View::setExerciseMetricsZero()
{
    if (currentMode == SportMode::Rope)
    {
        std::snprintf(mainValueBuffer, sizeof(mainValueBuffer), "0");
    }
    else
    {
        std::snprintf(mainValueBuffer, sizeof(mainValueBuffer), "0.00");
    }
    if (currentMode == SportMode::Walk)
    {
        std::snprintf(statValueBuffer[0], sizeof(statValueBuffer[0]), "0.0");
        std::snprintf(statValueBuffer[1], sizeof(statValueBuffer[1]), "0.0");
        std::snprintf(statValueBuffer[2], sizeof(statValueBuffer[2]), "0");
        std::snprintf(statValueBuffer[3], sizeof(statValueBuffer[3]), "0");
    }
    else
    {
        std::snprintf(statValueBuffer[0], sizeof(statValueBuffer[0]), "0%%");
        for (int i = 1; i < 4; i++)
        {
            std::snprintf(statValueBuffer[i], sizeof(statValueBuffer[i]), "0");
        }
    }

    mainValueText.setText(mainValueBuffer);

    for (int i = 0; i < 4; i++)
    {
        statValueText[i].setText(statValueBuffer[i]);
        statValueText[i].invalidate();
    }

    mainValueText.invalidate();
}

void Screen2View::storeCurrentSportSnapshot()
{
    const int mode = static_cast<int>(currentMode);

    if (mode < 0 || mode >= 3)
    {
        return;
    }

    std::snprintf(storedDurationBuffer[mode], sizeof(storedDurationBuffer[mode]), "%s", durationBuffer);
    std::snprintf(storedMainValueBuffer[mode], sizeof(storedMainValueBuffer[mode]), "%s", mainValueBuffer);
    for (int i = 0; i < 4; i++)
    {
        std::snprintf(storedStatValueBuffer[mode][i], sizeof(storedStatValueBuffer[mode][i]), "%s", statValueBuffer[i]);
    }
    hasStoredMetrics[mode] = true;
}

void Screen2View::applyStoredSportSnapshot(const char* fallbackMainValue, const char* const fallbackValues[4])
{
    const int mode = static_cast<int>(currentMode);
    const bool hasSnapshot = (mode >= 0 && mode < 3) ? hasStoredMetrics[mode] : false;

    if (hasSnapshot)
    {
        std::snprintf(durationBuffer, sizeof(durationBuffer), "%s", storedDurationBuffer[mode]);
        std::snprintf(mainValueBuffer, sizeof(mainValueBuffer), "%s", storedMainValueBuffer[mode]);
        for (int i = 0; i < 4; i++)
        {
            std::snprintf(statValueBuffer[i], sizeof(statValueBuffer[i]), "%s", storedStatValueBuffer[mode][i]);
        }
    }
    else
    {
        std::snprintf(durationBuffer, sizeof(durationBuffer), "00:00:00");
        std::snprintf(mainValueBuffer, sizeof(mainValueBuffer), "%s", fallbackMainValue);
        for (int i = 0; i < 4; i++)
        {
            std::snprintf(statValueBuffer[i], sizeof(statValueBuffer[i]), "%s", fallbackValues[i]);
        }
    }

    durationText.setText(durationBuffer);
    mainValueText.setText(mainValueBuffer);
    for (int i = 0; i < 4; i++)
    {
        statValueText[i].setText(statValueBuffer[i]);
    }
}

void Screen2View::recordTouchActivity()
{
    lastTouchMs = HAL_GetTick();
    if (screenLocked)
    {
        wakeScreen();
    }
}

void Screen2View::lockScreenIfIdle(uint32_t now)
{
    if (exerciseRunning || screenLocked)
    {
        return;
    }

    if ((now - lastTouchMs) >= 15000U)
    {
        hideConfirm();
        LCD_Port_SetBacklight(0U);
        screenLocked = true;
    }
}

void Screen2View::wakeScreen()
{
    LCD_Port_SetBacklight(1U);
    screenLocked = false;
    lastTouchMs = HAL_GetTick();
}

void Screen2View::showConfirm(PendingSwipeAction action)
{
    pendingSwipeAction = action;
    setConfirmVisible(true);
}

void Screen2View::hideConfirm()
{
    pendingSwipeAction = PendingSwipeAction::None;
    setConfirmVisible(false);
}

void Screen2View::confirmEndExercise()
{
    const PendingSwipeAction action = pendingSwipeAction;
    hideConfirm();
    stopExercise();

    if (action == PendingSwipeAction::ReturnNav)
    {
        showNav();
    }
    else if (action == PendingSwipeAction::NextMode)
    {
        advanceSportMode();
    }
}

void Screen2View::setConfirmVisible(bool visible)
{
    confirmVisible = visible;
    confirmScrim.setVisible(visible);
    confirmBox.setVisible(visible);
    confirmYesBox.setVisible(visible);
    confirmNoBox.setVisible(visible);
    confirmTitleText.setVisible(visible);
    confirmYesText.setVisible(visible);
    confirmNoText.setVisible(visible);

    confirmScrim.invalidate();
}

void Screen2View::handleSwipe(int dx, int dy)
{
    const int SWIPE_THRESHOLD = 40;

    if (confirmVisible)
    {
        return;
    }

    if (dy >= SWIPE_THRESHOLD || dy <= -SWIPE_THRESHOLD)
    {
        return;
    }

    if (pageState == PageState::Detail && dx > SWIPE_THRESHOLD)
    {
        if (exerciseRunning)
        {
            showConfirm(PendingSwipeAction::ReturnNav);
        }
        else
        {
            showNav();
        }
    }
    else if (pageState == PageState::Detail && dx < -SWIPE_THRESHOLD)
    {
        if (exerciseRunning)
        {
            showConfirm(PendingSwipeAction::NextMode);
        }
        else
        {
            advanceSportMode();
        }
    }
}
