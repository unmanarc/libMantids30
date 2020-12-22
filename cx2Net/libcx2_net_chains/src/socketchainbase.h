#ifndef SOCKETCHAINBASE_H
#define SOCKETCHAINBASE_H

#include <utility>
#include <cx2_net_sockets/streamsocket.h>

namespace CX2 { namespace Network { namespace Chains {


class SocketChainBase
{
public:
    SocketChainBase();
    virtual ~SocketChainBase();

    virtual bool isEndPoint();
    std::pair<CX2::Network::Streams::StreamSocket *, CX2::Network::Streams::StreamSocket*> makeSocketChainPair();
    bool isServerMode() const;
    void setServerMode(bool value);

protected:
    virtual void * getThis() = 0;

private:
    bool serverMode;
};

}}}

#endif // SOCKETCHAINBASE_H
