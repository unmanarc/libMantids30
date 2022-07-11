#include "streamableobject.h"
#include <stdarg.h>
#include <stdio.h>

#ifdef _WIN32
#include <stdlib.h>
#endif

using namespace Mantids::Memory::Streams;

StreamableObject::StreamableObject()
{
    failedWriteState = 0;
}

StreamableObject::~StreamableObject()
{
}

StreamableObject::Status StreamableObject::writeFullStream(const void *buf, const size_t &count, StreamableObject::Status &wrStatUpd)
{
    StreamableObject::Status cur;
    while (cur.bytesWritten<count)
    {
        if (!(cur += write((char *)buf+cur.bytesWritten,count-cur.bytesWritten, wrStatUpd)).succeed || cur.finish)
            return cur;
    }
    return cur;
}

StreamableObject::Status StreamableObject::writeString(const std::string &buf, StreamableObject::Status &wrStatUpd)
{
    return writeFullStream(buf.c_str(), buf.size(), wrStatUpd);
}

StreamableObject::Status StreamableObject::writeString(const std::string &buf)
{
    StreamableObject::Status cur;
    return writeString(buf,cur);
}

StreamableObject::Status StreamableObject::strPrintf(const char *format, ...)
{
    StreamableObject::Status cur;
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

StreamableObject::Status StreamableObject::strPrintf(StreamableObject::Status &wrStatUpd, const char *format,...)
{
    StreamableObject::Status cur;
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

void StreamableObject::writeEOF(bool )
{

}
/*
uint64_t Streamable::size() const
{
  return std::numeric_limits<uint64_t>::max();
}*/


uint16_t StreamableObject::getFailedWriteState() const
{
    return failedWriteState;
}

bool StreamableObject::setFailedWriteState(const uint16_t &value)
{
    failedWriteState = value;
    return value==0;
}

#ifdef _WIN32
int Streamable::vasprintf(char **strp, const char *fmt, va_list ap)
{
    char *cBuffer;
    int dNeededLength, iBytesWritten;

    // Account how much memory we are going to need:
    if ((dNeededLength =_vscprintf(fmt, ap)) == -1)
        return -1;

    // Allocate the required buffer with extra-byte:
    if ((cBuffer = (char *)malloc((size_t)dNeededLength + 2)) == nullptr)
        return -1;

    // fill the buffer with the printf function:
    if ((iBytesWritten = vsprintf_s(cBuffer, dNeededLength + 1, fmt, ap)) == -1)
    {
        // Free the buffer and return error...
        free(cBuffer);
        return -1;
    }

    // Fill the return address with our malloc addr
    *strp = cBuffer;

    // Return the count of written bytes.
    return iBytesWritten;
}
#endif
