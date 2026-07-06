#ifndef RUNVIEW_HPP
#define RUNVIEW_HPP

#include <gui/common/PixelText.hpp>
#include <gui/watch/WatchUiData.hpp>
#include <gui_generated/run_screen/runViewBase.hpp>
#include <touchgfx/events/ClickEvent.hpp>
#include <touchgfx/widgets/Box.hpp>
#include <touchgfx/widgets/Image.hpp>

class runView : public runViewBase
{
public:
    runView();
    virtual ~runView() {}

    virtual void setupScreen();
    virtual void tearDownScreen();
    virtual void handleClickEvent(const touchgfx::ClickEvent& evt);
    virtual void handleTickEvent();
    void updateWatchSnapshot(const WatchUi::WatchSnapshot& snapshot);

protected:
    int pressX;
    int pressY;
    bool detailBuilt;
    bool sportRunning;
    bool pressInSportButton;
    uint32_t sportStartMs;
    uint32_t displayedSportSeconds;

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

    void setupSportDetail();
    void applyStaticText();
    void applyWatchSnapshot();
    void startSportMode(uint32_t now);
    void stopSportMode();
    void updateSportDuration(uint32_t now);
    bool isInSportButton(int x, int y) const;
    void toggleSportMode();
    void setSportButtonLabel();
    void handleSwipe(int dx, int dy);
};

#endif // RUNVIEW_HPP
