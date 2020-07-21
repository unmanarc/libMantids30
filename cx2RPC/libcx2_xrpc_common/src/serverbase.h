#ifndef SERVERBASE_H
#define SERVERBASE_H

#include "request.h"

namespace CX2 { namespace RPC { namespace XRPC {

class ServerBase
{
public:
    ServerBase();
    virtual ~ServerBase() {}
    virtual bool sendAnswer(Request &response);
};

}}}


#endif // SERVERBASE_H
