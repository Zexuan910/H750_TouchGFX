#ifndef FRONTENDAPPLICATION_HPP
#define FRONTENDAPPLICATION_HPP

#include <gui_generated/common/FrontendApplicationBase.hpp>

class FrontendHeap;

using namespace touchgfx;

class FrontendApplication : public FrontendApplicationBase
{
public:
    FrontendApplication(Model& m, FrontendHeap& heap);
    virtual ~FrontendApplication() { }

    void gotolockScreenNoTransition();
    void gotoscreen1ScreenNoTransition();
    void gotoScreen2ScreenNoTransition();
    void gotoScreen3ScreenNoTransition();

    virtual void handleTickEvent()
    {
        model.tick();
        FrontendApplicationBase::handleTickEvent();
    }
private:
    touchgfx::Callback<FrontendApplication> lockTransitionCallback;
    touchgfx::Callback<FrontendApplication> screen1TransitionCallback;
    touchgfx::Callback<FrontendApplication> screen2TransitionCallback;
    touchgfx::Callback<FrontendApplication> screen3TransitionCallback;

    void gotolockScreenNoTransitionImpl();
    void gotoscreen1ScreenNoTransitionImpl();
    void gotoScreen2ScreenNoTransitionImpl();
    void gotoScreen3ScreenNoTransitionImpl();
};

#endif // FRONTENDAPPLICATION_HPP
