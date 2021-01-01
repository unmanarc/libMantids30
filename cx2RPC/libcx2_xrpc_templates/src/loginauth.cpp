#include "loginauth.h"

void CX2::RPC::Templates::LoginAuth::AddLoginAuthMethods(CX2::RPC::MethodsManager *methods)
{
    methods->addRPCMethod("authenticate", {"admin"}, {&authenticate,nullptr});
    methods->addRPCMethod("accountChangeSecret", {"admin"}, {&accountChangeSecret,nullptr});
    methods->addRPCMethod("accountAdd", {"admin"}, {&accountAdd,nullptr});
    methods->addRPCMethod("attribExist", {"admin"}, {&attribExist,nullptr});
}

Json::Value CX2::RPC::Templates::LoginAuth::authenticate(void *, CX2::Authentication::Manager *auth, CX2::Authentication::Session *, const Json::Value &payload)
{
    Json::Value payloadOut;

    payloadOut["retCode"] = (uint32_t)
            auth->authenticate(payload["accountName"].asString(),
            payload["password"].asString(),
            payload["passIndex"].asUInt(),
            CX2::Authentication::getAuthModeFromString(payload["authMode"].asString()),
            payload["cramSalt"].asString());

    payloadOut["retMessage"] = CX2::Authentication::getReasonText((CX2::Authentication::Reason)payloadOut["retCode"].asUInt());

    return payloadOut;
}

Json::Value CX2::RPC::Templates::LoginAuth::accountChangeSecret(void *, CX2::Authentication::Manager *auth, CX2::Authentication::Session *, const Json::Value &payload)
{
    Json::Value payloadOut;

    std::map<std::string,std::string> mNewSecret;
    for ( const auto & member : payload["newSecret"].getMemberNames() )
    {
        mNewSecret[member] = payload[member].asString();
    }

    CX2::Authentication::Secret newSecret;
    newSecret.fromMap(mNewSecret);

    payloadOut["retCode"] = auth->accountChangeAuthenticatedSecret(
                payload["accountName"].asString(),
                payload["currentPassword"].asString(),
                CX2::Authentication::getAuthModeFromString(payload["authMode"].asString()),
                payload["cramSalt"].asString(),
                newSecret,
                payload["passIndex"].asUInt());
    return payloadOut;

}

Json::Value CX2::RPC::Templates::LoginAuth::accountAdd(void *, CX2::Authentication::Manager *auth, CX2::Authentication::Session *, const Json::Value &payload)
{
    Json::Value payloadOut;

    std::map<std::string,std::string> mNewSecret;
    for ( const auto & member : payload["newSecret"].getMemberNames() )
    {
        mNewSecret[member] = payload[member].asString();
    }

    CX2::Authentication::Secret newSecret;
    newSecret.fromMap(mNewSecret);

    payloadOut["retCode"] =
            auth->accountAdd(   payload["accountName"].asString(),
                                newSecret,
                                payload["email"].asString(),
                                payload["description"].asString(),
                                payload["extraData"].asString(),
                                payload["expiration"].asUInt64(),
                                false,  // Disabled (will require manual activation)
                                true,   // Confirmed (TODO: Handle confirmation)
                                false); // Superuser
    return payloadOut;
}

Json::Value CX2::RPC::Templates::LoginAuth::attribExist(void *obj, CX2::Authentication::Manager *auth, CX2::Authentication::Session *session, const Json::Value &payload)
{
    // This function is important to aplications to understand if they have been installed into the user manager
    Json::Value payloadOut;
    payloadOut["retCode"] = auth->attribExist( payload["attribName"].asString() ); // Superuser
    return payloadOut;
}
