#include "fullauth.h"

using namespace CX2::RPC::Templates;
using namespace CX2;

void FullAuth::AddFullAuthMethods(MethodsManager *methods)
{
    methods->addRPCMethod("accountChangeSecret", {"admin"}, {&accountChangeSecret,nullptr});
    methods->addRPCMethod("accountRemove", {"admin"}, {&accountRemove,nullptr});
    methods->addRPCMethod("accountDisable", {"admin"}, {&accountDisable,nullptr});
    methods->addRPCMethod("accountConfirm", {"admin"}, {&accountConfirm,nullptr});
    methods->addRPCMethod("accountChangeDescription", {"admin"}, {&accountChangeDescription,nullptr});
    methods->addRPCMethod("accountChangeEmail", {"admin"}, {&accountChangeEmail,nullptr});
    methods->addRPCMethod("accountChangeExtraData", {"admin"}, {&accountChangeExtraData,nullptr});
    methods->addRPCMethod("accountChangeExpiration", {"admin"}, {&accountChangeExpiration,nullptr});

    methods->addRPCMethod("isAccountDisabled", {"admin"}, {&isAccountDisabled,nullptr});
    methods->addRPCMethod("isAccountConfirmed", {"admin"}, {&isAccountConfirmed,nullptr});
    methods->addRPCMethod("isAccountSuperUser", {"admin"}, {&isAccountSuperUser,nullptr});
    methods->addRPCMethod("accountDescription", {"admin"}, {&accountDescription,nullptr});
    methods->addRPCMethod("accountEmail", {"admin"}, {&accountEmail,nullptr});
    methods->addRPCMethod("accountExtraData", {"admin"}, {&accountExtraData,nullptr});
    methods->addRPCMethod("accountExpirationDate", {"admin"}, {&accountExpirationDate,nullptr});
    methods->addRPCMethod("isAccountExpired", {"admin"}, {&isAccountExpired,nullptr});
    methods->addRPCMethod("accountValidateAttribute", {"admin"}, {&accountValidateAttribute,nullptr});
    methods->addRPCMethod("accountsList", {"admin"}, {&accountsList,nullptr});
    methods->addRPCMethod("accountGroups", {"admin"}, {&accountGroups,nullptr});
    methods->addRPCMethod("accountDirectAttribs", {"admin"}, {&accountDirectAttribs,nullptr});
    methods->addRPCMethod("accountUsableAttribs", {"admin"}, {&accountUsableAttribs,nullptr});

    methods->addRPCMethod("attribAdd", {"admin"}, {&attribAdd,nullptr});
    methods->addRPCMethod("attribRemove", {"admin"}, {&attribRemove,nullptr});
    methods->addRPCMethod("attribGroupAdd", {"admin"}, {&attribGroupAdd,nullptr});
    methods->addRPCMethod("attribGroupRemove", {"admin"}, {&attribGroupRemove,nullptr});
    methods->addRPCMethod("attribAccountAdd", {"admin"}, {&attribAccountAdd,nullptr});
    methods->addRPCMethod("attribAccountRemove", {"admin"}, {&attribAccountRemove,nullptr});
    methods->addRPCMethod("attribChangeDescription", {"admin"}, {&attribChangeDescription,nullptr});
    methods->addRPCMethod("attribsList", {"admin"}, {&attribsList,nullptr});
    methods->addRPCMethod("attribGroups", {"admin"}, {&attribGroups,nullptr});
    methods->addRPCMethod("attribAccounts", {"admin"}, {&attribAccounts,nullptr});

    methods->addRPCMethod("groupAdd", {"admin"}, {&groupAdd,nullptr});
    methods->addRPCMethod("groupRemove", {"admin"}, {&groupRemove,nullptr});
    methods->addRPCMethod("groupExist", {"admin"}, {&groupExist,nullptr});
    methods->addRPCMethod("groupAccountAdd", {"admin"}, {&groupAccountAdd,nullptr});
    methods->addRPCMethod("groupAccountRemove", {"admin"}, {&groupAccountRemove,nullptr});
    methods->addRPCMethod("groupChangeDescription", {"admin"}, {&groupChangeDescription,nullptr});
    methods->addRPCMethod("groupValidateAttribute", {"admin"}, {&groupValidateAttribute,nullptr});
    methods->addRPCMethod("groupDescription", {"admin"}, {&groupDescription,nullptr});
    methods->addRPCMethod("groupsList", {"admin"}, {&groupsList,nullptr});
    methods->addRPCMethod("groupAttribs", {"admin"}, {&groupAttribs,nullptr});
    methods->addRPCMethod("groupAccounts", {"admin"}, {&groupAccounts,nullptr});
}

Json::Value FullAuth::accountChangeSecret(void *, CX2::Authentication::Manager *auth,CX2::Authentication::Session *session, const Json::Value &payload)
{
    Json::Value payloadOut;
    CX2::Authentication::Secret secretData;
    secretData.fromMap(jsonToMap(payload["secretData"]));
    payloadOut["retCode"] = auth->accountChangeSecret(payload["user"].asString(),  secretData, payload["passIndex"].asUInt());
    return payloadOut;
}

Json::Value FullAuth::accountRemove(void *, CX2::Authentication::Manager *auth,CX2::Authentication::Session *session, const Json::Value &payload)
{
    Json::Value payloadOut;
    payloadOut["retCode"] = auth->accountRemove(payload["user"].asString());
    return payloadOut;
}

Json::Value FullAuth::accountDisable(void *, CX2::Authentication::Manager *auth,CX2::Authentication::Session *session, const Json::Value &payload)
{
    Json::Value payloadOut;
    payloadOut["retCode"] = auth->accountDisable(payload["user"].asString(), payload["disabled"].asBool());
    return payloadOut;
}

Json::Value FullAuth::accountConfirm(void *, CX2::Authentication::Manager *auth,CX2::Authentication::Session *session, const Json::Value &payload)
{
    Json::Value payloadOut;
    payloadOut["retCode"] = auth->accountDisable(payload["user"].asString(), payload["disabled"].asBool());
    return payloadOut;
}

Json::Value FullAuth::accountChangeDescription(void *, CX2::Authentication::Manager *auth,CX2::Authentication::Session *session, const Json::Value &payload)
{
    Json::Value payloadOut;
    payloadOut["retCode"] = auth->accountChangeDescription(payload["user"].asString(), payload["description"].asString());
    return payloadOut;
}

Json::Value FullAuth::accountChangeEmail(void *, CX2::Authentication::Manager *auth,CX2::Authentication::Session *session, const Json::Value &payload)
{
    Json::Value payloadOut;
    payloadOut["retCode"] = auth->accountChangeEmail(payload["user"].asString(), payload["email"].asString());
    return payloadOut;
}

Json::Value FullAuth::accountChangeExtraData(void *, CX2::Authentication::Manager *auth,CX2::Authentication::Session *session, const Json::Value &payload)
{
    Json::Value payloadOut;
    payloadOut["retCode"] = auth->accountChangeExtraData(payload["user"].asString(), payload["extraData"].asString());
    return payloadOut;
}

Json::Value FullAuth::accountChangeExpiration(void *, CX2::Authentication::Manager *auth,CX2::Authentication::Session *session, const Json::Value &payload)
{
    Json::Value payloadOut;
    payloadOut["retCode"] = auth->accountChangeExpiration(payload["user"].asString(), payload["expiration"].asUInt64());
    return payloadOut;
}

Json::Value FullAuth::isAccountDisabled(void *, CX2::Authentication::Manager *auth,CX2::Authentication::Session *session, const Json::Value &payload)
{
    Json::Value payloadOut;
    payloadOut["disabled"] = auth->isAccountDisabled(payload["user"].asString());
    return payloadOut;
}

Json::Value FullAuth::isAccountConfirmed(void *, CX2::Authentication::Manager *auth,CX2::Authentication::Session *session, const Json::Value &payload)
{
    Json::Value payloadOut;
    payloadOut["confirmed"] = auth->isAccountConfirmed(payload["user"].asString());
    return payloadOut;
}

Json::Value FullAuth::isAccountSuperUser(void *, CX2::Authentication::Manager *auth,CX2::Authentication::Session *session, const Json::Value &payload)
{
    Json::Value payloadOut;
    payloadOut["superuser"] = auth->isAccountSuperUser(payload["user"].asString());
    return payloadOut;
}

Json::Value FullAuth::accountDescription(void *, CX2::Authentication::Manager *auth,CX2::Authentication::Session *session, const Json::Value &payload)
{
    Json::Value payloadOut;
    payloadOut["description"] = auth->accountDescription(payload["user"].asString());
    return payloadOut;
}

Json::Value FullAuth::accountEmail(void *, CX2::Authentication::Manager *auth,CX2::Authentication::Session *session, const Json::Value &payload)
{
    Json::Value payloadOut;
    payloadOut["email"] = auth->accountEmail(payload["user"].asString());
    return payloadOut;
}

Json::Value FullAuth::accountExtraData(void *, CX2::Authentication::Manager *auth,CX2::Authentication::Session *session, const Json::Value &payload)
{
    Json::Value payloadOut;
    payloadOut["extraData"] = auth->accountExtraData(payload["user"].asString());
    return payloadOut;
}

Json::Value FullAuth::accountExpirationDate(void *, CX2::Authentication::Manager *auth,CX2::Authentication::Session *session, const Json::Value &payload)
{
    Json::Value payloadOut;
    payloadOut["expirationDate"] = Json::Int64(auth->accountExpirationDate(payload["user"].asString()));
    return payloadOut;
}

Json::Value FullAuth::isAccountExpired(void *, CX2::Authentication::Manager *auth,CX2::Authentication::Session *session, const Json::Value &payload)
{
    Json::Value payloadOut;
    payloadOut["isAccountExpired"] = auth->isAccountExpired(payload["user"].asString());
    return payloadOut;
}

Json::Value FullAuth::accountValidateAttribute(void *, CX2::Authentication::Manager *auth,CX2::Authentication::Session *session, const Json::Value &payload)
{
    Json::Value payloadOut;
    payloadOut["isAccountExpired"] = auth->accountValidateAttribute(payload["user"].asString(),  {payload["appName"].asString(),payload["attribName"].asString()});
    return payloadOut;
}

Json::Value FullAuth::accountsList(void *, CX2::Authentication::Manager *auth,CX2::Authentication::Session *session, const Json::Value &payload)
{
    Json::Value payloadOut;
    payloadOut["accountsList"] = stringListToValue(auth->accountsList());
    return payloadOut;
}

Json::Value FullAuth::accountGroups(void *, CX2::Authentication::Manager *auth,CX2::Authentication::Session *session, const Json::Value &payload)
{
    Json::Value payloadOut;

    payloadOut["accountGroups"] = stringListToValue(auth->accountGroups(payload["user"].asString()));
    return payloadOut;
}

Json::Value FullAuth::accountDirectAttribs(void *, CX2::Authentication::Manager *auth,CX2::Authentication::Session *session, const Json::Value &payload)
{
    Json::Value payloadOut;

    payloadOut["accountDirectAttribs"] = attribListToValue(auth->accountDirectAttribs(payload["user"].asString()));
    return payloadOut;
}

Json::Value FullAuth::accountUsableAttribs(void *, CX2::Authentication::Manager *auth,CX2::Authentication::Session *session, const Json::Value &payload)
{
    Json::Value payloadOut;

    payloadOut["accountUsableAttribs"] = attribListToValue(auth->accountUsableAttribs(payload["user"].asString()));
    return payloadOut;
}

Json::Value FullAuth::attribAdd(void *, CX2::Authentication::Manager *auth,CX2::Authentication::Session *session, const Json::Value &payload)
{
    Json::Value payloadOut;

    payloadOut["retCode"] = auth->attribAdd({payload["appName"].asString(),payload["attribName"].asString()},
                                            payload["description"].asString());
    return payloadOut;
}

Json::Value FullAuth::attribRemove(void *, CX2::Authentication::Manager *auth,CX2::Authentication::Session *session, const Json::Value &payload)
{
    Json::Value payloadOut;

    payloadOut["retCode"] = auth->attribRemove( {payload["appName"].asString(),payload["attribName"].asString()});
    return payloadOut;
}

Json::Value FullAuth::attribGroupAdd(void *, CX2::Authentication::Manager *auth,CX2::Authentication::Session *session, const Json::Value &payload)
{
    Json::Value payloadOut;
    payloadOut["retCode"] = auth->attribGroupAdd( {payload["appName"].asString(),payload["attribName"].asString()},payload["groupName"].asString());
    return payloadOut;
}

Json::Value FullAuth::attribGroupRemove(void *, CX2::Authentication::Manager *auth,CX2::Authentication::Session *session, const Json::Value &payload)
{
    Json::Value payloadOut;
    payloadOut["retCode"] = auth->attribGroupRemove( {payload["appName"].asString(),payload["attribName"].asString()},payload["groupName"].asString());
    return payloadOut;
}

Json::Value FullAuth::attribAccountAdd(void *, CX2::Authentication::Manager *auth,CX2::Authentication::Session *session, const Json::Value &payload)
{
    Json::Value payloadOut;
    payloadOut["retCode"] = auth->attribAccountAdd( {payload["appName"].asString(),payload["attribName"].asString()},payload["user"].asString());
    return payloadOut;
}

Json::Value FullAuth::attribAccountRemove(void *, CX2::Authentication::Manager *auth,CX2::Authentication::Session *session, const Json::Value &payload)
{
    Json::Value payloadOut;
    payloadOut["retCode"] = auth->attribAccountRemove( {payload["appName"].asString(),payload["attribName"].asString()},payload["user"].asString());
    return payloadOut;
}

Json::Value FullAuth::attribChangeDescription(void *, CX2::Authentication::Manager *auth,CX2::Authentication::Session *session, const Json::Value &payload)
{
    Json::Value payloadOut;
    payloadOut["retCode"] = auth->attribAccountRemove( {payload["appName"].asString(),payload["attribName"].asString()},payload["attribDescription"].asString());
    return payloadOut;
}

Json::Value FullAuth::attribsList(void *, CX2::Authentication::Manager *auth,CX2::Authentication::Session *session, const Json::Value &payload)
{
    Json::Value payloadOut;
    payloadOut["attribsList"] = attribListToValue(auth->attribsList());
    return payloadOut;
}

Json::Value FullAuth::attribGroups(void *, CX2::Authentication::Manager *auth,CX2::Authentication::Session *session, const Json::Value &payload)
{
    Json::Value payloadOut;
    payloadOut["attribGroups"] = stringListToValue(auth->attribGroups( {payload["appName"].asString(),payload["attribName"].asString()}));
    return payloadOut;
}

Json::Value FullAuth::attribAccounts(void *, CX2::Authentication::Manager *auth,CX2::Authentication::Session *session, const Json::Value &payload)
{
    Json::Value payloadOut;
    payloadOut["attribAccounts"] = stringListToValue(auth->attribAccounts( {payload["appName"].asString(),payload["attribName"].asString()}));
    return payloadOut;
}

Json::Value FullAuth::groupAdd(void *, CX2::Authentication::Manager *auth,CX2::Authentication::Session *session, const Json::Value &payload)
{
    Json::Value payloadOut;
    payloadOut["retCode"] = auth->groupAdd(payload["groupName"].asString(), payload["groupDescription"].asString());
    return payloadOut;
}

Json::Value FullAuth::groupRemove(void *, CX2::Authentication::Manager *auth,CX2::Authentication::Session *session, const Json::Value &payload)
{
    Json::Value payloadOut;
    payloadOut["retCode"] = auth->groupRemove(payload["groupName"].asString());
    return payloadOut;
}

Json::Value FullAuth::groupExist(void *, CX2::Authentication::Manager *auth,CX2::Authentication::Session *session, const Json::Value &payload)
{
    Json::Value payloadOut;
    payloadOut["retCode"] = auth->groupExist(payload["groupName"].asString());
    return payloadOut;
}

Json::Value FullAuth::groupAccountAdd(void *, CX2::Authentication::Manager *auth,CX2::Authentication::Session *session, const Json::Value &payload)
{
    Json::Value payloadOut;
    payloadOut["retCode"] = auth->groupAccountAdd(payload["groupName"].asString(),payload["user"].asString());
    return payloadOut;
}

Json::Value FullAuth::groupAccountRemove(void *, CX2::Authentication::Manager *auth,CX2::Authentication::Session *session, const Json::Value &payload)
{
    Json::Value payloadOut;
    payloadOut["retCode"] = auth->groupAccountRemove(payload["groupName"].asString(),payload["user"].asString());
    return payloadOut;
}

Json::Value FullAuth::groupChangeDescription(void *, CX2::Authentication::Manager *auth,CX2::Authentication::Session *session, const Json::Value &payload)
{
    Json::Value payloadOut;
    payloadOut["retCode"] = auth->groupChangeDescription(payload["groupName"].asString(),payload["groupDescription"].asString());
    return payloadOut;
}

Json::Value FullAuth::groupValidateAttribute(void *, CX2::Authentication::Manager *auth,CX2::Authentication::Session *session, const Json::Value &payload)
{
    Json::Value payloadOut;
    payloadOut["retCode"] = auth->groupValidateAttribute(payload["groupName"].asString(), {payload["appName"].asString(),payload["attribName"].asString()});
    return payloadOut;
}

Json::Value FullAuth::groupDescription(void *, CX2::Authentication::Manager *auth,CX2::Authentication::Session *session, const Json::Value &payload)
{
    Json::Value payloadOut;
    payloadOut["retCode"] = auth->groupDescription(payload["groupName"].asString());
    return payloadOut;
}

Json::Value FullAuth::groupsList(void *, CX2::Authentication::Manager *auth,CX2::Authentication::Session *session, const Json::Value &payload)
{
    Json::Value payloadOut;
    payloadOut["groupsList"] = stringListToValue(auth->groupsList());
    return payloadOut;
}

Json::Value FullAuth::groupAttribs(void *, CX2::Authentication::Manager *auth,CX2::Authentication::Session *session, const Json::Value &payload)
{
    Json::Value payloadOut;
    payloadOut["groupAttribs"] = attribListToValue(auth->groupAttribs(payload["groupName"].asString()));
    return payloadOut;
}

Json::Value FullAuth::groupAccounts(void *, CX2::Authentication::Manager *auth,CX2::Authentication::Session *session, const Json::Value &payload)
{
    Json::Value payloadOut;
    payloadOut["groupAccounts"] = stringListToValue(auth->groupAccounts(payload["groupName"].asString()));
    return payloadOut;
}

std::map<std::string, std::string> FullAuth::jsonToMap(const Json::Value &jValue)
{
    std::map<std::string, std::string> r;
    for (const std::string & memberName : jValue.getMemberNames())
    {
        if (jValue[memberName].isString())
            r[memberName] = jValue[memberName].asString();
    }
    return r;
}
