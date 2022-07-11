#ifndef BUFFERED_READER_H
#define BUFFERED_READER_H

#include "socket_streambase.h"

namespace Mantids { namespace Network { namespace Sockets { namespace NetStreams {


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

    BufferedReader( Network::Sockets::Socket_StreamBase * stream, const size_t & maxBufferSize );
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

    bool bufferOK;
    void * buffer;
    Network::Sockets::Socket_StreamBase * stream;
    size_t maxBufferSize, currentBufferSize;
};

typedef std::shared_ptr<BufferedReader> Stream_Buffer_SP;

}}}}

#endif // BUFFERED_READER_H
