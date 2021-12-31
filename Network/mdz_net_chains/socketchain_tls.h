#ifndef SOCKETCHAIN_TLS_H
#define SOCKETCHAIN_TLS_H

#include <mdz_net_sockets/socket_tls.h>
#include "socketchainbase.h"

namespace Mantids { namespace Network { namespace Chains { namespace Protocols {

class SocketChain_TLS : public TLS::Socket_TLS, public SocketChainBase
{
public:
    SocketChain_TLS();

protected:
    void * getThis() override { return this; }

};

}}}}

#endif // SOCKETCHAIN_TLS_H
