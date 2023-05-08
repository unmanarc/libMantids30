#pragma once

#include "socket_stream_base.h"
#include "callbacks_socket_tls.h"

namespace Mantids29 { namespace Network { namespace Sockets {

class Callbacks_Socket_TLS_Server : public Callbacks_Socket_TLS
{
public:
    Callbacks_Socket_TLS_Server( void * obj );

    // Callback implementations

    /**
     * @brief onTLSListeningSuccess Callback to Notify when the socket started to listen
     */
    void (*onTLSListeningSuccess)(void * obj, Mantids29::Network::Sockets::Socket_TLS *);
    /**
     * @brief onTLSListeningFailed Callback to Notify if the socket failed to listen
     */
    void (*onTLSListeningFailed)(void * obj, Mantids29::Network::Sockets::Socket_TLS *);
    /**
     * @brief onTLSClientDisconnected Callback to Notify just after one client was disconnected (including the CN)
     */
    void (*onTLSClientDisconnected)(void * obj, Mantids29::Network::Sockets::Socket_TLS *, const std::string &, int);
    /**
     * @brief onTLSClientConnected Callback to Notify when a TLS client has been connected (including the CN)
     */
    void (*onTLSClientConnected)(void * obj, Mantids29::Network::Sockets::Socket_TLS *, const std::string &);
    /**
     * @brief onTLSClientAuthenticationError Callback to Notify when there is an authentication error during the incomming TLS/TCP-IP Connection
     */
    bool (*onTLSClientAuthenticationError)(void * obj, Mantids29::Network::Sockets::Socket_Stream_Base *, const char *, bool);
    /**
     * @brief onTLSClientAcceptTimeoutOccurred Callback to Notify when the acceptor failed to allocate a thread
     */
    void (*onTLSClientAcceptTimeoutOccurred)(void * obj, Mantids29::Network::Sockets::Socket_Stream_Base *, const char *, bool);
    /**
     * @brief onTLSClientConnectionLimitPerIPReached Callback to Notify that the connection can't accepted due to max connections reached by this IP source.
     */
    void (*onTLSClientConnectionLimitPerIPReached)(void * obj, Mantids29::Network::Sockets::Socket_Stream_Base *, const char *);
};

}}}

