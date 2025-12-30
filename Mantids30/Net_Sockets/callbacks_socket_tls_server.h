#pragma once

#include "socket_stream.h"
#include "callbacks_socket_tls.h"

namespace Mantids30::Network::Sockets {

class Callbacks_Socket_TLS_Server : public Callbacks_Socket_TLS
{
public:
    Callbacks_Socket_TLS_Server( ) = default;


    // Server Callback implementations
    /**
     * @brief onProtocolInitializationFailure Callback to Notify when there is an authentication error during the incoming TLS/TCP-IP Connection
     */
    void (*onProtocolInitializationFailure)(void *context, std::shared_ptr<Sockets::Socket_Stream>) = nullptr;
};

}

