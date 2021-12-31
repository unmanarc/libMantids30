#include "socketchainendpointbase.h"

using namespace Mantids::Network::Chains;

SocketChainEndPointBase::SocketChainEndPointBase()
{

}

SocketChainEndPointBase::~SocketChainEndPointBase()
{

}

bool SocketChainEndPointBase::isEndPoint()
{
    return true;
}
