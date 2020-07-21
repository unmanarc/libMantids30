#include "streamsocketwriter.h"
#ifdef _WIN32
#include <Winsock2.h>
#else
#include <arpa/inet.h>
#endif
#include <string.h>
#include <ctgmath>

#if __BIG_ENDIAN__
# define htonll(x) (x)
# define ntohll(x) (x)
#else
# define htonll(x) (((uint64_t)htonl((x) & 0xFFFFFFFF) << 32) | htonl((x) >> 32))
# define ntohll(x) (((uint64_t)ntohl((x) & 0xFFFFFFFF) << 32) | ntohl((x) >> 32))
#endif

using namespace CX2::Network::Streams;

StreamSocketWriter::StreamSocketWriter()
{

}

StreamSocketWriter::~StreamSocketWriter()
{

}

bool StreamSocketWriter::writeU8(const unsigned char& c)
{
    unsigned char snd[1];
    snd[0] = c;
    return writeBlock(&snd, 1); // Send 1-byte
}


bool StreamSocketWriter::writeU16(const uint16_t& c)
{
    // Write 16bit unsigned integer as network short.
    uint16_t nbo;
    nbo = htons(c);
    return writeBlock(&nbo, sizeof(uint16_t));
}


bool StreamSocketWriter::writeU32(const uint32_t& c)
{
    // Write 32bit unsigned integer as network long.
    uint32_t nbo;
    nbo = htonl(c);
    return writeBlock(&nbo, sizeof(unsigned int));
}

bool StreamSocketWriter::writeU64(const uint64_t &c)
{
    // Write 32bit unsigned integer as network long.
    uint64_t nbo;
    nbo = htonll(c);
    return writeBlock(&nbo, sizeof(uint64_t));
}

bool StreamSocketWriter::writeBlock32(const void* data, const uint32_t &datalen)
{
    if (!writeU32(datalen))
        return false;
    return (writeBlock(data, datalen));
}

bool StreamSocketWriter::writeBlock16(const void* data, const uint16_t & datalen)
{
    if (!writeU16(datalen))
        return false;
    return (writeBlock(data, datalen));
}

bool StreamSocketWriter::writeBlock8(const void* data, const uint8_t &datalen)
{
    if (!writeU8(datalen))
        return false;
    return (writeBlock(data, datalen));
}

bool StreamSocketWriter::writeString32(const std::string &str, uint32_t maxSize)
{
    if (str.size()>maxSize) return false;
    return writeBlock32(str.c_str(), str.size());
}

bool StreamSocketWriter::writeString16(const std::string& str)
{
    if (str.size()>65535) return false;
    return writeBlock16(str.c_str(), str.size());
}

bool StreamSocketWriter::writeString8(const std::string& str)
{
    if (str.size()>255) return false;
    return writeBlock8(str.c_str(), str.size());
}
