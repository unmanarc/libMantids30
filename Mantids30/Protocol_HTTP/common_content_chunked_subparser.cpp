#include "common_content_chunked_subparser.h"
#include <stdio.h>
#include <string.h>

using namespace Mantids30::Network::Protocols;
using namespace Mantids30;

HTTP::ContentChunkedTransformer::ContentChunkedTransformer(
    StreamableObject *upStreamOut)
{
    this->upStreamOut = upStreamOut;
}

HTTP::ContentChunkedTransformer::~ContentChunkedTransformer()
{
    writeEOF();
}

size_t HTTP::ContentChunkedTransformer::write(const void *buf, const size_t &count)
{
    if (count == 0)
    {
        // Set finished status (EOF)
        if (!upStreamOut->writeString(m_pos == 0? "0\r\n\r\n" : "\r\n0\r\n\r\n"))
            writeStatus+=-1;
        return 0;
    }

    if (count+64<count)
    {
        // Error writting on this (source error...).
        writeStatus+=-1;
        upStreamOut->writeStatus+=-1;
        return 0;
    }

    upStreamOut->strPrintf(m_pos == 0?"%X\r\n":"\r\n%X\r\n", (unsigned int)count);
    if (!upStreamOut->writeStatus.succeed)
    {
        writeStatus+=-1;
        return 0;
    }

    if (!upStreamOut->writeFullStream(buf,count))
    {
        writeStatus+=-1;
    }
    else
    {
        m_pos+=count;
    }

    return count;
}
