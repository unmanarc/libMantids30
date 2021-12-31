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
    std::pair<Mantids::Network::Streams::StreamSocket *, Mantids::Network::Streams::StreamSocket *> pair = Mantids::Network::Streams::StreamSocket::GetSocketPair();
    if (pair.first == nullptr) return pair;

    // Transfer the socket from pairsocket to the chained implementation...
    Mantids::Network::Streams::StreamSocket * realSock = (Mantids::Network::Streams::StreamSocket *)getThis();
    realSock->setSocketFD(pair.first->adquireSocketFD());
    delete pair.first;
    pair.first = realSock;
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

