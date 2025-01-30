#pragma once

#include "socket_stream_base.h"

namespace Mantids30 { namespace Network { namespace Sockets {

class Callbacks_Socket_TCP_Server
{
public:
    Callbacks_Socket_TCP_Server( ) {}

    // Callback implementations

    /**
     * @brief onListeningSuccess Callback to Notify when the socket started to listen
     */
    void (*onListeningSuccess)(std::shared_ptr<void> context, std::shared_ptr<Sockets::Socket_Stream_Base>) = nullptr;
    /**
     * @brief onListeningFailed Callback to Notify if the socket failed to listen
     */
    void (*onListeningFailed)(std::shared_ptr<void> context, std::shared_ptr<Sockets::Socket_Stream_Base>) = nullptr;
    /**
     * @brief onClientDisconnected Callback to Notify just after one client was disconnected
     */
    void (*onClientDisconnected)(std::shared_ptr<void> context, std::shared_ptr<Sockets::Socket_Stream_Base>, int) = nullptr;
    /**
     * @brief onClientConnected Callback to Notify when a client has been connected
     */
    void (*onClientConnected)(std::shared_ptr<void> context, std::shared_ptr<Sockets::Socket_Stream_Base>) = nullptr;
    /**
     * @brief onClientAcceptTimeoutOccurred Callback to Notify when the acceptor failed to allocate a thread
     */
    void (*onClientAcceptTimeoutOccurred)(std::shared_ptr<void> context, std::shared_ptr<Sockets::Socket_Stream_Base>, const char *, bool) = nullptr;
    /**
     * @brief onClientConnectionLimitPerIPReached Callback to Notify that the connection can't accepted due to max connections reached by this IP source.
     */
    void (*onClientConnectionLimitPerIPReached)(std::shared_ptr<void> context, std::shared_ptr<Sockets::Socket_Stream_Base>, const char *) = nullptr;
};

}}}

