#include "socket_udp.h"
#include <sys/types.h>
#include <stdio.h>
#include <string.h>

#include <stdexcept>

#ifdef _WIN32
#include <mdz_mem_vars/w32compat.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#endif

using namespace Mantids::Network::Sockets;


Socket_UDP::Socket_UDP()
{
    res = nullptr;
}

Socket_UDP::~Socket_UDP()
{
    freeAddrInfo();
}

bool Socket_UDP::isConnected()
{
    return true;
}

bool Socket_UDP::listenOn(const uint16_t &port, const char *listenOnAddr, const int32_t &recvbuffer, const int32_t &backlog)
{
    int on = 1;

    if (isActive()) closeSocket(); // close and release first
    // Socket initialization.
    sockfd = socket(useIPv6?AF_INET6:AF_INET, SOCK_DGRAM, 0);
    if (!isActive())
    {
        lastError = "socket() failed";
        return false;
    }

    // Set to reuse address (if released and wait)..
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char *) &on, sizeof(on)) < 0)
    {
        lastError = "setsockopt(SO_REUSEADDR) failed";
        closeSocket();
        return false;
    }

    if (!bindTo(listenOnAddr,port))
    {
        return false;
    }

    listenMode = true;

    // Done!
    return true;
}

bool Socket_UDP::connectFrom(const char *bindAddress, const char *remoteHost, const uint16_t &port, const uint32_t &timeout)
{
    if (isActive()) closeSocket(); // close and release first

    // Clean up any previously resolved addrinfo.
    freeAddrInfo();

    if (!getAddrInfo(remoteHost,port,SOCK_DGRAM,(void **)&res))
    {
        // Bad name resolution...
        return false;
    }

    // Initialize the socket...
    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (!isActive())
    {
        lastError = "socket() failed";
        return false;
    }

    if (!bindTo(bindAddress))
    {
        return false;
    }

    // UDP connection does not establish the connection. is enough to have the remote address resolved and the socket file descriptor...
    bool connected = (res ? true : false);
    if (!connected)
    {
        lastError = std::string("Connection using UDP Socket to ") + remoteHost + (":") + std::to_string(port) + (" Failed with ") + strerrorname_np(errno) + ": " + strerrordesc_np(errno);
        return false;
    }

    // Set the timeout here.
    setReadTimeout(timeout);

    return true;
}

bool Socket_UDP::writeBlock(const void *data, const uint32_t & datalen)
{
    if (!isActive()) return false;
    if (!res) return false;
#ifdef _WIN32
    if (sendto(sockfd, (char *)data, datalen, 0, res->ai_addr, res->ai_addrlen) == -1)
#else
    if (sendto(sockfd, data, datalen, 0, res->ai_addr, res->ai_addrlen) == -1)
#endif
    {
        return false;
    }
    return true;
}

#define SOCKADDR_IN_SIZE sizeof(struct sockaddr)

uint32_t Socket_UDP::getMinReadSize()
{
    return (SOCKADDR_IN_SIZE + sizeof(int));
}

void Socket_UDP::freeAddrInfo()
{
    if (res)
        freeaddrinfo(res);
    res = nullptr;
}

bool Socket_UDP::readBlock(void *, const uint32_t &)
{
    // USE: readBlock
    throw std::runtime_error("Don't use Socket_UDP::readBlock(data,len)");
    return false;
}

std::shared_ptr<Datagram::Block> Socket_UDP::readBlock()
{
    std::shared_ptr<Datagram::Block> datagramBlock;
    datagramBlock.reset(new Datagram::Block);
    if (!isActive()) return datagramBlock;

    socklen_t fromlen = SOCKADDR_IN_SIZE;

    char bigBlock[65536];
#ifdef _WIN32
    (*datagramBlock).datalen = recvfrom(sockfd, bigBlock, 65536, 0, &((*datagramBlock).addr) , &fromlen);
#else
    (*datagramBlock).datalen = recvfrom(sockfd, (void *) bigBlock, 65536, 0, &((*datagramBlock).addr) , &fromlen);
#endif

    (*datagramBlock).copy(bigBlock, (*datagramBlock).datalen);
    return datagramBlock;
}

