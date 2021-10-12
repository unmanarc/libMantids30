#ifndef XRPC_FUNCTIONS_AUTH_H
#define XRPC_FUNCTIONS_AUTH_H

#include <cx2_xrpc_common/methodsmanager.h>
#include <cx2_hlp_functions/json.h>

namespace CX2 { namespace RPC { namespace Templates {

class FullAuth {
public:
    static void AddFullAuthMethods(MethodsManager *methods, const std::string & _dirAppName);
private:

    /////////////////////////////////////////////////////////////////////////////////
    // account:
    static json accountAdd(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const json & payload);
    static json accountExist(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const json & payload);
    static json accountChangeSecret(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const json & payload);
    static json accountRemove(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const json & payload);
    static json accountDisable(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const json & payload);
    static json accountConfirm(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const json & payload);
    static json accountChangeBasicInfo(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const json & payload);
    static json accountChangeDescription(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const json & payload);
    static json accountChangeGivenName(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const json & payload);
    static json accountChangeLastName(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const json & payload);
    static json accountChangeEmail(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const json & payload);
    static json accountChangeExtraData(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const json & payload);
    static json accountChangeExpiration(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const json & payload);
    static json accountChangeGroupSet(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const json & payload);
    static json isAccountDisabled(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const json & payload);
    static json isAccountConfirmed(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const json & payload);
    static json isAccountSuperUser(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const json & payload);
    static json accountDescription(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const json & payload);
    static json accountBasicInfo(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const json & payload);
    static json accountGivenName(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const json & payload);
    static json accountLastName(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const json & payload);
    static json accountLastLogin(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const json & payload);
    static json resetBadAttempts(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const json & payload);
    static json accountEmail(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const json & payload);
    static json accountExtraData(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const json & payload);
    static json accountExpirationDate(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const json & payload);
    static json isAccountExpired(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const json & payload);
    static json accountValidateAttribute(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const json & payload);
    static json accountsList(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const json & payload);
    static json accountsBasicInfoSearch(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const json & payload);
    static json accountGroups(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const json & payload);
    static json accountDirectAttribs(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const json & payload);
    static json accountUsableAttribs(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const json & payload);

    /////////////////////////////////////////////////////////////////////////////////
    // applications:
    static json applicationAdd(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const json & payload);
    static json applicationRemove(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const json & payload);
    static json applicationExist(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const json & payload);
    static json applicationDescription(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const json & payload);
//    static json applicationKey(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const json & payload);
    static json applicationBasicInfo(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const json & payload);
    static json applicationChangeDescription(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const json & payload);
    static json applicationChangeKey(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const json & payload);
    static json applicationList(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const json & payload);
    static json applicationValidateOwner(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const json & payload);
    static json applicationValidateAccount(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const json & payload);
    static json applicationOwners(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const json & payload);
    static json applicationAccounts(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const json & payload);
    static json accountApplications(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const json & payload);
    static json applicationAccountAdd(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const json & payload);
    static json applicationAccountRemove(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const json & payload);
    static json applicationOwnerAdd(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const json & payload);
    static json applicationOwnerRemove(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const json & payload);
    static json applicationsBasicInfoSearch(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const json & payload);

    /////////////////////////////////////////////////////////////////////////////////
    // attributes:
    static json attribAdd(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const json & payload);
    static json attribRemove(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const json & payload);
    static json attribGroupAdd(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const json & payload);
    static json attribGroupRemove(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const json & payload);
    static json attribAccountAdd(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const json & payload);
    static json attribAccountRemove(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const json & payload);
    static json attribChangeDescription(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const json & payload);
    static json attribsList(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const json & payload);
    static json attribDescription(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const json & payload);
    static json attribGroups(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const json & payload);
    static json attribAccounts(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const json & payload);
    static json attribsBasicInfoSearch(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const json & payload);
    static json attribsLeftListForGroup(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const json & payload);

    /////////////////////////////////////////////////////////////////////////////////
    // group:
    static json groupAdd(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const json & payload);
    static json groupRemove(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const json & payload);
    static json groupExist(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const json & payload);
    static json groupAccountAdd(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const json & payload);
    static json groupAccountRemove(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const json & payload);
    static json groupChangeDescription(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const json & payload);
    static json groupValidateAttribute(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const json & payload);
    static json groupDescription(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const json & payload);
    static json groupsList(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const json & payload);
    static json groupAttribs(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const json & payload);
    static json groupAccounts(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const json & payload);
    static json groupsBasicInfoSearch(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const json & payload);
    static json groupBasicInfo(void * obj, CX2::Authentication::Manager * auth, CX2::Authentication::Session * session, const json & payload);

    static std::map<std::string,std::string> jsonToMap(const json & jValue);

    static std::set<CX2::Authentication::sApplicationAttrib> iAttribsLeftListForGroup(Authentication::Manager *auth, const std::string & appName, const std::string & groupName);

    static json attribListToValue(const std::set<CX2::Authentication::sApplicationAttrib> & value, CX2::Authentication::Manager * auth)
    {
        json x;
        int i=0;
        for (const auto & attrib : value)
        {
            x[i]["appName"] = attrib.appName;
            x[i]["name"] = attrib.attribName;
            x[i++]["description"] = auth->attribDescription(attrib);
        }
        return x;
    }


    template<typename T> static json stringListToValue(const T & value)
    {
        json x;
        int i=0;
        for (const std::string & strVal : value)
        {
            x[i++] = strVal;
        }
        return x;
    }

    static std::string dirAppName;

};

}}}

#endif // XRPC_FUNCTIONS_AUTH_H
