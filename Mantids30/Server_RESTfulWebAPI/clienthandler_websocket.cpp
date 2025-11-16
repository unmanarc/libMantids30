#include "clienthandler.h"

#include <json/config.h>
#include <string>

#include <string>
#include <json/json.h>

using namespace Mantids30::Network::Servers::RESTful;
using namespace Mantids30::Program::Logs;
using namespace Mantids30::Network;
using namespace Mantids30::Network::Protocols;
using namespace Mantids30::DataFormat;
using namespace Mantids30::Memory;
using namespace Mantids30;
using namespace std;

HTTP::Status::Codes ClientHandler::checkWebSocketRequestURI(const std::string &path)
{
    set<string> currentScopes;
    bool isAdmin = false;

    if ( isSessionActive() )
    {
        isAdmin = m_JWTToken.isAdmin();
        currentScopes = m_JWTToken.getAllScopes();
    }

    API::WebSocket::Endpoints::SecurityParameters securityParameters;
    securityParameters.haveJWTAuthCookie = m_JWTCookieTokenVerified;
    securityParameters.haveJWTAuthHeader = m_JWTHeaderTokenVerified;

    switch(m_websocketEndpoints.checkEndpoint(path,currentScopes,isAdmin,securityParameters))
    {
    case API::WebSocket::Endpoints::SUCCESS:
        return Protocols::HTTP::Status::Codes::S_200_OK;
    case API::WebSocket::Endpoints::INVALID_EVENT_TYPE:
        return Protocols::HTTP::Status::Codes::S_500_INTERNAL_SERVER_ERROR;
    case API::WebSocket::Endpoints::ENDPOINT_NOT_FOUND:
        return Protocols::HTTP::Status::Codes::S_404_NOT_FOUND;
    case API::WebSocket::Endpoints::AUTHENTICATION_REQUIRED:
        return Protocols::HTTP::Status::Codes::S_401_UNAUTHORIZED;
    case API::WebSocket::Endpoints::INVALID_SCOPE:
        return Protocols::HTTP::Status::Codes::S_403_FORBIDDEN;
    case API::WebSocket::Endpoints::INTERNAL_ERROR:
    default:
        return Protocols::HTTP::Status::Codes::S_500_INTERNAL_SERVER_ERROR;
        break;
    }
    return Protocols::HTTP::Status::Codes::S_500_INTERNAL_SERVER_ERROR;
}

void ClientHandler::handleWebSocketEvent(Protocols::WebSocket::EventType eventType)
{
    API::WebSocket::WebSocketParameters inputParameters;

    inputParameters.jwtSigner       = this->config->jwtSigner;
    inputParameters.jwtValidator    = this->config->jwtValidator;
    inputParameters.clientRequest   = &clientRequest;

    if ( isSessionActive() )
    {
        inputParameters.jwtToken = &m_JWTToken;
    }

    m_websocketEndpoints.handleEvent(eventType,webSocketCurrentFrame.content.getContent(),inputParameters);
}
