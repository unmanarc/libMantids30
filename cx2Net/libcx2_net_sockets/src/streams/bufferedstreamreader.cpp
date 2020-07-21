#include "bufferedstreamreader.h"
#include <string.h>

using namespace CX2::Network::Streams;

BufferedStreamReader::BufferedStreamReader(StreamSocket *stream, const size_t &bufferSize)
{
    this->stream = stream;
    buffer = malloc(bufferSize);
    bufferOK = buffer!=nullptr;
    currentBufferSize=0;
}

BufferedStreamReader::~BufferedStreamReader()
{
    if (buffer) free(buffer);
}

eStreamBufferReadErrors BufferedStreamReader::bufferedReadUntil(void *data, size_t *len, int delimiter)
{
    while (true)
    {
        // Check in the buffer.
        void *needle = memchr(buffer, delimiter, currentBufferSize);
        if (needle)
            return displaceAndCopy(data,len,(size_t)((char *)needle-(char *)buffer)+1);

        // Check if full, we can't add anymore
        if (maxBufferSize == currentBufferSize) return E_STREAMBUFFER_READ_FULL;

        // Refill from socket...
        int readSize = stream->partialRead((char *)buffer+currentBufferSize,maxBufferSize-currentBufferSize);

        // If disconneted, report, if not, add to the buffer...
        if (readSize < 0 )
            return E_STREAMBUFFER_READ_DISCONNECTED;
        else
            currentBufferSize+=readSize;
    }
}

eStreamBufferReadErrors BufferedStreamReader::bufferedReadUntil(std::string *str, int delimiter)
{
    while (true)
    {
        // Check in the buffer.
        void *needle = memchr(buffer, delimiter, currentBufferSize);
        if (needle)
            return displaceAndCopy(str,(size_t)((char *)needle-(char *)buffer)+1);

        // Check if full, we can't add anymore
        if (maxBufferSize == currentBufferSize) return E_STREAMBUFFER_READ_FULL;

        // Refill from socket...
        int readSize = stream->partialRead((char *)buffer+currentBufferSize,maxBufferSize-currentBufferSize);

        // If disconneted, report, if not, add to the buffer...
        if (readSize < 0 )
            return E_STREAMBUFFER_READ_DISCONNECTED;
        else
            currentBufferSize+=readSize;
    }
}

eStreamBufferReadErrors BufferedStreamReader::readLineCR(std::string *str, int delimiter)
{
    return bufferedReadUntil(str,delimiter);
}

eStreamBufferReadErrors BufferedStreamReader::readLineLF(std::string *str, int delimiter)
{
    return bufferedReadUntil(str,delimiter);
}

bool BufferedStreamReader::getBufferOK() const
{
    return bufferOK;
}

size_t BufferedStreamReader::getBufferSize() const
{
    return maxBufferSize;
}

eStreamBufferReadErrors BufferedStreamReader::displaceAndCopy(void *data, size_t *len, size_t dlen)
{
    // check output buffer availability...
    if (dlen>*len) return E_STREAMBUFFER_READ_MAXSIZEEXCEED;
    // Null terminate it.
    ((char *)buffer)[dlen-1]=0;
    // Copy to output
    memcpy(data,buffer,dlen);
    // Copy bytes
    *len = dlen;
    // Reduce byte count
    currentBufferSize-=dlen;
    // Displace data.
    if (currentBufferSize)
        memmove(buffer,(char *)buffer+dlen,currentBufferSize);
    // Get out.
    return E_STREAMBUFFER_READ_OK;
}

eStreamBufferReadErrors BufferedStreamReader::displaceAndCopy(std::string *str, size_t dlen)
{
    // Null terminate it.
    ((char *)buffer)[dlen-1]=0;
    // Copy to output
    *str = std::string((char *)buffer,dlen);
    // Reduce byte count
    currentBufferSize-=dlen;
    // Displace data.
    if (currentBufferSize)
        memmove(buffer,(char *)buffer+dlen,currentBufferSize);
    // Get out.
    return E_STREAMBUFFER_READ_OK;
}
