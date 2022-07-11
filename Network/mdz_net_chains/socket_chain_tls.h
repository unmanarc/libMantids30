#ifndef SOCKETCHAIN_TLS_H
#define SOCKETCHAIN_TLS_H

#include <mdz_net_sockets/socket_tls.h>
#include "socket_chain_protocolbase.h"

namespace Mantids { namespace Network { namespace Sockets { namespace ChainProtocols {

class Socket_Chain_TLS : public Sockets::Socket_TLS, public Socket_Chain_ProtocolBase
{
public:
    Socket_Chain_TLS();

protected:
    void * getThis() override { return this; }

};

}}}}

#endif // SOCKETCHAIN_TLS_H
