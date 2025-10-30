#pragma once


#include <Mantids30/Server_WebCore/apiclienthandler.h>
#include <Mantids30/Protocol_HTTP/rsp_status.h>
#include "sessionsmanager.h"

#include <Mantids30/Memory/streamablejson.h>
#include <Mantids30/API_Monolith/endpointshandler.h>

#include <Mantids30/Program_Logs/rpclog.h>
#include <memory>

namespace Mantids30 { namespace Network { namespace Servers { namespace WebMonolith {

class ClientHandler : public Servers::Web::APIClientHandler
{
public:
    ClientHandler(void *parent, std::shared_ptr<Memory::Streams::StreamableObject> sock);

protected:
    /**
     * @brief sessionStart Retrieve/Start the session
     * @return S_200_OK for everything ok, any other value will return with that code immediately.
     */
    Protocols::HTTP::Status::Codes sessionStart() override;
    /**
     * @brief sessionCleanUp Clean up / release the session when finishing all the processing...
     * @return S_200_OK for good cleaning.
     */
    void sessionCleanup() override;

    /**
     * @brief Handles an API request and writes the response to the client.
     *
     * This method processes an API request by interpreting the provided parameters,
     * executing the appropriate logic for the specified API version, method, and mode,
     * and preparing a response that is written back to the client.
     *
     * @param apiReturn Pointer to an object where the result of the API request will be stored.
     * @param baseApiUrl The base URL for the API, used to construct resource paths or endpoints.
     * @param apiVersion The version of the API being requested.
     * @param httpMethodMode The mode of the API method, such as GET, POST, PUT, DELETE, etc. (not used in monolith)
     * @param endpointName The name of the API method to execute.
     * @param postParameters A JSON object containing parameters sent in the POST body.
     *
     * @return Returns an appropriate API return code indicating success or the type of error encountered.
     */
    API::APIReturn handleAPIRequest(const std::string &baseApiUrl, const uint32_t &apiVersion, const std::string &httpMethodMode, const std::string &endpointName, const Json::Value &postParameters) override;

    /**
     * @brief handleAuthFunctions Handle API Authentication Functions (login, logout, etc) and write the response to the client...
     * @return return code for api request
     */
    Protocols::HTTP::Status::Codes handleAuthFunctions(const std::string & baseApiUrl,const std::string & authFunctionName) override;

    /**
     * @brief handleAuthFunctions Handle API Authentication Functions (login, logout, etc) and write the response to the client...
     * @return return code for api request
     */
    json handleAPIInfo(const std::string & baseApiUrl) override;

    /**
     * @brief doesSessionVariableExist check if a sesion variable exist.
     * @param varName variable name
     * @return return true if variable exist, else otherwise
     */
    bool doesSessionVariableExist( const std::string & varName ) override;
    /**
     * @brief getSessionVariableValue Get the session variable by name
     * @param varName variable name
     * @return return the session variable
     */
    json getSessionVariableValue( const std::string & varName  ) override;


    /**
     * @brief fillSessionExtraInfo Fill vars like session max age and other related data to the session...
     * @param jVars vars to be filled
     */
    void fillSessionExtraInfo( json & jVars ) override;

    bool isSessionActive() override;
    std::set<std::string> getSessionScopes() override;
    std::set<std::string> getSessionRoles() override;


private:
    void updateActivityOnImpersonatorSession();

    /**
     * This function sets a JavaScript-readable cookie named "jsSessionTimeout" to track the session's remaining time.
     * The cookie is configured to be secure (transmitted only over HTTPS), readable by client-side JavaScript, and set
     * with an expiration and maximum age based on the given `maxAge` parameter. The cookie's SameSite attribute is
     * set to "Strict" to enhance security by limiting cross-site requests.
     */
    void setJSSessionTimeOutCookie(const uint64_t & maxAge);

    /**
     * This function sets a JavaScript-readable cookie named "jsHalfIDCookie" that gives the half visible part of the session id
     * This part should be sent in the X-HalfSession-ID header for anti-csrf purposes during sessions.
     *
     * The cookie is configured to be secure (transmitted only over HTTPS), readable by client-side JavaScript, and set
     * with an expiration and maximum age based on the given `maxAge` parameter. The cookie's SameSite attribute is
     * set to "Strict" to enhance security by limiting cross-site requests.
     */
    void setJSSessionHalfIDCookie(const std::string &sessionID);


    void sessionLogout();

    Protocols::HTTP::Status::Codes handleAuthUpdateLastActivityFunction();
    Protocols::HTTP::Status::Codes handleAuthLoginFunction();
    Protocols::HTTP::Status::Codes handleAuthRetrieveInfoFunction();
    Protocols::HTTP::Status::Codes handleAuthLogoutFunction();

    bool validateSessionAntiCSRFMechanism();

    Program::Logs::RPCLog * m_rpcLog = nullptr;
    std::map<uint32_t,API::Monolith::Endpoints *> m_endpointsHandlerByAPIVersion;
    WebSessionsManager * m_sessionsManager = nullptr;

    // Current Session Vars:
    WebSession * m_currentWebSession = nullptr;
    //std::shared_ptr<Mantids30::Sessions::Session> m_session = nullptr;
    uint64_t m_sessionMaxAge = 0;
    std::string m_sessionID, m_impersonatorSessionID;
    bool m_destroySession = false;
    //bool m_isSessionLoaded = false;

    friend class Engine;
};

}}}}

