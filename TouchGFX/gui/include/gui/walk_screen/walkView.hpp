#ifndef WALKVIEW_HPP
#define WALKVIEW_HPP

#include <gui/common/PixelText.hpp>
#include <gui/watch/WatchUiData.hpp>
#include <gui_generated/walk_screen/walkViewBase.hpp>
#include <touchgfx/events/ClickEvent.hpp>
#include <touchgfx/widgets/Box.hpp>
#include <touchgfx/widgets/Image.hpp>
#include "walk_metrics.h"

class walkView : public walkViewBase
{
public:
    walkView();
    virtual ~walkView() {}

    virtual void setupScreen();
    virtual void tearDownScreen();
    virtual void handleClickEvent(const touchgfx::ClickEvent& evt);
    virtual void handleTickEvent();
    void updateWatchSnapshot(const WatchUi::WatchSnapshot& snapshot);

protected:
    int pressX;
    int pressY;
    bool detailBuilt;
    bool walkMetricsRunning;
    bool pressInSportButton;
    uint32_t walkStartMs;
    uint32_t lastWalkSampleMs;
    uint32_t lastWalkDisplayMs;
    uint32_t displayedWalkSeconds;

    touchgfx::Image backgroundImage;
    touchgfx::Box dimOverlay;
    touchgfx::Box topAccent;
    touchgfx::Box statBox[5];
    PixelText titleText;
    PixelText heartText;
    PixelText mainValueText;
    PixelText mainLabelText;
    PixelText counterText;
    touchgfx::Box sportButtonBox;
    PixelText sportButtonText;
    PixelText statValueText[5];
    PixelText statLabelText[5];
    WatchUi::WatchSnapshot lastSnapshot;
    char heartBuffer[16];
    char spo2Buffer[8];
    char rawRedBuffer[12];
    char rawIrBuffer[12];
    char fifoBuffer[16];
    char regBuffer[12];
    char ledBuffer[12];
    char addrBuffer[24];
    char failBuffer[16];
    char counterBuffer[40];
    char timeBuffer[9];
    char distanceBuffer[12];
    char nowSpeedBuffer[12];
    char avgSpeedBuffer[12];
    char stepBuffer[12];
    WalkMetricsState walkMetricsState;
    WalkMetricsOutput walkMetricsOutput;

    void setupSportDetail();
    void applyStaticText();
    void applyWatchSnapshot();
    void startWalkMetrics(uint32_t now);
    void stopWalkMetrics();
    void updateWalkDuration(uint32_t now);
    void updateWalkMetrics(uint32_t now);
    void applyWalkMetricsOutput();
    bool isInSportButton(int x, int y) const;
    void toggleSportMode();
    void setSportButtonLabel();
    void handleSwipe(int dx, int dy);
};

#endif // WALKVIEW_HPP
