#include "socket_tcp.h"

#include <sys/types.h>

#ifdef _WIN32
#include <cx2_mem_vars/w32compat.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#else

#include <sys/socket.h>
#include <netdb.h>

#include <arpa/inet.h>
#include <netinet/tcp.h>
#endif

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

using namespace CX2::Network;
using namespace CX2::Network::Sockets;

Socket_TCP::Socket_TCP()
{
    ovrReadTimeout = -1;
    ovrWriteTimeout = -1;
}

Socket_TCP::~Socket_TCP()
{
}

bool Socket_TCP::connectFrom(const char *bindAddress, const char *remoteHost, const uint16_t &port, const uint32_t &timeout)
{
    addrinfo *res = nullptr;
    lastError = "";
    if (!getAddrInfo(remoteHost,port,SOCK_STREAM,(void **)&res))
    {
        // Bad name resolution...
        return false;
    }

    bool connected = false;

    for (struct addrinfo *resiter=res; resiter && !connected; resiter = resiter->ai_next)
    {
        if (sockfd >=0 ) closeSocket();
        sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if (!isActive())
        {
            lastError = "socket() failed";
            break;
        }

        if (!bindTo(bindAddress))
        {
            break;
        }

        // Set the read timeout here. (to zero)
        setReadTimeout(0);

        sockaddr * curAddr = resiter->ai_addr;
        struct sockaddr_in * curAddrIn = ((sockaddr_in *)curAddr);

        if (    ( (resiter->ai_addr->sa_family == AF_INET) ||                // IPv4 always have the permission to go.
                  (resiter->ai_addr->sa_family == AF_INET6 && useIPv6))   // Check if ipv6 have our permission to go.
                && tcpConnect(curAddr, resiter->ai_addrlen,timeout)
                )
        {

            if (ovrReadTimeout!=-1) setReadTimeout(ovrReadTimeout);
            if (ovrWriteTimeout!=-1) setWriteTimeout(ovrWriteTimeout);

            // Set remote pairs...
            switch (curAddr->sa_family)
            {
            case AF_INET6:
            {
                char ipAddr6[INET6_ADDRSTRLEN]="";
                inet_ntop(AF_INET6, &(curAddrIn->sin_addr), ipAddr6, INET6_ADDRSTRLEN);
                setRemotePair(ipAddr6);
            }break;
            case AF_INET:
            {
                char ipAddr4[INET_ADDRSTRLEN]="";
                inet_ntop(AF_INET, &(curAddrIn->sin_addr), ipAddr4, INET_ADDRSTRLEN);
                setRemotePair(ipAddr4);
            }break;
            default:
                break;
            }

            setRemotePort(port);

            // now it's connected...
            if (postConnectSubInitialization())
            {
                connected = true;
            }
            else
            {
                // should disconnect here.
                shutdownSocket();
                // drop the socket descriptor. we don't need it anymore.
                closeSocket();
            }
            break;
        }
        else
        {
            // drop the current socket... (and free the resource :))
            shutdownSocket();
            closeSocket();
        }
    }

    freeaddrinfo(res);

    if (!connected)
    {
        if (lastError == "")
            lastError = "connect() failed";
        return false;
    }

    return true;
}

Streams::StreamSocket * Socket_TCP::acceptConnection()
{
    int sdconn;

    if (!isActive()) return nullptr;

    StreamSocket * cursocket;

    int32_t clilen;
    struct sockaddr_in cli_addr;
    clilen = sizeof(cli_addr);

    if ((sdconn = accept(sockfd, (struct sockaddr *) &cli_addr, (socklen_t *)&clilen)) >= 0)
    {
        cursocket = new Socket_TCP;
        // Set the proper socket-
        cursocket->setSocketFD(sdconn);
        char ipAddr[INET6_ADDRSTRLEN];
        inet_ntop(AF_INET, &cli_addr.sin_addr, ipAddr, sizeof(ipAddr)-1);
        cursocket->setRemotePort(ntohs(cli_addr.sin_port));
        cursocket->setRemotePair(ipAddr);

        if (readTimeout) cursocket->setReadTimeout(readTimeout);
        if (writeTimeout) cursocket->setWriteTimeout(writeTimeout);
        if (recvBuffer) cursocket->setRecvBuffer(recvBuffer);
    }
    // Establish the error.
    else
    {
        lastError = "accept() failed";
        return nullptr;
    }

    // return the socket class.
    return cursocket;
}

bool Socket_TCP::tcpConnect(const sockaddr *addr, socklen_t addrlen, uint32_t timeout)
{
    int res2,valopt;

    // Non-blocking connect with timeout...
    if (!setBlockingMode(false)) return false;

#ifdef _WIN32
    if (timeout == 0)
    {
        // in windows, if the timeval is 0,0, then it will return immediately.
        // however, our lib state that 0 represent that we sleep for ever.
        timeout = 365*24*3600; // how about 1 year.
    }
#endif

    // Trying to connect with timeout.
    res2 = connect(sockfd, addr, addrlen);
    if (res2 < 0)
    {
        if (errno == EINPROGRESS || !errno)
        {
            fd_set myset;

            struct timeval tv;
            tv.tv_sec = timeout;
            tv.tv_usec = 0;
            FD_ZERO(&myset);
            FD_SET(sockfd, &myset);

            res2 = select(sockfd+1, nullptr, &myset, nullptr, timeout?&tv:nullptr);

            if (res2 < 0 && errno != EINTR)
            {
                lastError = "Error selecting...";
                return false;
            }
            else if (res2 > 0)
            {
                // Socket selected for write
                socklen_t lon;
                lon = sizeof(int);
#ifdef _WIN32
                if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, (char*)(&valopt), &lon) < 0)
#else
                if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, (void*)(&valopt), &lon) < 0)
#endif
                {
                    lastError = "Error in getsockopt(SOL_SOCKET)";
                    return false;
                }
                // Check the value returned...
                if (valopt)
                {
                    lastError = "Error in delayed connection()";
                    return false;
                }

                // Even if we are connected, if we can't go back to blocking, disconnect.
                if (!setBlockingMode(true)) return false;

                // Connected!!!
                // Pass to blocking mode socket instead select it.
                return true;
            }
            else
            {
                lastError = "Timeout in select() - Cancelling!";
                return false;
            }
        }
        else
        {
            lastError = "Error connecting - (2)";
            return false;
        }
    }
    // What we are doing here?
    setBlockingMode(true);
    return false;
}

bool Socket_TCP::listenOn(const uint16_t & port, const char * listenOnAddr, const int32_t & recvbuffer,const int32_t & backlog)
{
#ifdef _WIN32
    BOOL bOn = TRUE;
#else
    int on=1;
#endif

    sockfd = socket(useIPv6?AF_INET6:AF_INET, SOCK_STREAM, 0);
    if (!isActive())
    {
        lastError = "socket() failed";
        return false;
    }

    if (recvbuffer) setRecvBuffer(recvbuffer);

    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,
               #ifdef _WIN32
                   (char *)(&bOn),sizeof(bOn)
               #else
                   static_cast<void *>(&on),sizeof(on)
               #endif
                   ) < 0)
    {
        lastError = "setsockopt(SO_REUSEADDR) failed";
        closeSocket();
        return false;
    }

    if (!bindTo(listenOnAddr,port))
    {
        return false;
    }

    if (listen(sockfd, backlog) < 0)
    {
        lastError = "listen() failed";
        closeSocket();
        return false;
    }

    listenMode = true;

    return true;
}


bool Socket_TCP::postAcceptSubInitialization()
{
    return true;
}

int Socket_TCP::setTCPOptionBool(const int32_t &optname, bool value)
{
    int flag = value?1:0;
    return setTCPOption(optname, (char *) &flag, sizeof(int));
}

int Socket_TCP::setTCPOption(const int32_t &optname, const void *optval, socklen_t optlen)
{
    return setSockOpt(IPPROTO_TCP, optname, optval, optlen);
}

int Socket_TCP::getTCPOption(const int32_t & optname, void *optval, socklen_t *optlen)
{
    return getSockOpt(IPPROTO_TCP, optname, optval, optlen);
}

void Socket_TCP::overrideReadTimeout(int32_t tout)
{
    ovrReadTimeout = tout;
}

void Socket_TCP::overrideWriteTimeout(int32_t tout)
{
    ovrWriteTimeout = tout;
}

bool Socket_TCP::isSecure()
{
    return false;
}
/*
bool Socket_TCP::postConnectSubInitialization()
{
    return true;
}
*/
