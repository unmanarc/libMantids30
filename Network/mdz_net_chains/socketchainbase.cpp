#include "socketchainbase.h"

using namespace Mantids::Network::Chains;



SocketChainBase::SocketChainBase()
{
    serverMode = false;
}

SocketChainBase::~SocketChainBase()
{

}

bool SocketChainBase::isEndPoint()
{
    return false;
}

std::pair<Mantids::Network::Streams::StreamSocket *, Mantids::Network::Streams::StreamSocket *> SocketChainBase::makeSocketChainPair()
{
    // Build 2 sockets from socketpair <--->
    std::pair<Mantids::Network::Streams::StreamSocket *, Mantids::Network::Streams::StreamSocket *> pair = Mantids::Network::Streams::StreamSocket::GetSocketPair();

    // IF there is an error (nullptr socket in the pair), return the null pair.
    if (pair.first == nullptr)
        return pair;

    // Transfer the socket (sockfd) from the first pairsocket to the chained implementation...
    Mantids::Network::Streams::StreamSocket * realSock = (Mantids::Network::Streams::StreamSocket *)getThis();
    realSock->setSocketFD(pair.first->adquireSocketFD());
    delete pair.first;
    pair.first = realSock;

    // The first from the pair is this class, the second is the connected (pipe/pairsocket)
    return pair;
}

bool SocketChainBase::isServerMode() const
{
    return serverMode;
}

void SocketChainBase::setServerMode(bool value)
{
    serverMode = value;
}

