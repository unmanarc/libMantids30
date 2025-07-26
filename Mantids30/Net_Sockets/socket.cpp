#include "socket.h"

#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <inttypes.h>
#include <stdexcept>

#ifdef _WIN32
#include <Mantids30/Memory/w32compat.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#else

#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>

#endif

#include <Mantids30/Helpers/mem.h>

using namespace Mantids30::Network::Sockets;

#ifdef _WIN32
// Init winsock when the program begins...
bool Socket::m_isWinSockInitialized = Socket::win32Init();
#endif

bool Socket::m_globalSocketInitialized = false;

bool Socket::bindTo(const char *bindAddress, const uint16_t &port)
{
    if (bindAddress == nullptr)
        return true;

    if (!m_useIPv6)
    {
        ZeroBStruct(m_lastBindIPv4);

        m_lastBindIPv4.sin_family = AF_INET;
        m_lastBindIPv4.sin_port = htons(port);

        // Accept * or :: as a generic listen address
        if ((bindAddress[0] == '*' && bindAddress[1] == 0) ||
            (bindAddress[0] == ':' && bindAddress[1] == ':' && bindAddress[2] == 0))
        {
            inet_pton(AF_INET, "0.0.0.0", &m_lastBindIPv4.sin_addr);
        }
        else
        {
            if (inet_pton(AF_INET, bindAddress, &m_lastBindIPv4.sin_addr) <= 0)
            {
                m_lastError = "bindTo failed / Invalid IPv4 address format";
                return false;
            }
        }

        if (::bind(m_sockFD, (struct sockaddr *)&m_lastBindIPv4, sizeof(m_lastBindIPv4)) < 0)
        {
            m_lastError = "bind() failed";
            close(m_sockFD);
            m_lastBindValid = false;
            return false;
        }
    }
    else
    {
        ZeroBStruct(m_lastBindIPv6);

        m_lastBindIPv6.sin6_family = AF_INET6;
        m_lastBindIPv6.sin6_port = htons(port);

        if (bindAddress[0] == '*' && bindAddress[1] == 0)
        {
            inet_pton(AF_INET6, "::", &m_lastBindIPv6.sin6_addr);
        }
        else
        {
            if (inet_pton(AF_INET6, bindAddress, &m_lastBindIPv6.sin6_addr) <= 0)
            {
                m_lastError = "bind() failed / Invalid IPv6 address format";
                return false;
            }
        }

        if (::bind(m_sockFD, (struct sockaddr *)&m_lastBindIPv6, sizeof(m_lastBindIPv6)) < 0)
        {
            m_lastError = "bind() failed";
            close(m_sockFD);
            m_lastBindValid = false;
            return false;
        }
    }

    m_lastBindValid = true; // Successfully bound
    return true;
}



bool Socket::getAddrInfo(const char *remoteHost, const uint16_t &remotePort, int ai_socktype, void **res)
{
    addrinfo hints;
    int rc;

    ZeroBStruct(hints);

#ifdef _WIN32
    hints.ai_flags    = 0;
#else
    hints.ai_flags    = AI_NUMERICSERV;
#endif
    hints.ai_socktype = ai_socktype;
    hints.ai_family   = AF_UNSPEC;

    if (m_useIPv6)
    {
        struct in6_addr serveraddr;
        rc = inet_pton(AF_INET6, remoteHost, &serveraddr);
        if (rc == 1)
        {
            hints.ai_family = AF_INET6;
            hints.ai_flags |= AI_NUMERICHOST;
        }
    }
    else
    {
        struct in_addr serveraddr;
        rc = inet_pton(AF_INET, remoteHost, &serveraddr);
        if (rc == 1)
        {
            hints.ai_family = AF_INET;
            hints.ai_flags |= AI_NUMERICHOST;
        }
    }

    char serverPort[8];
    snprintf(serverPort,sizeof(serverPort),"%" PRIu16,remotePort);

    rc = getaddrinfo(remoteHost, serverPort, &hints, (addrinfo **)res);

    switch (rc)
    {
    case 0:
        return true;
#ifndef _WIN32
    case EAI_ADDRFAMILY:
        m_lastError = "getaddrinfo() - The specified network host does not have any network addresses in the requested address family.";
        return false;
#endif
    case EAI_AGAIN:
        m_lastError = "getaddrinfo() - The name server returned a temporary failure indication. Try again later.";
        return false;
    case EAI_BADFLAGS:
        m_lastError = "getaddrinfo() - hints.ai_flags contains invalid flags; or, hints.ai_flags included AI_CANONNAME and name was NULL.";
        return false;
    case EAI_FAIL:
        m_lastError = "getaddrinfo() - The name server returned a permanent failure indication.";
        return false;
    case EAI_FAMILY:
        m_lastError = "getaddrinfo() - The requested address family is not supported.";
        return false;
    case EAI_MEMORY:
        m_lastError = "getaddrinfo() - Out of memory during name resolution.";
        return false;
    case EAI_NODATA:
        m_lastError = "getaddrinfo() - The specified network host exists, but does not have any network addresses defined.";
        return false;
    case EAI_NONAME:
        m_lastError = "getaddrinfo() - The node or service is not known"; //; or both node and service are NULL; or AI_NUMERICSERV was specified in hints.ai_flags and service was not a numeric port-number string.";
        return false;
    case EAI_SERVICE:
        m_lastError = "getaddrinfo() - The requested service is not available for the requested socket type.";
        return false;
    case EAI_SOCKTYPE:
        m_lastError = "getaddrinfo() - The requested socket type is not supported.";
        return false;
#ifndef _WIN32
    case EAI_SYSTEM:
        m_lastError = "getaddrinfo() - System Error duing name resolution.";
        return false;
#endif
    default:
        m_lastError = "getaddrinfo() - Unknown name resolution error.";
        break;
    }

    return false;
}

std::string Socket::getConnectionName() const
{
    return m_connectionName;
}

void Socket::setConnectionName(const std::string &newConnectionName)
{
    m_connectionName = newConnectionName;
}

bool Socket::getUseIPv6() const
{
    return m_useIPv6;
}

void Socket::setUseIPv6(bool value)
{
    m_useIPv6 = value;
}

Socket::Socket()
{
    //initVars();
}

Socket::~Socket()
{
    closeSocket();
}

#ifdef _WIN32
bool Socket::win32Init()
{
    socketSystemInitialization();
    return true;
}
#endif

void Socket::setUseWriteInsteadRecv()
{
    // prevent the application from crash, ignore the sigpipes:
#ifndef _WIN32
    signal(SIGPIPE, SIG_IGN);
#endif
    // use write/read functions instead send/recv
    m_useWriteInsteadRecv = true;
}

void Socket::setRecvBuffer(int buffsize)
{
    m_recvBuffer = buffsize;

    if (!isActive()) 
        return;
#ifdef _WIN32
    setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, (char *) &buffsize, sizeof(buffsize));
#else
    setsockopt(m_sockFD, SOL_SOCKET, SO_RCVBUF, &buffsize, sizeof(buffsize));
#endif
}

bool Socket::isConnected()
{
    return false;
}

bool Socket::connectTo(const char *remoteHost, const uint16_t &remotePort, const uint32_t &timeout)
{
    return connectFrom(nullptr,remoteHost,remotePort,timeout);
}

bool Socket::connectFrom(const char *, const char *, const uint16_t &, const uint32_t &)
{
    return false;
}

void Socket::tryConnect(const char *remoteHost, const uint16_t &port,
                        const uint32_t &timeout)
{
    while (!connectTo(remoteHost, port, timeout))
    {
        // Try to reconnect if fail...
    }
}

bool Socket::listenOn(const uint16_t &, const char *, const int32_t &, const int32_t &)
{
    return false;
}

int Socket::closeSocket()
{
    if (!isActive())
        return 0;

    mutexClose.lock();
    // Prevent socket utilization / race condition.
    auto socktmp = (int)m_sockFD;
    m_sockFD = -1;
#ifdef _WIN32
    int i = closesocket(socktmp);
#else
    int i = close(socktmp);
#endif
    mutexClose.unlock();

    return i;
}

std::string Socket::getLastError() const
{
    return m_lastError;
}

std::string Socket::getLastBindAddress() const
{
    /*if (!m_lastBindValid)
        return "";*/

    char addrStr[INET6_ADDRSTRLEN];
    uint16_t port;

    if (!m_useIPv6)
    {
        inet_ntop(AF_INET, &m_lastBindIPv4.sin_addr, addrStr, sizeof(addrStr));
        port = ntohs(m_lastBindIPv4.sin_port);
    }
    else
    {
        inet_ntop(AF_INET6, &m_lastBindIPv6.sin6_addr, addrStr, sizeof(addrStr));
        port = ntohs(m_lastBindIPv6.sin6_port);
    }

    return std::string(addrStr) + ":" + std::to_string(port);
}

std::string Socket::getRemotePairStr()
{
    return std::string(m_remotePair);
}

uint16_t Socket::getPort()
{
    if (!isActive()) 
        return 0;

    struct sockaddr_in sin;
    socklen_t len = sizeof(sin);
    if (getsockname(m_sockFD, (struct sockaddr *) &sin, &len) == -1)
    {
        m_lastError = "Error resolving port";
        return 0;
    }
    return ntohs(sin.sin_port);
}

ssize_t Socket::partialRead(void *data, const size_t &datalen)
{
    if (!isActive()) 
        return -1;
    if (!datalen) 
        return 0;
    if (!m_useWriteInsteadRecv)
    {
        ssize_t recvLen = recv(m_sockFD, static_cast<char *>(data), datalen, 0);
        return recvLen;
    }
    else
    {
        ssize_t recvLen = read(m_sockFD, static_cast<char *>(data), datalen);
        return recvLen;
    }
}

ssize_t Socket::partialWrite(const void *data, const size_t &datalen)
{
    if (!isActive()) 
        return -1;
    if (!datalen) 
        return 0;
    if (!m_useWriteInsteadRecv)
    {
#ifdef _WIN32
        ssize_t sendLen = send(sockfd, static_cast<const char *>(data), datalen, 0);
#else
        ssize_t sendLen = send(m_sockFD, static_cast<const char *>(data), datalen, MSG_NOSIGNAL);
#endif
        return sendLen;
    }
    else
    {
        ssize_t sendLen = write(m_sockFD, static_cast<const char *>(data), datalen);
        return sendLen;
    }
}

int Socket::iShutdown(
    int mode)
{
    if (!isActive())
        return -10;

    bool rd_to_shutdown = false;
    bool wr_to_shutdown = false;

    switch (mode)
    {
    case SHUT_WR:
        wr_to_shutdown = true;
        break;
    case SHUT_RD:
        rd_to_shutdown = true;
        break;
    case SHUT_RDWR:
        rd_to_shutdown = true;
        wr_to_shutdown = true;
        break;
    default:
        break;
    }

    // Already shutted down:
    if (m_shutdownProtocolOnRead == true)
        rd_to_shutdown = false;
    if (m_shutdownProtocolOnWrite == true)
        wr_to_shutdown = false;

    if (rd_to_shutdown && wr_to_shutdown)
    {
        int x = _shutdownSocket(SHUT_RDWR);

        m_shutdownProtocolOnRead = true;
        m_shutdownProtocolOnWrite = true;

        return x;
    }
    else if (rd_to_shutdown)
    {
        int x = _shutdownSocket(SHUT_RD);

        m_shutdownProtocolOnRead = true;

        return x;
    }
    else if (wr_to_shutdown)
    {
        int x = _shutdownSocket(SHUT_WR);

        m_shutdownProtocolOnWrite = true;

        return x;
    }
    else
    {
        // Double shutdown?
        //fprintf(stderr,"Double shutdown detected at socket: %i in mode %s @%p\n", sockfd, mode==SHUT_RD?"RD": ( mode==SHUT_WR?"WR":"RDWR") ,this); fflush(stderr);
        //        throw std::runtime_error("Double shutdown on Socket");
        return -1;
    }
}

void Socket::socketSystemInitialization()
{
    if (!m_globalSocketInitialized)
    {
#ifdef _WIN32
        int wsaerr;

        WORD wVersionRequested;
        WSADATA wsaData;

        wVersionRequested = MAKEWORD(2, 2);
        wsaerr = WSAStartup(wVersionRequested, &wsaData);
        if (wsaerr != 0)
        {
            // dll not found.
            badSocket = true;
            return;
        }

        if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2 )
        {
            // not supported.
            WSACleanup();
            badSocket = true;
            return;
        }
#endif
        m_globalSocketInitialized = true;
    }
}


unsigned short Socket::getRemotePort() const
{
    return m_remotePort;
}

void Socket::setRemotePort(unsigned short value)
{
    m_remotePort = value;
}

int Socket::getSocketOption(int level, int optname, void *optval, socklen_t *optlen)
{
#ifdef _WIN32
    return getsockopt(sockfd, level, optname, (char *)optval, optlen);
#else
    return getsockopt(m_sockFD, level, optname, optval, optlen);
#endif
}

int Socket::setSocketOption(int level, int optname, const void *optval, socklen_t optlen)
{
#ifdef _WIN32
    return setsockopt(sockfd,  level, optname, (char *)optval, optlen);
#else
    return setsockopt(m_sockFD,  level, optname, optval, optlen);
#endif
}

int Socket::setSocketOptionBool(int level, int optname, bool value)
{
    int flag = value?1:0;
    return setSocketOption(level,optname,(char *) &flag, sizeof(int));
}

bool Socket::setReadTimeout(unsigned int _timeout)
{
    if (!isActive()) 
        return false;

    m_readTimeout = _timeout;

    if (m_isInListenMode) 
        return true;

#ifdef _WIN32
    DWORD tout = _timeout*1000;
    if ((setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tout, sizeof(DWORD))) == -1)
#else
    struct timeval timeout;
    timeout.tv_sec = _timeout;
    timeout.tv_usec = 0;
    if ((setsockopt(m_sockFD, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout))) == -1)
#endif
    {
        return false;
    }
    return true;
}

bool Socket::setWriteTimeout(unsigned int _timeout)
{
    if (!isActive()) 
        return false;
    m_writeTimeout = _timeout;
    if (m_isInListenMode) 
        return true;
#ifdef _WIN32
    int tout = _timeout*1000;
    if ((setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (char *)&tout, sizeof(int))) == -1)
#else
    struct timeval timeout;
    timeout.tv_sec = _timeout;
    timeout.tv_usec = 0;
    if ((setsockopt(m_sockFD, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout))) == -1)
#endif
    {
        return false;
    }
    return true;
}

bool Socket::isActive() const
{
    return m_sockFD!=-1;
}

void Socket::setSocketFD(int _sockfd)
{
    if (m_sockFD != -1)
    {
        throw std::runtime_error("Assiging a file descriptor to an initialized Socket.");
    }
    m_sockFD = _sockfd;
}

int Socket::adquireSocketFD()
{
    int sockret = m_sockFD;
    m_sockFD = -1;
    return sockret;
}

void Socket::getRemotePair(char * address) const
{
    memset(address,0,INET6_ADDRSTRLEN);
    strncpy(address, m_remotePair, INET6_ADDRSTRLEN-1);
}

void Socket::setRemotePair(const char * address)
{
    SecBACopy(m_remotePair, address);
}

int Socket::shutdownSocket(int mode)
{
    if (!isActive()) 
        return -1;

    int i;
    // Shutdown sub-protocol:
    if ((i=iShutdown(mode))!=0)
    {
        return i;
    }

    return 0;
}

int Socket::_shutdownSocket(int mode)
{
    //printf("Shutting down socket: %i in mode %s @%p\n", sockfd, mode==SHUT_RD?"RD": ( mode==SHUT_WR?"WR":"RDWR") ,this); fflush(stdout);
    int x = shutdown(m_sockFD, mode);

#ifdef WIN32
    if ( mode == SHUT_RDWR )
    {
        closeSocket();
    }
#endif

    return x;
}

bool Socket::setBlockingMode(bool blocking)
{
#ifdef _WIN32
    int iResult;
    unsigned long int iMode = (!blocking)?1:0;
    iResult = ioctlsocket(sockfd, FIONBIO, &iMode);

    if (iResult!=NO_ERROR)
    {
        switch (WSAGetLastError())
        {
        case WSANOTINITIALISED:
            m_lastError = "A successful WSAStartup call must occur before using ioctlsocket.";
            break;
        case WSAENETDOWN:
            m_lastError = "The network subsystem has failed. (ioctlsocket)";
            break;
        case WSAEINPROGRESS:
            m_lastError = "A blocking Windows Sockets 1.1 call is in progress, or the service provider is still processing a callback function. (ioctlsocket)";
            break;
        case WSAENOTSOCK:
            m_lastError = "The socket descriptor is not a socket. (ioctlsocket)";
            break;
        case WSAEFAULT:
            m_lastError = "The argp parameter is not a valid part of the user address space. (ioctlsocket)";
            break;
        default:
            m_lastError = "Uknown error in ioctlsocket.";
            break;
        }
    }

    return (iResult == NO_ERROR);
#else
    long arg;
    // Set to blocking mode again...
    if( (arg = fcntl(m_sockFD, F_GETFL, nullptr)) < 0)
    {
        m_lastError = "Error getting blocking mode... ";
        return false;
    }
    if (blocking)
        arg &= (~O_NONBLOCK);
    else
        arg |= (O_NONBLOCK);
    
    if( fcntl(m_sockFD, F_SETFL, arg) < 0)
    {
        m_lastError = "Error setting blocking...";
        return false;
    }
    return true;
#endif
}
