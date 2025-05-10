#include "socket_chain_protocolbase.h"
#include <memory>

using namespace Mantids30::Network;
using namespace Sockets::ChainProtocols;



bool Socket_Chain_ProtocolBase::isEndPoint()
{
    return false;
}

std::pair<std::shared_ptr<Sockets::Socket_Stream> , std::shared_ptr<Sockets::Socket_Stream> > Socket_Chain_ProtocolBase::makeSocketChainPair()
{
    // Build 2 sockets from socketpair <--->
    std::pair<std::shared_ptr<Sockets::Socket_Stream> , std::shared_ptr<Sockets::Socket_Stream> > pair = Sockets::Socket_Stream::GetSocketPair();

    // IF there is an error (nullptr socket in the pair), return the null pair.
    if (pair.first == nullptr)
        return pair;

    // Transfer the socket (sockfd) from the first pairsocket to the chained implementation...
    Sockets::Socket_Stream * realSock = (Sockets::Socket_Stream *)getThis(); // Give me the address of any of the impl that have access to stream base...
    // Set the socket:
    realSock->setSocketFD(pair.first->adquireSocketFD());
    //delete pair.first;
    // first, get the realSock shared_ptr from the parent (streamable object), and then... go to the children (stream base).
    pair.first =  std::dynamic_pointer_cast<Sockets::Socket_Stream>(realSock->shared_from_this());
    // The first from the pair is this class, the second is the connected (pipe/pairsocket)
    return pair;
}

bool Socket_Chain_ProtocolBase::isServerMode() const
{
    return m_serverMode;
}

void Socket_Chain_ProtocolBase::setServerMode(bool value)
{
    m_serverMode = value;
}

