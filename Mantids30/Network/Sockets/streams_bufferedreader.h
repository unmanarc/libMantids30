#pragma once

#include <vector>
#include "socket_stream.h"

namespace Mantids30::Network::Sockets::NetStreams {

class BufferedReader
{
public:
    enum class ReadResult : uint8_t
    {
        READ_OK = 0,
        READ_FULL = 1,
        READ_NOTFOUND = 2,
        READ_DISCONNECTED = 3,
        READ_INVALID_ARG = 4
    };

    BufferedReader(const std::shared_ptr<Sockets::Socket_Stream> &stream, const size_t &maxBufferSize);
    ~BufferedReader() = default;

    // Delete copy to avoid double stream access
    BufferedReader(const BufferedReader &) = delete;
    BufferedReader &operator=(const BufferedReader &) = delete;

    // Allow move
    BufferedReader(BufferedReader &&) = default;
    BufferedReader &operator=(BufferedReader &&) = default;

    ReadResult bufferedReadUntil(void *data, size_t *len, const char &delimiter);
    ReadResult bufferedReadUntil(std::string &str, const char &delimiter);

    ReadResult readLineCR(std::string &str);
    ReadResult readLineLF(std::string &str);

    [[nodiscard]] size_t getBufferSize() const;

private:
    ReadResult displaceAndCopy(void *data, size_t *len, const size_t &dlen);
    ReadResult displaceAndCopy(std::string &str, const size_t &dlen);
    ReadResult refillBuffer();

    std::vector<char> m_buffer;
    std::shared_ptr<Sockets::Socket_Stream> m_stream;
    size_t m_currentBufferSize;
};

using Stream_Buffer_SP = std::shared_ptr<BufferedReader>;

} // namespace Mantids30::Network::Sockets::NetStreams