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

    CX2::Authentication::sClientDetails clientDetails;
    clientDetails.sIPAddr = payload["clientDetails"]["ipAddr"].asString();
    clientDetails.sExtraData = payload["clientDetails"]["extraData"].asString();
    clientDetails.sTLSCommonName = payload["clientDetails"]["tlsCN"].asString();
    clientDetails.sUserAgent = payload["clientDetails"]["userAgent"].asString();

    payloadOut["retCode"] = (uint32_t) auth->authenticate(
                payload["appName"].asString(),
            clientDetails,
            payload["user"].asString(),
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

    CX2::Authentication::sClientDetails clientDetails;
    clientDetails.sIPAddr = payload["clientDetails"]["ipAddr"].asString();
    clientDetails.sExtraData = payload["clientDetails"]["extraData"].asString();
    clientDetails.sTLSCommonName = payload["clientDetails"]["tlsCN"].asString();
    clientDetails.sUserAgent = payload["clientDetails"]["userAgent"].asString();

    payloadOut["retCode"] = auth->accountChangeAuthenticatedSecret( payload["appName"].asString(),
            payload["user"].asString(),
            payload["passIndex"].asUInt(),
            payload["currentPassword"].asString(),
            newSecret,
            clientDetails,
            CX2::Authentication::getAuthModeFromString(payload["authMode"].asString()),
            payload["challengeSalt"].asString()
            );
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


    CX2::Authentication::sAccountDetails accountDetails;
    accountDetails.sDescription = payload["accountDetails"]["description"].asString();
    accountDetails.sEmail = payload["accountDetails"]["email"].asString();
    accountDetails.sExtraData = payload["accountDetails"]["extraData"].asString();
    accountDetails.sGivenName = payload["accountDetails"]["givenName"].asString();
    accountDetails.sLastName = payload["accountDetails"]["lastName"].asString();
    CX2::Authentication::sAccountAttribs accountAttribs;
    accountAttribs.canCreateAccounts = false;
    accountAttribs.canCreateApplications = false;
    accountAttribs.confirmed = true;
    accountAttribs.enabled = false;
    accountAttribs.superuser = false;


    payloadOut["retCode"] =
            auth->accountAdd(   payload["userName"].asString(),
                                newSecret,
                                accountDetails,
                                payload["expiration"].asUInt64(),
                                accountAttribs); // Superuser
    return payloadOut;
}

Json::Value CX2::RPC::Templates::LoginAuth::attribExist(void *obj, const Json::Value &payload)
{
    CX2::Authentication::Manager * auth = (CX2::Authentication::Manager *)obj;

    // This function is important to aplications to understand if they have been installed into the user manager
    Json::Value payloadOut;
    payloadOut["retCode"] = auth->attribExist( { payload["appName"].asString(), payload["attribName"].asString() } ); // Superuser
    return payloadOut;
}

