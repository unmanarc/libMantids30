#include "responsebody.h"

#include <utility>

namespace Mantids30::Client::Detail {

HTTPv1ResponseBody::HTTPv1ResponseBody(const std::shared_ptr<StreamableObject> &destination, size_t maxSize,
                           DataCallback onData, ProgressCallback onProgress)
    : m_destination(destination), m_maxSize(maxSize), m_onData(std::move(onData)), m_onProgress(std::move(onProgress))
{
}

std::optional<size_t> HTTPv1ResponseBody::write(const void *buf, const size_t &count)
{
    if (!writeStatus.succeed || writeStatus.finished)
    {
        return std::nullopt;
    }
    if (count > m_maxSize - writeStatus.bytesWritten)
    {
        m_limitExceeded = true;
        writeStatus += -1;
        return std::nullopt;
    }
    if (!count)
    {
        if (m_destination && !m_destination->writeEOF())
        {
            writeStatus += -1;
            return std::nullopt;
        }
        writeStatus += 0;
        return 0;
    }
    if (m_onData && !m_onData(buf, count))
    {
        m_cancelled = true;
        writeStatus += -1;
        return std::nullopt;
    }
    if (m_destination && !m_destination->writeFullStream(buf, count))
    {
        writeStatus += -1;
        return std::nullopt;
    }
    writeStatus += static_cast<ssize_t>(count);
    if (m_onProgress && !m_onProgress(writeStatus.bytesWritten, m_expectedSize))
    {
        m_cancelled = true;
        writeStatus += -1;
        return std::nullopt;
    }
    return count;
}

void HTTPv1ResponseBody::setExpectedSize(std::optional<size_t> expectedSize)
{
    m_expectedSize = expectedSize;
}

} // namespace Mantids30::Client::Detail
