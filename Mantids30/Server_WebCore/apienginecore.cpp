#include "apienginecore.h"

#include <Mantids30/Protocol_HTTP/httpv1_server.h>
#include <Mantids30/Net_Sockets/socket_tls.h>

#include <memory>

using namespace Mantids30::Network::Servers::Web;
using namespace Mantids30;

APIEngineCore::APIEngineCore()
{
    m_multiThreadedAcceptor = std::make_shared<Network::Sockets::Acceptors::MultiThreaded>();
    m_poolThreadedAcceptor = std::make_shared<Network::Sockets::Acceptors::PoolThreaded>();
}

void APIEngineCore::setAcceptPoolThreaded(
    const std::shared_ptr<Sockets::Socket_Stream> &listenerSocket, const uint32_t &threadCount, const uint32_t &taskQueues)
{
    m_poolThreadedAcceptor->setAcceptorSocket(listenerSocket);
    this->listenerSocket = listenerSocket;

    m_poolThreadedAcceptor->callbacks.setAllContexts(this);
    m_poolThreadedAcceptor->callbacks.onClientConnected = handleConnect;
    m_poolThreadedAcceptor->callbacks.onProtocolInitializationFailure = handleInitFailed;
    m_poolThreadedAcceptor->callbacks.onClientAcceptTimeoutOccurred = handleTimeOut;

    m_poolThreadedAcceptor->parameters.threadsCount = threadCount;
    m_poolThreadedAcceptor->parameters.taskQueues = taskQueues;

    // Set acceptor type
    m_acceptorType = AcceptorType::POOL_THREADED;
}

void APIEngineCore::setAcceptMultiThreaded(
    const std::shared_ptr<Sockets::Socket_Stream> &listenerSocket, const uint32_t &maxConcurrentConnections)
{
    m_multiThreadedAcceptor->setAcceptorSocket(listenerSocket);
    this->listenerSocket = listenerSocket;

    m_multiThreadedAcceptor->callbacks.setAllContexts(this);
    m_multiThreadedAcceptor->callbacks.onClientConnected = handleConnect;
    m_multiThreadedAcceptor->callbacks.onProtocolInitializationFailure = handleInitFailed;
    m_multiThreadedAcceptor->callbacks.onClientAcceptTimeoutOccurred = handleTimeOut;
    m_multiThreadedAcceptor->callbacks.onClientConnectionLimitPerIPReached = handleConnectionLimit;

    m_multiThreadedAcceptor->parameters.setMaxConcurrentClients(maxConcurrentConnections);

    // Set acceptor type
    m_acceptorType = AcceptorType::MULTI_THREADED;
}

void APIEngineCore::startInBackground()
{
    checkEngineStatus();

    // Start the appropriate acceptor based on the set mode
    switch (m_acceptorType)
    {
    case AcceptorType::MULTI_THREADED:
        m_multiThreadedAcceptor->startInBackground();
        break;
    case AcceptorType::POOL_THREADED:
        m_poolThreadedAcceptor->startInBackground();
        break;
    case AcceptorType::NONE:
        throw std::runtime_error("Acceptor type not defined in API Web Engine Core.");
        break;
    }
}

void APIEngineCore::setWebsocketEndpoints(const std::shared_ptr<API::WebSocket::Endpoints> &newWebsocketEndpoints)
{
    newWebsocketEndpoints->setTranslateTextMessagesToJSON(config.translateWebSocketTextMessagesToJSON);
    websocketEndpoints = newWebsocketEndpoints;
}

std::shared_ptr<Mantids30::Network::Sockets::Socket_Stream> APIEngineCore::getListenerSocket() const
{
    return listenerSocket;
}

bool APIEngineCore::handleVirtualConnection(std::shared_ptr<Sockets::Socket_Stream_Dummy> virtualConnection)
{
    return handleConnect(this,virtualConnection);
}

bool APIEngineCore::handleConnect(void *context, std::shared_ptr<Sockets::Socket_Stream> sock)
{
    APIEngineCore * webserver = static_cast<APIEngineCore *>(context);

    std::string tlsCN;
    if (sock->isSecure())
    {
        std::shared_ptr<Network::Sockets::Socket_TLS> tlsSock = std::dynamic_pointer_cast<Network::Sockets::Socket_TLS>( sock );
        tlsCN = tlsSock->getTLSPeerCN();
    }

    // Prepare the web services handler.
    std::shared_ptr<APIClientHandler> apiWebServerClientHandler = webserver->createNewAPIClientHandler(webserver,sock);

    apiWebServerClientHandler->m_websocketEndpoints = webserver->websocketEndpoints;

    apiWebServerClientHandler->clientRequest.networkClientInfo.setClientInformation( sock->getRemotePairStr(), sock->isSecure(), tlsCN );

    // Set the configuration:
    apiWebServerClientHandler->config = &(webserver->config);

    if (webserver->callbacks.onClientConnected.call(webserver,sock))
    {
        // Handle the webservice.
        Memory::Streams::Parser::ErrorMSG err;
        apiWebServerClientHandler->parseObject(&err);
    }

    return true;
}

bool APIEngineCore::handleInitFailed(void * context, std::shared_ptr<Sockets::Socket_Stream> s)
{
    APIEngineCore * webserver = static_cast<APIEngineCore *>(context);
    webserver->callbacks.onProtocolInitializationFailure.call(webserver,s);
    return true;
}

void APIEngineCore::handleTimeOut(void *context, std::shared_ptr<Sockets::Socket_Stream> s)
{
    APIEngineCore * webserver = static_cast<APIEngineCore *>(context);
    if (webserver->callbacks.onClientAcceptTimeoutOccurred.call(webserver,s))
    {
        s->writeString("HTTP/1.1 503 Service Temporarily Unavailable\r\n");
        s->writeString("Content-Type: text/html; charset=UTF-8\r\n");
        s->writeString("Connection: close\r\n");
        s->writeString("\r\n");
        s->writeString("<center><h1>503 Service Temporarily Unavailable</h1></center><hr>\r\n");
    }
}

void APIEngineCore::handleConnectionLimit(void *context, std::shared_ptr<Sockets::Socket_Stream> s)
{
    APIEngineCore * webserver = static_cast<APIEngineCore *>(context);
    if (webserver->callbacks.onClientAcceptTimeoutOccurred.call(webserver,s))
    {
        s->writeString("HTTP/1.1 503 Service Temporarily Unavailable\r\n");
        s->writeString("Content-Type: text/html; charset=UTF-8\r\n");
        s->writeString("Connection: close\r\n");
        s->writeString("\r\n");
        s->writeString("<center><h1>503 Service Temporarily Unavailable</h1></center><hr>\r\n");
    }
}
