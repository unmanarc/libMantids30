#include "streams_bufferedreader.h"
#include <cstring>
#include <iterator>

using namespace Mantids30::Network::Sockets;
using namespace NetStreams;

BufferedReader::BufferedReader(const std::shared_ptr<Sockets::Socket_Stream> &stream, const size_t &bufferSize)
    : m_buffer(bufferSize)
    , m_stream(stream)
    , m_currentBufferSize(0)
{}

BufferedReader::ReadResult BufferedReader::refillBuffer()
{
    // Check if full, we can't add anymore
    if (m_currentBufferSize >= m_buffer.size())
    {
        return ReadResult::READ_FULL;
    }

    size_t availableSpace = m_buffer.size() - m_currentBufferSize;

    // Refill from socket...
    ssize_t readSize = m_stream->partialRead(m_buffer.data() + m_currentBufferSize, availableSpace);

    // Handle errors and EOF
    if (readSize <= 0)
    {
        // readSize == 0 means EOF/graceful close, < 0 means error
        return ReadResult::READ_DISCONNECTED;
    }

    m_currentBufferSize += readSize;
    return ReadResult::READ_OK;
}

BufferedReader::ReadResult BufferedReader::bufferedReadUntil(void *data, size_t *len, const char &delimiter)
{
    // Validate input pointers
    if (data == nullptr || len == nullptr)
    {
        return ReadResult::READ_INVALID_ARG;
    }

    for (;;)
    {
        // Search for delimiter in the current buffer
        char *needle = static_cast<char *>(memchr(m_buffer.data(), delimiter, m_currentBufferSize));
        if (needle)
        {
            size_t dlen = static_cast<size_t>(std::distance(m_buffer.data(), needle) + 1);
            return displaceAndCopy(data, len, dlen);
        }

        // Refill buffer from socket
        ReadResult result = refillBuffer();
        if (result != ReadResult::READ_OK)
        {
            return result;
        }
    }
}

BufferedReader::ReadResult BufferedReader::bufferedReadUntil(std::string &str, const char &delimiter)
{
    for (;;)
    {
        // Search for delimiter in the current buffer
        char *needle = static_cast<char *>(memchr(m_buffer.data(), delimiter, m_currentBufferSize));
        if (needle)
        {
            size_t dlen = static_cast<size_t>(std::distance(m_buffer.data(), needle) + 1);
            return displaceAndCopy(str, dlen);
        }

        // Refill buffer from socket
        ReadResult result = refillBuffer();
        if (result != ReadResult::READ_OK)
        {
            return result;
        }
    }
}

BufferedReader::ReadResult BufferedReader::readLineCR(std::string &str)
{
    return bufferedReadUntil(str, '\r');
}

BufferedReader::ReadResult BufferedReader::readLineLF(std::string &str)
{
    return bufferedReadUntil(str, '\n');
}

size_t BufferedReader::getBufferSize() const
{
    return m_buffer.size();
}

BufferedReader::ReadResult BufferedReader::displaceAndCopy(void *data, size_t *len, const size_t &dlen)
{
    // Null terminate it (overwrite the delimiter position with null)
    m_buffer[dlen - 1] = '\0';

    // Copy to output (includes the null terminator, so caller gets a null-terminated string)
    memcpy(data, m_buffer.data(), dlen);

    // Return copied byte count (including null terminator)
    *len = dlen;

    // Reduce byte count
    m_currentBufferSize -= dlen;

    // Displace remaining data to front of buffer
    if (m_currentBufferSize > 0)
    {
        memmove(m_buffer.data(), m_buffer.data() + dlen, m_currentBufferSize);
    }

    return ReadResult::READ_OK;
}

BufferedReader::ReadResult BufferedReader::displaceAndCopy(std::string &str, const size_t &dlen)
{
    // Copy to output (dlen - 1 to exclude the delimiter character)
    str.assign(m_buffer.data(), dlen - 1);

    // Reduce byte count
    m_currentBufferSize -= dlen;

    // Displace remaining data to front of buffer
    if (m_currentBufferSize > 0)
    {
        memmove(m_buffer.data(), m_buffer.data() + dlen, m_currentBufferSize);
    }

    return ReadResult::READ_OK;
}