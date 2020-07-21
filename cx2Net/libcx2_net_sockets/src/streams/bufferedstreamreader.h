#ifndef STREAM_BUFFER_H
#define STREAM_BUFFER_H

#include "streamsocket.h"

namespace CX2 { namespace Network { namespace Streams {

enum eStreamBufferReadErrors
{
    E_STREAMBUFFER_READ_OK = 0,
    E_STREAMBUFFER_READ_FULL = 1,
    E_STREAMBUFFER_READ_NOTFOUND = 2,
    E_STREAMBUFFER_READ_DISCONNECTED = 3,
    E_STREAMBUFFER_READ_MAXSIZEEXCEED = 4
};

class BufferedStreamReader
{
public:
    BufferedStreamReader( StreamSocket * stream, const size_t & maxBufferSize );
    ~BufferedStreamReader();

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
    StreamSocket * stream;
    size_t maxBufferSize, currentBufferSize;
};

typedef std::shared_ptr<BufferedStreamReader> Stream_Buffer_SP;

}}}

#endif // STREAM_BUFFER_H
