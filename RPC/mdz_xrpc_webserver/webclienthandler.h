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

    void setUserIP(const std::string &value);
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
    void sessionOpen();
    void sessionRelease();
    void sessionDestroy();

    Network::HTTP::Response::StatusCode procResource_File(MultiAuths *extraAuths);
    Network::HTTP::Response::StatusCode procResource_HTMLIEngine(const std::string &sRealFullPath, MultiAuths *extraAuths);

    Network::HTTP::Response::StatusCode procJAPI_Session();
    Network::HTTP::Response::StatusCode procJAPI_Session_AUTHINFO();
    Network::HTTP::Response::StatusCode procJAPI_Session_CSRFTOKEN();
    Network::HTTP::Response::StatusCode procJAPI_Session_LOGIN(const Authentication & auth);
    Network::HTTP::Response::StatusCode procJAPI_Session_POSTLOGIN(const Authentication & auth);
    Network::HTTP::Response::StatusCode procJAPI_Session_CHPASSWD(const Authentication &auth);
    Network::HTTP::Response::StatusCode procJAPI_Session_TESTPASSWD(const Authentication &auth);
    Network::HTTP::Response::StatusCode procJAPI_Session_PASSWDLIST();

    Network::HTTP::Response::StatusCode procJAPI_Exec( MultiAuths *extraAuths,
                                                       std::string sMethodName,
                                                       std::string sPayloadIn,
                                                       Memory::Streams::JSON_Streamable * jPayloadOutStr = nullptr
                                                       );
    bool csrfValidate();

    Network::HTTP::Response::StatusCode procJAPI_Version();


    std::string persistentAuthentication(const std::string & userName, const std::string &domainName, const Authentication &authData, Mantids::Authentication::Session *session, Mantids::Authentication::Reason *authReason);
    Mantids::Authentication::Reason temporaryAuthentication(const std::string &userName, const std::string &domainName, const Authentication &authData);

    //std::string getAuthSessionID(Mantids::Authentication::Session *authSession);

    void log(Mantids::Application::Logs::eLogLevels logSeverity,  const std::string &module, const uint32_t &outSize, const char *fmtLog,... );

    Application::Logs::RPCLog * rpcLog;

    MethodsManager * methodsManager;
    Mantids::Authentication::Domains * authDomains;
    SessionsManager * sessionsManager;

    // Current Session Vars:
    WebSession * webSession;
    Mantids::Authentication::Session *authSession;
    uint64_t uSessionMaxAge;
    std::string sSessionId;
    bool bDestroySession;
    bool bReleaseSessionHandler;
    MultiAuths extraCredentials;
    Authentication credentials;

    // Current User Security Vars:
    std::string sClientCSRFToken;



    ResourcesFilter * resourceFilter;
    std::string appName;
    std::string userIP, userTLSCommonName;
    std::string resourcesLocalPath;
    std::string redirectOn404;
    bool useFormattedJSONOutput, usingCSRFToken, useHTMLIEngine;
    std::string webServerName;
    std::string softwareVersion;
};

}}}

#endif // XRPC_SERVER_H
