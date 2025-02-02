#pragma once

#include "socket_tls.h"
#include <memory>

namespace Mantids30 { namespace Network { namespace Sockets {

class Callbacks_Socket_TLS
{
public:
    Callbacks_Socket_TLS() {}

    // Generic TLS Callbacks:

    void (*onInvalidCACertificate)(void *context, std::shared_ptr<Mantids30::Network::Sockets::Socket_TLS>, const std::string & ) = nullptr;
    void (*onInvalidClientCertificate)(void *context,std::shared_ptr<Mantids30::Network::Sockets::Socket_TLS>, const std::string & ) = nullptr;
    void (*onInvalidPrivateKey)(void *context, std::shared_ptr<Mantids30::Network::Sockets::Socket_TLS>, const std::string & ) = nullptr;

};

}}}

