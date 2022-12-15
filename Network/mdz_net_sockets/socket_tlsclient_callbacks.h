#ifndef SOCKET_TLSCLIENT_CALLBACKS_H
#define SOCKET_TLSCLIENT_CALLBACKS_H

#include "socket_tls_callbacks.h"

class Socket_TLSClient_Callbacks : public Socket_TLS_Callbacks
{
public:
    Socket_TLSClient_Callbacks(void * obj);


    // Callback implementations
    /**
     * @brief notifyTLSConnectingCB Callback to Notify just before the TLS/TCP-IP Connection
     */
    void (*CB_TLS_Connecting)(void * obj, Mantids::Network::Sockets::Socket_TLS * , const std::string & , const uint16_t &);
    /**
     * @brief notifyTLSDisconnectedCB Callback to Notify just after the TLS/TCP-IP Connection (with the error code as integer)
     */
    void (*CB_TLS_Disconnected)(void * obj, Mantids::Network::Sockets::Socket_TLS *, int);
    /**
     * @brief notifyTLSConnectedOKCB Callback to Notify when the TLS/TCP-IP connection is established and we are about to authenticate
     */
    void (*CB_TLS_ConnectedOK)(void * obj, Mantids::Network::Sockets::Socket_TLS * );
    /**
     * @brief notifyTLSErrorConnectingCB Callback to Notify when there is an error during the TLS/TCP-IP Connection
     */
    void (*CB_TLS_ErrorConnecting)(void * obj, Mantids::Network::Sockets::Socket_TLS *, const std::string &, const uint16_t & );

};

#endif // SOCKET_TLSCLIENT_CALLBACKS_H
