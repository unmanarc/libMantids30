#pragma once

#include <Mantids30/Net_Sockets/socket_stream.h>
#include <memory>
#include <utility>

namespace Mantids30::Network::Sockets::ChainProtocols {

class Socket_Chain_ProtocolBase
{
public:
    Socket_Chain_ProtocolBase() = default;
    virtual ~Socket_Chain_ProtocolBase() = default;

    [[nodiscard]] virtual bool isEndPoint();
    [[nodiscard]] std::pair<std::shared_ptr<Mantids30::Network::Sockets::Socket_Stream>, std::shared_ptr<Mantids30::Network::Sockets::Socket_Stream> > makeSocketChainPair();
    [[nodiscard]] bool isServerMode() const;
    void setServerMode(bool value);

protected:
    virtual void *getThis() = 0;

private:
    bool m_serverMode = false;
};

} // namespace Mantids30::Network::Sockets::ChainProtocols
