#ifndef WALKPRESENTER_HPP
#define WALKPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

class walkView;

class walkPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    walkPresenter(walkView& v);
    virtual ~walkPresenter() {}

    virtual void activate();
    virtual void deactivate();
    virtual void watchDataUpdated(const WatchUi::WatchSnapshot& snapshot);

private:
    walkPresenter();

    walkView& view;
};

#endif // WALKPRESENTER_HPP
