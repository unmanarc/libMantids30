#include "streamencoder_url.h"
#include "Mantids30/Memory/streamableobject.h"

#include <limits>
#include <inttypes.h>

#include <Mantids30/Memory/b_mem.h>
#include <Mantids30/Memory/b_chunks.h>

using namespace Mantids30::Memory::Streams;
using namespace Mantids30::Memory::Streams::Encoders;


WriteStatus URL::writeTo(Memory::Streams::StreamableObject * dst, const void *buf, const size_t &count,Streams::WriteStatus &wrStat)
{
    Streams::WriteStatus cur;
    size_t pos=0;

    size_t maxStream=std::numeric_limits<size_t>::max();
    maxStream/=3;
    maxStream-=3;

    if (count>maxStream)
    {
        cur.succeed=wrStat.succeed=setFailedWriteState();
        return cur;
    }

    ///////////////////////
    while (pos<count)
    {
        size_t bytesToTransmitInPlain;
        if ((bytesToTransmitInPlain=getPlainBytesSize( ((const unsigned char *)buf)+pos,count-pos))>0)
        {
            if (!(cur+=dst->writeFullStream( ((const unsigned char *)buf)+pos ,bytesToTransmitInPlain,wrStat)).succeed)
            {
                m_finalBytesWritten+=cur.bytesWritten;
                return cur;
            }
            pos+=bytesToTransmitInPlain;
        }
        else
        {
            char encodedByte[8];
            snprintf(encodedByte,sizeof(encodedByte), "%%%02" PRIX8, *(((const unsigned char *)buf)+pos));
            if (!(cur+=dst->writeFullStream(encodedByte,3, wrStat)).succeed)
            {
                m_finalBytesWritten+=cur.bytesWritten;
                return cur;
            }
            pos++;
        }
    }
    m_finalBytesWritten+=cur.bytesWritten;
    return cur;
}

size_t URL::getPlainBytesSize(const unsigned char *buf, size_t count)
{
    for (size_t i=0;i<count;i++)
    {
        if (shouldEncodeThisByte(buf[i])) return i;
    }
    return count;
}

inline bool URL::shouldEncodeThisByte(const unsigned char &byte) const
{
    return !(
            (byte>='A' && byte<='Z') ||
            (byte>='a' && byte<='z') ||
            (byte>='0' && byte<='9')
    );
}


std::string URL::encodeURLStr(const std::string &url)
{
    Mantids30::Memory::Containers::B_MEM uriDecoded( url.c_str(), url.size() );
    Memory::Containers::B_Chunks uriEncoded;

    // Encode URI...
    Memory::Streams::Encoders::URL uriEncoder;

    auto cur = uriEncoder.transform( &uriDecoded, &uriEncoded );
    if (cur.succeed && cur.finish)
    {
        return uriEncoded.toString();
    }

    return url;
}
