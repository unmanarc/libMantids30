#include "streams_bufferedreader.h"
#include <string.h>

using namespace Mantids30::Network::Sockets;
using namespace NetStreams;

BufferedReader::BufferedReader( std::shared_ptr<Sockets::Socket_Stream_Base> stream, const size_t &bufferSize)
{
    this->m_stream = stream;
    m_buffer = malloc(bufferSize);
    m_bufferOK = m_buffer!=nullptr;
    m_currentBufferSize=0;
}

BufferedReader::~BufferedReader()
{
    if (m_buffer) free(m_buffer);
}

BufferedReader::eStreamBufferReadErrors BufferedReader::bufferedReadUntil(void *data, size_t *len, int delimiter)
{
    while (true)
    {
        // Check in the buffer.
        void *needle = memchr(m_buffer, delimiter, m_currentBufferSize);
        if (needle)
            return displaceAndCopy(data,len,(size_t)((char *)needle-(char *)m_buffer)+1);

        // Check if full, we can't add anymore
        if (m_maxBufferSize == m_currentBufferSize) return E_STREAMBUFFER_READ_FULL;

        // Refill from socket...
        int readSize = m_stream->partialRead((char *)m_buffer+m_currentBufferSize,m_maxBufferSize-m_currentBufferSize);

        // If disconneted, report, if not, add to the buffer...
        if (readSize < 0 )
            return E_STREAMBUFFER_READ_DISCONNECTED;
        else
            m_currentBufferSize+=readSize;
    }
}

BufferedReader::eStreamBufferReadErrors BufferedReader::bufferedReadUntil(std::string *str, int delimiter)
{
    while (true)
    {
        // Check in the buffer.
        void *needle = memchr(m_buffer, delimiter, m_currentBufferSize);
        if (needle)
            return displaceAndCopy(str,(size_t)((char *)needle-(char *)m_buffer)+1);

        // Check if full, we can't add anymore
        if (m_maxBufferSize == m_currentBufferSize) return E_STREAMBUFFER_READ_FULL;

        // Refill from socket...
        int readSize = m_stream->partialRead((char *)m_buffer+m_currentBufferSize,m_maxBufferSize-m_currentBufferSize);

        // If disconneted, report, if not, add to the buffer...
        if (readSize < 0 )
            return E_STREAMBUFFER_READ_DISCONNECTED;
        else
            m_currentBufferSize+=readSize;
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
    return m_bufferOK;
}

size_t BufferedReader::getBufferSize() const
{
    return m_maxBufferSize;
}

BufferedReader::eStreamBufferReadErrors BufferedReader::displaceAndCopy(void *data, size_t *len, size_t dlen)
{
    // check output buffer availability...
    if (dlen>*len) return E_STREAMBUFFER_READ_MAXSIZEEXCEED;
    // Null terminate it.
    ((char *)m_buffer)[dlen-1]=0;
    // Copy to output
    memcpy(data,m_buffer,dlen);
    // Copy bytes
    *len = dlen;
    // Reduce byte count
    m_currentBufferSize-=dlen;
    // Displace data.
    if (m_currentBufferSize)
        memmove(m_buffer,(char *)m_buffer+dlen,m_currentBufferSize);
    // Get out.
    return E_STREAMBUFFER_READ_OK;
}

BufferedReader::eStreamBufferReadErrors BufferedReader::displaceAndCopy(std::string *str, size_t dlen)
{
    // Null terminate it.
    ((char *)m_buffer)[dlen-1]=0;
    // Copy to output
    *str = std::string((char *)m_buffer,dlen);
    // Reduce byte count
    m_currentBufferSize-=dlen;
    // Displace data.
    if (m_currentBufferSize)
        memmove(m_buffer,(char *)m_buffer+dlen,m_currentBufferSize);
    // Get out.
    return E_STREAMBUFFER_READ_OK;
}
