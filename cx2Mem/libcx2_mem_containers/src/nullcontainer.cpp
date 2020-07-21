#include "nullcontainer.h"

using namespace CX2::Memory::Containers;

NullContainer::NullContainer()
{
    bytes = 0;
}

bool NullContainer::streamTo(Memory::Streams::Streamable *, Streams::Status &)
{
    return false;
}

CX2::Memory::Streams::Status NullContainer::write(const void *, const size_t &count, Streams::Status &)
{
    bytes+=count;
    return true;
}
