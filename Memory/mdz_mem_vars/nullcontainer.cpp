#include "nullcontainer.h"

using namespace Mantids::Memory::Containers;

NullContainer::NullContainer()
{
    bytes = 0;
}

bool NullContainer::streamTo(Memory::Streams::StreamableObject *, Streams::StreamableObject::Status &)
{
    return false;
}

Mantids::Memory::Streams::StreamableObject::Status NullContainer::write(const void *, const size_t &count, Streams::StreamableObject::Status &)
{
    bytes+=count;
    return true;
}
