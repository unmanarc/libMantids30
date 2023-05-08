#pragma once

#include <utility>
#include <Mantids29/Net_Sockets/socket_stream_base.h>

namespace Mantids29 { namespace Network { namespace Sockets { namespace ChainProtocols {


class Socket_Chain_ProtocolBase
{
public:
    Socket_Chain_ProtocolBase();
    virtual ~Socket_Chain_ProtocolBase();

    virtual bool isEndPoint();
    std::pair<Mantids29::Network::Sockets::Socket_Stream_Base *, Mantids29::Network::Sockets::Socket_Stream_Base*> makeSocketChainPair();
    bool isServerMode() const;
    void setServerMode(bool value);

protected:
    virtual void * getThis() = 0;

private:
    bool serverMode;
};

}}}}

