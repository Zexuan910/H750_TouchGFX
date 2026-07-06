#ifndef RUNPRESENTER_HPP
#define RUNPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

class runView;

class runPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    runPresenter(runView& v);
    virtual ~runPresenter() {}

    virtual void activate();
    virtual void deactivate();
    virtual void watchDataUpdated(const WatchUi::WatchSnapshot& snapshot);

private:
    runPresenter();

    runView& view;
};

#endif // RUNPRESENTER_HPP
