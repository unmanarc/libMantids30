#ifndef XRPC_FUNCTIONS_AUTH_H
#define XRPC_FUNCTIONS_AUTH_H

#include <cx2_xrpc_common/methodsmanager.h>
#include <json/json.h>

namespace CX2 { namespace RPC { namespace Templates {

class FullAuth {
public:
    static void AddFullAuthMethods(MethodsManager *methods);
private:
    static Json::Value accountChangeSecret(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const Json::Value & payload);
    static Json::Value accountRemove(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const Json::Value & payload);
    static Json::Value accountDisable(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const Json::Value & payload);
    static Json::Value accountConfirm(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const Json::Value & payload);
    static Json::Value accountChangeDescription(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const Json::Value & payload);
    static Json::Value accountChangeEmail(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const Json::Value & payload);
    static Json::Value accountChangeExtraData(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const Json::Value & payload);
    static Json::Value accountChangeExpiration(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const Json::Value & payload);
    static Json::Value isAccountDisabled(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const Json::Value & payload);
    static Json::Value isAccountConfirmed(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const Json::Value & payload);
    static Json::Value isAccountSuperUser(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const Json::Value & payload);
    static Json::Value accountDescription(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const Json::Value & payload);
    static Json::Value accountEmail(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const Json::Value & payload);
    static Json::Value accountExtraData(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const Json::Value & payload);
    static Json::Value accountExpirationDate(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const Json::Value & payload);
    static Json::Value isAccountExpired(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const Json::Value & payload);
    static Json::Value accountValidateAttribute(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const Json::Value & payload);
    static Json::Value accountsList(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const Json::Value & payload);
    static Json::Value accountGroups(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const Json::Value & payload);
    static Json::Value accountDirectAttribs(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const Json::Value & payload);
    static Json::Value accountUsableAttribs(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const Json::Value & payload);
    static Json::Value attribAdd(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const Json::Value & payload);
    static Json::Value attribRemove(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const Json::Value & payload);
    static Json::Value attribGroupAdd(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const Json::Value & payload);
    static Json::Value attribGroupRemove(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const Json::Value & payload);
    static Json::Value attribAccountAdd(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const Json::Value & payload);
    static Json::Value attribAccountRemove(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const Json::Value & payload);
    static Json::Value attribChangeDescription(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const Json::Value & payload);
    static Json::Value attribsList(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const Json::Value & payload);
    static Json::Value attribGroups(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const Json::Value & payload);
    static Json::Value attribAccounts(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const Json::Value & payload);
    static Json::Value groupAdd(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const Json::Value & payload);
    static Json::Value groupRemove(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const Json::Value & payload);
    static Json::Value groupExist(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const Json::Value & payload);
    static Json::Value groupAccountAdd(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const Json::Value & payload);
    static Json::Value groupAccountRemove(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const Json::Value & payload);
    static Json::Value groupChangeDescription(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const Json::Value & payload);
    static Json::Value groupValidateAttribute(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const Json::Value & payload);
    static Json::Value groupDescription(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const Json::Value & payload);
    static Json::Value groupsList(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const Json::Value & payload);
    static Json::Value groupAttribs(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const Json::Value & payload);
    static Json::Value groupAccounts(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const Json::Value & payload);

    static std::map<std::string,std::string> jsonToMap(const Json::Value & jValue);

    static Json::Value attribListToValue(const std::set<CX2::Authentication::sApplicationAttrib> & value)
    {
        Json::Value x;
        int i=0;
        for (const auto & strVal : value)
        {
            x[i++]["appName"] = strVal.appName;
            x[i++]["attribName"] = strVal.attribName;
        }
        return x;
    }


    template<typename T> static Json::Value stringListToValue(const T & value)
    {
        Json::Value x;
        int i=0;
        for (const std::string & strVal : value)
        {
            x[i++] = strVal;
        }
        return x;
    }
};
}}}

#endif // XRPC_FUNCTIONS_AUTH_H
