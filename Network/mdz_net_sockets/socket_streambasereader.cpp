#include "socket_streambasereader.h"
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

using namespace Mantids::Network::Sockets;

Socket_StreamBaseReader::Socket_StreamBaseReader()
{

}

Socket_StreamBaseReader::~Socket_StreamBaseReader()
{

}

unsigned char Socket_StreamBaseReader::readU8(bool* readOK)
{
    unsigned char rsp[1] =
    { 0 };
    if (readOK)
        *readOK = true;
    // Receive 1 byte, if fails, readOK is setted as false.
    uint64_t r;
    if ((!readFull(&rsp, 1, &r) || r!=1) && readOK)
        *readOK = false;
    return rsp[0];
}


uint16_t Socket_StreamBaseReader::readU16(bool* readOK)
{
    uint16_t ret = 0;
    if (readOK)
        *readOK = true;
    // Receive 2 bytes (unsigned short), if fails, readOK is setted as false.
    uint64_t r;
    if ((!readFull(&ret, sizeof(uint16_t), &r) || r!=sizeof(uint16_t)) && readOK)
        *readOK = false;
    ret = ntohs(ret); // Reconvert into host based integer.
    return ret;
}


uint32_t Socket_StreamBaseReader::readU32(bool* readOK)
{
    uint32_t ret = 0;
    if (readOK)
        *readOK = true;
    // Receive 4 bytes (unsigned int), if fails, readOK is setted as false.

    uint64_t r;
    if ((!readFull(&ret, sizeof(uint32_t), &r) || r!=sizeof(uint32_t)) && readOK)
        *readOK = false;
    ret = ntohl(ret); // Reconvert into host based integer.
    return ret;
}

uint64_t Socket_StreamBaseReader::readU64(bool *readOK)
{
    uint64_t ret = 0;
    if (readOK)
        *readOK = true;
    // Receive 4 bytes (unsigned int), if fails, readOK is setted as false.

    uint64_t r;
    if ((!readFull(&ret, sizeof(uint64_t), &r) || r!=sizeof(uint64_t)) && readOK)
        *readOK = false;

    ret = ntohll(ret); // Reconvert into host based integer.
    return ret;
}

// TODO: null termination
int32_t Socket_StreamBaseReader::read64KBlockDelim(char * block, const char *delim, const uint16_t &delimBytes, const uint32_t & blockNo)
{
    bool readOK;

    for (unsigned int pos=1; pos<=65536; pos++)
    {
        block[pos-1]=readU8(&readOK);
        if (!readOK) return -2; // FAILED WITH READ ERROR.

        if ((blockNo==0 && pos>=delimBytes) || blockNo>0)
        {
            char * comparisson_point = block+(pos-1)-(delimBytes-1);
            if (memcmp(comparisson_point, delim, delimBytes)==0)
                return static_cast<int32_t>(pos);
        }
    }
    return -1; // not found.
}

char* Socket_StreamBaseReader::readBlock32WAllocAndDelim(unsigned int* datalen,
        const char *delim, uint16_t delimBytes)
{
    if (*datalen<=65535) return nullptr; // It should at least have 65k of buffer.

    char * currentBlock = new char[65536];
    unsigned int currentBlockSize = 65536;
    unsigned int blockNo = 0;

    while (true)
    {
        int bytesRecv = read64KBlockDelim(currentBlock+currentBlockSize-65536, delim, delimBytes, blockNo);
        if (bytesRecv == -2)
        {
            // maybe connection closed... returning nullptr;
            delete [] currentBlock;
            return nullptr;
        }
        else if (bytesRecv == -1)
        {
            if (currentBlockSize+65536>*datalen)
            {
                // Can't request more memory. erase current...
                delete [] currentBlock;
                return nullptr;
            }
            else
            {
                // Requesting more memory... and copying the old block into the new block.
                char * nextBlock = new char[currentBlockSize+65536];
                blockNo++;
                memcpy(nextBlock, currentBlock, currentBlockSize);
                delete [] currentBlock;
                currentBlock = nextBlock;
                currentBlockSize+=65536;
            }
        }
        else if (bytesRecv>0)
        {
            *datalen = currentBlockSize-65536+bytesRecv;
            return currentBlock;
        }
    }
}

