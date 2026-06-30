#ifndef FRONTENDAPPLICATION_HPP
#define FRONTENDAPPLICATION_HPP

#include <gui_generated/common/FrontendApplicationBase.hpp>
#include <touchgfx/Callback.hpp>

class FrontendHeap;

using namespace touchgfx;

class FrontendApplication : public FrontendApplicationBase
{
public:
    FrontendApplication(Model& m, FrontendHeap& heap);
    virtual ~FrontendApplication() { }

    virtual void handleTickEvent()
    {
        model.tick();
        FrontendApplicationBase::handleTickEvent();
    }

    // 手动补充：Screen2 左滑时跳转到 Screen3
    void gotoScreen3ScreenNoTransition();

private:
    // 手动补充：Screen3 跳转的真正实现函数
    void gotoScreen3ScreenNoTransitionImpl();

    touchgfx::Callback<FrontendApplication> screen3TransitionCallback;
};

#endif // FRONTENDAPPLICATION_HPP
