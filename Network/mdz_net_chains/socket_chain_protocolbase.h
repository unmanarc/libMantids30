#ifndef SOCKETCHAINBASE_H
#define SOCKETCHAINBASE_H

#include <utility>
#include <mdz_net_sockets/socket_streambase.h>

namespace Mantids { namespace Network { namespace Sockets { namespace ChainProtocols {


class Socket_Chain_ProtocolBase
{
public:
    Socket_Chain_ProtocolBase();
    virtual ~Socket_Chain_ProtocolBase();

    virtual bool isEndPoint();
    std::pair<Mantids::Network::Sockets::Socket_StreamBase *, Mantids::Network::Sockets::Socket_StreamBase*> makeSocketChainPair();
    bool isServerMode() const;
    void setServerMode(bool value);

protected:
    virtual void * getThis() = 0;

private:
    bool serverMode;
};

}}}}

#endif // SOCKETCHAINBASE_H
