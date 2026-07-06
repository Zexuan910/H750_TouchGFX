#include <gui/rope_screen/ropePresenter.hpp>
#include <gui/rope_screen/ropeView.hpp>

ropePresenter::ropePresenter(ropeView& v)
    : view(v)
{
}

void ropePresenter::activate()
{
}

void ropePresenter::deactivate()
{
}

void ropePresenter::watchDataUpdated(const WatchUi::WatchSnapshot& snapshot)
{
    view.updateWatchSnapshot(snapshot);
}
