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

bool APIEngineCore::handleVirtualConnection(std::shared_ptr<Sockets::Socket_Stream_Dummy> virtualConnection)
{
    return handleConnect(this,virtualConnection);
}


void APIEngineCore::acceptMultiThreaded(const std::shared_ptr<Network::Sockets::Socket_Stream> & listenerSocket, const uint32_t &maxConcurrentConnections)
{
    checkEngineStatus();

    m_multiThreadedAcceptor->setAcceptorSocket(listenerSocket);

    m_multiThreadedAcceptor->callbacks.setAllContexts(this);
    m_multiThreadedAcceptor->callbacks.onClientConnected = handleConnect;
    m_multiThreadedAcceptor->callbacks.onProtocolInitializationFailure = handleInitFailed;
    m_multiThreadedAcceptor->callbacks.onClientAcceptTimeoutOccurred = handleTimeOut;
    m_multiThreadedAcceptor->callbacks.onClientConnectionLimitPerIPReached = handleConnectionLimit;

    m_multiThreadedAcceptor->parameters.setMaxConcurrentClients(maxConcurrentConnections);
    m_multiThreadedAcceptor->startInBackground();
}

void APIEngineCore::acceptPoolThreaded(const std::shared_ptr<Network::Sockets::Socket_Stream> & listenerSocket, const uint32_t &threadCount, const uint32_t &taskQueues)
{
    checkEngineStatus();

    m_poolThreadedAcceptor->setAcceptorSocket(listenerSocket);

    m_multiThreadedAcceptor->callbacks.setAllContexts(this);
    m_multiThreadedAcceptor->callbacks.onClientConnected = handleConnect;
    m_multiThreadedAcceptor->callbacks.onProtocolInitializationFailure = handleInitFailed;
    m_multiThreadedAcceptor->callbacks.onClientAcceptTimeoutOccurred = handleTimeOut;

    m_poolThreadedAcceptor->parameters.threadsCount = threadCount;
    m_poolThreadedAcceptor->parameters.taskQueues = taskQueues;

    m_poolThreadedAcceptor->startInBackground();
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

    apiWebServerClientHandler->setClientInfoVars( sock->getRemotePairStr().c_str(), sock->isSecure(), tlsCN );

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
