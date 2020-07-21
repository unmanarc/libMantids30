#include "socketchainbase.h"

using namespace CX2::Network::Chains;



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

std::pair<CX2::Network::Streams::StreamSocket *, CX2::Network::Streams::StreamSocket *> SocketChainBase::makeSocketChainPair()
{
    std::pair<CX2::Network::Streams::StreamSocket *, CX2::Network::Streams::StreamSocket *> pair = CX2::Network::Streams::StreamSocket::GetSocketPair();
    if (pair.first == nullptr) return pair;

    // Transfer the socket from pairsocket to the chained implementation...
    CX2::Network::Streams::StreamSocket * realSock = (CX2::Network::Streams::StreamSocket *)getThis();
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

