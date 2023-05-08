#pragma once

#include "socket_tls.h"

namespace Mantids29 { namespace Network { namespace Sockets {

class Callbacks_Socket_TLS
{
public:
    Callbacks_Socket_TLS(void * obj);

    void (*onTLSKeyInvalidCA)(void * obj, Mantids29::Network::Sockets::Socket_TLS *, const std::string & );
    void (*onTLSKeyInvalidCertificate)(void * obj, Mantids29::Network::Sockets::Socket_TLS *, const std::string & );
    void (*onTLSKeyInvalidPrivateKey)(void * obj, Mantids29::Network::Sockets::Socket_TLS *, const std::string & );

    void * obj;
};

}}}

