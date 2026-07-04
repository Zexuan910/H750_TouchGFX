#ifndef HOMEPRESENTER_HPP
#define HOMEPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class homeView;

class homePresenter : public touchgfx::Presenter, public ModelListener
{
public:
    homePresenter(homeView& v);
    virtual void activate();
    virtual void deactivate();
    virtual ~homePresenter() {}

private:
    homePresenter();

    homeView& view;
};

#endif // HOMEPRESENTER_HPP
