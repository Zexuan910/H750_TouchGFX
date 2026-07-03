#ifndef SCREEN2VIEW_HPP
#define SCREEN2VIEW_HPP

#include <gui_generated/screen2_screen/Screen2ViewBase.hpp>
#include <gui/screen2_screen/Screen2Presenter.hpp>
#include <gui/common/PixelText.hpp>
#include <gui/common/SportMode.hpp>
#include <touchgfx/events/ClickEvent.hpp>
#include <touchgfx/widgets/Box.hpp>
#include <touchgfx/widgets/Image.hpp>

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

    int pressX;
    int pressY;
    bool menuBuilt;
    bool detailBuilt;
    PageState pageState;
    SportMode currentMode;

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
    touchgfx::Box statBox[5];
    PixelText detailTitleText;
    PixelText heartText;
    PixelText mainValueText;
    PixelText mainLabelText;
    PixelText statValueText[5];
    PixelText statLabelText[5];

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
    void handleSwipe(int dx, int dy);
};

#endif // SCREEN2VIEW_HPP
