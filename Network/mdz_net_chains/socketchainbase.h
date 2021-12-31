#ifndef SOCKETCHAINBASE_H
#define SOCKETCHAINBASE_H

#include <utility>
#include <mdz_net_sockets/streamsocket.h>

namespace Mantids { namespace Network { namespace Chains {


class SocketChainBase
{
public:
    SocketChainBase();
    virtual ~SocketChainBase();

    virtual bool isEndPoint();
    std::pair<Mantids::Network::Streams::StreamSocket *, Mantids::Network::Streams::StreamSocket*> makeSocketChainPair();
    bool isServerMode() const;
    void setServerMode(bool value);

protected:
    virtual void * getThis() = 0;

private:
    bool serverMode;
};

}}}

#endif // SOCKETCHAINBASE_H
