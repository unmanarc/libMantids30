#ifndef LOGINAUTH_H
#define LOGINAUTH_H

#include <cx2_auth/manager.h>
#include <cx2_xrpc_fast/fastrpc.h>
#include <json/json.h>

namespace CX2 { namespace RPC { namespace Templates {

// This template is for FastRPC
class LoginAuth
{
public:
    static void AddLoginAuthMethods(CX2::Authentication::Manager * auth, CX2::RPC::Fast::FastRPC * fastRPC);

private:
    static Json::Value authenticate(void * obj, const Json::Value & payload);
    static Json::Value accountChangeSecret(void * obj,  const Json::Value & payload);
    static Json::Value accountAdd(void * obj,  const Json::Value & payload);
    static Json::Value attribExist(void * obj,  const Json::Value & payload);
};

}}}
#endif // LOGINAUTH_H
