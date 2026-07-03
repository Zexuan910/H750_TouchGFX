#ifndef SCREEN3VIEW_HPP
#define SCREEN3VIEW_HPP

#include <gui_generated/screen3_screen/Screen3ViewBase.hpp>
#include <gui/screen3_screen/Screen3Presenter.hpp>
#include <gui/common/PixelText.hpp>
#include <gui/common/SportMode.hpp>
#include <touchgfx/events/ClickEvent.hpp>
#include <touchgfx/widgets/Box.hpp>
#include <touchgfx/widgets/Image.hpp>

class Screen3View : public Screen3ViewBase
{
public:
    Screen3View();
    virtual ~Screen3View() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
    virtual void handleClickEvent(const touchgfx::ClickEvent& evt);
    virtual void handleTickEvent();
    static void setPendingSportMode(SportMode mode);

protected:
    static SportMode pendingSportMode;

    int pressX;
    int pressY;
    SportMode currentMode;
    bool detailBuilt;

    touchgfx::Image backgroundImage;
    touchgfx::Box dimOverlay;
    touchgfx::Box topAccent;
    touchgfx::Box statBox[5];
    PixelText titleText;
    PixelText heartText;
    PixelText mainValueText;
    PixelText mainLabelText;
    PixelText statValueText[5];
    PixelText statLabelText[5];

    void setupSportDetail();
    void applySportMode();
    void advanceCarousel();
    void handleSwipe(int dx, int dy);
};

#endif // SCREEN3VIEW_HPP
