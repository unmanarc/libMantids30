#include "callbacks_socket_tls.h"

using namespace Mantids29::Network::Sockets;

Callbacks_Socket_TLS::Callbacks_Socket_TLS(void *obj)
{
    this->obj = obj;

    onTLSKeyInvalidCA = nullptr;
    onTLSKeyInvalidCertificate = nullptr;
    onTLSKeyInvalidPrivateKey = nullptr;
}
