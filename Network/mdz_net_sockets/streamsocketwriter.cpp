#include "streamsocketwriter.h"
#ifdef _WIN32
#include <winsock2.h>
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

using namespace Mantids::Network::Streams;

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
    return writeFull(&snd, 1); // Send 1-byte
}


bool StreamSocketWriter::writeU16(const uint16_t& c)
{
    // Write 16bit unsigned integer as network short.
    uint16_t nbo;
    nbo = htons(c);
    return writeFull(&nbo, sizeof(uint16_t));
}


bool StreamSocketWriter::writeU32(const uint32_t& c)
{
    // Write 32bit unsigned integer as network long.
    uint32_t nbo;
    nbo = htonl(c);
    return writeFull(&nbo, sizeof(unsigned int));
}

bool StreamSocketWriter::writeU64(const uint64_t &c)
{
    // Write 32bit unsigned integer as network long.
    uint64_t nbo;
    nbo = htonll(c);
    return writeFull(&nbo, sizeof(uint64_t));
}
