#pragma once

#include "socket_chain_protocolbase.h"
#include <Mantids30/Network/Sockets/socket_tls.h>

namespace Mantids30::Network::Chains {

class Socket_Chain_TLS : public Mantids30::Network::Sockets::Socket_TLS, public Socket_Chain_ProtocolBase
{
public:
    Socket_Chain_TLS() = default;

protected:
    void *getThis() override { return this; }
};

} // namespace Mantids30::Network::Chains
