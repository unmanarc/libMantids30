#ifndef SOCKET_BASE_DATAGRAM_H_
#define SOCKET_BASE_DATAGRAM_H_

#include "socket.h"
#include <memory>
#include <string.h>

#ifdef _WIN32
#include <winsock2.h>
#else
#include <netinet/in.h>
#endif

namespace CX2 { namespace Network { namespace Sockets { namespace Datagram {

struct Block
{
    Block()
    {
        data = nullptr;
        datalen = -1;
    }
    ~Block()
    {
        this->free();
    }
    void free()
    {
        if (data) delete [] data;
    }
    void copy(void * _data, int dlen)
    {
        if (dlen>0 && dlen<1024*1024) // MAX: 1Mb.
        {
            this->free();
            data = new unsigned char[dlen];
            memcpy(data,_data,dlen);
        }
    }
    struct sockaddr addr;
    unsigned char * data;
    int datalen;
};


class DatagramSocket : public Socket
{
public:
    DatagramSocket();
    virtual ~DatagramSocket();

    // Datagram Specific Functions.
    virtual std::shared_ptr<Block> readBlock() = 0;

    // Socket specific functions:
    virtual bool isConnected() = 0;
    virtual bool listenOn(const uint16_t & port, const char * listenOnAddr = "::", bool useIPv4 =false, const int32_t &recvbuffer = 0, const int32_t &backlog = 10)  = 0;
    virtual bool connectTo(const char * hostname, const uint16_t & port, const uint32_t &timeout) = 0;
    virtual bool writeBlock(const void * data, const uint32_t &datalen) = 0;
    virtual bool readBlock(void * data, const uint32_t & datalen) = 0;
};

typedef std::shared_ptr<DatagramSocket> DatagramSocket_SP;

}}}}

#endif /* SOCKET_BASE_DATAGRAM_H_ */
