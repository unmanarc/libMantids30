#ifndef XRPC_SERVER_H
#define XRPC_SERVER_H

#include "sessionsmanager.h"
#include "resourcesfilter.h"

#include <cx2_xrpc_common/methodsmanager.h>
#include <cx2_auth/domains.h>
#include <cx2_netp_http/httpv1_server.h>
#include <cx2_xrpc_common/multiauths.h>

#include <cx2_prg_logs/rpclog.h>
#include <mutex>

namespace CX2 { namespace RPC { namespace Web {

class WebClientHandler : public Network::HTTP::HTTPv1_Server
{
public:
    WebClientHandler(void *parent, Memory::Streams::Streamable *sock);
    ~WebClientHandler() override;

    //////////////////////////////////////////////
    // Initialization:
    void setAuthenticators(CX2::Authentication::Domains * authenticator);
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


    std::string getAppName() const;
    void setAppName(const std::string &value);
    void setRemoteTLSCN(const std::string &value);

    void setRPCLog(Application::Logs::RPCLog *value);

protected:
    /**
     * @brief processClientRequest Process web client request
     * @return http responce code.
     */
    Network::HTTP::eHTTP_RetCode processClientRequest() override;

private:
    Network::HTTP::eHTTP_RetCode processHTMLIEngine(const std::string &sRealFullPath,WebSession * hSession);
    Network::HTTP::eHTTP_RetCode processRPCRequest();
    Network::HTTP::eHTTP_RetCode processRPCRequest_VERSION();
    Network::HTTP::eHTTP_RetCode processRPCRequest_AUTHINFO(WebSession * wSession, const uint32_t & uMaxAge);
    Network::HTTP::eHTTP_RetCode processRPCRequest_CSRFTOKEN(WebSession * wSession);
    Network::HTTP::eHTTP_RetCode processRPCRequest_INITAUTH(const Authentication & auth, std::string sSessionId);
    Network::HTTP::eHTTP_RetCode processRPCRequest_POSTAUTH(const Authentication & auth, WebSession * wSession, bool * destroySession);
    Network::HTTP::eHTTP_RetCode processRPCRequest_EXEC(WebSession * wSession, MultiAuths *extraAuths);
    Network::HTTP::eHTTP_RetCode processRPCRequest_CHPASSWD(const Authentication &auth, WebSession * wSession, bool * destroySession);
    Network::HTTP::eHTTP_RetCode processRPCRequest_TESTPASSWD(const Authentication &auth, WebSession * wSession, bool * destroySession);
    Network::HTTP::eHTTP_RetCode processRPCRequest_PASSWDLIST(WebSession * wSession);

                   std::string persistentAuthentication(const std::string & userName, const std::string &domainName, const Authentication &authData, CX2::Authentication::Session *session, CX2::Authentication::Reason *authReason);
    CX2::Authentication::Reason temporaryAuthentication(const std::string &userName, const std::string &domainName, const Authentication &authData);

    std::string getAuthSessionID(CX2::Authentication::Session *authSession);

    void log(CX2::Application::Logs::eLogLevels logSeverity, WebSession * wSession, const std::string &module, const uint32_t &outSize, const char *fmtLog,... );

    Application::Logs::RPCLog * rpcLog;

    MethodsManager * methodsManager;
    CX2::Authentication::Domains * authDomains;
    SessionsManager * sessionsManager;
    ResourcesFilter * resourceFilter;
    std::string appName;
    std::string remoteIP, remoteTLSCN, remoteUserAgent;
    std::string resourcesLocalPath;
    bool useFormattedJSONOutput, usingCSRFToken, useHTMLIEngine;
    std::string webServerName;
    std::string softwareVersion;
};

}}}

#endif // XRPC_SERVER_H
