#pragma once

#include "engine.h"
#include <Mantids30/Server/WebRESTFul/api_restful_endpoints.h>
#include <Mantids30/Server/WebRESTFul/api_websocket_endpoints.h>
#include <Mantids30/DataFormat/JWT/jwt.h>
#include <Mantids30/Server/WebRESTFul/apiserver_clienthandler.h>
#include <cstdint>
#include <memory>

namespace Mantids30::Server::WebRESTFul {

class ClientHandler : public Mantids30::Server::WebRESTFul::APIServer_ClientHandler
{
public:
    ClientHandler(void *parent, const std::shared_ptr<Mantids30::Core::Memory::Streams::StreamableObject> &sock);

protected:
    Mantids30::Protocol::HTTP::Status::Code checkWebSocketRequestURI(const std::string &path) override;

    void handleWebSocketEvent(Mantids30::Protocol::HTTP::WebSocket::EventType, const Mantids30::Server::WebRESTFul::WebSocket::Endpoint *) override;
    /**
     * @brief sessionStart Retrieve/Start the session
     * @return S_200_OK for everything ok, any other value will return with that code immediately.
     */
    Mantids30::Protocol::HTTP::Status::Code sessionStart() override;
    /**
     * @brief sessionCleanUp Clean up / release the session when finishing all the processing...
     */
    void sessionCleanup() override;
    /**
     * @brief fillSessionExtraInfo Fill vars like session max age and other related data to the session...
     * @param jVars vars to be filled
     */
    void fillSessionExtraInfo(boost::json::value &jVars) override;
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
    boost::json::value getSessionVariableValue(const std::string &varName) override;
    /**
     * @brief handleAPIRequest Handle API Request and write the response to the client...
     * @return return code for api request
     */
    Mantids30::Protocol::HTTP::APIReturn handleAPIRequest(const std::string &baseApiUrl, const uint32_t &apiVersion, const std::string &methodMode, const std::string &endpointName,
                                    const boost::json::value &postParameters) override;

    Mantids30::Protocol::HTTP::APIReturn handleOptionsRequest(const std::string &baseApiUrl, const uint32_t &apiVersion, const std::string &endpointName) override;

    /**
     * @brief isSessionActive Check if the session is active (JWT is valid)
     * @return
     */
    [[nodiscard]] bool isSessionActive() override;
    /**
     * @brief getSessionScopes If session is active (Valid JWT), returns all the session scopes
     * @return
     */
    [[nodiscard]] std::set<std::string> getSessionScopes() override;
    /**
     * @brief getSessionRoles If session is active (Valid JWT), returns all the session roles
     * @return
     */
    [[nodiscard]] std::set<std::string> getSessionRoles() override;


    [[nodiscard]] bool isAdmin() override;


private:
    // API Version -> Endpoints:
    std::map<uint32_t, std::shared_ptr<Mantids30::Server::WebRESTFul::RESTful::Endpoints>> m_endpointsHandler;
    std::string jwtAccessTokenName;

    bool m_destroySession = false;
    bool m_isAuthorizationHeaderJWTVerified = false;
    bool m_isAccessTokenCookieJWTVerified = false;
    friend class Engine;
};

} // namespace Mantids30::Server::WebRESTFul
