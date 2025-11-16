#include "streamable_null.h"

using namespace Mantids30::Memory::Streams;

std::optional<size_t> StreamableNull::write(
    const void *buf, const size_t &count)
{
    writeStatus+=count;
    bytes = safeAdd(bytes,count);
    return count;
}
