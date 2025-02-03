#pragma once

#include "socket_chain_protocolbase.h"

namespace Mantids30 { namespace Network { namespace Sockets { namespace ChainProtocols {


class Socket_Chain_EndPointBase : public Socket_Chain_ProtocolBase
{
public:
    Socket_Chain_EndPointBase() = default;
    virtual ~Socket_Chain_EndPointBase() = default;
    bool isEndPoint();

protected:
    virtual void * getThis() = 0;
};

}}}}

