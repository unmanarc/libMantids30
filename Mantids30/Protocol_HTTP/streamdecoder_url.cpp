#include "streamdecoder_url.h"

#include <Mantids30/Memory/b_mem.h>
#include <Mantids30/Memory/b_chunks.h>

#include <Mantids30/Helpers/encoders.h>
#include <memory>

using namespace Mantids30;
using namespace Mantids30::Memory::Streams;
using namespace Mantids30::Memory::Streams::Decoders;

URL::URL(std::shared_ptr<Memory::Streams::StreamableObject>  orig)
{
    this->m_orig = orig;
    m_filled=0;
    m_finalBytesWritten =0;
}

bool URL::streamTo(std::shared_ptr<Memory::Streams::StreamableObject> , StreamableObject::Status & )
{
    return false;
}

StreamableObject::Status URL::write(const void *buf, const size_t &count, Status &wrStat)
{
    StreamableObject::Status cur;
    size_t pos=0;

    while (pos<count)
    {
        switch (m_filled)
        {
        // No URL Encoded %XX is pending...
        case 0:
        {
            unsigned char byteDetected=0;
            size_t bytesToTransmitInPlain;
            // Get number of bytes that are in plain text (not encoded)
            if ((bytesToTransmitInPlain=getPlainBytesSize(((unsigned char *)buf)+pos,count-pos,&byteDetected))>0)
            {
                // If there are bytes to transmit, transmit them.
                if (!(cur+=m_orig->writeFullStream(((unsigned char *)buf)+pos,bytesToTransmitInPlain,wrStat)).succeed)
                {
                    m_finalBytesWritten+=cur.bytesWritten;
                    return cur;
                }
                pos+=bytesToTransmitInPlain;
            }
            else
            {
                // Current byte should be % (since pos<count prevents to be the last byte)
                if (byteDetected == '%')
                {
                    // Pass to the next scenario.
                    m_bytes[0]='%';
                    pos++;
                    m_filled = 1;
                }
                else if (byteDetected == '+')
                {
                    // Change the + by space...
                    m_bytes[0]=' ';
                    pos++;
                    m_filled = 1;
                    // Flush this decoded byte and set filled=0 (continue decoding)
                    if (!(cur+=flushBytes(wrStat)).succeed)
                    {
                        m_finalBytesWritten+=cur.bytesWritten;
                        return cur;
                    }
                }
            }
        }break;
        // We are waiting for the first byte of the URL Encoding: %[X]X
        case 1:
        {
            // Put the first byte in the URL Encoding Stack:
            m_bytes[1]=*(((unsigned char *)buf)+pos);
            pos++;
            m_filled = 2;
            if (!isxdigit(m_bytes[1]))
            {
                // If malformed: flush byte 0,1 from the URL Encoding stack:
                // Write original 2 bytes... and set filled to 0.
                if (!(cur+=flushBytes(wrStat)).succeed)
                {
                    m_finalBytesWritten+=cur.bytesWritten;
                    return cur;
                }
            }
        }break;
        // We are waiting for the second byte of the URL Encoding: %X[X]
        case 2:
        {
            // Put the second byte in the URL Encoding Stack:
            m_bytes[2]=*(((unsigned char *)buf)+pos);
            // Return to normal operation...
            pos++;

            if (!isxdigit(m_bytes[2]))
            {
                // If malformed: flush the byte 0,1,2 from the URL Encoding stack:
                // Write original 3 bytes...
                m_filled = 3;
                if (!(cur+=flushBytes(wrStat)).succeed)
                {
                    m_finalBytesWritten+=cur.bytesWritten;
                    return cur;
                }
                m_filled = 0;
            }
            else
            {
                // If not malformed... perform the transform from hex to uchar (URL Decoding)
                m_filled = 0;
                unsigned char val[2];
                val[0] = Helpers::Encoders::hexPairToByte( (char *) m_bytes+1 );

                // Transmit the decoded byte back to the decoded stream.
                if (!(cur+=m_orig->writeFullStream(val,1, wrStat)).succeed)
                {
                    m_finalBytesWritten+=cur.bytesWritten;
                    return cur;
                }
            }
        }break;
        default:
            break;
        }
    }
    m_finalBytesWritten+=cur.bytesWritten;
    return cur;
}

size_t URL::getPlainBytesSize(const unsigned char *buf, size_t count, unsigned char * byteDetected)
{
    for (size_t i=0;i<count;i++)
    {
        if (buf[i]=='%')
        {
            *byteDetected = '%';
            return i;
        }
        else if (buf[i]=='+')
        {
            *byteDetected = '+';
            return i;
        }
    }
    return count;
}

StreamableObject::Status URL::flushBytes(Status & wrStat)
{
    auto x= m_orig->writeFullStream(m_bytes,m_filled, wrStat);
    m_filled = 0;
    return x;
}

uint64_t URL::getFinalBytesWritten() const
{
    return m_finalBytesWritten;
}

void URL::writeEOF(bool )
{
    // flush intermediary bytes...
    Status w;
    flushBytes(w);
}

std::string URL::decodeURLStr(const std::string &url)
{
    Mantids30::Memory::Containers::B_MEM uriEncoded( url.c_str(), url.size() );
    std::shared_ptr<Memory::Containers::B_Chunks> uriDecoded = std::make_shared<Memory::Containers::B_Chunks>();

    // Decode URI (maybe it's url encoded)...
    std::shared_ptr<Memory::Streams::Decoders::URL> uriDecoder = std::make_shared<Memory::Streams::Decoders::URL>(uriDecoded);
    Memory::Streams::StreamableObject::Status cur;
    Memory::Streams::StreamableObject::Status wrsStat;

    if ((cur+=uriEncoded.streamTo(uriDecoder, wrsStat)).succeed)
    {
        return uriDecoded->toString();
    }
    return url;
}
