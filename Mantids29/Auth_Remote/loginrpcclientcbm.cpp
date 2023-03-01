#include "loginrpcclientcbm.h"
#include <Mantids29/Helpers/callbacks.h>

Mantids29::Authentication::LoginRPCClientCBM::LoginRPCClientCBM(void *obj) : Callbacks_Socket_TLS_Client(obj)
{
    onAPIAuthenticated = nullptr;
    onAPIAuthenticationFailure = nullptr;
    this->obj = obj;
}

void Mantids29::Authentication::LoginRPCClientCBM::notifyTLSConnecting(Network::Sockets::Socket_TLS *a, const std::string &b, const uint16_t &c)
{
    CALLBACK(onTLSConnectionStart)(obj,a,b,c);
}

void Mantids29::Authentication::LoginRPCClientCBM::notifyTLSDisconnected(Network::Sockets::Socket_TLS *a, const std::string &b, const uint16_t &c, int d)
{
    CALLBACK(onTLSDisconnected)(obj,a,b,c,d);
}

void Mantids29::Authentication::LoginRPCClientCBM::notifyAPIProcessingOK(Network::Sockets::Socket_TLS *a)
{
    CALLBACK(onAPIAuthenticated)(obj,a);
}

void Mantids29::Authentication::LoginRPCClientCBM::notifyTLSConnectedOK(Network::Sockets::Socket_TLS *a)
{
    CALLBACK(onTLSConnectionSuccess)(obj,a);
}

void Mantids29::Authentication::LoginRPCClientCBM::notifyBadApiKey(Network::Sockets::Socket_TLS *a)
{
    CALLBACK(onAPIAuthenticationFailure)(obj,a);
}

void Mantids29::Authentication::LoginRPCClientCBM::notifyTLSErrorConnecting(Network::Sockets::Socket_TLS *a, const std::string &b, const uint16_t &c)
{
    CALLBACK(onTLSConnectionFailed)(obj,a,b,c);
}
