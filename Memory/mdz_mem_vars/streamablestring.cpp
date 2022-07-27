#include "streamablestring.h"
#include <string.h>
#include <vector>

using namespace Mantids::Memory::Streams;

StreamableString::StreamableString()
{
}

bool StreamableString::streamTo(Memory::Streams::StreamableObject *out, StreamableObject::Status &wrStatUpd)
{
    StreamableObject::Status cur;
    if (!(cur=out->writeFullStream(value.c_str(),value.size(),wrStatUpd)).succeed || cur.finish)
    {
        if (!cur.succeed)
        {
            out->writeEOF(false);
            return false;
        }
        else
        {
            out->writeEOF(true);
            return true;
        }
    }
    out->writeEOF(true);
    return true;
}

StreamableObject::Status StreamableString::write(const void *buf, const size_t &count, StreamableObject::Status &wrStatUpd)
{
    StreamableObject::Status cur;

    std::string x( ((char *)buf), count);
    value+=x;

    cur+=(uint64_t)count;
    wrStatUpd+=(uint64_t)count;
    return  cur;
}

const std::string &StreamableString::getValue() const
{
    return value;
}

void StreamableString::setValue(const std::string &newValue)
{
    value = newValue;
}
