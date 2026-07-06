#include <gui/common/FrontendApplication.hpp>
#include <gui/common/FrontendHeap.hpp>
#include <gui/lock_screen/lockPresenter.hpp>
#include <gui/lock_screen/lockView.hpp>
#include <gui/rope_screen/ropePresenter.hpp>
#include <gui/rope_screen/ropeView.hpp>
#include <gui/run_screen/runPresenter.hpp>
#include <gui/run_screen/runView.hpp>
#include <gui/screen1_screen/screen1Presenter.hpp>
#include <gui/screen1_screen/screen1View.hpp>
#include <gui/walk_screen/walkPresenter.hpp>
#include <gui/walk_screen/walkView.hpp>
#include <touchgfx/transitions/NoTransition.hpp>

using namespace touchgfx;

FrontendApplication::FrontendApplication(Model& m, FrontendHeap& heap)
    : FrontendApplicationBase(m, heap),
      lockTransitionCallback(),
      screen1TransitionCallback(),
      walkTransitionCallback(),
      runTransitionCallback(),
      ropeTransitionCallback()
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

void FrontendApplication::gotowalkScreenNoTransition()
{
    walkTransitionCallback = touchgfx::Callback<FrontendApplication>(this, &FrontendApplication::gotowalkScreenNoTransitionImpl);
    pendingScreenTransitionCallback = &walkTransitionCallback;
}

void FrontendApplication::gotowalkScreenNoTransitionImpl()
{
    touchgfx::makeTransition<walkView, walkPresenter, touchgfx::NoTransition, Model>(&currentScreen, &currentPresenter, frontendHeap, &currentTransition, &model);
}

void FrontendApplication::gotorunScreenNoTransition()
{
    runTransitionCallback = touchgfx::Callback<FrontendApplication>(this, &FrontendApplication::gotorunScreenNoTransitionImpl);
    pendingScreenTransitionCallback = &runTransitionCallback;
}

void FrontendApplication::gotorunScreenNoTransitionImpl()
{
    touchgfx::makeTransition<runView, runPresenter, touchgfx::NoTransition, Model>(&currentScreen, &currentPresenter, frontendHeap, &currentTransition, &model);
}

void FrontendApplication::gotoropeScreenNoTransition()
{
    ropeTransitionCallback = touchgfx::Callback<FrontendApplication>(this, &FrontendApplication::gotoropeScreenNoTransitionImpl);
    pendingScreenTransitionCallback = &ropeTransitionCallback;
}

void FrontendApplication::gotoropeScreenNoTransitionImpl()
{
    touchgfx::makeTransition<ropeView, ropePresenter, touchgfx::NoTransition, Model>(&currentScreen, &currentPresenter, frontendHeap, &currentTransition, &model);
}
