#pragma once

#include "socket_stream_base.h"
#include "callbacks_socket_tls.h"

namespace Mantids30 { namespace Network { namespace Sockets {

class Callbacks_Socket_TLS_Server : public Callbacks_Socket_TLS
{
public:
    Callbacks_Socket_TLS_Server( ) {}


    // Server Callback implementations
    /**
     * @brief onProtocolInitializationFailure Callback to Notify when there is an authentication error during the incoming TLS/TCP-IP Connection
     */
    bool (*onProtocolInitializationFailure)(void *context, std::shared_ptr<Sockets::Socket_Stream_Base>) = nullptr;
};

}}}

