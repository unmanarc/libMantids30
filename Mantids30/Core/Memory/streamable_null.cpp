#include "streamable_null.h"

using namespace Mantids30::Core::Memory::Streams;

std::optional<size_t> StreamableNull::writeImpl(const void *buf, const size_t &count)
{
    writeStatus += count;
    bytes = safe_add(bytes, count);
    return count;
}
