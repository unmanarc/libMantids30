#include "mapitem.h"

using namespace Mantids30::Threads::Safe;

MapItem::MapItem()
{
    m_mapFinished = false;
}

MapItem::~MapItem()
{

}

void MapItem::stopReaders()
{
    m_mapFinished = true;

    // Send signal stop for this element...
    stopSignal();
}

void MapItem::stopSignal()
{
    // NOT IMPLEMENTED HERE.
}
