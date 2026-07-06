#ifndef SCREEN1VIEW_HPP
#define SCREEN1VIEW_HPP

#include <gui_generated/screen1_screen/screen1ViewBase.hpp>
#include <gui/screen1_screen/screen1Presenter.hpp>
#include <gui/common/PixelText.hpp>
#include <touchgfx/events/ClickEvent.hpp>
#include <touchgfx/widgets/Box.hpp>
#include <touchgfx/widgets/Image.hpp>

class screen1View : public screen1ViewBase
{
public:
    screen1View();
    virtual ~screen1View() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
    virtual void handleClickEvent(const touchgfx::ClickEvent& evt);

protected:
    int pressX;
    int pressY;
    bool navigationBuilt;

    touchgfx::Image backgroundImage;
    touchgfx::Box dimOverlay;
    touchgfx::Box topAccent;
    touchgfx::Box cardBox[3];
    PixelText titleText;
    PixelText hintText;
    PixelText cardTitleText[3];
    PixelText cardHintText[3];

    bool isInCard(int index, int x, int y) const;
    void setupNavigation();
    void updateNavigation();
    void openCard(int index);
    void handleSwipe(int dx, int dy);
};

#endif // SCREEN1VIEW_HPP
