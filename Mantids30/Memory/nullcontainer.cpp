#include "nullcontainer.h"

using namespace Mantids30::Memory::Containers;

NullContainer::NullContainer()
{
    bytes = 0;
}

bool NullContainer::streamTo(std::shared_ptr<Memory::Streams::StreamableObject> , Streams::StreamableObject::Status &)
{
    return false;
}

Mantids30::Memory::Streams::StreamableObject::Status NullContainer::write(const void *, const size_t &count, Streams::StreamableObject::Status &)
{
    bytes+=count;
    return true;
}
