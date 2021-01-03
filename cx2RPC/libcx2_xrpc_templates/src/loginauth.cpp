#include "loginauth.h"

void CX2::RPC::Templates::LoginAuth::AddLoginAuthMethods(CX2::Authentication::Manager *auth, CX2::RPC::Fast::FastRPC *fastRPC)
{
    fastRPC->addMethod("authenticate",{&authenticate,auth});
    fastRPC->addMethod("accountChangeSecret",{&accountChangeSecret,auth});
    fastRPC->addMethod("accountAdd",{&accountAdd,auth});
    fastRPC->addMethod("attribExist",{&attribExist,auth});
}

Json::Value CX2::RPC::Templates::LoginAuth::authenticate(void * obj, const Json::Value &payload)
{
    CX2::Authentication::Manager * auth = (CX2::Authentication::Manager *)obj;
    Json::Value payloadOut;

    payloadOut["retCode"] = (uint32_t)
            auth->authenticate(payload["accountName"].asString(),
            payload["password"].asString(),
            payload["passIndex"].asUInt(),
            CX2::Authentication::getAuthModeFromString(payload["authMode"].asString()),
            payload["challengeSalt"].asString());

    payloadOut["retMessage"] = CX2::Authentication::getReasonText((CX2::Authentication::Reason)payloadOut["retCode"].asUInt());

    return payloadOut;
}

Json::Value CX2::RPC::Templates::LoginAuth::accountChangeSecret(void * obj, const Json::Value &payload)
{
    CX2::Authentication::Manager * auth = (CX2::Authentication::Manager *)obj;

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
                payload["challengeSalt"].asString(),
                newSecret,
                payload["passIndex"].asUInt());
    return payloadOut;

}

Json::Value CX2::RPC::Templates::LoginAuth::accountAdd(void * obj, const Json::Value &payload)
{
    CX2::Authentication::Manager * auth = (CX2::Authentication::Manager *)obj;

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

Json::Value CX2::RPC::Templates::LoginAuth::attribExist(void *obj, const Json::Value &payload)
{
    CX2::Authentication::Manager * auth = (CX2::Authentication::Manager *)obj;

    // This function is important to aplications to understand if they have been installed into the user manager
    Json::Value payloadOut;
    payloadOut["retCode"] = auth->attribExist( payload["attribName"].asString() ); // Superuser
    return payloadOut;
}

