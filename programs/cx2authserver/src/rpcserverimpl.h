#ifndef RPC_H
#define RPC_H

#include <cx2_net_sockets/streamsocket.h>

namespace AUTHSERVER { namespace RPC {

class LoginRPCServerImpl
{
public:
    LoginRPCServerImpl();
    static bool createRPCListener();

private:
    static bool callbackOnRPCConnect(void *, CX2::Network::Streams::StreamSocket *sock, const char *remoteAddr, bool secure);
};

}}

#endif // RPC_H
