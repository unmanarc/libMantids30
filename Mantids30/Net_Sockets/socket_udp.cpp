#include "socket_udp.h"
#include <sys/types.h>
#include <stdio.h>
#include <string.h>

#include <stdexcept>

#ifdef _WIN32
#include <Mantids30/Memory/w32compat.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <Mantids30/Memory/w32compat.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#endif

using namespace Mantids30::Network::Sockets;


Socket_UDP::Socket_UDP()
{
    m_addressInfoResolution = nullptr;
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
    m_sockFD = socket(m_useIPv6?AF_INET6:AF_INET, SOCK_DGRAM, 0);
    if (!isActive())
    {
        m_lastError = "socket() failed";
        return false;
    }

    // Set to reuse address (if released and wait)..
    if (setsockopt(m_sockFD, SOL_SOCKET, SO_REUSEADDR, (char *) &on, sizeof(on)) < 0)
    {
        m_lastError = "setsockopt(SO_REUSEADDR) failed";
        closeSocket();
        return false;
    }

    if (!bindTo(listenOnAddr,port))
    {
        return false;
    }

    m_isInListenMode = true;

    // Done!
    return true;
}

bool Socket_UDP::connectFrom(const char *bindAddress, const char *remoteHost, const uint16_t &port, const uint32_t &timeout)
{
    if (isActive()) closeSocket(); // close and release first

    m_remoteServerHostname = remoteHost;

    // Clean up any previously resolved addrinfo.
    freeAddrInfo();
    
    if (!getAddrInfo(remoteHost,port,SOCK_DGRAM,(void **)&m_addressInfoResolution))
    {
        // Bad name resolution...
        return false;
    }

    // Initialize the socket...
    m_sockFD = socket(m_addressInfoResolution->ai_family, m_addressInfoResolution->ai_socktype, m_addressInfoResolution->ai_protocol);
    if (!isActive())
    {
        m_lastError = "socket() failed";
        return false;
    }

    if (!bindTo(bindAddress))
    {
        return false;
    }

    // UDP connection does not establish the connection. is enough to have the remote address resolved and the socket file descriptor...
    bool connected = (m_addressInfoResolution ? true : false);
    if (!connected)
    {
        char cError[1024]="Unknown Error";
        
        m_lastError = std::string("Connection using UDP Socket to ") + remoteHost + (":") + std::to_string(port) + (" Failed with error #") + std::to_string(errno) + ": " + strerror_r(errno,cError,sizeof(cError));
        return false;
    }

    // Set the timeout here.
    setReadTimeout(timeout);

    return true;
}

bool Socket_UDP::writeBlock(const void *data, const size_t & datalen)
{
    if (!isActive()) 
        return false;
    if (!m_addressInfoResolution) 
        return false;
#ifdef _WIN32
    if (sendto(sockfd, (char *)data, datalen, 0, res->ai_addr, res->ai_addrlen) == -1)
#else
    if (sendto(m_sockFD, data, datalen, 0, m_addressInfoResolution->ai_addr, m_addressInfoResolution->ai_addrlen) == -1)
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
    if (m_addressInfoResolution)
        freeaddrinfo(m_addressInfoResolution);
    m_addressInfoResolution = nullptr;
}

bool Socket_UDP::readBlock(void *, const size_t &)
{
    // USE: readBlock
    throw std::runtime_error("Don't use Socket_UDP::readBlock(data,len)");
    return false;
}

std::shared_ptr<Socket_UDP::Block> Socket_UDP::readBlock()
{
    std::shared_ptr<Socket_UDP::Block> datagramBlock;
    datagramBlock.reset(new Socket_UDP::Block);
    if (!isActive()) 
        return datagramBlock;

    socklen_t fromlen = SOCKADDR_IN_SIZE;

    char bigBlock[65536];
#ifdef _WIN32
    (*datagramBlock).datalen = recvfrom(sockfd, bigBlock, 65536, 0, &((*datagramBlock).addr) , &fromlen);
#else
    (*datagramBlock).dataLength = recvfrom(m_sockFD, (void *) bigBlock, 65536, 0, &((*datagramBlock).socketAddress) , &fromlen);
#endif
    
    (*datagramBlock).copy(bigBlock, (*datagramBlock).dataLength);
    return datagramBlock;
}

