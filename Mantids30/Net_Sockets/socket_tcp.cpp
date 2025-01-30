#include "socket_tcp.h"

#include <memory>
#include <sys/types.h>

#ifdef _WIN32
#include <Mantids30/Memory/w32compat.h>
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

using namespace Mantids30::Network;
using namespace Mantids30::Network::Sockets;

Socket_TCP::Socket_TCP()
{
    m_useTCPForceKeepAlive = false;
    
    m_tcpKeepIdle=10;
    m_tcpKeepCnt=5;
    m_tcpKeepInterval=5;
    
    m_useTcpNoDelayOption = true;
    
    m_overwriteReadTimeout = -1;
    m_overwriteWriteTimeout = -1;
}

Socket_TCP::~Socket_TCP()
{
}

bool Socket_TCP::connectFrom(const char *bindAddress, const char *remoteHost, const uint16_t &port, const uint32_t &timeout)
{
    addrinfo *res = nullptr;
    m_lastError = "";
    if (!getAddrInfo(remoteHost,port,SOCK_STREAM,(void **)&res))
    {
        // Bad name resolution...
        return false;
    }

    m_remoteServerHostname = remoteHost;
    setRemotePort(port);

    bool connected = false;

    // Iterate over DNS
    for (struct addrinfo *resiter=res; resiter && !connected; resiter = resiter->ai_next)
    {
        if (m_sockFD >=0 ) closeSocket();
        m_sockFD = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if (!isActive())
        {
            m_lastError = "socket() failed, " + std::string(strerror(errno));
            break;
        }

        if (!bindTo(bindAddress))
        {
            break;
        }

        setReadTimeout(0);

        sockaddr * curAddr = resiter->ai_addr;
        struct sockaddr_in * curAddrIn = ((sockaddr_in *)curAddr);

        if (     (resiter->ai_addr->sa_family == AF_INET) ||             // IPv4 always have the permission to go.
                 (resiter->ai_addr->sa_family == AF_INET6 && m_useIPv6)   // Check if ipv6 have our permission to go.
                 )
        {

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

            // Connect...
            if (tcpConnect(resiter->ai_addr->sa_family,curAddr, resiter->ai_addrlen,timeout))
            {
                if (m_overwriteReadTimeout!=-1)
                    setReadTimeout(m_overwriteReadTimeout);
                if (m_overwriteWriteTimeout!=-1)
                    setWriteTimeout(m_overwriteWriteTimeout);

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
        if (m_lastError == "")
            m_lastError = std::string("Protocol Initialization Error.");
        return false;
    }



    return true;
}

std::shared_ptr<Sockets::Socket_Stream_Base> Socket_TCP::acceptConnection()
{
    int sdconn;

    if (!isActive()) return nullptr;

    std::shared_ptr<Socket_Stream_Base> cursocket;

    int32_t clilen;
    struct sockaddr_in cli_addr;
    clilen = sizeof(cli_addr);
    
    if ((sdconn = accept(m_sockFD, (struct sockaddr *) &cli_addr, (socklen_t *)&clilen)) >= 0)
    {
        if (m_useTCPForceKeepAlive)
        {
            int flags =1;
#ifdef _WIN32
            if (setsockopt(sdconn, SOL_SOCKET, SO_KEEPALIVE, (const char *)&flags, sizeof(flags)))
#else
            if (setsockopt(sdconn, SOL_SOCKET, SO_KEEPALIVE, (void *)&flags, sizeof(flags)))
#endif
            {
                // BAD...
            }
        }

        cursocket = std::make_shared<Socket_TCP>();
        // Set the proper socket-
        cursocket->setSocketFD(sdconn);
        char ipAddr[INET6_ADDRSTRLEN];
        inet_ntop(AF_INET, &cli_addr.sin_addr, ipAddr, sizeof(ipAddr)-1);
        cursocket->setRemotePort(ntohs(cli_addr.sin_port));
        cursocket->setRemotePair(ipAddr);
        
        std::dynamic_pointer_cast<Socket_TCP>(cursocket)->setTcpNoDelayOption(m_useTcpNoDelayOption);

        if (m_readTimeout)
            cursocket->setReadTimeout(m_readTimeout);
        if (m_writeTimeout)
            cursocket->setWriteTimeout(m_writeTimeout);
        if (m_recvBuffer)
            cursocket->setRecvBuffer(m_recvBuffer);
    }
    // Establish the error.
    else
    {
        m_lastError = "accept() failed";
        return nullptr;
    }

    // return the socket class.
    return cursocket;
}

bool Socket_TCP::tcpConnect(const unsigned short & addrFamily, const sockaddr *addr, socklen_t addrlen, uint32_t timeout)
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
    
    if (m_useTCPForceKeepAlive)
    {
        // From: https://bz.apache.org/bugzilla/show_bug.cgi?id=57955#c2
        // https://linux.die.net/man/7/tcp
#ifdef TCP_KEEPIDLE
        //- the idle time (TCP_KEEPIDLE): time the connection needs to be idle to start sending keep-alive probes, 2 hours by default
        if (setTCPOptionBool(TCP_KEEPIDLE,m_tcpKeepIdle))
        {
        }
#endif

#ifdef TCP_KEEPCNT
        //- the count (TCP_KEEPCNT): the number of probes to send before concluding the connection is down, default is 9
        if (setTCPOptionBool(TCP_KEEPCNT,m_tcpKeepCnt))
        {
        }
#endif

#ifdef TCP_KEEPINTVL
        //- the time interval (TCP_KEEPINTVL): the interval between successive probes after the idle time, default is 75 seconds
        if (setTCPOptionBool(TCP_KEEPINTVL,m_tcpKeepInterval))
        {
        }
#endif
    }

    // Trying to connect with timeout.
    res2 = connect(m_sockFD, addr, addrlen);
    if (res2 < 0)
    {
#ifdef _WIN32
        auto werr = WSAGetLastError();
        if (werr == WSAEWOULDBLOCK)
#else
        if (errno == EINPROGRESS || !errno)
#endif
        {
            fd_set myset;

            struct timeval tv;
            tv.tv_sec = timeout;
            tv.tv_usec = 0;
            FD_ZERO(&myset);
            FD_SET(m_sockFD, &myset);
            
            res2 = select(m_sockFD+1, nullptr, &myset, nullptr, timeout?&tv:nullptr);
#ifdef _WIN32
            if (res2 < 0 && WSAGetLastError() != WSAEINTR)
#else
            if (res2 < 0 && errno != EINTR)
#endif
            {
#ifdef _WIN32
                switch (WSAGetLastError())
                {
                case WSANOTINITIALISED:
                    lastError = "select() - A successful WSAStartup call must occur before using select().";
                    break;
                case WSAEFAULT:
                    lastError = "select() - The Windows Sockets implementation was unable to allocate needed resources for its internal operations";
                    break;
                case WSAENETDOWN:
                    lastError = "select() - The network subsystem has failed.";
                    break;
                case WSAEINVAL:
                    lastError = "select() - The time-out value is not valid, or all three descriptor parameters were null.";
                    break;
                case WSAEINTR:
                    lastError = "select() - A blocking Windows Socket 1.1 call was canceled through WSACancelBlockingCall.";
                    break;
                case WSAEINPROGRESS:
                    lastError = "select() - A blocking Windows Sockets 1.1 call is in progress, or the service provider is still processing a callback function.";
                    break;
                case WSAENOTSOCK:
                    lastError = "select() - One of the descriptor sets contains an entry that is not a socket.";
                    break;
                default:
                    lastError = "select() - unknown error (" + std::to_string( WSAGetLastError() ) + ")";
                    break;

                }
#else
                // TODO: specific message.
                m_lastError = "select() - error (" + std::to_string(errno) + ")";
#endif
                return false;
            }
            else if (res2 > 0)
            {
                // Socket selected for write
                socklen_t lon;
                lon = sizeof(int);
                if (getSocketOption(SOL_SOCKET, SO_ERROR, (void*)(&valopt), &lon) < 0)
                {
                    m_lastError = "Error in getsockopt(SOL_SOCKET)";
                    return false;
                }
                // Check the value returned...
                if (valopt)
                {
                    char cError[1024]="Unknown Error";
                    m_lastError = std::string("Connection using TCP Socket to ") + m_remotePair + (":") + std::to_string(m_remotePort) + (" Failed with error #") + std::to_string(valopt) + ": " + strerror_r(valopt,cError,sizeof(cError));
                    return false;
                }

                // Pass to blocking mode socket instead select it.
                // And.. Even if we are connected, if we can't go back to blocking, disconnect.
                if (!setBlockingMode(true))
                {
                    return false;
                }
                
                if (m_useTCPForceKeepAlive)
                {
                    if (setSocketOptionBool(SOL_SOCKET, SO_KEEPALIVE, true))
                    {
                        m_lastError = "setsocketopt(SO_KEEPALIVE)";
                        return false;
                    }
                }
                
                if (setTCPOptionBool(TCP_NODELAY,m_useTcpNoDelayOption))
                {
                    m_lastError = "setsocketopt(TCP_NODELAY)";
                    return false;
                }

                // Connected!!!
                return true;
            }
            else
            {
                m_lastError = std::string("Connection using TCP Socket to ") + m_remotePair + (":") + std::to_string(m_remotePort) + (" Failed with SELTMOUT: Timeout while connecting...");
                return false;
            }
        }
        else
        {
#ifdef _WIN32
            switch(werr)
            {
            case WSANOTINITIALISED:
                lastError = "connect() - A successful WSAStartup call must occur before using this function.";
                break;
            case WSAENETDOWN:
                lastError = "connect() - The network subsystem has failed.";
                break;
            case WSAEADDRINUSE:
                lastError = "connect() - The socket's local address is already in use and the socket was not marked to allow address reuse with SO_REUSEADDR.";
                break;
            case WSAEINTR:
                lastError = "connect() - The blocking Windows Socket 1.1 call was canceled through WSACancelBlockingCall.";
                break;
            case WSAEINPROGRESS:
                lastError = "connect() - A blocking Windows Sockets 1.1 call is in progress, or the service provider is still processing a callback function.";
                break;
            case WSAEALREADY:
                lastError = "connect() - A nonblocking connect call is in progress on the specified socket.";
                break;
            case WSAEADDRNOTAVAIL:
                lastError = "connect() - The remote address is not a valid address (such as INADDR_ANY or in6addr_any)";
                break;
            case WSAEAFNOSUPPORT:
                lastError = "connect() - Addresses in the specified family cannot be used with this socket.";
                break;
            case WSAECONNREFUSED:
                lastError = "connect() - The attempt to connect was forcefully rejected.";
                break;
            case WSAEFAULT:
                lastError = "connect() - The sockaddr structure pointed to by the name contains incorrect address format for the associated address family or the namelen parameter is too small.";
                break;
            case WSAEINVAL:
                lastError = "connect() - The parameter is a listening socket.";
                break;
            case WSAEISCONN:
                lastError = "connect() - The socket is already connected (connection-oriented sockets only).";
                break;
            case WSAENETUNREACH:
                lastError = "connect() - The network cannot be reached from this host at this time.";
                break;
            case WSAEHOSTUNREACH:
                lastError = "connect() - A socket operation was attempted to an unreachable host.";
                break;
            case WSAENOBUFS:
                lastError = "connect() - Note  No buffer space is available. The socket cannot be connected.";
                break;
            case WSAENOTSOCK:
                lastError = "connect() - The descriptor specified in the s parameter is not a socket.";
                break;
            case WSAETIMEDOUT:
                lastError = "connect() - An attempt to connect timed out without establishing a connection.";
                break;
            case WSAEWOULDBLOCK:
                lastError = "connect() - The socket is marked as nonblocking and the connection cannot be completed immediately.";
                break;
            case WSAEACCES:
                lastError = "connect() - An attempt to connect a datagram socket to broadcast address failed because setsockopt option SO_BROADCAST is not enabled.";
            default:
                lastError = "connect() - unknown error (" + std::to_string( WSAGetLastError() ) + ")";
                break;
            }
#else

            char cError[1024]="Unknown Error";
            
            m_lastError = std::string("Connection using TCP Socket to ") + m_remotePair + (":") + std::to_string(m_remotePort) + (" Failed with error #") + std::to_string(errno) + ": " + strerror_r(errno,cError,sizeof(cError));
#endif
            return false;
        }
    }
    // What we are doing here?
    setBlockingMode(true);
    return false;
}

bool Socket_TCP::getTcpNoDelayOption() const
{
    return m_useTcpNoDelayOption;
}

void Socket_TCP::setTcpNoDelayOption(bool newTcpNoDelayOption)
{
    m_useTcpNoDelayOption = newTcpNoDelayOption;
    setTCPOptionBool(TCP_NODELAY,m_useTcpNoDelayOption);
}

bool Socket_TCP::getTcpUseKeepAlive() const
{
    return m_useTCPForceKeepAlive;
}

void Socket_TCP::setTcpUseKeepAlive(bool newTcpUseKeepAlive)
{
    m_useTCPForceKeepAlive = newTcpUseKeepAlive;
}

int Socket_TCP::getTcpKeepIntvl() const
{
    return m_tcpKeepInterval;
}

void Socket_TCP::setTcpKeepIntvl(int newTcpKeepIntvl)
{
    m_tcpKeepInterval = newTcpKeepIntvl;
}

int Socket_TCP::getTcpKeepCnt() const
{
    return m_tcpKeepCnt;
}

void Socket_TCP::setTcpKeepCnt(int newTcpKeepCnt)
{
    m_tcpKeepCnt = newTcpKeepCnt;
}

int Socket_TCP::getTcpKeepIdle() const
{
    return m_tcpKeepIdle;
}

void Socket_TCP::setTcpKeepIdle(int newTcpKeepIdle)
{
    m_tcpKeepIdle = newTcpKeepIdle;
}

bool Socket_TCP::listenOn(const uint16_t & port, const char * listenOnAddr, const int32_t & recvbuffer,const int32_t & backlog)
{
    m_sockFD = socket(m_useIPv6?AF_INET6:AF_INET, SOCK_STREAM, 0);
    if (!isActive())
    {
        m_lastError = "socket() failed";
        return false;
    }

    if (recvbuffer) setRecvBuffer(recvbuffer);

    if (setSocketOptionBool(SOL_SOCKET, SO_REUSEADDR, true))
    {
        m_lastError = "setsockopt(SO_REUSEADDR) failed";
        closeSocket();
        return false;
    }
    
    if (m_useTCPForceKeepAlive)
    {
        // From: https://bz.apache.org/bugzilla/show_bug.cgi?id=57955#c2
        // https://linux.die.net/man/7/tcp
#ifdef TCP_KEEPIDLE
        //- the idle time (TCP_KEEPIDLE): time the connection needs to be idle to start sending keep-alive probes, 2 hours by default
        if (setTCPOptionBool(TCP_KEEPIDLE, m_tcpKeepIdle))
        {
        }
#endif

#ifdef TCP_KEEPCNT
        //- the count (TCP_KEEPCNT): the number of probes to send before concluding the connection is down, default is 9
        if (setTCPOptionBool(TCP_KEEPCNT, m_tcpKeepCnt))
        {
        }
#endif

#ifdef TCP_KEEPINTVL
        //- the time interval (TCP_KEEPINTVL): the interval between successive probes after the idle time, default is 75 seconds
        if (setTCPOptionBool(TCP_KEEPINTVL, m_tcpKeepInterval))
        {
        }
#endif
    }

    if (!bindTo(listenOnAddr,port))
    {
        return false;
    }
    
    if (listen(m_sockFD, backlog) < 0)
    {
        m_lastError = "listen() failed";
        closeSocket();
        return false;
    }

    m_isInListenMode = true;

    return true;
}


bool Socket_TCP::postAcceptSubInitialization()
{
    return true;
}

int Socket_TCP::setTCPOptionBool(const int32_t &optname, bool value)
{
    return setSocketOptionBool(
            #ifdef WIN32
                IPPROTO_TCP
            #else
                SOL_TCP
            #endif
                ,optname,value);
}

int Socket_TCP::setTCPOption(const int32_t &optname, const void *optval, socklen_t optlen)
{
    return setSocketOption(
            #ifdef WIN32
                IPPROTO_TCP
            #else
                SOL_TCP
            #endif
                , optname, optval, optlen);
}

int Socket_TCP::getTCPOption(const int32_t & optname, void *optval, socklen_t *optlen)
{
    return getSocketOption(
            #ifdef WIN32
                IPPROTO_TCP
            #else
                SOL_TCP
            #endif
                , optname, optval, optlen);
}

void Socket_TCP::overrideReadTimeout(int32_t tout)
{
    m_overwriteReadTimeout = tout;
}

void Socket_TCP::overrideWriteTimeout(int32_t tout)
{
    m_overwriteWriteTimeout = tout;
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
