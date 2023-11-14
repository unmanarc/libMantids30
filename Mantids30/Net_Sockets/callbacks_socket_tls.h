#pragma once

#include "socket_tls.h"

namespace Mantids30 { namespace Network { namespace Sockets {

class Callbacks_Socket_TLS
{
public:
    Callbacks_Socket_TLS(void * obj);

    void (*onTLSKeyInvalidCA)(void * obj, Mantids30::Network::Sockets::Socket_TLS *, const std::string & ) = nullptr;
    void (*onTLSKeyInvalidCertificate)(void * obj, Mantids30::Network::Sockets::Socket_TLS *, const std::string & ) = nullptr;
    void (*onTLSKeyInvalidPrivateKey)(void * obj, Mantids30::Network::Sockets::Socket_TLS *, const std::string & ) = nullptr;

    void * obj;
};

}}}

