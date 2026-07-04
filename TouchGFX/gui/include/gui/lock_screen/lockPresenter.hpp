#ifndef LOCKPRESENTER_HPP
#define LOCKPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class lockView;

class lockPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    lockPresenter(lockView& v);
    virtual void activate();
    virtual void deactivate();
    virtual ~lockPresenter() {}

private:
    lockPresenter();

    lockView& view;
};

#endif // LOCKPRESENTER_HPP
