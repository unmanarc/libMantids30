#include "streamable.h"
#include <stdarg.h>
#include <stdio.h>

using namespace CX2::Memory::Streams;

Streamable::Streamable()
{
    failedWriteState = 0;
}

Streamable::~Streamable()
{
}

Status Streamable::writeFullStream(const void *buf, const size_t &count, Status &wrStatUpd)
{
    Status cur;
    while (cur.bytesWritten<count)
    {
        if (!(cur += write((char *)buf+cur.bytesWritten,count-cur.bytesWritten, wrStatUpd)).succeed || cur.finish)
            return cur;
    }
    return cur;
}

Status Streamable::writeString(const std::string &buf, Status &wrStatUpd)
{
    return writeFullStream(buf.c_str(), buf.size(), wrStatUpd);
}

Status Streamable::writeString(const std::string &buf)
{
    Status cur;
    return writeString(buf,cur);
}

Status Streamable::strPrintf(const char *format, ...)
{
    Status cur;
    char * var = nullptr;
    int retval;

    //////

    va_list argv;
    va_start( argv, format );
    retval = vasprintf( &var, format, argv );
    va_end( argv );

    if (retval!=-1)
        cur = writeString(var, cur);
    else
        cur.succeed=false;

    if (var) free(var);

    //////

    return cur;
}

Status Streamable::strPrintf(Status &wrStatUpd, const char *format,...)
{
    Status cur;
    char * var = nullptr;
    int retval;

    //////

    va_list argv;
    va_start( argv, format );
    retval = vasprintf( &var, format, argv );
    va_end( argv );

    if (retval!=-1)
    {
        cur = writeString(var, cur);
        wrStatUpd+=cur;
    }
    else
    {
        wrStatUpd.succeed = cur.succeed = false;
    }

    if (var) free(var);

    //////

    return cur;
}

void Streamable::writeEOF(bool )
{

}
/*
uint64_t Streamable::size() const
{
  return std::numeric_limits<uint64_t>::max();
}*/


uint16_t Streamable::getFailedWriteState() const
{
    return failedWriteState;
}

bool Streamable::setFailedWriteState(const uint16_t &value)
{
    failedWriteState = value;
    return value==0;
}
