#ifndef SCREEN2VIEW_HPP
#define SCREEN2VIEW_HPP

#include <gui_generated/screen2_screen/Screen2ViewBase.hpp>
#include <gui/screen2_screen/Screen2Presenter.hpp>
#include <gui/common/PixelText.hpp>
#include <gui/common/SportMode.hpp>
#include <touchgfx/events/ClickEvent.hpp>
#include <touchgfx/widgets/Box.hpp>
#include <touchgfx/widgets/Image.hpp>
#include "walk_metrics.h"
#include <stdint.h>

class Screen2View : public Screen2ViewBase
{
public:
    Screen2View();
    virtual ~Screen2View() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
    virtual void handleClickEvent(const touchgfx::ClickEvent& evt);
    virtual void handleTickEvent();

protected:
    enum class PageState
    {
        Nav,
        Detail
    };

    enum class PendingSwipeAction
    {
        None,
        ReturnNav,
        NextMode
    };

    int pressX;
    int pressY;
    bool menuBuilt;
    bool detailBuilt;
    bool exerciseRunning;
    bool confirmVisible;
    bool startDataResetPending;
    bool screenLocked;
    bool ignoreReleaseAfterWake;
    bool hasStoredMetrics[3];
    PageState pageState;
    PendingSwipeAction pendingSwipeAction;
    SportMode currentMode;
    uint32_t exerciseStartMs;
    uint32_t lastMetricUpdateMs;
    uint32_t lastTouchMs;
    uint32_t lastWalkSampleMs;
    uint32_t displayedExerciseSeconds;
    char durationBuffer[9];
    char mainValueBuffer[12];
    char statValueBuffer[4][12];
    char storedDurationBuffer[3][9];
    char storedMainValueBuffer[3][12];
    char storedStatValueBuffer[3][4][12];
    WalkMetricsState walkMetricsState;
    WalkMetricsOutput walkMetricsOutput;

    touchgfx::Image navBackgroundImage;
    touchgfx::Image detailBackgroundImage;
    touchgfx::Box dimOverlay;
    touchgfx::Box headerBand;
    touchgfx::Box cardBox[3];
    PixelText titleText;
    PixelText subtitleText;
    PixelText cardTitleText[3];
    PixelText cardHintText[3];
    touchgfx::Box topAccent;
    touchgfx::Box statBox[4];
    PixelText detailTitleText;
    PixelText heartText;
    PixelText mainValueText;
    PixelText mainLabelText;
    PixelText statValueText[4];
    PixelText statLabelText[4];
    touchgfx::Box startButtonBox;
    PixelText startButtonText;
    PixelText durationText;
    touchgfx::Box confirmScrim;
    touchgfx::Box confirmBox;
    touchgfx::Box confirmYesBox;
    touchgfx::Box confirmNoBox;
    PixelText confirmTitleText;
    PixelText confirmYesText;
    PixelText confirmNoText;

    void setupSportMenu();
    void setupSportDetail();
    void updateSportMenu();
    void openSport(SportMode mode);
    void showNav();
    void showDetail(SportMode mode);
    void setNavVisible(bool visible);
    void setDetailVisible(bool visible);
    void applySportMode();
    void advanceSportMode();
    bool isInCard(int index, int x, int y) const;
    bool isInStartButton(int x, int y) const;
    bool isInConfirmYes(int x, int y) const;
    bool isInConfirmNo(int x, int y) const;
    void resetExercise();
    void startExercise();
    void stopExercise();
    void toggleExercise();
    void setStartButtonLabel();
    void updateExerciseDuration(bool force);
    void updateExerciseMetrics(bool force);
    bool updateWalkMetrics(uint32_t now);
    void applyWalkMetricsOutput();
    void setExerciseMetricsZero();
    void storeCurrentSportSnapshot();
    void applyStoredSportSnapshot(const char* fallbackMainValue, const char* const fallbackValues[4]);
    void recordTouchActivity();
    void lockScreenIfIdle(uint32_t now);
    void wakeScreen();
    void showConfirm(PendingSwipeAction action);
    void hideConfirm();
    void confirmEndExercise();
    void setConfirmVisible(bool visible);
    void handleSwipe(int dx, int dy);
};

#endif // SCREEN2VIEW_HPP
