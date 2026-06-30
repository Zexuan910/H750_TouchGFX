#ifndef SCREEN3VIEW_HPP
#define SCREEN3VIEW_HPP

#include <gui_generated/screen3_screen/Screen3ViewBase.hpp>
#include <gui/screen3_screen/Screen3Presenter.hpp>
#include <touchgfx/events/ClickEvent.hpp>

class Screen3View : public Screen3ViewBase
{
public:
    Screen3View();
    virtual ~Screen3View() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
    virtual void handleClickEvent(const touchgfx::ClickEvent& evt);

protected:
    int pressX;
    int pressY;

    void handleSwipe(int dx, int dy);
};

#endif // SCREEN3VIEW_HPP
