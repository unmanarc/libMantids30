#include "socket_unix.h"
#include <memory>
#include <system_error>

#ifndef _WIN32

#include <cstring>
#include <sys/types.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <chrono>
#include <poll.h>
#include <algorithm>
#include <climits>

#include <Mantids30/Core/Helpers/mem.h>

using namespace Mantids30::Network;
using namespace Mantids30::Network::Sockets;

bool Socket_UNIX::listenOn(const uint16_t &, const char *path, const int32_t &recvbuffer, const int32_t &backlog)
{
    if (isActive())
    {
        closeSocket(); // close first
    }

    // use the addr as path.
    sockaddr_un server_address{};
    int server_len = sizeof(server_address);

    if (!path || strlen(path) >= sizeof(server_address.sun_path))
    {
        m_lastError = "Invalid UNIX socket path";
        return false;
    }
    struct stat existing{};
    if (lstat(path, &existing) == 0)
    {
        if (!S_ISSOCK(existing.st_mode) || unlink(path) != 0)
        {
            m_lastError = "UNIX socket path already exists and cannot be replaced";
            return false;
        }
    }

    setSocketFD(socket(AF_UNIX, SOCK_STREAM, 0));
    if (!isActive())
    {
        m_lastError = "socket() failed";
        return false;
    }

    if (recvbuffer)
    {
        setRecvBuffer(recvbuffer);
    }

    server_address.sun_family = AF_UNIX;

    SecBACopy(server_address.sun_path, path);

    if (::bind(m_sockFD, reinterpret_cast<struct sockaddr *>(&server_address), server_len) < 0)
    {
        m_lastError = "bind() failed";
        closeSocket();
        return false;
    }
    if (listen(m_sockFD, backlog) < 0)
    {
        m_lastError = "bind() failed";
        closeSocket();
        return false;
    }

    m_isInListenMode = true;
    return true;
}

bool Socket_UNIX::listenOn(const char *path, const int32_t &recvbuffer, const int32_t &backlog)
{
    return listenOn(0, path, recvbuffer, backlog);
}

bool Socket_UNIX::connectFrom(const char *, const char *path, const uint16_t &, const uint32_t &timeout)
{
    if (isActive())
    {
        closeSocket(); // close first
    }

    int len;
    sockaddr_un address{};

    if (!path || strlen(path) >= sizeof(address.sun_path))
    {
        m_lastError = "Invalid UNIX socket path";
        return false;
    }
    m_remoteServerHostname = path;

    setSocketFD(socket(AF_UNIX, SOCK_STREAM, 0));
    if (!isActive())
    {
        m_lastError = "socket() failed";
        return false;
    }

    address.sun_family = AF_UNIX;
    strncpy(address.sun_path, path, sizeof(address.sun_path));
    len = sizeof(address);

    setReadTimeout(timeout);
    if (!setBlockingMode(false))
    {
        closeSocket();
        return false;
    }
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(timeout);
    int result;
    // Unlike TCP, Linux AF_UNIX reports EAGAIN when the listen backlog is full;
    // that means retry connect(), rather than wait for a pending connection.
    for (;;)
    {
        result = connect(m_sockFD, reinterpret_cast<sockaddr *>(&address), len);
        if (result == 0 || (errno != EAGAIN && errno != EINTR))
        {
            break;
        }
        if (timeout && std::chrono::steady_clock::now() >= deadline)
        {
            m_lastError = "UNIX connection timed out";
            closeSocket();
            return false;
        }
        poll(nullptr, 0, 1);
    }
    if (result < 0 && errno == EINPROGRESS)
    {
        int ready;
        do
        {
            const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(deadline - std::chrono::steady_clock::now()).count();
            const int waitMS = timeout ? static_cast<int>(std::min<int64_t>(std::max<int64_t>(remaining, 0), INT_MAX)) : -1;
            pollfd descriptor{m_sockFD, POLLOUT, 0};
            ready = poll(&descriptor, 1, waitMS);
        } while (ready < 0 && errno == EINTR && (timeout == 0 || std::chrono::steady_clock::now() < deadline));
        if (ready <= 0)
        {
            if (ready == 0)
            {
                m_lastError = "UNIX connection timed out: " + std::string(path);
            }
            else
            {
                const int error = errno;
                m_lastError = "poll(AF_UNIX) failed with error #" + std::to_string(error) + ": " + std::system_category().message(error);
            }
            closeSocket();
            return false;
        }
        int connectionError = 0;
        socklen_t length = sizeof(connectionError);
        if (getSocketOption(SOL_SOCKET, SO_ERROR, &connectionError, &length) < 0)
        {
            const int error = errno;
            m_lastError = "Error in getsockopt(SOL_SOCKET, SO_ERROR): #" + std::to_string(error) + ": " + std::system_category().message(error);
            closeSocket();
            return false;
        }
        if (connectionError != 0)
        {
            m_lastError = "Connection to AF_UNIX Socket " + std::string(path) + " failed with error #" + std::to_string(connectionError) + ": " + std::system_category().message(connectionError);
            closeSocket();
            return false;
        }
        result = 0;
    }
    if (result < 0)
    {
        const int error = errno;
        m_lastError = "Connection to AF_UNIX Socket " + std::string(path) + " failed with error #" + std::to_string(error) + ": " + std::system_category().message(error);
        closeSocket();
        return false;
    }
    if (!setBlockingMode(true))
    {
        closeSocket();
        return false;
    }

    return true;
}

std::shared_ptr<Mantids30::Network::Sockets::Socket_Stream> Socket_UNIX::acceptConnection()
{
    int sdconn;
    std::shared_ptr<Mantids30::Network::Sockets::Socket_Stream> cursocket = nullptr;

    do
    {
        sdconn = accept(m_sockFD, nullptr, nullptr);
    } while (sdconn < 0 && errno == EINTR);
    if (sdconn >= 0)
    {
        cursocket = std::make_shared<Socket_UNIX>();
        // Set the proper socket-
        cursocket->setSocketFD(sdconn);
        cursocket->setReadTimeout(m_readTimeout);
        cursocket->setWriteTimeout(m_writeTimeout);
    }
    else
    {
        // Establish the error.
        m_lastError = "accept() failed";
    }

    // return the socket class.
    return cursocket;
}

#endif
