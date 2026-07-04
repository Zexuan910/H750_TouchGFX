#ifndef MODELLISTENER_HPP
#define MODELLISTENER_HPP

#include <gui/model/Model.hpp>
#include <gui/watch/WatchUiData.hpp>

class ModelListener
{
public:
    ModelListener() : model(0) {}
    
    virtual ~ModelListener() {}

    void bind(Model* m)
    {
        model = m;
    }

    virtual void watchDataUpdated(const WatchUi::WatchSnapshot& snapshot)
    {
        (void)snapshot;
    }
protected:
    Model* model;
};

#endif // MODELLISTENER_HPP
