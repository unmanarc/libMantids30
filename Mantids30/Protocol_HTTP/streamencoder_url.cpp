#include "streamencoder_url.h"

#include <limits>
#include <inttypes.h>

#include <Mantids30/Memory/b_mem.h>
#include <Mantids30/Memory/b_chunks.h>

using namespace Mantids30::Memory::Streams;
using namespace Mantids30::Memory::Streams::Encoders;

URL::URL(std::shared_ptr<StreamableObject> orig)
{
    this->m_orig = orig;
    m_finalBytesWritten =0;
}

bool URL::streamTo(std::shared_ptr<Memory::Streams::StreamableObject> , Status & )
{
    return false;
}

StreamableObject::Status URL::write(const void *buf, const size_t &count, Status &wrStat)
{
    Status cur;
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
            if (!(cur+=m_orig->writeFullStream( ((const unsigned char *)buf)+pos ,bytesToTransmitInPlain,wrStat)).succeed)
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
            if (!(cur+=m_orig->writeFullStream(encodedByte,3, wrStat)).succeed)
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

uint64_t URL::getFinalBytesWritten() const
{
    return m_finalBytesWritten;
}


std::string URL::encodeURLStr(const std::string &url)
{
    Mantids30::Memory::Containers::B_MEM uriDecoded( url.c_str(), url.size() );
    std::shared_ptr<Memory::Containers::B_Chunks> uriEncoded = std::make_shared<Memory::Containers::B_Chunks>();

    // Encode URI...
    std::shared_ptr<Memory::Streams::Encoders::URL> uriEncoder = std::make_shared<Memory::Streams::Encoders::URL>(uriEncoded);
    Memory::Streams::StreamableObject::Status cur;
    Memory::Streams::StreamableObject::Status wrsStat;

    if ((cur+=uriDecoded.streamTo(uriEncoder, wrsStat)).succeed)
    {
        return uriEncoded->toString();
    }
    return url;
}
