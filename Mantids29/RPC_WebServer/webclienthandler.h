#ifndef XRPC_SERVER_H
#define XRPC_SERVER_H

#include <Mantids29/Auth/data.h>
#include "sessionsmanager.h"
#include "resourcesfilter.h"

#include <Mantids29/RPC_Common/streamablejson.h>
#include <Mantids29/RPC_Common/methodsmanager.h>
#include <Mantids29/Auth/domains.h>
#include <Mantids29/Auth/multi.h>
#include <Mantids29/Protocol_HTTP/httpv1_server.h>

#include <Mantids29/Program_Logs/rpclog.h>
#include <mutex>

namespace Mantids29 { namespace RPC { namespace Web {

class WebClientHandler : public Network::Protocols::HTTP::HTTPv1_Server
{
public:
    WebClientHandler(void *parent, Memory::Streams::StreamableObject *sock);
    ~WebClientHandler() override;

    //////////////////////////////////////////////
    // Initialization:
    void setAuthenticators(Mantids29::Authentication::Domains * authenticator);
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
     * @brief procHTTPClientContent Process web client request
     * @return http response code.
     */
    Network::Protocols::HTTP::Status::eRetCode procHTTPClientContent() override;
private:
    void sessionOpen();
    void sessionRelease();
    void sessionDestroy();

    Network::Protocols::HTTP::Status::eRetCode procResource_File(Authentication::Multi *extraAuths);
    Network::Protocols::HTTP::Status::eRetCode procResource_HTMLIEngine(const std::string &sRealFullPath, Authentication::Multi *extraAuths);

    Network::Protocols::HTTP::Status::eRetCode procJAPI_Session();
    Network::Protocols::HTTP::Status::eRetCode procJAPI_Session_AUTHINFO();
    Network::Protocols::HTTP::Status::eRetCode procJAPI_Session_CSRFTOKEN();
    Network::Protocols::HTTP::Status::eRetCode procJAPI_Session_LOGIN(const Authentication::Data & auth);
    Network::Protocols::HTTP::Status::eRetCode procJAPI_Session_POSTLOGIN(const Authentication::Data & auth);
    Network::Protocols::HTTP::Status::eRetCode procJAPI_Session_CHPASSWD(const Authentication::Data &auth);
    Network::Protocols::HTTP::Status::eRetCode procJAPI_Session_TESTPASSWD(const Authentication::Data &auth);
    Network::Protocols::HTTP::Status::eRetCode procJAPI_Session_PASSWDLIST();

    Network::Protocols::HTTP::Status::eRetCode procJAPI_Exec( Authentication::Multi *extraAuths,
                                                       std::string sMethodName,
                                                       std::string sPayloadIn,
                                                       Memory::Streams::StreamableJSON * jPayloadOutStr = nullptr
                                                       );
    bool csrfValidate();

    Network::Protocols::HTTP::Status::eRetCode procJAPI_Version();


    std::string persistentAuthentication(const std::string & userName, const std::string &domainName, const Authentication::Data &authData, Mantids29::Authentication::Session *session, Mantids29::Authentication::Reason *authReason);
    Mantids29::Authentication::Reason temporaryAuthentication(const std::string &userName, const std::string &domainName, const Authentication::Data &authData);

    //std::string getAuthSessionID(Mantids29::Authentication::Session *authSession);

    void log(Mantids29::Application::Logs::eLogLevels logSeverity,  const std::string &module, const uint32_t &outSize, const char *fmtLog,... );

    Application::Logs::RPCLog * rpcLog;

    MethodsManager * methodsManager;
    Mantids29::Authentication::Domains * authDomains;
    SessionsManager * sessionsManager;

    // Current Session Vars:
    WebSession * webSession;
    Mantids29::Authentication::Session *authSession;
    uint64_t uSessionMaxAge;
    std::string sSessionId;
    bool bDestroySession;
    bool bReleaseSessionHandler;
    Authentication::Multi extraCredentials;
    Authentication::Data credentials;

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
