#include "clienthandler.h"

#include <string>

using namespace Mantids30::Servers::RESTfulWebServer;
using namespace Mantids30::Runtime::Logs;
using namespace Mantids30::Network;
using namespace Mantids30::Protocols;
using namespace Mantids30::DataFormats::JWT;
using namespace Mantids30::Core::Memory;
using namespace Mantids30;
using namespace std;

Mantids30::Protocols::HTTP::Status::Code ClientHandler::checkWebSocketRequestURI(const std::string &path)
{
    set<string> currentScopes;
    bool isAdmin = false;

    if (isSessionActive())
    {
        isAdmin = jwtToken.isAdmin();
        currentScopes = jwtToken.getAllScopes();
    }

    Mantids30::Servers::RESTfulWebServer::WebSocket::Endpoints::SecurityParameters securityParameters;
    securityParameters.haveJWTAuthCookie = m_isAccessTokenCookieJWTVerified;
    securityParameters.haveJWTAuthHeader = m_isAuthorizationHeaderJWTVerified;

    switch (m_websocketEndpoints->checkEndpoint(path, currentScopes, isAdmin, securityParameters))
    {
    case Mantids30::Servers::RESTfulWebServer::WebSocket::Endpoints::HandleResult::SUCCESS:
        return Mantids30::Protocols::HTTP::Status::Code::S_200_OK;
    case Mantids30::Servers::RESTfulWebServer::WebSocket::Endpoints::HandleResult::INVALID_EVENT_TYPE:
        return Mantids30::Protocols::HTTP::Status::Code::S_500_INTERNAL_SERVER_ERROR;
    case Mantids30::Servers::RESTfulWebServer::WebSocket::Endpoints::HandleResult::ENDPOINT_NOT_FOUND:
        return Mantids30::Protocols::HTTP::Status::Code::S_404_NOT_FOUND;
    case Mantids30::Servers::RESTfulWebServer::WebSocket::Endpoints::HandleResult::AUTHENTICATION_REQUIRED:
        return Mantids30::Protocols::HTTP::Status::Code::S_401_UNAUTHORIZED;
    case Mantids30::Servers::RESTfulWebServer::WebSocket::Endpoints::HandleResult::INVALID_SCOPE:
        return Mantids30::Protocols::HTTP::Status::Code::S_403_FORBIDDEN;
    case Mantids30::Servers::RESTfulWebServer::WebSocket::Endpoints::HandleResult::INTERNAL_ERROR:
    default:
        return Mantids30::Protocols::HTTP::Status::Code::S_500_INTERNAL_SERVER_ERROR;
        break;
    }
    return Mantids30::Protocols::HTTP::Status::Code::S_500_INTERNAL_SERVER_ERROR;
}

void ClientHandler::handleWebSocketEvent(Mantids30::Protocols::HTTP::WebSocket::EventType eventType, const Mantids30::Servers::RESTfulWebServer::WebSocket::Endpoint *currentWebSocketEndpoint)
{
    Mantids30::Servers::RESTfulWebServer::WebSocket::WebSocketParameters inputParameters;

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
