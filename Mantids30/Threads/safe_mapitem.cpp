#include "safe_mapitem.h"

using namespace Mantids30::Threads::Safe;

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
