#include "socket_chain_protocolbase.h"

using namespace Mantids::Network;
using namespace Sockets::ChainProtocols;

Socket_Chain_ProtocolBase::Socket_Chain_ProtocolBase()
{
    serverMode = false;
}

Socket_Chain_ProtocolBase::~Socket_Chain_ProtocolBase()
{

}

bool Socket_Chain_ProtocolBase::isEndPoint()
{
    return false;
}

std::pair<Sockets::Socket_StreamBase *, Sockets::Socket_StreamBase *> Socket_Chain_ProtocolBase::makeSocketChainPair()
{
    // Build 2 sockets from socketpair <--->
    std::pair<Sockets::Socket_StreamBase *, Sockets::Socket_StreamBase *> pair = Sockets::Socket_StreamBase::GetSocketPair();

    // IF there is an error (nullptr socket in the pair), return the null pair.
    if (pair.first == nullptr)
        return pair;

    // Transfer the socket (sockfd) from the first pairsocket to the chained implementation...
    Sockets::Socket_StreamBase * realSock = (Sockets::Socket_StreamBase *)getThis();
    realSock->setSocketFD(pair.first->adquireSocketFD());
    delete pair.first;
    pair.first = realSock;

    // The first from the pair is this class, the second is the connected (pipe/pairsocket)
    return pair;
}

bool Socket_Chain_ProtocolBase::isServerMode() const
{
    return serverMode;
}

void Socket_Chain_ProtocolBase::setServerMode(bool value)
{
    serverMode = value;
}

