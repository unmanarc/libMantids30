#include "nullcontainer.h"

using namespace Mantids::Memory::Containers;

NullContainer::NullContainer()
{
    bytes = 0;
}

bool NullContainer::streamTo(Memory::Streams::Streamable *, Streams::Status &)
{
    return false;
}

Mantids::Memory::Streams::Status NullContainer::write(const void *, const size_t &count, Streams::Status &)
{
    bytes+=count;
    return true;
}
