#ifndef XRPC_SERVER_H
#define XRPC_SERVER_H

#include "sessionsmanager.h"

#include <cx2_xrpc_common/authentication.h>
#include <cx2_xrpc_common/methodsmanager.h>
#include <cx2_xrpc_common/request.h>

#include <cx2_auth/iauth_domains.h>
#include <cx2_netp_http/httpv1_server.h>

#include <mutex>

namespace CX2 { namespace RPC { namespace XRPCWeb {

class ClientHandler : public Network::Parsers::HTTPv1_Server
{
public:
    ClientHandler(void *parent, Memory::Streams::Streamable *sock);
    ~ClientHandler() override;

    //////////////////////////////////////////////
    // Initialization:
    void setAuthenticators(Authorization::IAuth_Domains * authenticator);
    void setMethodsManager(XRPC::MethodsManager *value);
    //////////////////////////////////////////////

    bool isValidHandShake() const;
    void setRemoteIP(const std::string &value);
    void setSessionsManagger(SessionsManager *value);

    bool getUseFormattedJSONOutput() const;
    void setUseFormattedJSONOutput(bool value);

protected:
    /**
     * @brief processClientRequest Process web client request
     * @return http responce code.
     */
    Network::Parsers::HttpRetCode processclientRequest() override;

private:
    Authorization::Session::IAuth_Session *persistentAuthentication(const std::string & userName, const std::string &domainName, const XRPC::Authentication &authData, Authorization::Session::IAuth_Session * session, Authorization::DataStructs::AuthReason *authReason);
    Authorization::DataStructs::AuthReason temporaryAuthentication(const XRPC::Authentication &authData, Authorization::Session::IAuth_Session *session);

   void sendAnswer(XRPC::Request *response, Memory::Streams::Streamable * out);

    XRPC::MethodsManager * methodsManager;
    Authorization::IAuth_Domains * authDomains;
    SessionsManager * sessionsManagger;
    std::string remoteIP;
    bool useFormattedJSONOutput;

};

}}}

#endif // XRPC_SERVER_H
