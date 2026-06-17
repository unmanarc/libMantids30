#include "socket_stream_writer.h"

#include <Mantids30/Memory/endian2.h>

#ifdef _WIN32
#include <winsock2.h>
#else
#include <arpa/inet.h>
#endif
#include <cstring>
#include <ctgmath>

using namespace Mantids30::Network::Sockets;

bool Socket_Stream_Writer::writeU8(const unsigned char &c)
{
    unsigned char snd[1];
    snd[0] = c;
    return writeFull(&snd, 1); // Send 1-byte
}

bool Socket_Stream_Writer::writeU16(const uint16_t &c)
{
    // Write 16bit unsigned integer as network short.
    uint16_t nbo;
    nbo = htons(c);
    return writeFull(&nbo, sizeof(uint16_t));
}

bool Socket_Stream_Writer::writeU32(const uint32_t &c)
{
    // Write 32bit unsigned integer as network long.
    uint32_t nbo;
    nbo = htonl(c);
    return writeFull(&nbo, sizeof(unsigned int));
}

bool Socket_Stream_Writer::writeU64(const uint64_t &c)
{
    // Write 32bit unsigned integer as network long.
    uint64_t nbo;
    nbo = htonll(c);
    return writeFull(&nbo, sizeof(uint64_t));
}
