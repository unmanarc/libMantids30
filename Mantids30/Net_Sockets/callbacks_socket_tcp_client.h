#pragma once


#include "socket_stream_base.h"

namespace Mantids30 { namespace Network { namespace Sockets {

class Callbacks_Socket_TCP_Client
{
public:
    Callbacks_Socket_TCP_Client() {}

    // Callback implementations
    /**
        * @brief onMaxRetryLimitReached Callback to notify when the maximum number of TLS/TCP-IP connection retries has been reached.
        */
    void (*onMaxRetryLimitReached)(std::shared_ptr<void> context, std::shared_ptr<Sockets::Socket_Stream_Base> , const std::string & , const uint16_t &) = nullptr;
    /**
     * @brief onPreConnectionAttempt Callback to Notify just before the TCP/TCP-IP Connection
     */
    void (*onPreConnectionAttempt)(std::shared_ptr<void> context, std::shared_ptr<Sockets::Socket_Stream_Base> , const std::string & , const uint16_t &) = nullptr;
    /**
     * @brief onConnectionTerminated Callback to Notify just after the TCP/TCP-IP Connection (with the error code as integer)
     */
    void (*onConnectionTerminated)(std::shared_ptr<void> context, std::shared_ptr<Sockets::Socket_Stream_Base>, const std::string & , const uint16_t &, int) = nullptr;
    /**
     * @brief onConnectionEstablished Callback to Notify when the TCP/TCP-IP connection is established and we are about to authenticate
     */
    void (*onConnectionEstablished)(std::shared_ptr<void> context, std::shared_ptr<Sockets::Socket_Stream_Base> ) = nullptr;
    /**
     * @brief onConnectionFailure Callback to Notify when there is an error during the TCP/TCP-IP Connection
     */
    bool (*onConnectionFailure)(std::shared_ptr<void> context, std::shared_ptr<Sockets::Socket_Stream_Base>, const std::string &, const uint16_t & ) = nullptr;

};
}}}

