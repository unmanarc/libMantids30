#include "clienthandler.h"

#include <json/config.h>
#include <string>

#include <json/json.h>
#include <string>

using namespace Mantids30::Network::Servers::RESTful;
using namespace Mantids30::Program::Logs;
using namespace Mantids30::Network;
using namespace Mantids30::Network::Protocol;
using namespace Mantids30::DataFormat;
using namespace Mantids30::Memory;
using namespace Mantids30;
using namespace std;

HTTP::Status::Code ClientHandler::checkWebSocketRequestURI(const std::string &path)
{
    set<string> currentScopes;
    bool isAdmin = false;

    if (isSessionActive())
    {
        isAdmin = jwtToken.isAdmin();
        currentScopes = jwtToken.getAllScopes();
    }

    API::WebSocket::Endpoints::SecurityParameters securityParameters;
    securityParameters.haveJWTAuthCookie = m_isAccessTokenCookieJWTVerified;
    securityParameters.haveJWTAuthHeader = m_isAuthorizationHeaderJWTVerified;

    switch (m_websocketEndpoints->checkEndpoint(path, currentScopes, isAdmin, securityParameters))
    {
    case API::WebSocket::Endpoints::SUCCESS:
        return HTTP::Status::Code::S_200_OK;
    case API::WebSocket::Endpoints::INVALID_EVENT_TYPE:
        return HTTP::Status::Code::S_500_INTERNAL_SERVER_ERROR;
    case API::WebSocket::Endpoints::ENDPOINT_NOT_FOUND:
        return HTTP::Status::Code::S_404_NOT_FOUND;
    case API::WebSocket::Endpoints::AUTHENTICATION_REQUIRED:
        return HTTP::Status::Code::S_401_UNAUTHORIZED;
    case API::WebSocket::Endpoints::INVALID_SCOPE:
        return HTTP::Status::Code::S_403_FORBIDDEN;
    case API::WebSocket::Endpoints::INTERNAL_ERROR:
    default:
        return HTTP::Status::Code::S_500_INTERNAL_SERVER_ERROR;
        break;
    }
    return HTTP::Status::Code::S_500_INTERNAL_SERVER_ERROR;
}

void ClientHandler::handleWebSocketEvent(Protocol::WebSocket::EventType eventType, const API::WebSocket::Endpoint *currentWebSocketEndpoint)
{
    API::WebSocket::WebSocketParameters inputParameters;

    inputParameters.jwtSigner = this->config->jwtSigner;
    inputParameters.jwtValidator = this->config->jwtValidator;
    inputParameters.clientRequest = &clientRequest;
    inputParameters.webSocketSessionId = this->m_webSocketSessionId;
    inputParameters.currentWebSocketEndpoint = currentWebSocketEndpoint;

    if (isSessionActive())
    {
        inputParameters.jwtToken = &jwtToken;
    }

    m_websocketEndpoints->handleEvent(eventType, webSocketCurrentFrame.content.getContent(), inputParameters);
}
