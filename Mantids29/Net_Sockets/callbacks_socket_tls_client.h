#pragma once

#include "callbacks_socket_tls.h"

namespace Mantids29 { namespace Network { namespace Sockets {

class Callbacks_Socket_TLS_Client : public Callbacks_Socket_TLS
{
public:
    Callbacks_Socket_TLS_Client(void * obj);


    // Callback implementations
    /**
     * @brief onTLSConnectionStart Callback to Notify just before the TLS/TCP-IP Connection
     */
    void (*onTLSConnectionStart)(void * obj, Mantids29::Network::Sockets::Socket_TLS * , const std::string & , const uint16_t &) = nullptr;
    /**
     * @brief onTLSDisconnected Callback to Notify just after the TLS/TCP-IP Connection (with the error code as integer)
     */
    void (*onTLSDisconnected)(void * obj, Mantids29::Network::Sockets::Socket_TLS *, const std::string & , const uint16_t &, int) = nullptr;
    /**
     * @brief onTLSConnectionSuccess Callback to Notify when the TLS/TCP-IP connection is established and we are about to authenticate
     */
    void (*onTLSConnectionSuccess)(void * obj, Mantids29::Network::Sockets::Socket_TLS * ) = nullptr;
    /**
     * @brief onTLSConnectionFailed Callback to Notify when there is an error during the TLS/TCP-IP Connection
     */
    bool (*onTLSConnectionFailed)(void * obj, Mantids29::Network::Sockets::Socket_TLS *, const std::string &, const uint16_t & ) = nullptr;

};
}}}

