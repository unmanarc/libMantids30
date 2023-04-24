#ifndef WEB_CLIENT_HANDLER_H
#define WEB_CLIENT_HANDLER_H

#include "resourcesfilter.h"

#include <Mantids29/Memory/streamablejson.h>
#include <Mantids29/Protocol_HTTP/httpv1_server.h>

#include <Mantids29/Program_Logs/rpclog.h>
#include <mutex>

namespace Mantids29 { namespace Network { namespace Servers { namespace Web {

class APIClientHandler : public Protocols::HTTP::HTTPv1_Server
{
public:
    APIClientHandler(void *parent, Memory::Streams::StreamableObject *sock);
    ~APIClientHandler() override;

    void setUserIP(const std::string &value);
    void setUseFormattedJSONOutput(bool value);
    void setResourcesFilter(API::Web::ResourcesFilter *value);
    void setDocumentRootPath(const std::string &value);
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
    Protocols::HTTP::Status::eRetCode procHTTPClientContent() override;
    /**
     * @brief sessionStart Retrieve/Start the session
     * @return S_200_OK for everything ok, any other value will return with that code immediatly.
     */
    virtual Protocols::HTTP::Status::eRetCode sessionStart() { return Protocols::HTTP::Status::S_200_OK; }
    /**
     * @brief sessionCleanUp Clean up / release the session when finishing all the processing...
     * @return S_200_OK for good cleaning.
     */
    virtual Protocols::HTTP::Status::eRetCode sessionCleanup() { return Protocols::HTTP::Status::S_200_OK; }
    /**
     * @brief sessionRenew Renew the session
     */
    virtual void sessionRenew() { }
    /**
     * @brief sessionFillVars Fill vars like csrf token, session max age and other related data to the session...
     * @param jVars vars to be filled
     */
    virtual void fillSessionVars( json & jVars ) {};
    /**
     * @brief fillUserDataVars Fill user data vars like username, domain, TLS Common Name...
     * @param jVars vars to be filled
     */
    virtual void fillUserDataVars ( json & jVars );
    /**
     * @brief doesSessionVariableExist check if a sesion variable exist.
     * @param varName variable name
     * @return return true if variable exist, else otherwise
     */
    virtual bool doesSessionVariableExist( const std::string & varName ) { return false; }
    /**
     * @brief getSessionVariableValue Get the session variable by name
     * @param varName variable name
     * @return return the session variable
     */
    virtual json getSessionVariableValue( const std::string & varName  ) { return {}; }
    /**
     * @brief handleAPIRequest Handle API Request and write the response to the client...
     * @return return code for api request
     */
    virtual Protocols::HTTP::Status::eRetCode handleAPIRequest(const std::string & baseApiUrl,const uint32_t & apiVersion, const std::string & resourceAndPathParameters) { return Protocols::HTTP::Status::S_404_NOT_FOUND; }

    struct ServerConfig {
        API::Web::ResourcesFilter * resourceFilter = nullptr;
        bool useFormattedJSONOutput = true;
        bool useHTMLIEngine = true;

        std::string applicationName;
        std::string redirectPathOn404;
        std::string resourcesLocalPath;
        std::string webServerName;
        std::string softwareVersion;
    };

    std::set<std::string> m_APIURLs;
    API::Web::UserData m_userData;
    ServerConfig m_config;

    void log(Mantids29::Program::Logs::eLogLevels logSeverity,  const std::string &module, const uint32_t &outSize, const char *fmtLog,... );

    Program::Logs::RPCLog * m_rpcLog = nullptr;

private:
    Protocols::HTTP::Status::eRetCode handleFileRequest();

    friend class HTMLIEngine;
};

}}}}

#endif // WEB_CLIENT_HANDLER_H
