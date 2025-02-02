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
    void (*onListeningSuccess)(void *context, std::shared_ptr<Sockets::Socket_Stream_Base>) = nullptr;
    /**
     * @brief onListeningFailed Callback to Notify if the socket failed to listen
     */
    void (*onListeningFailed)(void *context, std::shared_ptr<Sockets::Socket_Stream_Base>) = nullptr;
    /**
     * @brief onClientDisconnected Callback to Notify just after one client was disconnected
     */
    void (*onClientDisconnected)(void *context, std::shared_ptr<Sockets::Socket_Stream_Base>, int) = nullptr;
    /**
     * @brief onClientConnected Callback to Notify when a client has been connected
     */
    void (*onClientConnected)(void *context, std::shared_ptr<Sockets::Socket_Stream_Base>) = nullptr;
    /**
     * @brief onClientAcceptTimeoutOccurred Callback to Notify when the acceptor failed to allocate a thread
     */
    void (*onClientAcceptTimeoutOccurred)(void *context, std::shared_ptr<Sockets::Socket_Stream_Base>) = nullptr;
    /**
     * @brief onClientConnectionLimitPerIPReached Callback to Notify that the connection can't accepted due to max connections reached by this IP source.
     */
    void (*onClientConnectionLimitPerIPReached)(void *context, std::shared_ptr<Sockets::Socket_Stream_Base>) = nullptr;
};

}}}

