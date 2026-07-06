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
    void gotowalkScreenNoTransition();
    void gotorunScreenNoTransition();
    void gotoropeScreenNoTransition();

    virtual void handleTickEvent()
    {
        model.tick();
        FrontendApplicationBase::handleTickEvent();
    }

private:
    touchgfx::Callback<FrontendApplication> lockTransitionCallback;
    touchgfx::Callback<FrontendApplication> screen1TransitionCallback;
    touchgfx::Callback<FrontendApplication> walkTransitionCallback;
    touchgfx::Callback<FrontendApplication> runTransitionCallback;
    touchgfx::Callback<FrontendApplication> ropeTransitionCallback;

    void gotolockScreenNoTransitionImpl();
    void gotoscreen1ScreenNoTransitionImpl();
    void gotowalkScreenNoTransitionImpl();
    void gotorunScreenNoTransitionImpl();
    void gotoropeScreenNoTransitionImpl();
};

#endif // FRONTENDAPPLICATION_HPP
