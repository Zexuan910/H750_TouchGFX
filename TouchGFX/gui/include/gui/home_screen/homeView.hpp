#ifndef HOMEVIEW_HPP
#define HOMEVIEW_HPP

#include <gui_generated/home_screen/homeViewBase.hpp>
#include <gui/home_screen/homePresenter.hpp>
#include <gui/common/LockHomeState.hpp>
#include <touchgfx/events/ClickEvent.hpp>
#include <touchgfx/events/DragEvent.hpp>
#include <touchgfx/events/GestureEvent.hpp>

class homeView : public homeViewBase
{
public:
    homeView();
    virtual ~homeView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
    virtual void handleClickEvent(const touchgfx::ClickEvent& event);
    virtual void handleDragEvent(const touchgfx::DragEvent& event);
    virtual void handleGestureEvent(const touchgfx::GestureEvent& event);
    virtual void handleTickEvent();

private:
    LockHomeState lockHomeState;
};

#endif // HOMEVIEW_HPP
