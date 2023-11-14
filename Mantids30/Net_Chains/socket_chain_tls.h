#pragma once

#include <Mantids30/Net_Sockets/socket_tls.h>
#include "socket_chain_protocolbase.h"

namespace Mantids30 { namespace Network { namespace Sockets { namespace ChainProtocols {

class Socket_Chain_TLS : public Sockets::Socket_TLS, public Socket_Chain_ProtocolBase
{
public:
    Socket_Chain_TLS();

protected:
    void * getThis() override { return this; }

};

}}}}

