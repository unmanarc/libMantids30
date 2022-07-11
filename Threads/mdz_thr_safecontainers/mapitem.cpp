#include "mapitem.h"

using namespace Mantids::Threads::Safe;

MapItem::MapItem()
{
    xMapFinished = false;
}

MapItem::~MapItem()
{

}

void MapItem::stopReaders()
{
    xMapFinished = true;

    // Send signal stop for this element...
    stopSignal();
}

void MapItem::stopSignal()
{
    // NOT IMPLEMENTED HERE.
}
