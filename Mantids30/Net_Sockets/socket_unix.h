#pragma once

#ifndef _WIN32
#include "socket_stream.h"

namespace Mantids30::Network::Sockets {

/**
 * @brief Socket_UNIX Class for handling UNIX domain sockets.
 *
 * This class provides methods to listen on and connect using UNIX domain sockets.
 */
class  Socket_UNIX : public Sockets::Socket_Stream {
public:
    /**
     * @brief Constructor for the Socket_UNIX class.
     */
    Socket_UNIX() = default;

    /**
     * @brief Listen on a specific UNIX socket path.
     *
     * This method sets up the UNIX socket to listen for incoming connections.
     * 
     * @param path The path of the UNIX socket file.
     * @param recvbuffer Size in bytes of the receive buffer.
     * @param backlog Number of unaccepted connections that the system will allow before refusing new connections.
     * @return True if the socket is successfully set to listen, false otherwise.
     */
    bool listenOn(const char* path, const int32_t& recvbuffer = 0, const int32_t& backlog = 10);
    
    /**
     * @brief Overridden method for listening on a specific UNIX socket path and port.
     *
     * This method is overridden from the base class but does not apply to UNIX domain sockets, which do not use ports.
     * The method still takes a port parameter for compatibility reasons but it will be ignored.
     * 
     * @param port Port number (unused in UNIX domain sockets).
     * @param path The path of the UNIX socket file.
     * @param recvbuffer Size in bytes of the receive buffer.
     * @param backlog Number of unaccepted connections that the system will allow before refusing new connections.
     * @return True if the socket is successfully set to listen, false otherwise.
     */
    bool listenOn(const uint16_t& port, const char* path, const int32_t& recvbuffer = 0, const int32_t& backlog = 10) override;

    /**
     * @brief Connect to a remote UNIX socket.
     *
     * This method attempts to connect to the specified UNIX socket file.
     * 
     * @param localPath Local path of the UNIX socket (unused in client connections).
     * @param remotePath Path of the remote UNIX socket to which the connection is attempted.
     * @param port Port number (unused in UNIX domain sockets).
     * @param timeout Timeout in seconds for the connection attempt.
     * @return True if the connection is successful, false otherwise.
     */
    bool connectFrom(const char* localPath, const char* remotePath, const uint16_t& port, const uint32_t& timeout = 30) override;

    /**
     * @brief Accept an incoming connection on a listening UNIX socket.
     *
     * This method accepts the next incoming connection on a listening UNIX socket and returns a new Socket_UNIX object
     * representing the accepted connection.
     * 
     * @return A shared pointer to a new Socket_UNIX object if a connection is successfully accepted, or nullptr if an error occurs.
     */
    std::shared_ptr<Socket_Stream> acceptConnection() override;
};

/**
 * @brief Shared pointer type for Socket_UNIX instances.
 *
 * This typedef simplifies the usage of shared pointers with Socket_UNIX objects.
 */
typedef std::shared_ptr<Socket_UNIX> Socket_UNIX_SP;

}  // namespace Mantids30::Network::Sockets
#endif
