#include "streams_bufferedreader.h"
#include <string.h>

using namespace Mantids::Network::Sockets;
using namespace NetStreams;

BufferedReader::BufferedReader(Socket_StreamBase *stream, const size_t &bufferSize)
{
    this->stream = stream;
    buffer = malloc(bufferSize);
    bufferOK = buffer!=nullptr;
    currentBufferSize=0;
}

BufferedReader::~BufferedReader()
{
    if (buffer) free(buffer);
}

BufferedReader::eStreamBufferReadErrors BufferedReader::bufferedReadUntil(void *data, size_t *len, int delimiter)
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

BufferedReader::eStreamBufferReadErrors BufferedReader::bufferedReadUntil(std::string *str, int delimiter)
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

BufferedReader::eStreamBufferReadErrors BufferedReader::readLineCR(std::string *str, int delimiter)
{
    return bufferedReadUntil(str,delimiter);
}

BufferedReader::eStreamBufferReadErrors BufferedReader::readLineLF(std::string *str, int delimiter)
{
    return bufferedReadUntil(str,delimiter);
}

bool BufferedReader::getBufferOK() const
{
    return bufferOK;
}

size_t BufferedReader::getBufferSize() const
{
    return maxBufferSize;
}

BufferedReader::eStreamBufferReadErrors BufferedReader::displaceAndCopy(void *data, size_t *len, size_t dlen)
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

BufferedReader::eStreamBufferReadErrors BufferedReader::displaceAndCopy(std::string *str, size_t dlen)
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
