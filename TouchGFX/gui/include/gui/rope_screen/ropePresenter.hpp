#ifndef ROPEPRESENTER_HPP
#define ROPEPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

class ropeView;

class ropePresenter : public touchgfx::Presenter, public ModelListener
{
public:
    ropePresenter(ropeView& v);
    virtual ~ropePresenter() {}

    virtual void activate();
    virtual void deactivate();
    virtual void watchDataUpdated(const WatchUi::WatchSnapshot& snapshot);

private:
    ropePresenter();

    ropeView& view;
};

#endif // ROPEPRESENTER_HPP
