#include "streamable_object.h"
#include "streamable_string.h"
#include <cstdarg>
#include <cstdio>
#include <algorithm>
#include <cmath>
#include <optional>
#include <utility>

#ifdef _WIN32
#include <cstdlib>
#endif

using namespace Mantids30::Core::Memory::Streams;

void StreamableObject::setWriteProgressCallback(WriteProgressCallback callback, double percentageInterval, size_t byteInterval)
{
    if (!std::isfinite(percentageInterval) || percentageInterval < 0 || percentageInterval > 100)
    {
        throw std::invalid_argument("Progress percentage interval must be between 0 and 100.");
    }
    m_writeProgressCallback = std::move(callback);
    m_percentageInterval = percentageInterval;
    m_byteInterval = byteInterval;
    m_lastProgressBytes = m_progressBytes;
    m_lastProgressPercentage = m_writeTargetSize && *m_writeTargetSize
        ? std::min(100.0, 100.0 * static_cast<double>(m_progressBytes) / static_cast<double>(*m_writeTargetSize)) : 0;
    m_completionReported = false;
}

void StreamableObject::setWriteTargetSize(std::optional<size_t> totalSize)
{
    m_writeTargetSize = totalSize;
    m_progressBytes = 0;
    m_lastProgressBytes = 0;
    m_lastProgressPercentage = 0;
    m_completionReported = false;
}

std::optional<size_t> StreamableObject::write(const void *buf, const size_t &count)
{
    const auto written = writeImpl(buf, count);
    if (!written || *written > count || !writeStatus.succeed || (count && !*written))
    {
        return written;
    }
    m_progressBytes = safe_add(m_progressBytes, *written);
    if (!m_writeProgressCallback)
    {
        return written;
    }
    std::optional<double> percentage;
    if (m_writeTargetSize)
    {
        percentage = *m_writeTargetSize
            ? std::min(100.0, 100.0 * static_cast<double>(m_progressBytes) / static_cast<double>(*m_writeTargetSize)) : 100.0;
    }
    const bool atTarget = m_writeTargetSize && m_progressBytes >= *m_writeTargetSize;
    const bool completed = atTarget && !m_completionReported;
    const bool pending = m_progressBytes != m_lastProgressBytes;
    const bool intervalReached = !m_writeTargetSize || (!m_percentageInterval && !m_byteInterval)
        || (m_byteInterval && m_progressBytes - m_lastProgressBytes >= m_byteInterval)
        || (percentage && m_percentageInterval && *percentage > m_lastProgressPercentage
            && *percentage - m_lastProgressPercentage >= m_percentageInterval - 100 * std::numeric_limits<double>::epsilon());
    if (completed || (pending && (!count || intervalReached)))
    {
        m_lastProgressBytes = m_progressBytes;
        m_lastProgressPercentage = percentage.value_or(0);
        m_completionReported = atTarget;
        // Copy so a callback can safely replace or clear itself.
        const auto callback = m_writeProgressCallback;
        callback({m_progressBytes, m_writeTargetSize, percentage});
    }
    return written;
}

std::string StreamableObject::toString()
{
    StreamableString s;
    this->streamTo(&s);
    return s.getValue();
}

bool StreamableObject::writeEOF()
{
    // Writting 0 (ZERO) to the socket will trigger the EOF.
    if (write(nullptr, 0) != std::nullopt)
    {
        return true;
    }
    else
    {
        writeStatus += -1;
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
        writeStatus += -1;
        return false;
    }

    return true;
}

bool StreamableObject::writeFullStream(const void *buf, const size_t &count)
{
    size_t writtenBytes = 0;

    while (writtenBytes < count)
    {
        std::optional<size_t> cur = write(static_cast<const char *>(buf) + writtenBytes, count - writtenBytes);
        if (cur == std::nullopt || !writeStatus.succeed || *cur == 0 || *cur > count - writtenBytes)
        {
            writeStatus += -1;
            return false;
        }
        writtenBytes += cur.value();
    }

    return true;
}

bool StreamableObject::writeString(std::string_view buf)
{
    return writeFullStream(buf.data(), buf.size());
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
        // Use the formatted byte count so embedded null characters are written as payload.
        writtenBytes = writeFullStream(var, static_cast<size_t>(retval)) ? static_cast<size_t>(retval) : 0;
    }
    else
    {
        writtenBytes = 0;
        // Set error writting...
        writeStatus += -1;
    }

    if (var)
    {
        free(var);
    }

    //////

    return writtenBytes;
}

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
