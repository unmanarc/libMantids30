#include <Mantids30/DataFormats/JSON/json.h>
#include  "apiserver_clienthandler.h"
#include <Mantids30/Servers/RESTfulWebServer/api_websocket_connection.h>
#include <Mantids30/Core/Helpers/random.h>

using namespace Mantids30::Runtime::Logs;
using namespace Mantids30::Network;
using namespace Mantids30::Protocols;
using namespace Mantids30::Core::Memory;
using namespace Mantids30::Servers::RESTfulWebServer;
using namespace Mantids30;
using namespace std;

bool APIServer_ClientHandler::onWebSocketHTTPClientHeadersReceived()
{
    std::string requestURI = clientRequest.getURI();
    bool isAPIURI = false;

    if (!config->webServerName.empty())
    {
        serverResponse.setServerName(config->webServerName);
    }

    Mantids30::Protocols::HTTP::Status::Code rtmp;
    if ((rtmp = sessionStart()) != Mantids30::Protocols::HTTP::Status::Code::S_200_OK)
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
    if ((rtmp = checkWebSocketRequestURI(requestURI)) != Mantids30::Protocols::HTTP::Status::Code::S_200_OK)
    {
        sessionCleanup();
        serverResponse.status.setCode(rtmp);
        return false;
    }

    m_webSocketCurrentEndpoint = m_websocketEndpoints->getWebSocketEndpointByURI(clientRequest.getURI());

    size_t userConnectionCount = m_webSocketCurrentEndpoint->getActiveUserConnectionsCount(currentSessionInfo.authSession->getUser());
    if (userConnectionCount >= config->webSockets.maxConnectionsPerUserPerEndpoint)
    {
        sessionCleanup();
        serverResponse.status.setCode(Mantids30::Protocols::HTTP::Status::Code::S_429_TOO_MANY_REQUESTS);
        return false;
    }

    return true;
}

void APIServer_ClientHandler::onWebSocketConnectionEstablished()
{
    if (!m_webSocketCurrentEndpoint)
    {
        return;
    }

    m_webSocketSessionId = Mantids30::Core::Helpers::Random::createRandomString(16);

    Mantids30::Servers::RESTfulWebServer::WebSocket::WebSocketConnection *connection = new Mantids30::Servers::RESTfulWebServer::WebSocket::WebSocketConnection;
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
        boost::json::value jSessionId;

        Mantids30::DataFormats::JSON::setMember(jSessionId, "type", "session_id");
        Mantids30::DataFormats::JSON::setMember(jSessionId, "sessionId", m_webSocketSessionId);

        sendWebSocketText(boost::json::serialize(jSessionId));
    }

    handleWebSocketEvent(Mantids30::Protocols::HTTP::WebSocket::EventType::SESSION_START, m_webSocketCurrentEndpoint);
}

void APIServer_ClientHandler::onWebSocketBinaryDataFrameReceived()
{
    handleWebSocketEvent(Mantids30::Protocols::HTTP::WebSocket::EventType::RECEIVED_MESSAGE_BINARY, m_webSocketCurrentEndpoint);
}

void APIServer_ClientHandler::onWebSocketTextFrameReceived()
{
    handleWebSocketEvent(Mantids30::Protocols::HTTP::WebSocket::EventType::RECEIVED_MESSAGE_TEXT, m_webSocketCurrentEndpoint);
}

void APIServer_ClientHandler::onWebSocketPingReceived() {}

void APIServer_ClientHandler::onWebSocketPongReceived() {}

void APIServer_ClientHandler::onWebSocketConnectionFinished()
{
    handleWebSocketEvent(Mantids30::Protocols::HTTP::WebSocket::EventType::SESSION_END, m_webSocketCurrentEndpoint);
    if (m_webSocketCurrentEndpoint)
    {
        m_webSocketCurrentEndpoint->connectionsByIdMap->destroyElement(m_webSocketSessionId);
    }
    sessionCleanup();
}
