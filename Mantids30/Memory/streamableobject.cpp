#include "streamableobject.h"
#include "Mantids30/Helpers/safeint.h"
#include "streamablestring.h"
#include <optional>
#include <stdarg.h>
#include <stdio.h>

#ifdef _WIN32
#include <stdlib.h>
#endif

using namespace Mantids30::Memory::Streams;

std::string StreamableObject::toString()
{
    StreamableString s;
    this->streamTo(&s);
    return s.getValue();
}

bool StreamableObject::writeEOF()
{
    // Writting 0 (ZERO) to the socket will trigger the EOF.
    if (write(nullptr, 0)>=0)
    {
        return true;
    }
    else
    {
        writeStatus.succeed+=-1;
        return false;
    }
}

bool StreamableObject::writeFullStreamWithEOF(const void *buf, const size_t &count)
{
    if (!writeFullStream(buf, count))
    {
        return false;
    }

    // Here everything is written OK.
    if (!writeEOF())
    {
        writeStatus.succeed+=-1;
        return false;
    }

    return true;
}

bool StreamableObject::writeFullStream(const void *buf, const size_t &count)
{
    size_t writtenBytes = 0;

    while (writtenBytes < count)
    {
        ssize_t cur = write(static_cast<const char *>(buf) + writtenBytes, count - writtenBytes);

        if (cur > 0)
            writtenBytes += cur;

        // Failed Somewhere...
        if (!writeStatus.succeed || cur < 0)
        {
            writeStatus.succeed+=-1;
            return false;
        }
    }

    return true;
}

bool StreamableObject::writeString(const std::string &buf)
{
    return writeFullStream(buf.c_str(), buf.size());
}

size_t StreamableObject::strPrintf(const char *format, ...)
{
    size_t writtenBytes = 0;
    char *var = nullptr;
    int retval;

    //////

    va_list argv;
    va_start(argv, format);
    retval = vasprintf(&var, format, argv);
    va_end(argv);

    if (retval != -1)
    {
        writtenBytes = writeString(var);
    }
    else
    {
        writtenBytes = 0;
        // Set error writting...
        writeStatus += -1;
    }

    if (var)
        free(var);

    //////

    return writtenBytes;
}

/*
size_t StreamableObject::size()
{
  return std::numeric_limits<size_t>::max();
}*/
/*
uint16_t StreamableObject::getFailedWriteState() const
{
    return m_failedWriteState;
}

bool StreamableObject::setFailedWriteState(const uint16_t &value)
{
    m_failedWriteState = value;
    return value==0;
}
*/
#ifdef _WIN32
int StreamableObject::vasprintf(char **strp, const char *fmt, va_list ap)
{
    char *cBuffer;
    int dNeededLength, iBytesWritten;

    // Account how much memory we are going to need:
    if ((dNeededLength = _vscprintf(fmt, ap)) == -1)
        return -1;

    // Allocate the required buffer with extra-byte:
    if ((cBuffer = (char *) malloc((size_t) dNeededLength + 2)) == nullptr)
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
