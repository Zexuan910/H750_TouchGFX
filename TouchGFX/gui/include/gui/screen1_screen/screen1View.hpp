#ifndef SCREEN1VIEW_HPP
#define SCREEN1VIEW_HPP

#include <gui_generated/screen1_screen/screen1ViewBase.hpp>
#include <gui/screen1_screen/screen1Presenter.hpp>
#include <touchgfx/events/ClickEvent.hpp>

class screen1View : public screen1ViewBase
{
public:
    screen1View();
    virtual ~screen1View() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
    virtual void handleClickEvent(const touchgfx::ClickEvent& evt);

protected:
    int demoState;
    int pressX;
    int pressY;

    bool isInTouchBox(int x, int y) const;
    void nextDemoState();
    void updateDisplay();
    void handleSwipe(int dx, int dy);
};

#endif // SCREEN1VIEW_HPP
