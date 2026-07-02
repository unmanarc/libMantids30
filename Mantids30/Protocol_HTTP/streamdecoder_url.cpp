#include "streamdecoder_url.h"

#include <cctype>

#include <Mantids30/Helpers/encoders.h>
#include <Mantids30/Memory/b_chunks.h>
#include <Mantids30/Memory/b_mem.h>
#include <Mantids30/Memory/streamable_object.h>

using namespace Mantids30;
using namespace Mantids30::Memory::Streams;
using namespace Mantids30::Memory::Streams::Decoders;

size_t URL::writeTo(Memory::Streams::StreamableObject *dst, const void *buf, const size_t &count)
{
    const unsigned char *data = static_cast<const unsigned char *>(buf);

    // Handle empty buffer
    if (count == 0)
    {
        if (m_filled != 0)
        {
            dst->writeStatus += -1;
        }
        return 0;
    }

    switch (m_filled)
    {
    case 0:
    {
        unsigned char byteDetected = 0;
        size_t plainBytes;

        plainBytes = getPlainBytesSize(data, count, &byteDetected);

        if (plainBytes > 0)
        {
            if (!dst->writeFullStream(data, plainBytes))
            {
                dst->writeStatus += -1;
            }
            return plainBytes;
        }
        else
        {
            if (byteDetected == '%')
            {
                m_bytes[0] = '%';
                m_filled = 1;
                return 1;
            }
            else if (byteDetected == '+')
            {
                m_bytes[0] = ' ';
                if (!dst->writeFullStream(m_bytes, 1))
                {
                    dst->writeStatus += -1;
                }
                return 1;
            }
        }
        break;
    }
    case 1:
    {
        m_bytes[1] = data[0];
        m_filled = 2;

        if (!isxdigit(m_bytes[1]))
        {
            flushBytesAndResetFilledCounter(dst);
        }
        return 1;
    }
    case 2:
    {
        m_bytes[2] = data[0];

        if (!isxdigit(m_bytes[2]))
        {
            m_filled = 3;
            flushBytesAndResetFilledCounter(dst);
        }
        else
        {
            m_filled = 0;
            unsigned char val;
            val = Helpers::Encoders::hexPairToByte(reinterpret_cast<char *>(m_bytes) + 1);

            if (!dst->writeFullStream(&val, 1))
            {
                dst->writeStatus += -1;
            }
        }
        return 1;
    }
    default:
        break;
    }

    return 0;
}

size_t URL::writeTransformerEOF(Memory::Streams::StreamableObject *dst)
{
    // flush intermediary bytes...
    return flushBytesAndResetFilledCounter(dst);
}

size_t URL::getPlainBytesSize(const unsigned char *buf, size_t count, unsigned char *byteDetected)
{
    for (size_t i = 0; i < count; i++)
    {
        if (buf[i] == '%')
        {
            *byteDetected = '%';
            return i;
        }
        else if (buf[i] == '+')
        {
            *byteDetected = '+';
            return i;
        }
    }
    return count;
}

bool URL::flushBytesAndResetFilledCounter(Memory::Streams::StreamableObject *dst)
{
    bool r = dst->writeFullStream(m_bytes, m_filled);
    m_filled = 0;
    return r;
}

std::string URL::decodeURLStr(const std::string &url)
{
    Mantids30::Memory::Containers::B_MEM uriEncoded(url.c_str(), url.size());
    Memory::Containers::B_Chunks uriDecoded;

    // Decode URI (maybe it's url encoded)...
    Memory::Streams::Decoders::URL uriDecoder;
    uriDecoder.transform(&uriEncoded, &uriDecoded);

    if (uriDecoded.writeStatus.succeed)
    {
        return uriDecoded.toStringEx();
    }
    return url;
}
