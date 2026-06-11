#include "apiserver_core.h"

#include <Mantids30/Protocol_HTTP/httpv1_server.h>
#include <Mantids30/Net_Sockets/socket_tls.h>

#include <memory>

using namespace Mantids30::Network::Servers::Web;
using namespace Mantids30;

APIServerCore::APIServerCore()
{
    m_multiThreadedAcceptor = std::make_shared<Network::Sockets::Acceptors::MultiThreaded>();
    m_poolThreadedAcceptor = std::make_shared<Network::Sockets::Acceptors::PoolThreaded>();
}

void APIServerCore::setAcceptPoolThreaded(
    const std::list<std::shared_ptr<Sockets::Socket_Stream>> &listenerSockets, const boost::property_tree::ptree &ptree)
{
    for (auto &socket : listenerSockets)
    {
        m_poolThreadedAcceptor->addAcceptorSocket(socket);
    }
    this->m_listenerSockets = listenerSockets;

    m_poolThreadedAcceptor->callbacks.setAllContexts(this);
    m_poolThreadedAcceptor->callbacks.onClientConnected = handleConnect;
    m_poolThreadedAcceptor->callbacks.onProtocolInitializationFailure = handleInitFailed;
    m_poolThreadedAcceptor->callbacks.onClientAcceptTimeoutOccurred = handleTimeOut;

    m_poolThreadedAcceptor->parameters.setConfig(ptree);

    // Set acceptor type
    m_acceptorType = AcceptorType::POOL_THREADED;
}

void APIServerCore::setAcceptMultiThreaded(
    const std::list<std::shared_ptr<Sockets::Socket_Stream>> &listenerSockets, const boost::property_tree::ptree &ptree)
{
    for (auto &socket : listenerSockets)
    {
        m_multiThreadedAcceptor->addAcceptorSocket(socket);
    }
    this->m_listenerSockets = listenerSockets;

    m_multiThreadedAcceptor->callbacks.setAllContexts(this);
    m_multiThreadedAcceptor->callbacks.onClientConnected = handleConnect;
    m_multiThreadedAcceptor->callbacks.onProtocolInitializationFailure = handleInitFailed;
    m_multiThreadedAcceptor->callbacks.onClientAcceptTimeoutOccurred = handleTimeOut;
    m_multiThreadedAcceptor->callbacks.onClientConnectionLimitPerIPReached = handleConnectionLimit;

    m_multiThreadedAcceptor->parameters.setConfig(ptree);

    // Set acceptor type
    m_acceptorType = AcceptorType::MULTI_THREADED;
}

void APIServerCore::startInBackground()
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

void APIServerCore::setWebsocketEndpoints(const std::shared_ptr<API::WebSocket::Endpoints> &newWebsocketEndpoints)
{
    m_websocketEndpoints = newWebsocketEndpoints;
    m_websocketEndpoints->config = &(this->config.webSockets);
}

std::list<std::shared_ptr<Mantids30::Network::Sockets::Socket_Stream>> APIServerCore::getListenerSockets() const
{
    return m_listenerSockets;
}

void APIServerCore::handleVirtualConnection(std::shared_ptr<Sockets::Socket_Stream_Dummy> virtualConnection)
{
    handleConnect(this,virtualConnection);
}

void APIServerCore::handleConnect(void *context, std::shared_ptr<Sockets::Socket_Stream> sock)
{
    APIServerCore * webserver = static_cast<APIServerCore *>(context);

    std::string tlsCN;
    if (sock->isSecure())
    {
        std::shared_ptr<Network::Sockets::Socket_TLS> tlsSock = std::dynamic_pointer_cast<Network::Sockets::Socket_TLS>( sock );
        tlsCN = tlsSock->getTLSPeerCN();
    }

    // Prepare the web services handler.
    std::shared_ptr<APIServer_ClientHandler> apiWebServerClientHandler = webserver->createNewAPIServer_ClientHandler(webserver,sock);
    // Assign endpoints:
    apiWebServerClientHandler->m_websocketEndpoints = webserver->m_websocketEndpoints;
    // Set client information:
    apiWebServerClientHandler->clientRequest.networkClientInfo.setClientInformation( sock->getRemotePairStr(), sock->isSecure(), tlsCN );
    // Set the configuration:
    apiWebServerClientHandler->config = &(webserver->config);

    // Callback on client connected.
    if (webserver->callbacks.onClientConnected.call(webserver,sock))
    {
        // Handle the Web Server.
        Memory::Streams::Parser::ErrorMSG err;
        apiWebServerClientHandler->parseObject(&err);
    }
}

void APIServerCore::handleInitFailed(void * context, std::shared_ptr<Sockets::Socket_Stream> s)
{
    APIServerCore * webserver = static_cast<APIServerCore *>(context);
    webserver->callbacks.onProtocolInitializationFailure.call(webserver,s);
}

void APIServerCore::handleTimeOut(void *context, std::shared_ptr<Sockets::Socket_Stream> s)
{
    APIServerCore * webserver = static_cast<APIServerCore *>(context);
    if (webserver->callbacks.onClientAcceptTimeoutOccurred.call(webserver,s))
    {
        s->writeString("HTTP/1.1 503 Service Temporarily Unavailable\r\n");
        s->writeString("Content-Type: text/html; charset=UTF-8\r\n");
        s->writeString("Connection: close\r\n");
        s->writeString("\r\n");
        s->writeString("<center><h1>503 Service Temporarily Unavailable</h1></center><hr>\r\n");
    }
}

void APIServerCore::handleConnectionLimit(void *context, std::shared_ptr<Sockets::Socket_Stream> s)
{
    APIServerCore * webserver = static_cast<APIServerCore *>(context);
    if (webserver->callbacks.onClientAcceptTimeoutOccurred.call(webserver,s))
    {
        s->writeString("HTTP/1.1 503 Service Temporarily Unavailable\r\n");
        s->writeString("Content-Type: text/html; charset=UTF-8\r\n");
        s->writeString("Connection: close\r\n");
        s->writeString("\r\n");
        s->writeString("<center><h1>503 Service Temporarily Unavailable</h1></center><hr>\r\n");
    }
}
