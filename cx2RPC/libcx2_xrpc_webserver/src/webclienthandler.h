#ifndef XRPC_SERVER_H
#define XRPC_SERVER_H

#include "sessionsmanager.h"
#include "resourcesfilter.h"

#include <cx2_xrpc_common/methodsmanager.h>
#include <cx2_xrpc_common/request.h>

#include <cx2_auth/iauth_domains.h>
#include <cx2_netp_http/httpv1_server.h>

#include <mutex>

namespace CX2 { namespace RPC { namespace Web {

class WebClientHandler : public Network::HTTP::HTTPv1_Server
{
public:
    WebClientHandler(void *parent, Memory::Streams::Streamable *sock);
    ~WebClientHandler() override;

    //////////////////////////////////////////////
    // Initialization:
    void setAuthenticators(Authorization::IAuth_Domains * authenticator);
    void setMethodsManager(MethodsManager *value);
    //////////////////////////////////////////////

    void setRemoteIP(const std::string &value);
    void setSessionsManagger(SessionsManager *value);
    void setUseFormattedJSONOutput(bool value);
    void setResourceFilter(ResourcesFilter *value);
    void setResourcesLocalPath(const std::string &value);
    void setUsingCSRFToken(bool value);
    void setUseHTMLIEngine(bool value);

    void setWebServerName(const std::string &value);
    void setSoftwareVersion(const std::string &value);


protected:
    /**
     * @brief processClientRequest Process web client request
     * @return http responce code.
     */
    Network::HTTP::eHTTP_RetCode processClientRequest() override;

private:
    Network::HTTP::eHTTP_RetCode processHTMLIEngine(const std::string &sRealFullPath);
    Network::HTTP::eHTTP_RetCode processRPCRequest();
    Network::HTTP::eHTTP_RetCode processRPCRequest_VERSION();
    Network::HTTP::eHTTP_RetCode processRPCRequest_AUTHINFO(WebSession * wSession);
    Network::HTTP::eHTTP_RetCode processRPCRequest_CSRFTOKEN(WebSession * wSession);
    Network::HTTP::eHTTP_RetCode processRPCRequest_AUTH(Request * request, std::string sSessionId);
    Network::HTTP::eHTTP_RetCode processRPCRequest_EXEC(Authorization::Session::IAuth_Session * hSession, Request * request);


    std::string persistentAuthentication(const std::string & userName, const std::string &domainName, const Authentication &authData, Authorization::Session::IAuth_Session * session, Authorization::DataStructs::AuthReason *authReason);
    Authorization::DataStructs::AuthReason temporaryAuthentication(const Authentication &authData, Authorization::Session::IAuth_Session *session);

    MethodsManager * methodsManager;
    Authorization::IAuth_Domains * authDomains;
    SessionsManager * sessionsManager;
    ResourcesFilter * resourceFilter;
    std::string remoteIP;
    std::string resourcesLocalPath;
    bool useFormattedJSONOutput, usingCSRFToken, useHTMLIEngine;
    std::string webServerName;
    std::string softwareVersion;
};

}}}

#endif // XRPC_SERVER_H
