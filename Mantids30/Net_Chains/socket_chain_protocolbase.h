#pragma once

#include <memory>
#include <utility>
#include <Mantids30/Net_Sockets/socket_stream.h>

namespace Mantids30 { namespace Network { namespace Sockets { namespace ChainProtocols {


class Socket_Chain_ProtocolBase
{
public:
    Socket_Chain_ProtocolBase() = default;
    virtual ~Socket_Chain_ProtocolBase() = default;

    virtual bool isEndPoint();
    std::pair<std::shared_ptr<Mantids30::Network::Sockets::Socket_Stream> , std::shared_ptr<Mantids30::Network::Sockets::Socket_Stream> > makeSocketChainPair();
    bool isServerMode() const;
    void setServerMode(bool value);

protected:
    virtual void * getThis() = 0;

private:
    bool m_serverMode = false;
};

}}}}

