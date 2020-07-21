#include "server.h"
#include <cx2_netp_http/httpv1_server.h>
#include "clienthandler.h"

using namespace CX2::RPC::XRPCWeb;
using namespace CX2::RPC;
using namespace CX2;

Server::Server()
{
    obj = nullptr;
    sessionsManager.startGC( SessionsManager::threadGC, &sessionsManager );
}

Server::~Server()
{
}

void Server::acceptMultiThreaded(Network::Streams::StreamSocket *listenerSocket, const uint32_t &maxConcurrentConnections)
{
    obj = this;
    multiThreadedAcceptor.setAcceptorSocket(listenerSocket);
    multiThreadedAcceptor.setCallbackOnConnect(_callbackOnConnect,obj);
    multiThreadedAcceptor.setCallbackOnInitFail(_callbackOnInitFailed,obj);
    multiThreadedAcceptor.setCallbackOnTimedOut(_callbackOnTimedOut,obj);
    multiThreadedAcceptor.setMaxConcurrentClients(maxConcurrentConnections);
    multiThreadedAcceptor.startThreaded();
}

void Server::acceptPoolThreaded(Network::Streams::StreamSocket *listenerSocket, const uint32_t &threadCount, const uint32_t &threadMaxQueuedElements)
{
    obj = this;
    poolThreadedAcceptor.setAcceptorSocket(listenerSocket);
    poolThreadedAcceptor.setCallbackOnConnect(_callbackOnConnect,obj);
    poolThreadedAcceptor.setCallbackOnInitFail(_callbackOnInitFailed,obj);
    poolThreadedAcceptor.setCallbackOnTimedOut(_callbackOnTimedOut,obj);
    poolThreadedAcceptor.setThreadsCount(threadCount);
    poolThreadedAcceptor.setTaskQueues(threadMaxQueuedElements);
    poolThreadedAcceptor.start();
}

bool Server::_callbackOnConnect(void * obj, Network::Streams::StreamSocket * s, const char *remotePairIPAddr)
{
    Server * webserver = ((Server *)obj);

    // Prepare the web services handler.
    ClientHandler webHandler(webserver,s);
    webHandler.setRemoteIP(remotePairIPAddr);
    webHandler.setMethodsManager(webserver->getMethodManagers());
    webHandler.setAuthenticators(webserver->getAuthenticator());
    webHandler.setSessionsManagger(webserver->getSessionsManager());

    // Handle the webservice.
    Memory::Streams::Parsing::ParseErrorMSG err;
    webHandler.parseObject(&err);
    return true;
}

bool Server::_callbackOnInitFailed(void * obj, Network::Streams::StreamSocket *, const char * remotePairIPAddr)
{
//    XRPC_WebServer * webserver = ((XRPC_WebServer *)obj);
    return true;
}

void Server::_callbackOnTimedOut(void * obj, Network::Streams::StreamSocket *s, const char * remotePairIPAddr)
{
//    XRPC_WebServer * webserver = ((XRPC_WebServer *)obj);
    // TODO: logs?
    s->writeString("HTTP/1.1 503 Service Temporarily Unavailable\r\n");
    s->writeString("Content-Type: text/html; charset=UTF-8\r\n");
    s->writeString("Connection: close\r\n");
    s->writeString("\r\n");
    s->writeString("<center><h1>503 Service Temporarily Unavailable</h1></center><hr>\r\n");
}

SessionsManager *Server::getSessionsManager()
{
    return &sessionsManager;
}

Authorization::IAuth_Domains *Server::getAuthenticator() const
{
    return authenticator;
}

XRPC::MethodsManager *Server::getMethodManagers() const
{
    return methodManagers;
}

void Server::setMethodManagers(XRPC::MethodsManager *value)
{
    methodManagers = value;
}

void Server::setAuthenticator(Authorization::IAuth_Domains *value)
{
    authenticator = value;
}

void Server::setObj(void *value)
{
    obj = value;

}
