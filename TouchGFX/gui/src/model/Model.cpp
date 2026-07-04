#include <gui/model/Model.hpp>
#include <gui/model/ModelListener.hpp>
#include <gui/watch/WatchUiData.hpp>

Model::Model() : modelListener(0), tickCounter(125)
{

}

void Model::tick()
{
    if (modelListener != 0 && (tickCounter % 15U) == 0U)
    {
        modelListener->watchDataUpdated(WatchUi::sampleSnapshot(tickCounter));
    }
    ++tickCounter;
}
