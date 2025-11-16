#include "apiclienthandler.h"

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
    if (m_currentSessionInfo.authSession)
    {
        logUsername = m_currentSessionInfo.authSession->getUser();
    }

    // Session started here.
    if ( (rtmp=checkWebSocketRequestURI( requestURI ))!= HTTP::Status::S_200_OK )
    {
        sessionCleanup();
        serverResponse.status.setCode(rtmp);
        return false;
    }

    return true;
}

void APIClientHandler::onWebSocketConnectionEstablished()
{
    handleWebSocketEvent( Network::Protocols::WebSocket::EventType::SESSION_START );
}

void APIClientHandler::onWebSocketBinaryDataFrameReceived()
{
    handleWebSocketEvent( Network::Protocols::WebSocket::EventType::MESSAGE_RECEIVED );
}

void APIClientHandler::onWebSocketTextFrameReceived()
{
    handleWebSocketEvent( Network::Protocols::WebSocket::EventType::MESSAGE_RECEIVED );
}

void APIClientHandler::onWebSocketPingReceived()
{

}

void APIClientHandler::onWebSocketPongReceived()
{

}

void APIClientHandler::onWebSocketConnectionFinished()
{
    handleWebSocketEvent( Network::Protocols::WebSocket::EventType::SESSION_END );
    sessionCleanup();
}
