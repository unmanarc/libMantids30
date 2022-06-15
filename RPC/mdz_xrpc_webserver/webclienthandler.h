#ifndef XRPC_SERVER_H
#define XRPC_SERVER_H

#include "sessionsmanager.h"
#include "resourcesfilter.h"

#include <mdz_xrpc_common/methodsmanager.h>
#include <mdz_auth/domains.h>
#include <mdz_proto_http/httpv1_server.h>
#include <mdz_xrpc_common/multiauths.h>

#include <mdz_prg_logs/rpclog.h>
#include <mutex>

namespace Mantids { namespace RPC { namespace Web {

class WebClientHandler : public Network::HTTP::HTTPv1_Server
{
public:
    WebClientHandler(void *parent, Memory::Streams::Streamable *sock);
    ~WebClientHandler() override;

    //////////////////////////////////////////////
    // Initialization:
    void setAuthenticators(Mantids::Authentication::Domains * authenticator);
    void setMethodsManager(MethodsManager *value);
    //////////////////////////////////////////////

    void setRemoteIP(const std::string &value);
    void setSessionsManagger(SessionsManager *value);
    void setUseFormattedJSONOutput(bool value);
    void setResourceFilter(ResourcesFilter *value);
    void setDocumentRootPath(const std::string &value);
    void setUsingCSRFToken(bool value);
    void setUseHTMLIEngine(bool value);
    void setRedirectOn404(const std::string &newRedirectOn404);

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
    Network::HTTP::Response::StatusCode processClientRequest() override;

private:
    Network::HTTP::Response::StatusCode processHTMLIEngine(const std::string &sRealFullPath,WebSession * hSession, uint64_t uMaxAge);
    Network::HTTP::Response::StatusCode processRPCRequest();
    Network::HTTP::Response::StatusCode processWebResource();
    Network::HTTP::Response::StatusCode processRPCRequest_VERSION();
    Network::HTTP::Response::StatusCode processRPCRequest_AUTHINFO(WebSession * wSession, const uint32_t & uMaxAge);
    Network::HTTP::Response::StatusCode processRPCRequest_CSRFTOKEN(WebSession * wSession);
    Network::HTTP::Response::StatusCode processRPCRequest_INITAUTH(const Authentication & auth, std::string sSessionId);
    Network::HTTP::Response::StatusCode processRPCRequest_POSTAUTH(const Authentication & auth, WebSession * wSession, bool * destroySession);
    Network::HTTP::Response::StatusCode processRPCRequest_EXEC(WebSession * wSession, MultiAuths *extraAuths);
    Network::HTTP::Response::StatusCode processRPCRequest_CHPASSWD(const Authentication &auth, WebSession * wSession, bool * destroySession);
    Network::HTTP::Response::StatusCode processRPCRequest_TESTPASSWD(const Authentication &auth, WebSession * wSession, bool * destroySession);
    Network::HTTP::Response::StatusCode processRPCRequest_PASSWDLIST(WebSession * wSession);

                   std::string persistentAuthentication(const std::string & userName, const std::string &domainName, const Authentication &authData, Mantids::Authentication::Session *session, Mantids::Authentication::Reason *authReason);
    Mantids::Authentication::Reason temporaryAuthentication(const std::string &userName, const std::string &domainName, const Authentication &authData);

    std::string getAuthSessionID(Mantids::Authentication::Session *authSession);

    void log(Mantids::Application::Logs::eLogLevels logSeverity, WebSession * wSession, const std::string &module, const uint32_t &outSize, const char *fmtLog,... );

    Application::Logs::RPCLog * rpcLog;

    MethodsManager * methodsManager;
    Mantids::Authentication::Domains * authDomains;
    SessionsManager * sessionsManager;
    ResourcesFilter * resourceFilter;
    std::string appName;
    std::string remoteIP, remoteTLSCN, remoteUserAgent;
    std::string resourcesLocalPath;
    std::string redirectOn404;
    bool useFormattedJSONOutput, usingCSRFToken, useHTMLIEngine;
    std::string webServerName;
    std::string softwareVersion;
};

}}}

#endif // XRPC_SERVER_H
