#ifndef SCREEN2VIEW_HPP
#define SCREEN2VIEW_HPP

#include <gui_generated/screen2_screen/Screen2ViewBase.hpp>
#include <gui/screen2_screen/Screen2Presenter.hpp>
#include <touchgfx/events/ClickEvent.hpp>

class Screen2View : public Screen2ViewBase
{
public:
    Screen2View();
    virtual ~Screen2View() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
    virtual void handleClickEvent(const touchgfx::ClickEvent& evt);

protected:
    int pageIndex;
    int pressX;
    int pressY;

    void showPage();
    void nextPage();
    void prevPage();
    void handleSwipe(int dx, int dy);
};

#endif // SCREEN2VIEW_HPP
