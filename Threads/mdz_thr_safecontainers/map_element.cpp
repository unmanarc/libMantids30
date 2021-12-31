#include "map_element.h"

using namespace Mantids::Threads::Safe;

Map_Element::Map_Element()
{
    xMapFinished = false;
}

Map_Element::~Map_Element()
{

}

void Map_Element::stopReaders()
{
    xMapFinished = true;

    // Send signal stop for this element...
    stopSignal();
}

void Map_Element::stopSignal()
{
    // NOT IMPLEMENTED HERE.
}
