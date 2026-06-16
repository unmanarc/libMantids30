#pragma once

#include "sessionsmanager.h"
#include <Mantids30/Protocol_HTTP/rsp_status.h>
#include <Mantids30/Server_WebCore/apiserver_clienthandler.h>

#include <Mantids30/API_EndpointsAndSessions/api_monolith_endpoints.h>
#include <Mantids30/API_EndpointsAndSessions/api_options_handler.h>
#include <Mantids30/API_EndpointsAndSessions/api_websocket_endpoints.h>
#include <Mantids30/Memory/streamable_json.h>

#include <Mantids30/Program_Logs/rpclog.h>
#include <memory>

#define IMPERSONATOR_SESSIONID_COOKIENAME "impersonatorSessionId"
#define CURRENT_SESSIONID_COOKIENAME "sessionId"

namespace Mantids30::Network::Servers::WebMonolith {

class ClientHandler : public Servers::Web::APIServer_ClientHandler
{
public:
    ClientHandler(void *parent, const std::shared_ptr<Memory::Streams::StreamableObject> &sock);

protected:
    /**
     * @brief sessionStart Retrieve/Start the session
     * @return S_200_OK for everything ok, any other value will return with that code immediately.
     */
    Protocol::HTTP::Status::Code sessionStart() override;
    /**
     * @brief sessionCleanUp Clean up / release the session when finishing all the processing...
     * @return S_200_OK for good cleaning.
     */
    void sessionCleanup() override;

    /**
     * @brief handleWebSocketEvent Handle Web Socket Event from the client
     * @return return code for api request
     */
    void handleWebSocketEvent(Network::Protocol::WebSocket::EventType, const API::WebSocket::Endpoint *) override;

    Protocol::HTTP::Status::Code checkWebSocketRequestURI(const std::string &path) override;

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
    API::APIReturn handleAPIRequest(const std::string &baseApiUrl, const uint32_t &apiVersion, const std::string &httpMethodMode, const std::string &endpointName,
                                    const Json::Value &postParameters) override;

    API::APIReturn handleOptionsRequest(const std::string &baseApiUrl, const uint32_t &apiVersion, const std::string &endpointName) override;

    /**
     * @brief handleAuthFunctions Handle API Authentication Functions (login, logout, etc) and write the response to the client...
     * @return return code for api request
     */
    Protocol::HTTP::Status::Code handleAuthFunctions(const std::string &baseApiUrl, const std::string &authFunctionName) override;

    /**
     * @brief handleAuthFunctions Handle API Authentication Functions (login, logout, etc) and write the response to the client...
     * @return return code for api request
     */
    json handleAPIInfo(const std::string &baseApiUrl) override;

    /**
     * @brief doesSessionVariableExist check if a sesion variable exist.
     * @param varName variable name
     * @return return true if variable exist, else otherwise
     */
    bool doesSessionVariableExist(const std::string &varName) override;
    /**
     * @brief getSessionVariableValue Get the session variable by name
     * @param varName variable name
     * @return return the session variable
     */
    json getSessionVariableValue(const std::string &varName) override;

    /**
     * @brief fillSessionExtraInfo Fill vars like session max age and other related data to the session...
     * @param jVars vars to be filled
     */
    void fillSessionExtraInfo(json &jVars) override;

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
    void setJSSessionTimeOutCookie(const uint64_t &maxAge);

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

    Protocol::HTTP::Status::Code handleAuthUpdateLastActivityFunction();
    Protocol::HTTP::Status::Code handleAuthLoginFunction();
    Protocol::HTTP::Status::Code handleAuthRetrieveInfoFunction();
    Protocol::HTTP::Status::Code handleAuthLogoutFunction();

    bool validateSessionAntiCSRFMechanism();

    Program::Logs::RPCLog *m_rpcLog = nullptr;

    std::map<uint32_t, API::Monolith::Endpoints *> m_endpointsHandlerByAPIVersion;

    WebSessionsManager *m_sessionsManager = nullptr;

    // Current Session Vars:
    WebSession *m_currentWebSession = nullptr;
    uint64_t m_sessionMaxAge = 0;
    std::string m_sessionID, m_impersonatorSessionID;
    bool m_destroySession = false;
    friend class Engine;
};

} // namespace Mantids30::Network::Servers::WebMonolith
