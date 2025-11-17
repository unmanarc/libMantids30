#pragma once

#include "engine.h"
#include <Mantids30/API_EndpointsAndSessions/api_restful_endpoints.h>
#include <Mantids30/API_EndpointsAndSessions/api_websocket_endpoints.h>
#include <Mantids30/Server_WebCore/apiclienthandler.h>
#include <Mantids30/DataFormat_JWT/jwt.h>
#include <cstdint>
#include <memory>

namespace Mantids30::Network::Servers::RESTful {

class ClientHandler : public Servers::Web::APIClientHandler
{
public:
    ClientHandler(void *parent, std::shared_ptr<Memory::Streams::StreamableObject> sock);

protected:
    Protocols::HTTP::Status::Codes checkWebSocketRequestURI(const std::string & path) override;

    void handleWebSocketEvent( Network::Protocols::WebSocket::EventType, const API::WebSocket::Endpoint * ) override;
    /**
     * @brief sessionStart Retrieve/Start the session
     * @return S_200_OK for everything ok, any other value will return with that code immediately.
     */
    Protocols::HTTP::Status::Codes sessionStart() override;
    /**
     * @brief sessionCleanUp Clean up / release the session when finishing all the processing...
     */
    void sessionCleanup() override;
    /**
     * @brief fillSessionExtraInfo Fill vars like session max age and other related data to the session...
     * @param jVars vars to be filled
     */
    void fillSessionExtraInfo( json & jVars ) override;
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
     * @brief handleAPIRequest Handle API Request and write the response to the client...
     * @return return code for api request
     */
    API::APIReturn handleAPIRequest(const std::string &baseApiUrl, const uint32_t &apiVersion, const std::string &methodMode, const std::string & endpointName, const Json::Value &postParameters) override;

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
    std::map<uint32_t,std::shared_ptr<API::RESTful::Endpoints>> m_endpointsHandler;

    bool m_destroySession = false;
    bool m_JWTHeaderTokenVerified = false;
    bool m_JWTCookieTokenVerified = false;
    friend class Engine;

};

}

