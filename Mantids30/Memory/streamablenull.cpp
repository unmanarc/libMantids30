#include "streamablenull.h"

using namespace Mantids30::Memory::Streams;

bool StreamableNull::streamTo(Memory::Streams::StreamableObject * out, WriteStatus &wrStatUpd)
{
    return true;
}

WriteStatus StreamableNull::write(const void *buf, const size_t &count, WriteStatus &wrStatUpd)
{
    WriteStatus cur;
    cur.succeed=true;
    return  cur;
}
