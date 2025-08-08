#include "streamencoder_url.h"
#include "Mantids30/Memory/streamableobject.h"

#include <limits>
#include <inttypes.h>

#include <Mantids30/Memory/b_mem.h>
#include <Mantids30/Memory/b_chunks.h>

using namespace Mantids30::Memory::Streams;
using namespace Mantids30::Memory::Streams::Encoders;

size_t URL::writeTo(Memory::Streams::StreamableObject *dst, const void *buf, const size_t &count)
{
    size_t readenBytes=0;

    size_t maxStream=std::numeric_limits<size_t>::max();
    maxStream/=3;
    maxStream-=3;

    if (count>maxStream)
    {
        // FAILED:
        dst->writeStatus+=-1;
        return -1;
    }

    ///////////////////////
    while (readenBytes<count)
    {
        size_t bytesToTransmitInPlain=getPlainBytesSize( ((const unsigned char *)buf)+readenBytes,count-readenBytes);

        if (bytesToTransmitInPlain>0)
        {
            if (dst->writeFullStream( ((const unsigned char *)buf)+readenBytes ,bytesToTransmitInPlain))
            {
                readenBytes+=bytesToTransmitInPlain;
            }
        }
        else
        {
            char encodedByte[8];
            snprintf(encodedByte,sizeof(encodedByte), "%%%02" PRIX8, *(((const unsigned char *)buf)+readenBytes));
            if ( dst->writeFullStream(encodedByte,3) )
            {
                readenBytes++;
            }
        }

        // Stop reading on bad transmition..
        if (dst->writeStatus.succeed==false)
            return readenBytes;

    }
    //m_finalBytesWritten+=cur.bytesWritten;
    return readenBytes;
}

size_t URL::getPlainBytesSize(const unsigned char *buf, size_t count)
{
    for (size_t i=0;i<count;i++)
    {
        if (shouldEncodeThisByte(buf[i])) 
        return i;
    }
    return count;
}

inline bool URL::shouldEncodeThisByte(const unsigned char &byte) const
{
    return !isalnum(byte);
}


std::string URL::encodeURLStr(const std::string &url)
{
    Mantids30::Memory::Containers::B_MEM uriDecoded( url.c_str(), url.size() );
    Memory::Containers::B_Chunks uriEncoded;

    // Encode URI...
    Memory::Streams::Encoders::URL uriEncoder;

    uriEncoder.transform( &uriDecoded, &uriEncoded );

    if (uriEncoded.writeStatus.succeed)
    {
        return uriEncoded.toStringEx();
    }

    return url;
}
