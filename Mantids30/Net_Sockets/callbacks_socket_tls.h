#pragma once

#include "socket_tls.h"
#include <memory>

namespace Mantids30 { namespace Network { namespace Sockets {

class Callbacks_Socket_TLS
{
public:
    Callbacks_Socket_TLS() {}

    // Generic TLS Callbacks:

    void (*onInvalidCACertificate)(std::shared_ptr<void> context, std::shared_ptr<Mantids30::Network::Sockets::Socket_TLS>, const std::string & ) = nullptr;
    void (*onInvalidClientCertificate)(std::shared_ptr<void> context,std::shared_ptr<Mantids30::Network::Sockets::Socket_TLS>, const std::string & ) = nullptr;
    void (*onInvalidPrivateKey)(std::shared_ptr<void> context, std::shared_ptr<Mantids30::Network::Sockets::Socket_TLS>, const std::string & ) = nullptr;

};

}}}

