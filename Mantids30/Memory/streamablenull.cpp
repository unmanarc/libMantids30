#include "streamablenull.h"

using namespace Mantids30::Memory::Streams;

bool StreamableNull::streamTo(std::shared_ptr<Memory::Streams::StreamableObject> out, StreamableObject::Status &wrStatUpd)
{
    return true;
}

StreamableObject::Status StreamableNull::write(const void *buf, const size_t &count, StreamableObject::Status &wrStatUpd)
{
    StreamableObject::Status cur;
    cur.succeed=true;
    return  cur;
}
