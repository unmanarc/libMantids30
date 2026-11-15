#pragma once

#include "socket_chain_protocolbase.h"

namespace Mantids30::Network::Sockets::ChainProtocols {

class Socket_Chain_EndPointBase : public Socket_Chain_ProtocolBase
{
public:
    Socket_Chain_EndPointBase() = default;
    ~Socket_Chain_EndPointBase() override = default;
    bool isEndPoint() override;

protected:
    void *getThis() override = 0;
};

} // namespace Mantids30::Network::Sockets::ChainProtocols
