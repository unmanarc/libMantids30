#ifndef SOCKETCHAINENDPOINTBASE_H
#define SOCKETCHAINENDPOINTBASE_H

#include "socketchainbase.h"

namespace Mantids { namespace Network { namespace Chains {


class SocketChainEndPointBase : public SocketChainBase
{
public:
    SocketChainEndPointBase();
    virtual ~SocketChainEndPointBase();
    bool isEndPoint();

protected:
    virtual void * getThis() = 0;
};

}}}

#endif // SOCKETCHAINENDPOINTBASE_H
