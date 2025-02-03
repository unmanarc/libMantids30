#pragma once

#include "socket_stream_base.h"

namespace Mantids30 { namespace Network { namespace Sockets { namespace NetStreams {


class BufferedReader
{
public:
    enum eStreamBufferReadErrors
    {
        E_STREAMBUFFER_READ_OK = 0,
        E_STREAMBUFFER_READ_FULL = 1,
        E_STREAMBUFFER_READ_NOTFOUND = 2,
        E_STREAMBUFFER_READ_DISCONNECTED = 3,
        E_STREAMBUFFER_READ_MAXSIZEEXCEED = 4
    };

    BufferedReader( std::shared_ptr<Sockets::Socket_Stream_Base> stream, const size_t & maxBufferSize );
    ~BufferedReader();

    eStreamBufferReadErrors bufferedReadUntil(void *data, size_t * len, int delimiter );
    eStreamBufferReadErrors bufferedReadUntil( std::string * str, int delimiter );

    eStreamBufferReadErrors readLineCR( std::string * str, int delimiter = '\r' );
    eStreamBufferReadErrors readLineLF( std::string * str, int delimiter = '\n' );

    bool getBufferOK() const;

    size_t getBufferSize() const;

private:
    eStreamBufferReadErrors displaceAndCopy(void *data, size_t *len, size_t dlen);
    eStreamBufferReadErrors displaceAndCopy(std::string * str, size_t dlen);

    bool m_bufferOK;
    void * m_buffer;
    std::shared_ptr<Sockets::Socket_Stream_Base> m_stream;
    size_t m_maxBufferSize, m_currentBufferSize;
};

typedef std::shared_ptr<BufferedReader> Stream_Buffer_SP;

}}}}

