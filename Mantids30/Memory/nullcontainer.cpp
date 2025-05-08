#include "nullcontainer.h"

using namespace Mantids30::Memory::Containers;

NullContainer::NullContainer()
{
    bytes = 0;
}

bool NullContainer::streamTo(Memory::Streams::StreamableObject * , Streams::WriteStatus &)
{
    return false;
}

Mantids30::Memory::Streams::WriteStatus NullContainer::write(const void *, const size_t &count, Streams::WriteStatus &)
{
    bytes+=count;
    return true;
}
