#include <gui/common/FrontendApplication.hpp>
#include <gui/common/FrontendHeap.hpp>
#include <gui/lock_screen/lockView.hpp>
#include <gui/lock_screen/lockPresenter.hpp>
#include <touchgfx/transitions/NoTransition.hpp>

using namespace touchgfx;

FrontendApplication::FrontendApplication(Model& m, FrontendHeap& heap)
    : FrontendApplicationBase(m, heap),
      lockTransitionCallback()
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
