#include <gui/common/FrontendApplication.hpp>
#include <gui/common/FrontendHeap.hpp>

#include <gui/screen3_screen/Screen3View.hpp>
#include <gui/screen3_screen/Screen3Presenter.hpp>

#include <touchgfx/transitions/NoTransition.hpp>

FrontendApplication::FrontendApplication(Model& m, FrontendHeap& heap)
    : FrontendApplicationBase(m, heap)
{

}

void FrontendApplication::gotoScreen3ScreenNoTransition()
{
    screen3TransitionCallback = touchgfx::Callback<FrontendApplication>(
        this,
        &FrontendApplication::gotoScreen3ScreenNoTransitionImpl
    );

    pendingScreenTransitionCallback = &screen3TransitionCallback;
}

void FrontendApplication::gotoScreen3ScreenNoTransitionImpl()
{
    touchgfx::makeTransition<
        Screen3View,
        Screen3Presenter,
        touchgfx::NoTransition,
        Model
    >(
        &currentScreen,
        &currentPresenter,
        frontendHeap,
        &currentTransition,
        &model
    );
}
