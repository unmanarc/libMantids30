#pragma once

#include "socket.h"
#include <cstring>
#include <memory>
#include <utility>
#include <stdexcept>

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
        Block(const Block &other) : socketAddress(other.socketAddress)
        {
            copy(other.data, other.dataLength);
        }
        Block &operator=(const Block &other)
        {
            if (this != &other)
            {
                copy(other.data, other.dataLength);
                socketAddress = other.socketAddress;
            }
            return *this;
        }
        Block(Block &&other) noexcept : socketAddress(other.socketAddress), data(std::exchange(other.data, nullptr)), dataLength(std::exchange(other.dataLength, -1)) {}
        Block &operator=(Block &&other) noexcept
        {
            if (this != &other)
            {
                free();
                socketAddress = other.socketAddress;
                data = std::exchange(other.data, nullptr);
                dataLength = std::exchange(other.dataLength, -1);
            }
            return *this;
        }
        void free()
        {
            delete[] data;
            data = nullptr;
            dataLength = -1;
        }
        void copy(const void *_data, int dlen)
        {
            if (dlen > 0 && (!_data || dlen >= 1024 * 1024))
            {
                throw std::invalid_argument("Invalid datagram block");
            }
            std::unique_ptr<unsigned char[]> replacement;
            if (dlen > 0)
            {
                replacement = std::make_unique<unsigned char[]>(dlen);
                memcpy(replacement.get(), _data, dlen);
            }
            free();
            data = replacement.release();
            dataLength = dlen;
        }
        struct sockaddr_storage socketAddress{};
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
