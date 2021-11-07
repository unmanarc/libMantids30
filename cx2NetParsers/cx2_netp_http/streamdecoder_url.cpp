#include "streamdecoder_url.h"

#include <cx2_mem_vars/b_mem.h>
#include <cx2_mem_vars/b_chunks.h>

using namespace CX2::Memory::Streams;
using namespace CX2::Memory::Streams::Decoders;

URL::URL(Memory::Streams::Streamable * orig)
{
    this->orig = orig;
    filled=0;
    finalBytesWritten =0;
}

bool URL::streamTo(Memory::Streams::Streamable *, Status & )
{
    return false;
}

Status URL::write(const void *buf, const size_t &count, Status &wrStat)
{
    Status cur;
    size_t pos=0;

    while (pos<count)
    {
        switch (filled)
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
                if (!(cur+=orig->writeFullStream(((unsigned char *)buf)+pos,bytesToTransmitInPlain,wrStat)).succeed)
                {
                    finalBytesWritten+=cur.bytesWritten;
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
                    bytes[0]='%';
                    pos++;
                    filled = 1;
                }
                else if (byteDetected == '+')
                {
                    // Change the + by space...
                    bytes[0]=' ';
                    pos++;
                    filled = 1;
                    // Flush this decoded byte and set filled=0 (continue decoding)
                    if (!(cur+=flushBytes(wrStat)).succeed)
                    {
                        finalBytesWritten+=cur.bytesWritten;
                        return cur;
                    }
                }
            }
        }break;
        // We are waiting for the first byte of the URL Encoding: %[X]X
        case 1:
        {
            // Put the first byte in the URL Encoding Stack:
            bytes[1]=*(((unsigned char *)buf)+pos);
            pos++;
            filled = 2;
            if (!isHexByte(bytes[1]))
            {
                // If malformed: flush byte 0,1 from the URL Encoding stack:
                // Write original 2 bytes... and set filled to 0.
                if (!(cur+=flushBytes(wrStat)).succeed)
                {
                    finalBytesWritten+=cur.bytesWritten;
                    return cur;
                }
            }
        }break;
        // We are waiting for the second byte of the URL Encoding: %X[X]
        case 2:
        {
            // Put the second byte in the URL Encoding Stack:
            bytes[2]=*(((unsigned char *)buf)+pos);
            // Return to normal operation...
            pos++;

            if (!isHexByte(bytes[2]))
            {
                // If malformed: flush the byte 0,1,2 from the URL Encoding stack:
                // Write original 3 bytes...
                filled = 3;
                if (!(cur+=flushBytes(wrStat)).succeed)
                {
                    finalBytesWritten+=cur.bytesWritten;
                    return cur;
                }
                filled = 0;
            }
            else
            {
                // If not malformed... perform the transform from hex to uchar (URL Decoding)
                filled = 0;
                unsigned char val[2];
                val[0] = hex2uchar();
                // Transmit the decoded byte back to the decoded stream.
                if (!(cur+=orig->writeFullStream(val,1, wrStat)).succeed)
                {
                    finalBytesWritten+=cur.bytesWritten;
                    return cur;
                }
            }
        }break;
        default:
            break;
        }
    }
    finalBytesWritten+=cur.bytesWritten;
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

Status URL::flushBytes(Status & wrStat)
{
    auto x= orig->writeFullStream(bytes,filled, wrStat);
    filled = 0;
    return x;
}

inline unsigned char URL::hex2uchar()
{
    return get16Value(bytes[1])*0x10+get16Value(bytes[2]);
}

inline bool URL::isHexByte(unsigned char byte)
{
    return  (byte>='A' && byte<='F') ||
            (byte>='a' && byte<='f') ||
            (byte>='0' && byte<='9');
}

inline unsigned char URL::get16Value(unsigned char byte)
{
    if (byte>='A' && byte<='F') return byte-'A'+10;
    else if (byte>='a' && byte<='f') return byte-'a'+10;
    else if (byte>='0' && byte<='9') return byte-'0';
    return 0;
}

uint64_t URL::getFinalBytesWritten() const
{
    return finalBytesWritten;
}

void URL::writeEOF(bool )
{
    // flush intermediary bytes...
    Status w;
    flushBytes(w);
}

std::string URL::decodeURLStr(const std::string &url)
{
    CX2::Memory::Containers::B_MEM uriEncoded( url.c_str(), url.size() );
    Memory::Containers::B_Chunks uriDecoded;

    // Decode URI (maybe it's url encoded)...
    Memory::Streams::Decoders::URL uriDecoder(&uriDecoded);
    Memory::Streams::Status cur;
    Memory::Streams::Status wrsStat;

    if ((cur+=uriEncoded.streamTo(&uriDecoder, wrsStat)).succeed)
    {
        return uriDecoded.toString();
    }
    return url;
}
