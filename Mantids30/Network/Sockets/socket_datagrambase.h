#pragma once

#include "socket.h"
#include <cstring>
#include <memory>

#ifdef _WIN32
#include <winsock2.h>
#else
#include <netinet/in.h>
#endif

namespace Mantids30::Network::Sockets {

class Socket_DatagramBase : public Socket
{
public:
    struct Block
    {
        Block() = default;
        ~Block() { this->free(); }
        void free() const { delete[] data; }
        void copy(void *_data, int dlen)
        {
            if (dlen > 0 && dlen < 1024 * 1024) // MAX: 1Mb.
            {
                this->free();
                data = new unsigned char[dlen];
                memcpy(data, _data, dlen);
            }
        }
        struct sockaddr socketAddress{0};
        unsigned char *data = nullptr;
        int dataLength = -1;
    };

    Socket_DatagramBase() = default;
    ~Socket_DatagramBase() override = default;

    // Datagram Specific Functions.
    virtual std::shared_ptr<Block> readBlock() = 0;

    // Socket specific functions:
    bool isConnected() override = 0;
    bool listenOn(const uint16_t &port, const char *listenOnAddr = "*", const int32_t &recvbuffer = 0, const int32_t &backlog = 10) override = 0;
    bool connectFrom(const char *bindAddress, const char *remoteHost, const uint16_t &port, const uint32_t &timeout = 30) override = 0;
    virtual bool writeBlock(const void *data, const size_t &datalen) = 0;
    virtual bool readBlock(void *data, const size_t &datalen) = 0;
};

using Socket_DatagramBase_SP = std::shared_ptr<Socket_DatagramBase>;

} // namespace Mantids30::Network::Sockets
