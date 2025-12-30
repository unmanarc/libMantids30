#pragma once

#include "socket_chain_protocolbase.h"
#include <Mantids30/Net_Sockets/socket_tls.h>

namespace Mantids30::Network::Sockets {
namespace ChainProtocols {

class Socket_Chain_TLS : public Sockets::Socket_TLS, public Socket_Chain_ProtocolBase
{
public:
    Socket_Chain_TLS() = default;

protected:
    void *getThis() override { return this; }
};

} // namespace ChainProtocols
} // namespace Mantids30::Network::Sockets
