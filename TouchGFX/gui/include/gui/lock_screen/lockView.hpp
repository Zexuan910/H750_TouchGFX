#ifndef LOCKVIEW_HPP
#define LOCKVIEW_HPP

#include <gui_generated/lock_screen/lockViewBase.hpp>
#include <gui/lock_screen/lockPresenter.hpp>
#include <gui/common/LockHomeState.hpp>
#include <touchgfx/events/ClickEvent.hpp>

class lockView : public lockViewBase
{
public:
    lockView();
    virtual ~lockView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
    virtual void handleClickEvent(const touchgfx::ClickEvent& event);
    virtual void handleTickEvent();

private:
    LockHomeState lockHomeState;
};

#endif // LOCKVIEW_HPP
