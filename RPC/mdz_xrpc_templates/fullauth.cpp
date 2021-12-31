#include "fullauth.h"
#include <regex>
using namespace Mantids::RPC::Templates;
using namespace Mantids;

std::string FullAuth::dirAppName;

void FullAuth::AddFullAuthMethods(MethodsManager *methods, const std::string &_dirAppName)
{
    dirAppName = _dirAppName;

    // Accounts:
    methods->addRPCMethod("accountAdd", {"DIRWRITE"}, {&accountAdd,nullptr});
    methods->addRPCMethod("accountExist",{"DIRREAD"},{&accountExist,nullptr});
    methods->addRPCMethod("accountChangeSecret", {"DIRWRITE"}, {&accountChangeSecret,nullptr});
    methods->addRPCMethod("accountRemove", {"DIRWRITE"}, {&accountRemove,nullptr});
    methods->addRPCMethod("accountDisable", {"DIRWRITE"}, {&accountDisable,nullptr});
    methods->addRPCMethod("accountConfirm", {"DIRWRITE"}, {&accountConfirm,nullptr});
    methods->addRPCMethod("accountChangeBasicInfo", {"DIRWRITE"}, {&accountChangeBasicInfo,nullptr});
    methods->addRPCMethod("accountChangeDescription", {"DIRWRITE"}, {&accountChangeDescription,nullptr});
    methods->addRPCMethod("accountChangeGivenName", {"DIRWRITE"}, {&accountChangeGivenName,nullptr});
    methods->addRPCMethod("accountChangeLastName", {"DIRWRITE"}, {&accountChangeLastName,nullptr});
    methods->addRPCMethod("accountChangeEmail", {"DIRWRITE"}, {&accountChangeEmail,nullptr});
    methods->addRPCMethod("accountChangeExtraData", {"DIRWRITE"}, {&accountChangeExtraData,nullptr});
    methods->addRPCMethod("accountChangeExpiration", {"DIRWRITE"}, {&accountChangeExpiration,nullptr});
    methods->addRPCMethod("accountChangeGroupSet", {"DIRWRITE"}, {&accountChangeGroupSet,nullptr});
    methods->addRPCMethod("isAccountDisabled", {"DIRWRITE"}, {&isAccountDisabled,nullptr});
    methods->addRPCMethod("isAccountConfirmed", {"DIRWRITE"}, {&isAccountConfirmed,nullptr});
    methods->addRPCMethod("isAccountSuperUser", {"DIRWRITE"}, {&isAccountSuperUser,nullptr});
    methods->addRPCMethod("accountGivenName", {"DIRREAD"}, {&accountGivenName,nullptr});
    methods->addRPCMethod("accountLastName", {"DIRREAD"}, {&accountLastName,nullptr});
    methods->addRPCMethod("accountBasicInfo", {"DIRREAD"}, {&accountBasicInfo,nullptr});
    methods->addRPCMethod("accountDescription", {"DIRREAD"}, {&accountDescription,nullptr});
    methods->addRPCMethod("accountEmail", {"DIRREAD"}, {&accountEmail,nullptr});
    methods->addRPCMethod("accountExtraData", {"DIRREAD"}, {&accountExtraData,nullptr});
    methods->addRPCMethod("isAccountExpired", {"DIRREAD"}, {&isAccountExpired,nullptr});
    methods->addRPCMethod("accountValidateAttribute", {"DIRREAD"}, {&accountValidateAttribute,nullptr});
    methods->addRPCMethod("accountsList", {"DIRREAD"}, {&accountsList,nullptr});
    methods->addRPCMethod("accountsBasicInfoSearch", {"DIRREAD"}, {&accountsBasicInfoSearch,nullptr});
    methods->addRPCMethod("accountGroups", {"DIRREAD"}, {&accountGroups,nullptr});
    methods->addRPCMethod("accountDirectAttribs", {"DIRREAD"}, {&accountDirectAttribs,nullptr});
    methods->addRPCMethod("accountUsableAttribs", {"DIRREAD"}, {&accountUsableAttribs,nullptr});
    methods->addRPCMethod("accountExpirationDate", {"DIRREAD"}, {&accountExpirationDate,nullptr});
    methods->addRPCMethod("accountLastLogin", {"DIRREAD"}, {&accountLastLogin,nullptr});
    methods->addRPCMethod("resetBadAttempts", {"DIRWRITE"}, {&resetBadAttempts,nullptr});

    // Attribs:
    methods->addRPCMethod("attribAdd", {"DIRWRITE"}, {&attribAdd,nullptr});
    methods->addRPCMethod("attribRemove", {"DIRWRITE"}, {&attribRemove,nullptr});
    methods->addRPCMethod("attribGroupAdd", {"DIRWRITE"}, {&attribGroupAdd,nullptr});
    methods->addRPCMethod("attribGroupRemove", {"DIRWRITE"}, {&attribGroupRemove,nullptr});
    methods->addRPCMethod("attribAccountAdd", {"DIRWRITE"}, {&attribAccountAdd,nullptr});
    methods->addRPCMethod("attribAccountRemove", {"DIRWRITE"}, {&attribAccountRemove,nullptr});
    methods->addRPCMethod("attribChangeDescription", {"DIRWRITE"}, {&attribChangeDescription,nullptr});
    methods->addRPCMethod("attribDescription", {"DIRREAD"}, {&attribDescription,nullptr});
    methods->addRPCMethod("attribsList", {"DIRREAD"}, {&attribsList,nullptr});
    methods->addRPCMethod("attribGroups", {"DIRREAD"}, {&attribGroups,nullptr});
    methods->addRPCMethod("attribAccounts", {"DIRREAD"}, {&attribAccounts,nullptr});
    methods->addRPCMethod("attribsBasicInfoSearch", {"DIRREAD"}, {&attribsBasicInfoSearch,nullptr});
    methods->addRPCMethod("attribsLeftListForGroup", {"DIRREAD"}, {&attribsLeftListForGroup,nullptr});

    //  Applications
    methods->addRPCMethod("applicationBasicInfo", {"DIRREAD"}, {&applicationBasicInfo,nullptr});
    methods->addRPCMethod("applicationAdd", {"DIRWRITE"}, {&applicationAdd,nullptr});
    methods->addRPCMethod("applicationRemove", {"DIRWRITE"}, {&applicationRemove,nullptr});
    methods->addRPCMethod("applicationExist", {"DIRREAD"}, {&applicationExist,nullptr});
    methods->addRPCMethod("applicationDescription", {"DIRREAD"}, {&applicationDescription,nullptr});
    methods->addRPCMethod("applicationChangeDescription", {"DIRWRITE"}, {&applicationChangeDescription,nullptr});
    methods->addRPCMethod("applicationChangeKey", {"DIRWRITE"}, {&applicationChangeKey,nullptr});
    methods->addRPCMethod("applicationList", {"DIRREAD"}, {&applicationList,nullptr});
    methods->addRPCMethod("applicationValidateOwner", {"DIRREAD"}, {&applicationValidateOwner,nullptr});
    methods->addRPCMethod("applicationValidateAccount", {"DIRREAD"}, {&applicationValidateAccount,nullptr});
    methods->addRPCMethod("applicationOwners", {"DIRREAD"}, {&applicationOwners,nullptr});
    methods->addRPCMethod("applicationAccounts", {"DIRREAD"}, {&applicationAccounts,nullptr});
    methods->addRPCMethod("accountApplications", {"DIRREAD"}, {&accountApplications,nullptr});
    methods->addRPCMethod("applicationAccountAdd", {"DIRWRITE"}, {&applicationAccountAdd,nullptr});
    methods->addRPCMethod("applicationAccountRemove", {"DIRWRITE"}, {&applicationAccountRemove,nullptr});
    methods->addRPCMethod("applicationOwnerAdd", {"DIRWRITE"}, {&applicationOwnerAdd,nullptr});
    methods->addRPCMethod("applicationOwnerRemove", {"DIRWRITE"}, {&applicationOwnerRemove,nullptr});
    methods->addRPCMethod("applicationsBasicInfoSearch", {"DIRREAD"}, {&applicationsBasicInfoSearch,nullptr});

    // Groups:
    methods->addRPCMethod("groupAdd", {"DIRWRITE"}, {&groupAdd,nullptr});
    methods->addRPCMethod("groupRemove", {"DIRWRITE"}, {&groupRemove,nullptr});
    methods->addRPCMethod("groupExist", {"DIRREAD"}, {&groupExist,nullptr});
    methods->addRPCMethod("groupAccountAdd", {"DIRWRITE"}, {&groupAccountAdd,nullptr});
    methods->addRPCMethod("groupAccountRemove", {"DIRWRITE"}, {&groupAccountRemove,nullptr});
    methods->addRPCMethod("groupChangeDescription", {"DIRWRITE"}, {&groupChangeDescription,nullptr});
    methods->addRPCMethod("groupValidateAttribute", {"DIRREAD"}, {&groupValidateAttribute,nullptr});
    methods->addRPCMethod("groupDescription", {"DIRREAD"}, {&groupDescription,nullptr});
    methods->addRPCMethod("groupsList", {"DIRREAD"}, {&groupsList,nullptr});
    methods->addRPCMethod("groupAttribs", {"DIRREAD"}, {&groupAttribs,nullptr});
    methods->addRPCMethod("groupAccounts", {"DIRREAD"}, {&groupAccounts,nullptr});
    methods->addRPCMethod("groupsBasicInfoSearch", {"DIRREAD"}, {&groupsBasicInfoSearch,nullptr});
    methods->addRPCMethod("groupBasicInfo", {"DIRREAD"}, {&groupBasicInfo,nullptr});

    //getBAuthPolicyMaxTries ?
}

json FullAuth::accountAdd(void *obj, Authentication::Manager *auth, Authentication::Session *session, const json &payload)
{
    json payloadOut;

    std::string accountName = JSON_ASSTRING(payload,"accountName","");

    if (auth->accountExist(accountName))
    {
        payloadOut["retCode"] =false;
        payloadOut["retMsg"] = "Account Already Exist";
        return payloadOut;
    }

    if (accountName.empty())
    {
        payloadOut["retCode"] =false;
        payloadOut["retMsg"] = "Account Name is Empty";
        return payloadOut;
    }

    std::regex accountNameExpr("[a-zA-Z0-9]+");
    if(!regex_match(accountName,accountNameExpr))
    {
        payloadOut["retCode"] =false;
        payloadOut["retMsg"] = "Account name have invalid characters";
        return payloadOut;
    }

    // Create Expired SSHA256 password (require change)
    Mantids::Authentication::Secret newSecretData = Mantids::Authentication::createNewSecret(JSON_ASSTRING(payload,"secretTempPass",""),Mantids::Authentication::FN_SSHA256,true);

    Mantids::Authentication::sAccountDetails accountDetails;
    accountDetails.sDescription = JSON_ASSTRING(payload,"description","");
    accountDetails.sEmail = JSON_ASSTRING(payload,"mail","");
    accountDetails.sExtraData = JSON_ASSTRING(payload,"extraData","");
    accountDetails.sGivenName = JSON_ASSTRING(payload,"givenName","");
    accountDetails.sLastName = JSON_ASSTRING(payload,"lastName","");

    Mantids::Authentication::sAccountAttribs accountAttribs;
    accountAttribs.confirmed  = JSON_ASBOOL(payload,"isConfirmed",false);
    accountAttribs.enabled  = JSON_ASBOOL(payload,"isEnabled",false);
    accountAttribs.superuser  = JSON_ASBOOL(payload,"isSuperuser",false);

    payloadOut["retCode"] =
            auth->accountAdd(
                accountName,
                newSecretData,
                accountDetails,
                JSON_ASUINT64(payload,"expirationDate",0),
            accountAttribs, session->getAuthUser()
            );

    if (!JSON_ASBOOL(payloadOut,"retCode",false))
    {
        payloadOut["retMsg"] = "Internal Error";
    }

    return payloadOut;
}

json FullAuth::accountExist(void *obj, Authentication::Manager *auth, Authentication::Session *session, const json &payload)
{
    json payloadOut;
    payloadOut["retCode"] = auth->accountExist(JSON_ASSTRING(payload,"accountName",""));
    return payloadOut;
}

json FullAuth::accountChangeSecret(void *, Mantids::Authentication::Manager *auth,Mantids::Authentication::Session *session, const json &payload)
{
    json payloadOut;
    Mantids::Authentication::Secret secretData;
    secretData.fromMap(jsonToMap(payload["secretData"]));
    payloadOut["retCode"] = auth->accountChangeSecret(JSON_ASSTRING(payload,"accountName",""),  secretData, JSON_ASUINT(payload,"passIndex",0));
    return payloadOut;
}

json FullAuth::accountRemove(void *, Mantids::Authentication::Manager *auth,Mantids::Authentication::Session *session, const json &payload)
{
    json payloadOut;
    payloadOut["retCode"] = auth->accountRemove(JSON_ASSTRING(payload,"accountName",""));
    return payloadOut;
}

json FullAuth::accountDisable(void *, Mantids::Authentication::Manager *auth,Mantids::Authentication::Session *session, const json &payload)
{
    json payloadOut;
    payloadOut["retCode"] = auth->accountDisable(JSON_ASSTRING(payload,"accountName",""), JSON_ASBOOL(payload,"disabled",false));
    return payloadOut;
}

json FullAuth::accountConfirm(void *, Mantids::Authentication::Manager *auth,Mantids::Authentication::Session *session, const json &payload)
{
    json payloadOut;
    payloadOut["retCode"] = auth->accountDisable(JSON_ASSTRING(payload,"accountName",""), JSON_ASBOOL(payload,"disabled",false));
    return payloadOut;
}

json FullAuth::accountChangeBasicInfo(void *obj, Authentication::Manager *auth, Authentication::Session *session, const json &payload)
{
    json payloadOut;
    Mantids::Authentication::sAccountAttribs attribs;

    attribs.enabled = JSON_ASBOOL(payload,"isEnabled",false);
    attribs.confirmed = JSON_ASBOOL(payload,"isConfirmed",false);
    attribs.superuser = JSON_ASBOOL(payload,"isSuperuser",false);

    payloadOut["retCode"] =
            auth->accountChangeDescription(JSON_ASSTRING(payload,"accountName",""), JSON_ASSTRING(payload,"description","")) &&
            auth->accountChangeGivenName(JSON_ASSTRING(payload,"accountName",""), JSON_ASSTRING(payload,"givenName","")) &&
            auth->accountChangeLastName(JSON_ASSTRING(payload,"accountName",""), JSON_ASSTRING(payload,"lastName","")) &&
            auth->accountChangeEmail(JSON_ASSTRING(payload,"accountName",""), JSON_ASSTRING(payload,"email","")) &&
            auth->accountChangeExtraData(JSON_ASSTRING(payload,"accountName",""), JSON_ASSTRING(payload,"extraData","")) &&
            auth->accountChangeAttribs(JSON_ASSTRING(payload,"accountName",""), attribs);
    return payloadOut;

}

json FullAuth::accountChangeDescription(void *, Mantids::Authentication::Manager *auth,Mantids::Authentication::Session *session, const json &payload)
{
    json payloadOut;
    payloadOut["retCode"] = auth->accountChangeDescription(JSON_ASSTRING(payload,"accountName",""), JSON_ASSTRING(payload,"description",""));
    return payloadOut;
}

json FullAuth::accountChangeGivenName(void *obj, Authentication::Manager *auth, Authentication::Session *session, const json &payload)
{
    json payloadOut;
    payloadOut["retCode"] = auth->accountChangeGivenName(JSON_ASSTRING(payload,"accountName",""), JSON_ASSTRING(payload,"givenName",""));
    return payloadOut;
}

json FullAuth::accountChangeLastName(void *obj, Authentication::Manager *auth, Authentication::Session *session, const json &payload)
{
    json payloadOut;
    payloadOut["retCode"] = auth->accountChangeLastName(JSON_ASSTRING(payload,"accountName",""), JSON_ASSTRING(payload,"lastName",""));
    return payloadOut;
}

json FullAuth::accountChangeEmail(void *, Mantids::Authentication::Manager *auth,Mantids::Authentication::Session *session, const json &payload)
{
    json payloadOut;
    payloadOut["retCode"] = auth->accountChangeEmail(JSON_ASSTRING(payload,"accountName",""), JSON_ASSTRING(payload,"email",""));
    return payloadOut;
}

json FullAuth::accountChangeExtraData(void *, Mantids::Authentication::Manager *auth,Mantids::Authentication::Session *session, const json &payload)
{
    json payloadOut;
    payloadOut["retCode"] = auth->accountChangeExtraData(JSON_ASSTRING(payload,"accountName",""), JSON_ASSTRING(payload,"extraData",""));
    return payloadOut;
}

json FullAuth::accountChangeExpiration(void *, Mantids::Authentication::Manager *auth,Mantids::Authentication::Session *session, const json &payload)
{
    json payloadOut;
    payloadOut["retCode"] = auth->accountChangeExpiration(JSON_ASSTRING(payload,"accountName",""), JSON_ASUINT64(payload,"expiration",0));
    return payloadOut;
}

json FullAuth::accountChangeGroupSet(void *obj, Authentication::Manager *auth, Authentication::Session *session, const json &payload)
{
    std::set<std::string> groupSet;
    json payloadOut;

    if (!payload["groups"].isArray())
    {
        payloadOut["retCode"] = false;
        return payloadOut;
    }

    for ( size_t i=0; i<payload["groups"].size();i++ )
    {
        groupSet.insert(payload["groups"][(int)i].asString());
    }

    payloadOut["retCode"] = auth->accountChangeGroupSet(JSON_ASSTRING(payload,"accountName",""), groupSet);
    return payloadOut;
}

json FullAuth::isAccountDisabled(void *, Mantids::Authentication::Manager *auth,Mantids::Authentication::Session *session, const json &payload)
{
    json payloadOut;
    payloadOut["disabled"] = auth->isAccountDisabled(JSON_ASSTRING(payload,"accountName",""));
    return payloadOut;
}

json FullAuth::isAccountConfirmed(void *, Mantids::Authentication::Manager *auth,Mantids::Authentication::Session *session, const json &payload)
{
    json payloadOut;
    payloadOut["confirmed"] = auth->isAccountConfirmed(JSON_ASSTRING(payload,"accountName",""));
    return payloadOut;
}

json FullAuth::isAccountSuperUser(void *, Mantids::Authentication::Manager *auth,Mantids::Authentication::Session *session, const json &payload)
{
    json payloadOut;
    payloadOut["superuser"] = auth->isAccountSuperUser(JSON_ASSTRING(payload,"accountName",""));
    return payloadOut;
}

json FullAuth::accountDescription(void *, Mantids::Authentication::Manager *auth,Mantids::Authentication::Session *session, const json &payload)
{
    json payloadOut;
    payloadOut["description"] = auth->accountDescription(JSON_ASSTRING(payload,"accountName",""));
    return payloadOut;
}

json FullAuth::accountBasicInfo(void *obj, Authentication::Manager *auth, Authentication::Session *session, const json &payload)
{
    json payloadOut;

    payloadOut["givenName"] = auth->accountGivenName(JSON_ASSTRING(payload,"accountName",""));
    payloadOut["lastName"] = auth->accountLastName(JSON_ASSTRING(payload,"accountName",""));
    payloadOut["description"] = auth->accountDescription(JSON_ASSTRING(payload,"accountName",""));
    payloadOut["email"] = auth->accountEmail(JSON_ASSTRING(payload,"accountName",""));
    payloadOut["extraData"] = auth->accountExtraData(JSON_ASSTRING(payload,"accountName",""));

    auto attribs = auth->accountAttribs(JSON_ASSTRING(payload,"accountName",""));

    payloadOut["enabled"] = attribs.enabled;
    payloadOut["confirmed"] = attribs.confirmed;
    payloadOut["superuser"] = attribs.superuser;

    int i=0;
    auto accountGroups = auth->accountGroups(JSON_ASSTRING(payload,"accountName",""));
    for (const auto & groupName : accountGroups)
    {
        payloadOut["groups"][i]["name"] = groupName;
        // TODO: optimize:
        payloadOut["groups"][i]["description"] = auth->groupDescription(groupName);
        i++;
    }

    i=0;
    for (const auto & groupName : auth->groupsList())
    {
        if (accountGroups.find(groupName)==accountGroups.end())
        {
            payloadOut["groupsLeft"][i]["name"] = groupName;
            payloadOut["groupsLeft"][i]["description"] = auth->groupDescription(groupName);
            i++;
        }
    }

    auto directAttribs = auth->accountDirectAttribs(JSON_ASSTRING(payload,"accountName",""));
    auto usableAttribs = auth->accountUsableAttribs(JSON_ASSTRING(payload,"accountName",""));

    auto accountApplications = auth->accountApplications(JSON_ASSTRING(payload,"accountName",""));
    i=0;
    for (const auto & applicationName : accountApplications)
    {
        payloadOut["applications"][i]["name"] = applicationName;
        // TODO: optimize:
        payloadOut["applications"][i]["description"] = auth->applicationDescription(applicationName);

        int j=0;
        for (const auto & directAttrib : directAttribs)
        {
            if (directAttrib.appName == applicationName)
            {
                payloadOut["applications"][i]["directAttribs"][j]["name"] = directAttrib.attribName;
                payloadOut["applications"][i]["directAttribs"][j]["description"] = auth->attribDescription(directAttrib);
                j++;
            }
        }

        j=0;
        for (const auto & attrib : auth->attribsList(applicationName))
        {
            if (directAttribs.find(attrib)==directAttribs.end())
            {
                payloadOut["applications"][i]["directAttribsLeft"][j]["name"] = attrib.attribName;
                payloadOut["applications"][i]["directAttribsLeft"][j]["description"] = auth->attribDescription(attrib);
                j++;
            }
        }

        j=0;
        for (const auto & usableAttrib : directAttribs)
        {
            if (usableAttrib.appName == applicationName)
            {
                payloadOut["applications"][i]["usableAttribs"][j]["name"] = usableAttrib.attribName;
                payloadOut["applications"][i]["usableAttribs"][j]["description"] = auth->attribDescription(usableAttrib);
                j++;
            }
        }
        i++;
    }


    i=0;

    for (const auto & applicationName : auth->applicationList())
    {
        if ( accountApplications.find(applicationName) == accountApplications.end()  )
        {
            payloadOut["applicationsLeft"][i]["name"] = applicationName;
            payloadOut["applicationsLeft"][i]["description"] = auth->applicationDescription(applicationName);
            i++;
        }
    }

    return payloadOut;
}

json FullAuth::attribDescription(void *obj, Authentication::Manager *auth, Authentication::Session *session, const json &payload)
{
    json payloadOut;
    payloadOut["description"] = auth->attribDescription({JSON_ASSTRING(payload,"appName",""),JSON_ASSTRING(payload,"attribName","")});
    return payloadOut;
}

json FullAuth::accountGivenName(void *obj, Authentication::Manager *auth, Authentication::Session *session, const json &payload)
{
    json payloadOut;
    payloadOut["givenName"] = auth->accountGivenName(JSON_ASSTRING(payload,"accountName",""));
    return payloadOut;
}

json FullAuth::accountLastName(void *obj, Authentication::Manager *auth, Authentication::Session *session, const json &payload)
{
    json payloadOut;
    payloadOut["lastName"] = auth->accountLastName(JSON_ASSTRING(payload,"accountName",""));
    return payloadOut;
}

json FullAuth::accountLastLogin(void *obj, Authentication::Manager *auth, Authentication::Session *session, const json &payload)
{
    json payloadOut;
    payloadOut["lastLogin"] = (Json::UInt64)auth->accountLastLogin(JSON_ASSTRING(payload,"accountName",""));
    return payloadOut;
}

json FullAuth::resetBadAttempts(void *obj, Authentication::Manager *auth, Authentication::Session *session, const json &payload)
{
    json payloadOut;
    auth->resetBadAttempts(JSON_ASSTRING(payload,"accountName",""),JSON_ASUINT(payload,"passIndex",0));
    return payloadOut;
}

json FullAuth::accountEmail(void *, Mantids::Authentication::Manager *auth,Mantids::Authentication::Session *session, const json &payload)
{
    json payloadOut;
    payloadOut["email"] = auth->accountEmail(JSON_ASSTRING(payload,"accountName",""));
    return payloadOut;
}

json FullAuth::accountExtraData(void *, Mantids::Authentication::Manager *auth,Mantids::Authentication::Session *session, const json &payload)
{
    json payloadOut;
    payloadOut["extraData"] = auth->accountExtraData(JSON_ASSTRING(payload,"accountName",""));
    return payloadOut;
}

json FullAuth::accountExpirationDate(void *, Mantids::Authentication::Manager *auth,Mantids::Authentication::Session *session, const json &payload)
{
    json payloadOut;
    payloadOut["expirationDate"] = Json::Int64(auth->accountExpirationDate(JSON_ASSTRING(payload,"accountName","")));
    return payloadOut;
}

json FullAuth::isAccountExpired(void *, Mantids::Authentication::Manager *auth,Mantids::Authentication::Session *session, const json &payload)
{
    json payloadOut;
    payloadOut["isAccountExpired"] = auth->isAccountExpired(JSON_ASSTRING(payload,"accountName",""));
    return payloadOut;
}

json FullAuth::accountValidateAttribute(void *, Mantids::Authentication::Manager *auth,Mantids::Authentication::Session *session, const json &payload)
{
    json payloadOut;
    payloadOut["isAccountExpired"] = auth->accountValidateAttribute(JSON_ASSTRING(payload,"accountName",""),  {JSON_ASSTRING(payload,"appName",""),JSON_ASSTRING(payload,"attribName","")});
    return payloadOut;
}

json FullAuth::accountsList(void *, Mantids::Authentication::Manager *auth,Mantids::Authentication::Session *session, const json &payload)
{
    json payloadOut;
    payloadOut["accountsList"] = stringListToValue(auth->accountsList());
    return payloadOut;
}

json FullAuth::accountsBasicInfoSearch(void *obj, Authentication::Manager *auth, Authentication::Session *session, const json &payload)
{
    json x;
    int i=0;
    for (const auto & strVal : auth->accountsBasicInfoSearch(
             JSON_ASSTRING(payload,"searchWords",""),
             JSON_ASUINT64(payload,"limit",0),
             JSON_ASUINT64(payload,"offset",0)
             ))
    {
        x[i]["lastName"] = strVal.sLastName;
        x[i]["givenName"] = strVal.sGivenName;
        x[i]["email"] = strVal.sEmail;
        x[i]["description"] = strVal.sDescription;
        x[i]["accountName"] = strVal.sAccountName;
        x[i]["superuser"] = strVal.superuser;
        x[i]["expired"] = strVal.expired;
        x[i]["enabled"] = strVal.enabled;
        x[i]["confirmed"] = strVal.confirmed;
        i++;
    }
    return x;
}

json FullAuth::accountGroups(void *, Mantids::Authentication::Manager *auth,Mantids::Authentication::Session *session, const json &payload)
{
    json payloadOut;

    payloadOut["accountGroups"] = stringListToValue(auth->accountGroups(JSON_ASSTRING(payload,"accountName","")));
    return payloadOut;
}

json FullAuth::accountDirectAttribs(void *, Mantids::Authentication::Manager *auth,Mantids::Authentication::Session *session, const json &payload)
{
    json payloadOut;

    payloadOut["accountDirectAttribs"] = attribListToValue(auth->accountDirectAttribs(JSON_ASSTRING(payload,"accountName","")),auth);
    return payloadOut;
}

json FullAuth::accountUsableAttribs(void *, Mantids::Authentication::Manager *auth,Mantids::Authentication::Session *session, const json &payload)
{
    json payloadOut;

    payloadOut["accountUsableAttribs"] = attribListToValue(auth->accountUsableAttribs(JSON_ASSTRING(payload,"accountName","")),auth);
    return payloadOut;
}

json FullAuth::applicationAdd(void *obj, Authentication::Manager *auth, Authentication::Session *session, const json &payload)
{
    json payloadOut;
    payloadOut["retCode"] = auth->applicationAdd(   JSON_ASSTRING(payload,"appName",""),
                                                    JSON_ASSTRING(payload,"description",""),
                                                    JSON_ASSTRING(payload,"appKey",""),
                                                    session->getAuthUser()
                                                );
    return payloadOut;
}

json FullAuth::applicationRemove(void *obj, Authentication::Manager *auth, Authentication::Session *session, const json &payload)
{
    json payloadOut;

    std::string appName = JSON_ASSTRING(payload,"appName","");

    if (appName == dirAppName)
    {
        payloadOut["retCode"] = false;
        return payloadOut;
    }

    payloadOut["retCode"] = auth->applicationRemove(appName);
    return payloadOut;
}

json FullAuth::applicationExist(void *obj, Authentication::Manager *auth, Authentication::Session *session, const json &payload)
{
    json payloadOut;
    payloadOut["retCode"] = auth->applicationExist( JSON_ASSTRING(payload,"appName",""));
    return payloadOut;
}

json FullAuth::applicationDescription(void *obj, Authentication::Manager *auth, Authentication::Session *session, const json &payload)
{
    json payloadOut;
    payloadOut["description"] = auth->applicationDescription( JSON_ASSTRING(payload,"appName",""));
    return payloadOut;
}
/*
json FullAuth::applicationKey(void *obj, Authentication::Manager *auth, Authentication::Session *session, const json &payload)
{
    json payloadOut;
    payloadOut["appKey"] = auth->applicationKey( JSON_ASSTRING(payload,"appName",""));
    return payloadOut;
}
*/
json FullAuth::applicationBasicInfo(void *obj, Authentication::Manager *auth, Authentication::Session *session, const json &payload)
{
    json payloadOut;
    std::string appName = JSON_ASSTRING(payload,"appName","");
    payloadOut["description"] = auth->applicationDescription( appName );

    // Get associated attribs...
    auto attrList = auth->attribsList(appName);
    int i=0;
    for ( const auto & attrib : attrList )
    {
        payloadOut["attribs"][i]["name"] = attrib.attribName;
        payloadOut["attribs"][i]["description"] = auth->attribDescription(attrib);
        i++;
    }

    // Get associated direct accounts...
    auto acctList = auth->applicationAccounts(appName);
    i=0;
    for ( const auto & acct : acctList )
    {
        payloadOut["accounts"][i]["name"] = acct;
        payloadOut["accounts"][i]["description"] = auth->accountDescription(acct);
        payloadOut["accounts"][i]["givenName"] = auth->accountGivenName(acct);
        payloadOut["accounts"][i]["lastName"] = auth->accountLastName(acct);
        i++;
    }

    return payloadOut;
}

json FullAuth::applicationChangeDescription(void *obj, Authentication::Manager *auth, Authentication::Session *session, const json &payload)
{
    json payloadOut;
    payloadOut["retCode"] = auth->applicationChangeDescription( JSON_ASSTRING(payload,"appName",""), JSON_ASSTRING(payload,"description","") );
    return payloadOut;
}

json FullAuth::applicationChangeKey(void *obj, Authentication::Manager *auth, Authentication::Session *session, const json &payload)
{
    json payloadOut;
    payloadOut["retCode"] = auth->applicationChangeKey( JSON_ASSTRING(payload,"appName",""), JSON_ASSTRING(payload,"appKey","") );
    return payloadOut;
}

json FullAuth::applicationList(void *obj, Authentication::Manager *auth, Authentication::Session *session, const json &payload)
{
    json payloadOut;
    payloadOut["applications"] = stringListToValue(auth->applicationList());
    return payloadOut;
}

json FullAuth::applicationValidateOwner(void *obj, Authentication::Manager *auth, Authentication::Session *session, const json &payload)
{
    json payloadOut;
    payloadOut["retCode"] = auth->applicationValidateOwner( JSON_ASSTRING(payload,"appName",""), JSON_ASSTRING(payload,"accountName","") );
    return payloadOut;
}

json FullAuth::applicationValidateAccount(void *obj, Authentication::Manager *auth, Authentication::Session *session, const json &payload)
{
    json payloadOut;
    payloadOut["retCode"] = auth->applicationValidateAccount( JSON_ASSTRING(payload,"appName",""), JSON_ASSTRING(payload,"accountName","") );
    return payloadOut;
}

json FullAuth::applicationOwners(void *obj, Authentication::Manager *auth, Authentication::Session *session, const json &payload)
{
    json payloadOut;
    payloadOut["owners"] = stringListToValue(auth->applicationOwners( JSON_ASSTRING(payload,"applicationName","")));
    return payloadOut;
}

json FullAuth::applicationAccounts(void *obj, Authentication::Manager *auth, Authentication::Session *session, const json &payload)
{
    json payloadOut;
    payloadOut["accounts"] = stringListToValue(auth->applicationAccounts( JSON_ASSTRING(payload,"applicationName","")));
    return payloadOut;
}

json FullAuth::accountApplications(void *obj, Authentication::Manager *auth, Authentication::Session *session, const json &payload)
{
    json payloadOut;
    payloadOut["applications"] = stringListToValue(auth->accountApplications( JSON_ASSTRING(payload,"accountName","")));
    return payloadOut;
}

json FullAuth::applicationAccountAdd(void *obj, Authentication::Manager *auth, Authentication::Session *session, const json &payload)
{
    json payloadOut;
    payloadOut["retCode"] = auth->applicationAccountAdd( JSON_ASSTRING(payload,"appName",""), JSON_ASSTRING(payload,"accountName","") );
    return payloadOut;
}

json FullAuth::applicationAccountRemove(void *obj, Authentication::Manager *auth, Authentication::Session *session, const json &payload)
{
    json payloadOut;
    payloadOut["retCode"] = auth->applicationAccountRemove( JSON_ASSTRING(payload,"appName",""), JSON_ASSTRING(payload,"accountName","") );
    return payloadOut;
}

json FullAuth::applicationOwnerAdd(void *obj, Authentication::Manager *auth, Authentication::Session *session, const json &payload)
{
    json payloadOut;
    payloadOut["retCode"] = auth->applicationOwnerAdd( JSON_ASSTRING(payload,"appName",""), JSON_ASSTRING(payload,"accountName","") );
    return payloadOut;
}

json FullAuth::applicationOwnerRemove(void *obj, Authentication::Manager *auth, Authentication::Session *session, const json &payload)
{
    json payloadOut;
    payloadOut["retCode"] = auth->applicationOwnerRemove( JSON_ASSTRING(payload,"appName",""), JSON_ASSTRING(payload,"accountName","") );
    return payloadOut;
}

json FullAuth::applicationsBasicInfoSearch(void *obj, Authentication::Manager *auth, Authentication::Session *session, const json &payload)
{
    json x;
    int i=0;
    for (const auto & strVal : auth->applicationsBasicInfoSearch(
             JSON_ASSTRING(payload,"searchWords",""),
             JSON_ASUINT64(payload,"limit",0),
             JSON_ASUINT64(payload,"offset",0)
             ))
    {
        x[i]["appCreator"] = strVal.sAppCreator;
        x[i]["appName"] = strVal.sApplicationName;
        x[i]["description"] = strVal.sDescription;
        i++;
    }
    return x;
}

json FullAuth::attribAdd(void *, Mantids::Authentication::Manager *auth,Mantids::Authentication::Session *session, const json &payload)
{
    json payloadOut;

    std::string appName = JSON_ASSTRING(payload,"appName","");

    // Don't modify attribs from MANTIDSDIR:
    if ( appName == dirAppName )
    {
        payloadOut["retCode"] = false;
        return payloadOut;
    }

    payloadOut["retCode"] = auth->attribAdd({appName,JSON_ASSTRING(payload,"attribName","")},
                                            JSON_ASSTRING(payload,"description",""));
    return payloadOut;
}

json FullAuth::attribRemove(void *, Mantids::Authentication::Manager *auth,Mantids::Authentication::Session *session, const json &payload)
{
    json payloadOut;

    std::string appName = JSON_ASSTRING(payload,"appName","");

    // Don't modify attribs from MANTIDSDIR:
    if ( appName == dirAppName )
    {
        payloadOut["retCode"] = false;
        return payloadOut;
    }

    payloadOut["retCode"] = auth->attribRemove( {appName,JSON_ASSTRING(payload,"attribName","")});
    return payloadOut;
}

json FullAuth::attribGroupAdd(void *, Mantids::Authentication::Manager *auth,Mantids::Authentication::Session *session, const json &payload)
{
    json payloadOut;
    payloadOut["retCode"] = auth->attribGroupAdd( {JSON_ASSTRING(payload,"appName",""),JSON_ASSTRING(payload,"attribName","")},JSON_ASSTRING(payload,"groupName",""));
    return payloadOut;
}

json FullAuth::attribGroupRemove(void *, Mantids::Authentication::Manager *auth,Mantids::Authentication::Session *session, const json &payload)
{
    json payloadOut;
    payloadOut["retCode"] = auth->attribGroupRemove( {JSON_ASSTRING(payload,"appName",""),JSON_ASSTRING(payload,"attribName","")},JSON_ASSTRING(payload,"groupName",""));
    return payloadOut;
}

json FullAuth::attribAccountAdd(void *, Mantids::Authentication::Manager *auth,Mantids::Authentication::Session *session, const json &payload)
{
    json payloadOut;
    payloadOut["retCode"] = auth->attribAccountAdd( {JSON_ASSTRING(payload,"appName",""),JSON_ASSTRING(payload,"attribName","")},JSON_ASSTRING(payload,"accountName",""));
    return payloadOut;
}

json FullAuth::attribAccountRemove(void *, Mantids::Authentication::Manager *auth,Mantids::Authentication::Session *session, const json &payload)
{
    json payloadOut;
    payloadOut["retCode"] = auth->attribAccountRemove( {JSON_ASSTRING(payload,"appName",""),JSON_ASSTRING(payload,"attribName","")},JSON_ASSTRING(payload,"accountName",""));
    return payloadOut;
}

json FullAuth::attribChangeDescription(void *, Mantids::Authentication::Manager *auth,Mantids::Authentication::Session *session, const json &payload)
{
    json payloadOut;

    std::string appName = JSON_ASSTRING(payload,"appName","");

    // Don't modify attribs from MANTIDSDIR:
    if ( appName == dirAppName )
    {
        payloadOut["retCode"] = false;
        return payloadOut;
    }

    payloadOut["retCode"] = auth->attribAccountRemove( {appName,JSON_ASSTRING(payload,"attribName","")},JSON_ASSTRING(payload,"attribDescription",""));
    return payloadOut;
}

json FullAuth::attribsList(void *, Mantids::Authentication::Manager *auth,Mantids::Authentication::Session *session, const json &payload)
{
    json payloadOut;
    payloadOut["attribsList"] = attribListToValue(auth->attribsList(JSON_ASSTRING(payload,"appName","")),auth);
    return payloadOut;
}

json FullAuth::attribGroups(void *, Mantids::Authentication::Manager *auth,Mantids::Authentication::Session *session, const json &payload)
{
    json payloadOut;
    payloadOut["attribGroups"] = stringListToValue(auth->attribGroups( {JSON_ASSTRING(payload,"appName",""),JSON_ASSTRING(payload,"attribName","")}));
    return payloadOut;
}

json FullAuth::attribAccounts(void *, Mantids::Authentication::Manager *auth,Mantids::Authentication::Session *session, const json &payload)
{
    json payloadOut;
    payloadOut["attribAccounts"] = stringListToValue(auth->attribAccounts( {JSON_ASSTRING(payload,"appName",""),JSON_ASSTRING(payload,"attribName","")}));
    return payloadOut;
}

json FullAuth::attribsBasicInfoSearch(void *obj, Authentication::Manager *auth, Authentication::Session *session, const json &payload)
{
    json payloadOut;
    int i=0;
    for (const auto & strVal : auth->attribsBasicInfoSearch(
             JSON_ASSTRING(payload,"appName",""),
             JSON_ASSTRING(payload,"searchWords",""),
             JSON_ASUINT64(payload,"limit",0),
             JSON_ASUINT64(payload,"offset",0)
             ))
    {
        payloadOut[i]["attribName"] = strVal.sAttributeName;
        payloadOut[i]["description"] = strVal.sDescription;
        i++;
    }
    return payloadOut;
}

json FullAuth::attribsLeftListForGroup(void *obj, Authentication::Manager *auth, Authentication::Session *session, const json &payload)
{
    json payloadOut;

    auto attribsLeft = iAttribsLeftListForGroup(auth,JSON_ASSTRING(payload,"appName",""),JSON_ASSTRING(payload,"groupName",""));

    payloadOut["attribsLeft"] = attribListToValue(attribsLeft,auth);

    return payloadOut;
}

json FullAuth::groupAdd(void *, Mantids::Authentication::Manager *auth,Mantids::Authentication::Session *session, const json &payload)
{
    json payloadOut;
    payloadOut["retCode"] = auth->groupAdd(JSON_ASSTRING(payload,"groupName",""), JSON_ASSTRING(payload,"groupDescription",""));
    return payloadOut;
}

json FullAuth::groupRemove(void *, Mantids::Authentication::Manager *auth,Mantids::Authentication::Session *session, const json &payload)
{
    json payloadOut;
    payloadOut["retCode"] = auth->groupRemove(JSON_ASSTRING(payload,"groupName",""));
    return payloadOut;
}

json FullAuth::groupExist(void *, Mantids::Authentication::Manager *auth,Mantids::Authentication::Session *session, const json &payload)
{
    json payloadOut;
    payloadOut["retCode"] = auth->groupExist(JSON_ASSTRING(payload,"groupName",""));
    return payloadOut;
}

json FullAuth::groupAccountAdd(void *, Mantids::Authentication::Manager *auth,Mantids::Authentication::Session *session, const json &payload)
{
    json payloadOut;
    payloadOut["retCode"] = auth->groupAccountAdd(JSON_ASSTRING(payload,"groupName",""),JSON_ASSTRING(payload,"accountName",""));
    return payloadOut;
}

json FullAuth::groupAccountRemove(void *, Mantids::Authentication::Manager *auth,Mantids::Authentication::Session *session, const json &payload)
{
    json payloadOut;
    payloadOut["retCode"] = auth->groupAccountRemove(JSON_ASSTRING(payload,"groupName",""),JSON_ASSTRING(payload,"accountName",""));
    return payloadOut;
}

json FullAuth::groupChangeDescription(void *, Mantids::Authentication::Manager *auth,Mantids::Authentication::Session *session, const json &payload)
{
    json payloadOut;
    payloadOut["retCode"] = auth->groupChangeDescription(JSON_ASSTRING(payload,"groupName",""),JSON_ASSTRING(payload,"groupDescription",""));
    return payloadOut;
}

json FullAuth::groupValidateAttribute(void *, Mantids::Authentication::Manager *auth,Mantids::Authentication::Session *session, const json &payload)
{
    json payloadOut;
    payloadOut["retCode"] = auth->groupValidateAttribute(JSON_ASSTRING(payload,"groupName",""), {JSON_ASSTRING(payload,"appName",""),JSON_ASSTRING(payload,"attribName","")});
    return payloadOut;
}

json FullAuth::groupDescription(void *, Mantids::Authentication::Manager *auth,Mantids::Authentication::Session *session, const json &payload)
{
    json payloadOut;
    payloadOut["description"] = auth->groupDescription(JSON_ASSTRING(payload,"groupName",""));
    return payloadOut;
}

json FullAuth::groupsList(void *, Mantids::Authentication::Manager *auth,Mantids::Authentication::Session *session, const json &payload)
{
    json payloadOut;
    payloadOut["groupsList"] = stringListToValue(auth->groupsList());
    return payloadOut;
}

json FullAuth::groupAttribs(void *, Mantids::Authentication::Manager *auth,Mantids::Authentication::Session *session, const json &payload)
{
    json payloadOut;
    payloadOut["groupAttribs"] = attribListToValue(auth->groupAttribs(JSON_ASSTRING(payload,"groupName","")),auth);
    return payloadOut;
}

json FullAuth::groupAccounts(void *, Mantids::Authentication::Manager *auth,Mantids::Authentication::Session *session, const json &payload)
{
    json payloadOut;
    payloadOut["groupAccounts"] = stringListToValue(auth->groupAccounts(JSON_ASSTRING(payload,"groupName","")));
    return payloadOut;
}

json FullAuth::groupsBasicInfoSearch(void *obj, Authentication::Manager *auth, Authentication::Session *session, const json &payload)
{
    json x;
    int i=0;
    for (const auto & strVal : auth->groupsBasicInfoSearch(
             JSON_ASSTRING(payload,"searchWords",""),
             JSON_ASUINT64(payload,"limit",0),
             JSON_ASUINT64(payload,"offset",0)
             ))
    {
        x[i]["description"] = strVal.sDescription;
        x[i]["groupName"] = strVal.sGroupName;
        i++;
    }
    return x;
}

json FullAuth::groupBasicInfo(void *obj, Authentication::Manager *auth, Authentication::Session *session, const json &payload)
{
    // TODO: optimize:

    json payloadOut;
    std::string groupName = JSON_ASSTRING(payload,"groupName","");

    payloadOut["description"] = auth->groupDescription(groupName);

    int i=0;
    auto groupAccounts = auth->groupAccounts(groupName);

    for (const auto & accountName : groupAccounts)
    {
        payloadOut["accounts"][i]["name"] = accountName;
        payloadOut["accounts"][i]["description"] = auth->accountDescription(accountName);
        payloadOut["accounts"][i]["lastName"] = auth->accountLastName(accountName);
        payloadOut["accounts"][i]["givenName"] = auth->accountGivenName(accountName);

        i++;
    }

    auto directAttribs = auth->groupAttribs(groupName);

    i=0;
    std::set<std::string> applications;
    for (const auto & attrib : directAttribs)
    {
        if (applications.find(attrib.appName) != applications.end())
            continue; // This application has been already mapped.

        applications.insert(attrib.appName);
        payloadOut["applications"][i]["name"] = attrib.appName;
        payloadOut["applications"][i]["description"] = auth->applicationDescription(attrib.appName);

        // Take the active application attribs for this group:
        int x=0;
        for (const auto & attrib2 : directAttribs)
        {
            if (attrib.appName == attrib2.appName)
            {
                payloadOut["applications"][i]["attribs"][x]["name"] = attrib.attribName;
                payloadOut["applications"][i]["attribs"][x]["description"] = auth->attribDescription(attrib);
                x++;
            }
        }
        // Take the unused application attribs for this group:

        x=0;
        for (const Mantids::Authentication::sApplicationAttrib & attrib : auth->attribsList(attrib.appName))
        {
            if (directAttribs.find(attrib)==directAttribs.end())
            {
                payloadOut["applications"][i]["attribsLeft"][x]["name"] = attrib.attribName;
                payloadOut["applications"][i]["attribsLeft"][x]["description"] = auth->attribDescription(attrib);
                x++;
            }
        }

        i++;
    }

    i=0;
    // put full application list:

    auto leftApplicationList = auth->applicationList();


    for (const auto & appName :auth->applicationList())
    {
        if ( !iAttribsLeftListForGroup(auth,appName,groupName ).empty() )
        {
            payloadOut["leftApplicationsList"][i]["name"] = appName;
            payloadOut["leftApplicationsList"][i]["description"] = auth->applicationDescription(appName);
            i++;
        }
    }



    return payloadOut;
}

std::map<std::string, std::string> FullAuth::jsonToMap(const json &jValue)
{
    std::map<std::string, std::string> r;
    for (const std::string & memberName : jValue.getMemberNames())
    {
        if (jValue[memberName].isString())
            r[memberName] = JSON_ASSTRING(jValue,memberName,"");
    }
    return r;
}

std::set<Authentication::sApplicationAttrib> FullAuth::iAttribsLeftListForGroup(Authentication::Manager *auth, const std::string &appName, const std::string &groupName)
{
    auto attribsLeft = auth->attribsList(appName);
    auto groupAttribs = auth->groupAttribs(groupName);

    for (const auto &groupAttrib : groupAttribs)
    {
        attribsLeft.erase(groupAttrib);
    }
    return attribsLeft;
}
