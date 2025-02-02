#include "apienginecore.h"
#include "apiclienthandler.h"

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

APIEngineCore::~APIEngineCore()
{
}

void APIEngineCore::acceptMultiThreaded(const std::shared_ptr<Network::Sockets::Socket_Stream_Base> & listenerSocket, const uint32_t &maxConcurrentConnections)
{
    checkEngineStatus();

    m_multiThreadedAcceptor->setAcceptorSocket(listenerSocket);

    // TODO: unificar con poolthreaded.
    m_multiThreadedAcceptor->callbacks.setAllContexts(this);
    m_multiThreadedAcceptor->callbacks.onConnect = _onConnect;
    m_multiThreadedAcceptor->callbacks.onInitFail = _onInitFailed;
    m_multiThreadedAcceptor->callbacks.onTimedOut = _onTimeOut;
    //m_multiThreadedAcceptor->callbacks.contextOnMaxConnectionsPerIP = ?? << todo

    m_multiThreadedAcceptor->setMaxConcurrentClients(maxConcurrentConnections);
    m_multiThreadedAcceptor->startInBackground();
}

void APIEngineCore::acceptPoolThreaded(const std::shared_ptr<Network::Sockets::Socket_Stream_Base> & listenerSocket, const uint32_t &threadCount, const uint32_t &threadMaxQueuedElements)
{
    checkEngineStatus();

    m_poolThreadedAcceptor->setAcceptorSocket(listenerSocket);

    m_multiThreadedAcceptor->callbacks.setAllContexts(this);
    m_multiThreadedAcceptor->callbacks.onConnect = _onConnect;
    m_multiThreadedAcceptor->callbacks.onInitFail = _onInitFailed;
    m_multiThreadedAcceptor->callbacks.onTimedOut = _onTimeOut;

    m_poolThreadedAcceptor->setThreadsCount(threadCount);
    m_poolThreadedAcceptor->setTaskQueues(threadMaxQueuedElements);
    m_poolThreadedAcceptor->startInBackground();
}

bool APIEngineCore::_onConnect(void *context, std::shared_ptr<Sockets::Socket_Stream_Base> s, const char *cUserIP, bool isSecure)
{
    APIEngineCore * webserver = ((APIEngineCore *)context);

    std::string tlsCN;
    if (s->isSecure())
    {
        std::shared_ptr<Network::Sockets::Socket_TLS> tlsSock = std::dynamic_pointer_cast<Network::Sockets::Socket_TLS>( s );
        tlsCN = tlsSock->getTLSPeerCN();
    }

    // Prepare the web services handler.
    auto webHandler = webserver->createNewAPIClientHandler(webserver,s);

    webHandler->setClientInfoVars( cUserIP, isSecure, tlsCN );

    // Set the configuration:
    webHandler->config = &(webserver->config);

    if (webserver->callbacks.onConnect.call(webserver,s,cUserIP,isSecure))
    {
        // Handle the webservice.
        Memory::Streams::Parser::ErrorMSG err;
        webHandler->parseObject(&err);
    }

    return true;
}

bool APIEngineCore::_onInitFailed(void * context, std::shared_ptr<Sockets::Socket_Stream_Base> s, const char * cUserIP, bool isSecure)
{
    APIEngineCore * webserver = ((APIEngineCore *)context);
    webserver->callbacks.onInitFailed.call(webserver,s,cUserIP,isSecure);
    return true;
}


void APIEngineCore::_onTimeOut(void *context, std::shared_ptr<Sockets::Socket_Stream_Base> s, const char * cUserIP, bool isSecure)
{
    APIEngineCore * webserver = ((APIEngineCore *)context);
    if (webserver->callbacks.onTimeOut.call(webserver,s,cUserIP,isSecure))
    {
        s->writeString("HTTP/1.1 503 Service Temporarily Unavailable\r\n");
        s->writeString("Content-Type: text/html; charset=UTF-8\r\n");
        s->writeString("Connection: close\r\n");
        s->writeString("\r\n");
        s->writeString("<center><h1>503 Service Temporarily Unavailable</h1></center><hr>\r\n");
    }
}


