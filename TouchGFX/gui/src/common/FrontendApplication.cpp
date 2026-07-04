#include <gui/common/FrontendApplication.hpp>
#include <gui/common/FrontendHeap.hpp>
#include <gui/lock_screen/lockView.hpp>
#include <gui/lock_screen/lockPresenter.hpp>
#include <gui/screen1_screen/screen1View.hpp>
#include <gui/screen1_screen/screen1Presenter.hpp>
#include <gui/screen2_screen/Screen2View.hpp>
#include <gui/screen2_screen/Screen2Presenter.hpp>
#include <gui/screen3_screen/Screen3View.hpp>
#include <gui/screen3_screen/Screen3Presenter.hpp>
#include <touchgfx/transitions/NoTransition.hpp>

using namespace touchgfx;

FrontendApplication::FrontendApplication(Model& m, FrontendHeap& heap)
    : FrontendApplicationBase(m, heap),
      lockTransitionCallback(),
      screen1TransitionCallback(),
      screen2TransitionCallback(),
      screen3TransitionCallback()
{

}

void FrontendApplication::gotolockScreenNoTransition()
{
    lockTransitionCallback = touchgfx::Callback<FrontendApplication>(this, &FrontendApplication::gotolockScreenNoTransitionImpl);
    pendingScreenTransitionCallback = &lockTransitionCallback;
}

void FrontendApplication::gotolockScreenNoTransitionImpl()
{
    touchgfx::makeTransition<lockView, lockPresenter, touchgfx::NoTransition, Model>(&currentScreen, &currentPresenter, frontendHeap, &currentTransition, &model);
}

void FrontendApplication::gotoscreen1ScreenNoTransition()
{
    screen1TransitionCallback = touchgfx::Callback<FrontendApplication>(this, &FrontendApplication::gotoscreen1ScreenNoTransitionImpl);
    pendingScreenTransitionCallback = &screen1TransitionCallback;
}

void FrontendApplication::gotoscreen1ScreenNoTransitionImpl()
{
    touchgfx::makeTransition<screen1View, screen1Presenter, touchgfx::NoTransition, Model>(&currentScreen, &currentPresenter, frontendHeap, &currentTransition, &model);
}

void FrontendApplication::gotoScreen2ScreenNoTransition()
{
    screen2TransitionCallback = touchgfx::Callback<FrontendApplication>(this, &FrontendApplication::gotoScreen2ScreenNoTransitionImpl);
    pendingScreenTransitionCallback = &screen2TransitionCallback;
}

void FrontendApplication::gotoScreen2ScreenNoTransitionImpl()
{
    touchgfx::makeTransition<Screen2View, Screen2Presenter, touchgfx::NoTransition, Model>(&currentScreen, &currentPresenter, frontendHeap, &currentTransition, &model);
}

void FrontendApplication::gotoScreen3ScreenNoTransition()
{
    screen3TransitionCallback = touchgfx::Callback<FrontendApplication>(this, &FrontendApplication::gotoScreen3ScreenNoTransitionImpl);
    pendingScreenTransitionCallback = &screen3TransitionCallback;
}

void FrontendApplication::gotoScreen3ScreenNoTransitionImpl()
{
    touchgfx::makeTransition<Screen3View, Screen3Presenter, touchgfx::NoTransition, Model>(&currentScreen, &currentPresenter, frontendHeap, &currentTransition, &model);
}
