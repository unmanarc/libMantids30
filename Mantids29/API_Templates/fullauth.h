#ifndef XRPC_FUNCTIONS_AUTH_H
#define XRPC_FUNCTIONS_AUTH_H

#include <Mantids29/API_Core/methodshandler.h>
#include <Mantids29/Helpers/json.h>

namespace Mantids29 { namespace Network { namespace API { namespace Templates {

class FullAuth {
public:
    /**
     * @brief AddFullAuthMethods Add full set of login authentication methods as web server functions
     * @param methods Methods Manager that manages the web server special methods/api requests
     * @param _dirAppName set your application name here to prevent that the application will be removing itself
     */
    static void AddFullAuthMethods(API::MethodsHandler *methods, const std::string & _dirAppName);
private:

    /////////////////////////////////////////////////////////////////////////////////
    // account:
    static json accountAdd(void * obj, Mantids29::Authentication::Manager * auth, Mantids29::Authentication::Session * session, const json & payload);
    static json accountExist(void * obj, Mantids29::Authentication::Manager * auth, Mantids29::Authentication::Session * session, const json & payload);
    static json accountChangeSecret(void * obj, Mantids29::Authentication::Manager * auth, Mantids29::Authentication::Session * session, const json & payload);
    static json accountRemove(void * obj, Mantids29::Authentication::Manager * auth, Mantids29::Authentication::Session * session, const json & payload);
    static json accountDisable(void * obj, Mantids29::Authentication::Manager * auth, Mantids29::Authentication::Session * session, const json & payload);
    static json accountConfirm(void * obj, Mantids29::Authentication::Manager * auth, Mantids29::Authentication::Session * session, const json & payload);
    static json accountChangeBasicInfo(void * obj, Mantids29::Authentication::Manager * auth, Mantids29::Authentication::Session * session, const json & payload);
    static json accountChangeDescription(void * obj, Mantids29::Authentication::Manager * auth, Mantids29::Authentication::Session * session, const json & payload);
    static json accountChangeGivenName(void * obj, Mantids29::Authentication::Manager * auth, Mantids29::Authentication::Session * session, const json & payload);
    static json accountChangeLastName(void * obj, Mantids29::Authentication::Manager * auth, Mantids29::Authentication::Session * session, const json & payload);
    static json accountChangeEmail(void * obj, Mantids29::Authentication::Manager * auth, Mantids29::Authentication::Session * session, const json & payload);
    static json accountChangeExtraData(void * obj, Mantids29::Authentication::Manager * auth, Mantids29::Authentication::Session * session, const json & payload);
    static json accountChangeExpiration(void * obj, Mantids29::Authentication::Manager * auth, Mantids29::Authentication::Session * session, const json & payload);
    static json accountChangeGroupSet(void * obj, Mantids29::Authentication::Manager * auth, Mantids29::Authentication::Session * session, const json & payload);
    static json isAccountDisabled(void * obj, Mantids29::Authentication::Manager * auth, Mantids29::Authentication::Session * session, const json & payload);
    static json isAccountConfirmed(void * obj, Mantids29::Authentication::Manager * auth, Mantids29::Authentication::Session * session, const json & payload);
    static json isAccountSuperUser(void * obj, Mantids29::Authentication::Manager * auth, Mantids29::Authentication::Session * session, const json & payload);
    static json accountDescription(void * obj, Mantids29::Authentication::Manager * auth, Mantids29::Authentication::Session * session, const json & payload);
    static json accountBasicInfo(void * obj, Mantids29::Authentication::Manager * auth, Mantids29::Authentication::Session * session, const json & payload);
    static json accountGivenName(void * obj, Mantids29::Authentication::Manager * auth, Mantids29::Authentication::Session * session, const json & payload);
    static json accountLastName(void * obj, Mantids29::Authentication::Manager * auth, Mantids29::Authentication::Session * session, const json & payload);
    static json accountLastLogin(void * obj, Mantids29::Authentication::Manager * auth, Mantids29::Authentication::Session * session, const json & payload);
    static json resetBadAttempts(void * obj, Mantids29::Authentication::Manager * auth, Mantids29::Authentication::Session * session, const json & payload);
    static json accountEmail(void * obj, Mantids29::Authentication::Manager * auth, Mantids29::Authentication::Session * session, const json & payload);
    static json accountExtraData(void * obj, Mantids29::Authentication::Manager * auth, Mantids29::Authentication::Session * session, const json & payload);
    static json accountExpirationDate(void * obj, Mantids29::Authentication::Manager * auth, Mantids29::Authentication::Session * session, const json & payload);
    static json isAccountExpired(void * obj, Mantids29::Authentication::Manager * auth, Mantids29::Authentication::Session * session, const json & payload);
    static json validateAccountAttribute(void * obj, Mantids29::Authentication::Manager * auth, Mantids29::Authentication::Session * session, const json & payload);
    static json accountsList(void * obj, Mantids29::Authentication::Manager * auth, Mantids29::Authentication::Session * session, const json & payload);
    static json accountsBasicInfoSearch(void * obj, Mantids29::Authentication::Manager * auth, Mantids29::Authentication::Session * session, const json & payload);
    static json accountGroups(void * obj, Mantids29::Authentication::Manager * auth, Mantids29::Authentication::Session * session, const json & payload);
    static json accountDirectAttribs(void * obj, Mantids29::Authentication::Manager * auth, Mantids29::Authentication::Session * session, const json & payload);
    static json accountUsableAttribs(void * obj, Mantids29::Authentication::Manager * auth, Mantids29::Authentication::Session * session, const json & payload);

    /////////////////////////////////////////////////////////////////////////////////
    // applications:
    static json applicationAdd(void * obj, Mantids29::Authentication::Manager * auth, Mantids29::Authentication::Session * session, const json & payload);
    static json applicationRemove(void * obj, Mantids29::Authentication::Manager * auth, Mantids29::Authentication::Session * session, const json & payload);
    static json applicationExist(void * obj, Mantids29::Authentication::Manager * auth, Mantids29::Authentication::Session * session, const json & payload);
    static json applicationDescription(void * obj, Mantids29::Authentication::Manager * auth, Mantids29::Authentication::Session * session, const json & payload);
//    static json applicationKey(void * obj, Mantids29::Authentication::Manager * auth, Mantids29::Authentication::Session * session, const json & payload);
    static json applicationBasicInfo(void * obj, Mantids29::Authentication::Manager * auth, Mantids29::Authentication::Session * session, const json & payload);
    static json applicationChangeDescription(void * obj, Mantids29::Authentication::Manager * auth, Mantids29::Authentication::Session * session, const json & payload);
    static json applicationChangeKey(void * obj, Mantids29::Authentication::Manager * auth, Mantids29::Authentication::Session * session, const json & payload);
    static json applicationList(void * obj, Mantids29::Authentication::Manager * auth, Mantids29::Authentication::Session * session, const json & payload);
    static json applicationValidateOwner(void * obj, Mantids29::Authentication::Manager * auth, Mantids29::Authentication::Session * session, const json & payload);
    static json applicationValidateAccount(void * obj, Mantids29::Authentication::Manager * auth, Mantids29::Authentication::Session * session, const json & payload);
    static json applicationOwners(void * obj, Mantids29::Authentication::Manager * auth, Mantids29::Authentication::Session * session, const json & payload);
    static json applicationAccounts(void * obj, Mantids29::Authentication::Manager * auth, Mantids29::Authentication::Session * session, const json & payload);
    static json accountApplications(void * obj, Mantids29::Authentication::Manager * auth, Mantids29::Authentication::Session * session, const json & payload);
    static json applicationAccountAdd(void * obj, Mantids29::Authentication::Manager * auth, Mantids29::Authentication::Session * session, const json & payload);
    static json applicationAccountRemove(void * obj, Mantids29::Authentication::Manager * auth, Mantids29::Authentication::Session * session, const json & payload);
    static json applicationOwnerAdd(void * obj, Mantids29::Authentication::Manager * auth, Mantids29::Authentication::Session * session, const json & payload);
    static json applicationOwnerRemove(void * obj, Mantids29::Authentication::Manager * auth, Mantids29::Authentication::Session * session, const json & payload);
    static json applicationsBasicInfoSearch(void * obj, Mantids29::Authentication::Manager * auth, Mantids29::Authentication::Session * session, const json & payload);

    /////////////////////////////////////////////////////////////////////////////////
    // attributes:
    static json attribAdd(void * obj, Mantids29::Authentication::Manager * auth, Mantids29::Authentication::Session * session, const json & payload);
    static json attribRemove(void * obj, Mantids29::Authentication::Manager * auth, Mantids29::Authentication::Session * session, const json & payload);
    static json attribGroupAdd(void * obj, Mantids29::Authentication::Manager * auth, Mantids29::Authentication::Session * session, const json & payload);
    static json attribGroupRemove(void * obj, Mantids29::Authentication::Manager * auth, Mantids29::Authentication::Session * session, const json & payload);
    static json attribAccountAdd(void * obj, Mantids29::Authentication::Manager * auth, Mantids29::Authentication::Session * session, const json & payload);
    static json attribAccountRemove(void * obj, Mantids29::Authentication::Manager * auth, Mantids29::Authentication::Session * session, const json & payload);
    static json attribChangeDescription(void * obj, Mantids29::Authentication::Manager * auth, Mantids29::Authentication::Session * session, const json & payload);
    static json attribsList(void * obj, Mantids29::Authentication::Manager * auth, Mantids29::Authentication::Session * session, const json & payload);
    static json attribDescription(void * obj, Mantids29::Authentication::Manager * auth, Mantids29::Authentication::Session * session, const json & payload);
    static json attribGroups(void * obj, Mantids29::Authentication::Manager * auth, Mantids29::Authentication::Session * session, const json & payload);
    static json attribAccounts(void * obj, Mantids29::Authentication::Manager * auth, Mantids29::Authentication::Session * session, const json & payload);
    static json attribsBasicInfoSearch(void * obj, Mantids29::Authentication::Manager * auth, Mantids29::Authentication::Session * session, const json & payload);
    static json attribsLeftListForGroup(void * obj, Mantids29::Authentication::Manager * auth, Mantids29::Authentication::Session * session, const json & payload);

    /////////////////////////////////////////////////////////////////////////////////
    // group:
    static json groupAdd(void * obj, Mantids29::Authentication::Manager * auth, Mantids29::Authentication::Session * session, const json & payload);
    static json groupRemove(void * obj, Mantids29::Authentication::Manager * auth, Mantids29::Authentication::Session * session, const json & payload);
    static json groupExist(void * obj, Mantids29::Authentication::Manager * auth, Mantids29::Authentication::Session * session, const json & payload);
    static json groupAccountAdd(void * obj, Mantids29::Authentication::Manager * auth, Mantids29::Authentication::Session * session, const json & payload);
    static json groupAccountRemove(void * obj, Mantids29::Authentication::Manager * auth, Mantids29::Authentication::Session * session, const json & payload);
    static json groupChangeDescription(void * obj, Mantids29::Authentication::Manager * auth, Mantids29::Authentication::Session * session, const json & payload);
    static json groupValidateAttribute(void * obj, Mantids29::Authentication::Manager * auth, Mantids29::Authentication::Session * session, const json & payload);
    static json groupDescription(void * obj, Mantids29::Authentication::Manager * auth, Mantids29::Authentication::Session * session, const json & payload);
    static json groupsList(void * obj, Mantids29::Authentication::Manager * auth, Mantids29::Authentication::Session * session, const json & payload);
    static json groupAttribs(void * obj, Mantids29::Authentication::Manager * auth, Mantids29::Authentication::Session * session, const json & payload);
    static json groupAccounts(void * obj, Mantids29::Authentication::Manager * auth, Mantids29::Authentication::Session * session, const json & payload);
    static json groupsBasicInfoSearch(void * obj, Mantids29::Authentication::Manager * auth, Mantids29::Authentication::Session * session, const json & payload);
    static json groupBasicInfo(void * obj, Mantids29::Authentication::Manager * auth, Mantids29::Authentication::Session * session, const json & payload);

    static std::map<std::string,std::string> jsonToMap(const json & jValue);

    static std::set<Mantids29::Authentication::ApplicationAttribute> iAttribsLeftListForGroup(Authentication::Manager *auth, const std::string & appName, const std::string & groupName);

    static json attribListToValue(const std::set<Mantids29::Authentication::ApplicationAttribute> & value, Mantids29::Authentication::Manager * auth)
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

}}}}

#endif // XRPC_FUNCTIONS_AUTH_H
