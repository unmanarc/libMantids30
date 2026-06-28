#pragma once

#include "engine.h"
#include <Mantids30/API_EndpointsAndSessions/api_restful_endpoints.h>
#include <Mantids30/API_EndpointsAndSessions/api_websocket_endpoints.h>
#include <Mantids30/DataFormat_JWT/jwt.h>
#include <Mantids30/Server_WebCore/apiserver_clienthandler.h>
#include <cstdint>
#include <memory>

namespace Mantids30::Network::Servers::RESTful {

class ClientHandler : public Servers::Web::APIServer_ClientHandler
{
public:
    ClientHandler(void *parent, const std::shared_ptr<Memory::Streams::StreamableObject> &sock);

protected:
    Protocol::HTTP::Status::Code checkWebSocketRequestURI(const std::string &path) override;

    void handleWebSocketEvent(Network::Protocol::WebSocket::EventType, const API::WebSocket::Endpoint *) override;
    /**
     * @brief sessionStart Retrieve/Start the session
     * @return S_200_OK for everything ok, any other value will return with that code immediately.
     */
    Protocol::HTTP::Status::Code sessionStart() override;
    /**
     * @brief sessionCleanUp Clean up / release the session when finishing all the processing...
     */
    void sessionCleanup() override;
    /**
     * @brief fillSessionExtraInfo Fill vars like session max age and other related data to the session...
     * @param jVars vars to be filled
     */
    void fillSessionExtraInfo(Json::Value &jVars) override;
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
    Json::Value getSessionVariableValue(const std::string &varName) override;
    /**
     * @brief handleAPIRequest Handle API Request and write the response to the client...
     * @return return code for api request
     */
    API::APIReturn handleAPIRequest(const std::string &baseApiUrl, const uint32_t &apiVersion, const std::string &methodMode, const std::string &endpointName,
                                    const Json::Value &postParameters) override;

    API::APIReturn handleOptionsRequest(const std::string &baseApiUrl, const uint32_t &apiVersion, const std::string &endpointName) override;

    /**
     * @brief isSessionActive Check if the session is active (JWT is valid)
     * @return
     */
    bool isSessionActive() override;
    /**
     * @brief getSessionScopes If session is active (Valid JWT), returns all the session scopes
     * @return
     */
    std::set<std::string> getSessionScopes() override;
    /**
     * @brief getSessionRoles If session is active (Valid JWT), returns all the session roles
     * @return
     */
    std::set<std::string> getSessionRoles() override;

private:
    // API Version -> Endpoints:
    std::map<uint32_t, std::shared_ptr<API::RESTful::Endpoints>> m_endpointsHandler;
    std::string jwtAccessTokenName;

    bool m_destroySession = false;
    bool m_isAuthorizationHeaderJWTVerified = false;
    bool m_isAccessTokenCookieJWTVerified = false;
    friend class Engine;
};

} // namespace Mantids30::Network::Servers::RESTful
