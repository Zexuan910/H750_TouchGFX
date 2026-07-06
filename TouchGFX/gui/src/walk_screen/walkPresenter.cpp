#include <gui/walk_screen/walkPresenter.hpp>
#include <gui/walk_screen/walkView.hpp>

walkPresenter::walkPresenter(walkView& v)
    : view(v)
{
}

void walkPresenter::activate()
{
}

void walkPresenter::deactivate()
{
}

void walkPresenter::watchDataUpdated(const WatchUi::WatchSnapshot& snapshot)
{
    view.updateWatchSnapshot(snapshot);
}
