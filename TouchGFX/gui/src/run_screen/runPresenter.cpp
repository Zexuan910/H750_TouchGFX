#include <gui/run_screen/runPresenter.hpp>
#include <gui/run_screen/runView.hpp>

runPresenter::runPresenter(runView& v)
    : view(v)
{
}

void runPresenter::activate()
{
}

void runPresenter::deactivate()
{
}

void runPresenter::watchDataUpdated(const WatchUi::WatchSnapshot& snapshot)
{
    view.updateWatchSnapshot(snapshot);
}
