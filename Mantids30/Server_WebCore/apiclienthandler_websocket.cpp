#include "Mantids30/Helpers/random.h"
#include "apiclienthandler.h"

#include <Mantids30/API_EndpointsAndSessions/api_websocket_connection.h>

#include "json/value.h"
#include <Mantids30/Helpers/random.h>

using namespace Mantids30::Program::Logs;
using namespace Mantids30::Network;
using namespace Mantids30::Network::Protocols;
using namespace Mantids30::Memory;
using namespace Mantids30::Network::Servers::Web;
using namespace Mantids30;
using namespace std;

bool APIClientHandler::onWebSocketHTTPClientHeadersReceived()
{
    std::string requestURI = clientRequest.getURI();
    bool isAPIURI = false;

    if (!config->webServerName.empty())
    {
        serverResponse.setServerName(config->webServerName);
    }

    HTTP::Status::Codes rtmp;
    if ((rtmp = sessionStart()) != HTTP::Status::S_200_OK)
    {
        serverResponse.status.setCode(rtmp);
        return false;
    }

    logUsername.clear();
    if (currentSessionInfo.authSession)
    {
        logUsername = currentSessionInfo.authSession->getUser();
    }

    // Session started here.
    if ((rtmp = checkWebSocketRequestURI(requestURI)) != HTTP::Status::S_200_OK)
    {
        sessionCleanup();
        serverResponse.status.setCode(rtmp);
        return false;
    }

    m_webSocketCurrentEndpoint = m_websocketEndpoints->getWebSocketEndpointByURI(clientRequest.getURI());

    size_t userConnectionCount = m_webSocketCurrentEndpoint->getActiveUserConnectionsCount( currentSessionInfo.authSession->getUser() );
    if (  userConnectionCount >= config->webSockets.maxConnectionsPerUserPerEndpoint )
    {
        sessionCleanup();
        serverResponse.status.setCode(HTTP::Status::S_429_TOO_MANY_REQUESTS);
        return false;
    }

    return true;
}

void APIClientHandler::onWebSocketConnectionEstablished()
{
    if (!m_webSocketCurrentEndpoint)
    {
        return;
    }

    m_webSocketSessionId = Mantids30::Helpers::Random::createRandomString(16);



    API::WebSocket::WebSocketConnection *connection = new API::WebSocket::WebSocketConnection;
    connection->webSocketHTTPServer = this;
    connection->sessionInfo = &currentSessionInfo;
    if (!m_webSocketCurrentEndpoint->connectionsByIdMap->addElement(m_webSocketSessionId, connection))
    {
        // This should not happen.
        delete connection;
        throw std::runtime_error("Web Socket ID is repeated. This should not happen. Reseting");
    }


    if (config->webSockets.sendWebSocketSessionIDAtConnection)
    {
        Json::Value jSessionId;

        jSessionId["type"] = "session_id";
        jSessionId["sessionId"] = m_webSocketSessionId;

        sendWebSocketText(jSessionId.toStyledString());
    }

    handleWebSocketEvent(Network::Protocols::WebSocket::EventType::SESSION_START, m_webSocketCurrentEndpoint);
}

void APIClientHandler::onWebSocketBinaryDataFrameReceived()
{
    handleWebSocketEvent(Network::Protocols::WebSocket::EventType::RECEIVED_MESSAGE_BINARY, m_webSocketCurrentEndpoint);
}

void APIClientHandler::onWebSocketTextFrameReceived()
{
    handleWebSocketEvent(Network::Protocols::WebSocket::EventType::RECEIVED_MESSAGE_TEXT, m_webSocketCurrentEndpoint);
}

void APIClientHandler::onWebSocketPingReceived() {}

void APIClientHandler::onWebSocketPongReceived() {}

void APIClientHandler::onWebSocketConnectionFinished()
{
    handleWebSocketEvent(Network::Protocols::WebSocket::EventType::SESSION_END, m_webSocketCurrentEndpoint);
    if (m_webSocketCurrentEndpoint)
    {
        m_webSocketCurrentEndpoint->connectionsByIdMap->destroyElement(m_webSocketSessionId);
    }
    sessionCleanup();
}
