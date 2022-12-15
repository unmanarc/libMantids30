#ifndef SOCKET_TLSSERVER_CALLBACKS_H
#define SOCKET_TLSSERVER_CALLBACKS_H

#include "socket_streambase.h"
#include "socket_tls_callbacks.h"

class Socket_TLSServer_Callbacks : public Socket_TLS_Callbacks
{
public:
    Socket_TLSServer_Callbacks( void * obj );

    // Callback implementations

    /**
     * @brief CB_TLS_ListeningOK Callback to Notify when the socket started to listen
     */
    void (*CB_TLS_ListeningOK)(void * obj, Mantids::Network::Sockets::Socket_TLS *);
    /**
     * @brief CB_TLS_ListeningFAILED Callback to Notify if the socket failed to listen
     */
    void (*CB_TLS_ListeningFAILED)(void * obj, Mantids::Network::Sockets::Socket_TLS *);
    /**
     * @brief CB_TLS_ClientDisconnected Callback to Notify just after one client was disconnected (including the CN)
     */
    void (*CB_TLS_ClientDisconnected)(void * obj, Mantids::Network::Sockets::Socket_TLS *, const std::string &);
    /**
     * @brief CB_TLS_ClientConnected Callback to Notify when a TLS client has been connected (including the CN)
     */
    void (*CB_TLS_ClientConnected)(void * obj, Mantids::Network::Sockets::Socket_TLS *, const std::string &);
    /**
     * @brief CB_TLS_ClientAuthError Callback to Notify when there is an authentication error during the incomming TLS/TCP-IP Connection
     */
    bool (*CB_TLS_ClientAuthError)(void * obj, Mantids::Network::Sockets::Socket_StreamBase *, const char *, bool);
    /**
     * @brief CB_TLS_ClientAcceptTimedOut Callback to Notify when the acceptor failed to allocate a thread
     */
    void (*CB_TLS_ClientAcceptTimedOut)(void * obj, Mantids::Network::Sockets::Socket_StreamBase *, const char *, bool);
    /**
     * @brief CB_TLS_ClientAcceptReachedMaxConnectionsPerIP Callback to Notify that the connection can't accepted due to max connections reached by this IP source.
     */
    void (*CB_TLS_ClientAcceptReachedMaxConnectionsPerIP)(void * obj, Mantids::Network::Sockets::Socket_StreamBase *, const char *);


};

#endif // SOCKET_TLSSERVER_CALLBACKS_H
