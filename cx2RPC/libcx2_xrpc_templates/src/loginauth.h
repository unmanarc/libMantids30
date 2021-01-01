#ifndef LOGINAUTH_H
#define LOGINAUTH_H

#include <cx2_xrpc_common/methodsmanager.h>
#include <cx2_auth/domains.h>
#include <json/json.h>

namespace CX2 { namespace RPC { namespace Templates {

class LoginAuth
{
public:
    static void AddLoginAuthMethods(MethodsManager *methods);

private:
    static Json::Value authenticate(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const Json::Value & payload);
    static Json::Value accountChangeSecret(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const Json::Value & payload);
    static Json::Value accountAdd(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const Json::Value & payload);

};

}}}
#endif // LOGINAUTH_H
