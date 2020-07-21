#ifndef SOCKETCHAIN_TLS_H
#define SOCKETCHAIN_TLS_H

#include <cx2_net_tls/socket_tls.h>
#include <cx2_net_chains/socketchainbase.h>

namespace CX2 { namespace Network { namespace Chains { namespace Protocols {

class SocketChain_TLS : public TLS::Socket_TLS, public SocketChainBase
{
public:
    SocketChain_TLS();

protected:
    void * getThis() override { return this; }

};

}}}}

#endif // SOCKETCHAIN_TLS_H
