#include "socket_tcp.h"

#include <memory>
#include <chrono>
#include <system_error>
#ifndef _WIN32
#include <poll.h>
#endif
#include <sys/types.h>

#ifdef _WIN32
#include <Mantids30/Core/Memory/w32compat.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#else

#include <netdb.h>
#include <sys/socket.h>

#include <arpa/inet.h>
#include <netinet/tcp.h>
#endif

#include <cerrno>
#include <climits>
#include <algorithm>
#include <cstring>
#include <unistd.h>

using namespace Mantids30::Network;
using namespace Mantids30::Network::Sockets;


std::string Socket_TCP::describeTCPError(const std::string &operation, int error)
{
#ifdef _WIN32
    if (operation == "connect()")
    {
        switch (error)
        {
        case WSANOTINITIALISED:
            return "connect() - A successful WSAStartup call must occur before using this function.";
        case WSAENETDOWN:
            return "connect() - The network subsystem has failed.";
        case WSAEADDRINUSE:
            return "connect() - The socket's local address is already in use and the socket was not marked to allow address reuse with SO_REUSEADDR.";
        case WSAEINTR:
            return "connect() - The blocking Windows Socket 1.1 call was canceled through WSACancelBlockingCall.";
        case WSAEINPROGRESS:
            return "connect() - A blocking Windows Sockets 1.1 call is in progress, or the service provider is still processing a callback function.";
        case WSAEALREADY:
            return "connect() - A nonblocking connect call is in progress on the specified socket.";
        case WSAEADDRNOTAVAIL:
            return "connect() - The remote address is not a valid address (such as INADDR_ANY or in6addr_any)";
        case WSAEAFNOSUPPORT:
            return "connect() - Addresses in the specified family cannot be used with this socket.";
        case WSAECONNREFUSED:
            return "connect() - The attempt to connect was forcefully rejected.";
        case WSAEFAULT:
            return "connect() - The sockaddr structure pointed to by the name contains incorrect address format for the associated address family or the namelen parameter is too small.";
        case WSAEINVAL:
            return "connect() - The parameter is a listening socket.";
        case WSAEISCONN:
            return "connect() - The socket is already connected (connection-oriented sockets only).";
        case WSAENETUNREACH:
            return "connect() - The network cannot be reached from this host at this time.";
        case WSAEHOSTUNREACH:
            return "connect() - A socket operation was attempted to an unreachable host.";
        case WSAENOBUFS:
            return "connect() - Note  No buffer space is available. The socket cannot be connected.";
        case WSAENOTSOCK:
            return "connect() - The descriptor specified in the s parameter is not a socket.";
        case WSAETIMEDOUT:
            return "connect() - An attempt to connect timed out without establishing a connection.";
        case WSAEWOULDBLOCK:
            return "connect() - The socket is marked as nonblocking and the connection cannot be completed immediately.";
        case WSAEACCES:
            return "connect() - An attempt to connect a datagram socket to broadcast address failed because setsockopt option SO_BROADCAST is not enabled.";
        default:
            break;
        }
    }
    if (operation == "select()")
    {
        switch (error)
        {
        case WSANOTINITIALISED:
            return "select() - A successful WSAStartup call must occur before using select().";
        case WSAEFAULT:
            return "select() - The Windows Sockets implementation was unable to allocate needed resources for its internal operations";
        case WSAENETDOWN:
            return "select() - The network subsystem has failed.";
        case WSAEINVAL:
            return "select() - The time-out value is not valid, or all three descriptor parameters were null.";
        case WSAEINTR:
            return "select() - A blocking Windows Socket 1.1 call was canceled through WSACancelBlockingCall.";
        case WSAEINPROGRESS:
            return "select() - A blocking Windows Sockets 1.1 call is in progress, or the service provider is still processing a callback function.";
        case WSAENOTSOCK:
            return "select() - One of the descriptor sets contains an entry that is not a socket.";
        default:
            break;
        }
    }
#endif
    return operation + " - " + std::system_category().message(error);
}

std::string Socket_TCP::connectionError(const std::string &operation, int error) const
{
    return "Connection using TCP Socket to " + std::string(m_remotePair) + ":" + std::to_string(m_remotePort) +
           " Failed with error #" + std::to_string(error) + ": " + describeTCPError(operation, error);
}

bool Socket_TCP::connectFrom(const char *bindAddress, const char *remoteHost, const uint16_t &port, const uint32_t &timeout)
{
    if (isActive())
    {
        closeSocket();
    }
    if (!remoteHost || !*remoteHost)
    {
        m_lastError = "Remote host is empty";
        return false;
    }
    addrinfo *res = nullptr;
    m_lastError.clear();
    if (!getAddrInfo(remoteHost, port, SOCK_STREAM, reinterpret_cast<void **>(&res)))
    {
        // Bad name resolution...
        return false;
    }

    m_remoteServerHostname = remoteHost;
    setRemotePort(port);

    bool connected = false;

    // Iterate over DNS
    for (struct addrinfo *resiter = res; resiter && !connected; resiter = resiter->ai_next)
    {
        if (m_sockFD >= 0)
        {
            closeSocket();
        }
        const int descriptor = socket(resiter->ai_family, resiter->ai_socktype, resiter->ai_protocol);
        setSocketFD(descriptor);
        m_useIPv6 = resiter->ai_family == AF_INET6;
        if (!isActive())
        {
            m_lastError = connectionError("socket()", getLastSocketError());
            continue;
        }

        if (!bindTo(bindAddress))
        {
            continue;
        }


        // Preserve the original client default: no read timeout unless overridden.
        setReadTimeout(0);

        sockaddr *curAddr = resiter->ai_addr;
        struct sockaddr_in *curAddrIn = (reinterpret_cast<sockaddr_in *>(curAddr));

        if (curAddr->sa_family == AF_INET || curAddr->sa_family == AF_INET6)
        {
            // Set remote pairs...
            switch (curAddr->sa_family)
            {
            case AF_INET6:
            {
                char ipAddr6[INET6_ADDRSTRLEN] = "";
                inet_ntop(AF_INET6, &reinterpret_cast<sockaddr_in6 *>(curAddr)->sin6_addr, ipAddr6, INET6_ADDRSTRLEN);
                setRemotePair(ipAddr6);
            }
            break;
            case AF_INET:
            {
                char ipAddr4[INET_ADDRSTRLEN] = "";
                inet_ntop(AF_INET, &(curAddrIn->sin_addr), ipAddr4, INET_ADDRSTRLEN);
                setRemotePair(ipAddr4);
            }
            break;
            default:
                break;
            }

            // Connect...
            if (tcpConnect(resiter->ai_addr->sa_family, curAddr, resiter->ai_addrlen, timeout))
            {
                if (m_overwriteReadTimeout != -1)
                {
                    setReadTimeout(m_overwriteReadTimeout);
                }
                if (m_overwriteWriteTimeout != -1)
                {
                    setWriteTimeout(m_overwriteWriteTimeout);
                }

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
        closeSocket();
        if (m_lastError.empty())
        {
            m_lastError = std::string("Protocol Initialization Error.");
        }
        return false;
    }

    m_lastError.clear();
    return true;
}

std::shared_ptr<Mantids30::Network::Sockets::Socket_Stream> Socket_TCP::acceptConnection()
{
    int sdconn;

    if (!isActive())
    {
        return nullptr;
    }

    std::shared_ptr<Socket_Stream> cursocket;

    sockaddr_storage cli_addr{};
    socklen_t clilen = sizeof(cli_addr);

    int acceptError = 0;
    do
    {
        sdconn = accept(m_sockFD, reinterpret_cast<sockaddr *>(&cli_addr), &clilen);
        if (sdconn < 0)
        {
            acceptError = getLastSocketError();
        }
    } while (sdconn < 0 && isSocketErrorInterrupted(acceptError));
    if (sdconn >= 0)
    {
        if (m_useTCPForceKeepAlive)
        {
            int flags = 1;
#ifdef _WIN32
            if (setsockopt(sdconn, SOL_SOCKET, SO_KEEPALIVE, (const char *) &flags, sizeof(flags)))
#else
            if (setsockopt(sdconn, SOL_SOCKET, SO_KEEPALIVE, reinterpret_cast<void *>(&flags), sizeof(flags)))
#endif
            {
                // BAD...
            }
        }

        cursocket = std::make_shared<Socket_TCP>();
        // Set the proper socket-
        cursocket->setUseIPv6(cli_addr.ss_family == AF_INET6);
        cursocket->setSocketFD(sdconn);
        char ipAddr[INET6_ADDRSTRLEN]{};
        if (cli_addr.ss_family == AF_INET6)
        {
            const auto *address = reinterpret_cast<const sockaddr_in6 *>(&cli_addr);
            inet_ntop(AF_INET6, &address->sin6_addr, ipAddr, sizeof(ipAddr));
            cursocket->setRemotePort(ntohs(address->sin6_port));
        }
        else
        {
            const auto *address = reinterpret_cast<const sockaddr_in *>(&cli_addr);
            inet_ntop(AF_INET, &address->sin_addr, ipAddr, sizeof(ipAddr));
            cursocket->setRemotePort(ntohs(address->sin_port));
        }
        cursocket->setRemotePair(ipAddr);

        std::dynamic_pointer_cast<Socket_TCP>(cursocket)->setTcpNoDelayOption(m_useTcpNoDelayOption);

        if (m_readTimeout)
        {
            cursocket->setReadTimeout(m_readTimeout);
        }
        if (m_writeTimeout)
        {
            cursocket->setWriteTimeout(m_writeTimeout);
        }
        if (m_recvBuffer)
        {
            cursocket->setRecvBuffer(m_recvBuffer);
        }
    }
    // Establish the error.
    else
    {
        m_lastError = "accept() failed: " + std::system_category().message(acceptError);
        return nullptr;
    }

    // return the socket class.
    return cursocket;
}

bool Socket_TCP::tcpConnect(const unsigned short &, const sockaddr *addr, socklen_t addrlen, uint32_t timeout)
{
    if (!setBlockingMode(false))
    {
        return false;
    }
    int result = connect(m_sockFD, addr, addrlen);
    if (result < 0)
    {
        const int connectError = getLastSocketError();
#ifdef _WIN32
        if (connectError != WSAEWOULDBLOCK)
#else
        if (connectError != EINPROGRESS && connectError != EINTR)
#endif
        {
            m_lastError = connectionError("connect()", connectError);
            return false;
        }
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(timeout);
        int waitError = 0;
        do
        {
            const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(deadline - std::chrono::steady_clock::now()).count();
#ifdef _WIN32
            fd_set writable;
            FD_ZERO(&writable);
            FD_SET(m_sockFD, &writable);
            fd_set exceptional = writable;
            timeval waitTime{};
            waitTime.tv_sec = static_cast<long>(std::max<int64_t>(remaining, 0) / 1000);
            waitTime.tv_usec = static_cast<long>(std::max<int64_t>(remaining, 0) % 1000) * 1000;
            result = select(0, nullptr, &writable, &exceptional, timeout ? &waitTime : nullptr);
#else
            pollfd descriptor{m_sockFD, POLLOUT, 0};
            const int waitMS = timeout ? static_cast<int>(std::min<int64_t>(std::max<int64_t>(remaining, 0), INT_MAX)) : -1;
            result = poll(&descriptor, 1, waitMS);
#endif
            if (result < 0)
            {
                waitError = getLastSocketError();
            }
            if (result >= 0 || !isSocketErrorInterrupted(waitError))
            {
                break;
            }
        } while (timeout == 0 || std::chrono::steady_clock::now() < deadline);
        if (result <= 0)
        {
            if (result == 0)
            {
                m_lastError = "Connection using TCP Socket to " + std::string(m_remotePair) + ":" + std::to_string(m_remotePort) +
                              " Failed with SELTMOUT: Timeout while connecting...";
            }
            else
            {
#ifdef _WIN32
                m_lastError = connectionError("select()", waitError);
#else
                m_lastError = connectionError("poll()", waitError);
#endif
            }
            return false;
        }
        int error = 0;
        socklen_t length = sizeof(error);
        const int optionResult = getSocketOption(SOL_SOCKET, SO_ERROR, &error, &length);
        if (optionResult < 0)
        {
            const int optionError = getLastSocketError();
            m_lastError = connectionError("getsockopt(SO_ERROR)", optionError);
            return false;
        }
        if (error != 0)
        {
            m_lastError = connectionError("connect()", error);
            return false;
        }
    }
    if (!setBlockingMode(true))
    {
        return false;
    }
    if (m_useTCPForceKeepAlive)
    {
        if (setSocketOptionBool(SOL_SOCKET, SO_KEEPALIVE, true) != 0)
        {
            m_lastError = "setsockopt(SO_KEEPALIVE) failed";
            return false;
        }
#ifdef TCP_KEEPIDLE
        setTCPOption(TCP_KEEPIDLE, &m_tcpKeepIdle, sizeof(m_tcpKeepIdle));
#endif
#ifdef TCP_KEEPCNT
        setTCPOption(TCP_KEEPCNT, &m_tcpKeepCnt, sizeof(m_tcpKeepCnt));
#endif
#ifdef TCP_KEEPINTVL
        setTCPOption(TCP_KEEPINTVL, &m_tcpKeepInterval, sizeof(m_tcpKeepInterval));
#endif
    }
    if (setTCPOptionBool(TCP_NODELAY, m_useTcpNoDelayOption) != 0)
    {
        m_lastError = "setsockopt(TCP_NODELAY) failed";
        return false;
    }
    return true;
}

bool Socket_TCP::getTcpNoDelayOption() const
{
    return m_useTcpNoDelayOption;
}

void Socket_TCP::setTcpNoDelayOption(bool newTcpNoDelayOption)
{
    m_useTcpNoDelayOption = newTcpNoDelayOption;
    setTCPOptionBool(TCP_NODELAY, m_useTcpNoDelayOption);
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

bool Socket_TCP::listenOn(const uint16_t &port, const char *listenOnAddr, const int32_t &recvbuffer, const int32_t &backlog)
{
    closeSocket();
    setSocketFD(socket(m_useIPv6 ? AF_INET6 : AF_INET, SOCK_STREAM, 0));
    if (!isActive())
    {
        m_lastError = "socket() failed";
        return false;
    }

    if (recvbuffer)
    {
        setRecvBuffer(recvbuffer);
    }

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
        if (setTCPOption(TCP_KEEPIDLE, &m_tcpKeepIdle, sizeof(m_tcpKeepIdle)))
        {
        }
#endif

#ifdef TCP_KEEPCNT
        //- the count (TCP_KEEPCNT): the number of probes to send before concluding the connection is down, default is 9
        if (setTCPOption(TCP_KEEPCNT, &m_tcpKeepCnt, sizeof(m_tcpKeepCnt)))
        {
        }
#endif

#ifdef TCP_KEEPINTVL
        //- the time interval (TCP_KEEPINTVL): the interval between successive probes after the idle time, default is 75 seconds
        if (setTCPOption(TCP_KEEPINTVL, &m_tcpKeepInterval, sizeof(m_tcpKeepInterval)))
        {
        }
#endif
    }

    if (!bindTo(listenOnAddr, port))
    {
        closeSocket();
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

int Socket_TCP::setTCPOptionBool(const int32_t &optname, bool value)
{
    return setSocketOptionBool(
#ifdef WIN32
        IPPROTO_TCP
#else
        SOL_TCP
#endif
        ,
        optname, value);
}

int Socket_TCP::setTCPOption(const int32_t &optname, const void *optval, socklen_t optlen)
{
    return setSocketOption(
#ifdef WIN32
        IPPROTO_TCP
#else
        SOL_TCP
#endif
        ,
        optname, optval, optlen);
}

int Socket_TCP::getTCPOption(const int32_t &optname, void *optval, socklen_t *optlen)
{
    return getSocketOption(
#ifdef WIN32
        IPPROTO_TCP
#else
        SOL_TCP
#endif
        ,
        optname, optval, optlen);
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
