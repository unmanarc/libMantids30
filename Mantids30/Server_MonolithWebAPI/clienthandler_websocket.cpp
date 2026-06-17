#include "clienthandler.h"

using namespace Mantids30::Network::Servers::WebMonolith;
using namespace Mantids30::Program::Logs;
using namespace Mantids30::Network;
using namespace Mantids30::Network::Protocol;
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
    securityParameters.haveJWTAuthCookie = false;
    securityParameters.haveJWTAuthHeader = false;
    securityParameters.haveSession = currentSessionInfo.authSession != nullptr;

    switch (m_websocketEndpoints->checkEndpoint(path, currentScopes, isAdmin, securityParameters))
    {
    case API::WebSocket::Endpoints::HandleResult::SUCCESS:
        return HTTP::Status::Code::S_200_OK;
    case API::WebSocket::Endpoints::HandleResult::INVALID_EVENT_TYPE:
        return HTTP::Status::Code::S_500_INTERNAL_SERVER_ERROR;
    case API::WebSocket::Endpoints::HandleResult::ENDPOINT_NOT_FOUND:
        return HTTP::Status::Code::S_404_NOT_FOUND;
    case API::WebSocket::Endpoints::HandleResult::AUTHENTICATION_REQUIRED:
        return HTTP::Status::Code::S_401_UNAUTHORIZED;
    case API::WebSocket::Endpoints::HandleResult::INVALID_SCOPE:
        return HTTP::Status::Code::S_403_FORBIDDEN;
    case API::WebSocket::Endpoints::HandleResult::INTERNAL_ERROR:
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
        inputParameters.session = currentSessionInfo.authSession;
    }

    m_websocketEndpoints->handleEvent(eventType, webSocketCurrentFrame.content.getContent(), inputParameters);
}
