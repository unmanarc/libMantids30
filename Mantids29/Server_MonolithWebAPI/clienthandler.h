#ifndef WEBMONOLITH_CLIENT_HANDLER_H
#define WEBMONOLITH_CLIENT_HANDLER_H

#include <Mantids29/Auth/data.h>
#include "sessionsmanager.h"
#include "resourcesfilter.h"

#include <Mantids29/API_Core/streamablejson.h>
#include <Mantids29/API_Core/methodshandler.h>
#include <Mantids29/Auth/domains.h>
#include <Mantids29/Auth/multi.h>
#include <Mantids29/Protocol_HTTP/httpv1_server.h>

#include <Mantids29/Program_Logs/rpclog.h>
#include <mutex>

namespace Mantids29 { namespace Network { namespace Servers { namespace WebMonolith {

class ClientHandler : public Network::Protocols::HTTP::HTTPv1_Server
{
public:
    ClientHandler(void *parent, Memory::Streams::StreamableObject *sock);
    ~ClientHandler() override;

    //////////////////////////////////////////////
    // Initialization:
    void setAuthenticators(Mantids29::Authentication::Domains * authenticator);
    void setMethodsHandler(API::MethodsHandler *value);
    //////////////////////////////////////////////

    void setUserIP(const std::string &value);
    void setSessionsManagger(SessionsManager *value);
    void setUseFormattedJSONOutput(bool value);
    void setResourceFilter(ResourcesFilter *value);
    void setDocumentRootPath(const std::string &value);
    void setUsingCSRFToken(bool value);
    void setUseHTMLIEngine(bool value);
    void setRedirectPathOn404(const std::string &newRedirectOn404);

    void setWebServerName(const std::string &value);
    void setSoftwareVersion(const std::string &value);

    std::string getApplicationName() const;
    void setAppName(const std::string &value);
    void setRemoteTLSCN(const std::string &value);
    void setRPCLog(Program::Logs::RPCLog *value);

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

    void log(Mantids29::Program::Logs::eLogLevels logSeverity,  const std::string &module, const uint32_t &outSize, const char *fmtLog,... );

    Program::Logs::RPCLog * m_rpcLog;

    API::MethodsHandler * m_methodsHandler;
    Mantids29::Authentication::Domains * m_authDomains;
    SessionsManager * m_sessionsManager;

    // Current Session Vars:
    WebSession * m_webSession;
    Mantids29::Authentication::Session *m_authSession;
    uint64_t m_sessionMaxAge;
    std::string m_sessionId;
    bool m_destroySession;
    bool m_releaseSessionHandler;
    Authentication::Multi m_extraCredentials;
    Authentication::Data m_credentials;

    // Current User Security Vars:
    std::string m_clientCSRFToken;

    ResourcesFilter * m_resourceFilter;
    std::string m_applicationName;
    std::string m_userIP, m_userTLSCommonName;
    std::string m_resourcesLocalPath;
    std::string m_redirectPathOn404;
    bool m_useFormattedJSONOutput, m_usingCSRFToken, m_useHTMLIEngine;
    std::string m_webServerName;
    std::string m_softwareVersion;
};

}}}}

#endif // WEBMONOLITH_CLIENT_HANDLER_H
