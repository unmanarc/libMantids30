#include "webserver.h"
#include <cx2_netp_http/httpv1_server.h>
#include "webclienthandler.h"

using namespace CX2::RPC::Web;
using namespace CX2::RPC;
using namespace CX2;

WebServer::WebServer()
{
    resourceFilter = nullptr;
    obj = nullptr;
    sessionsManager.startGC( SessionsManager::threadGC, &sessionsManager );
    useFormattedJSONOutput = true;
    usingCSRFToken = true;
    useHTMLIEngine = true;
    authenticator = nullptr;
    methodManagers = nullptr;
}

WebServer::~WebServer()
{
}

void WebServer::acceptMultiThreaded(Network::Streams::StreamSocket *listenerSocket, const uint32_t &maxConcurrentConnections)
{
    if (!methodManagers) throw std::runtime_error("Don't Accept XRPC Web before setting some methodsmanager");
    if (!authenticator) throw std::runtime_error("Don't Accept XRPC Web before setting some authenticator");

    obj = this;
    multiThreadedAcceptor.setAcceptorSocket(listenerSocket);
    multiThreadedAcceptor.setCallbackOnConnect(_callbackOnConnect,obj);
    multiThreadedAcceptor.setCallbackOnInitFail(_callbackOnInitFailed,obj);
    multiThreadedAcceptor.setCallbackOnTimedOut(_callbackOnTimeOut,obj);
    multiThreadedAcceptor.setMaxConcurrentClients(maxConcurrentConnections);
    multiThreadedAcceptor.startThreaded();
}

void WebServer::acceptPoolThreaded(Network::Streams::StreamSocket *listenerSocket, const uint32_t &threadCount, const uint32_t &threadMaxQueuedElements)
{
    if (!methodManagers) throw std::runtime_error("Don't Accept XRPC Web before setting some methodsmanager");
    if (!authenticator) throw std::runtime_error("Don't Accept XRPC Web before setting some authenticator");

    obj = this;
    poolThreadedAcceptor.setAcceptorSocket(listenerSocket);
    poolThreadedAcceptor.setCallbackOnConnect(_callbackOnConnect,obj);
    poolThreadedAcceptor.setCallbackOnInitFail(_callbackOnInitFailed,obj);
    poolThreadedAcceptor.setCallbackOnTimedOut(_callbackOnTimeOut,obj);
    poolThreadedAcceptor.setThreadsCount(threadCount);
    poolThreadedAcceptor.setTaskQueues(threadMaxQueuedElements);
    poolThreadedAcceptor.start();
}

bool WebServer::_callbackOnConnect(void * obj, Network::Streams::StreamSocket * s, const char *remotePairIPAddr, bool isSecure)
{
    WebServer * webserver = ((WebServer *)obj);

    // Prepare the web services handler.
    WebClientHandler webHandler(webserver,s);

    webHandler.setIsSecure(isSecure);
    webHandler.setRemoteIP(remotePairIPAddr);
    webHandler.setMethodsManager(webserver->getMethodManagers());
    webHandler.setAuthenticators(webserver->getAuthenticator());
    webHandler.setSessionsManagger(webserver->getSessionsManager());
    webHandler.setUseFormattedJSONOutput(webserver->getUseFormattedJSONOutput());
    webHandler.setUsingCSRFToken(webserver->getUsingCSRFToken());
    webHandler.setResourceFilter(webserver->getResourceFilter());
    webHandler.setResourcesLocalPath(webserver->getResourcesLocalPath());
    webHandler.setWebServerName(webserver->getWebServerName());
    webHandler.setSoftwareVersion(webserver->getSoftwareVersion());
    webHandler.setUseHTMLIEngine(webserver->getUseHTMLIEngine());

    if (webserver->getExtCallBackOnConnect().call(obj,s,remotePairIPAddr,isSecure))
    {
        // Handle the webservice.
        Memory::Streams::Parsing::ParseErrorMSG err;
        webHandler.parseObject(&err);
    }
    return true;
}

bool WebServer::_callbackOnInitFailed(void * obj, Network::Streams::StreamSocket * s, const char * remotePairIPAddr, bool isSecure)
{
    WebServer * webserver = ((WebServer *)obj);
    webserver->getExtCallBackOnInitFailed().call(obj,s,remotePairIPAddr,isSecure);
    return true;
}

void WebServer::_callbackOnTimeOut(void * obj, Network::Streams::StreamSocket *s, const char * remotePairIPAddr, bool isSecure)
{
    WebServer * webserver = ((WebServer *)obj);
    if (webserver->getExtCallBackOnInitFailed().call(obj,s,remotePairIPAddr,isSecure))
    {
        s->writeString("HTTP/1.1 503 Service Temporarily Unavailable\r\n");
        s->writeString("Content-Type: text/html; charset=UTF-8\r\n");
        s->writeString("Connection: close\r\n");
        s->writeString("\r\n");
        s->writeString("<center><h1>503 Service Temporarily Unavailable</h1></center><hr>\r\n");
    }
}

bool WebServer::getUseHTMLIEngine() const
{
    return useHTMLIEngine;
}

void WebServer::setUseHTMLIEngine(bool value)
{
    useHTMLIEngine = value;
}

std::string WebServer::getWebServerName() const
{
    return webServerName;
}

void WebServer::setWebServerName(const std::string &value)
{
    webServerName = value;
}

std::string WebServer::getSoftwareVersion() const
{
    return softwareVersion;
}

void WebServer::setSoftwareVersion(const std::string &value)
{
    softwareVersion = value;
}

void WebServer::setSoftwareVersion(const uint32_t major, const uint32_t minor, const uint32_t subminor, const std::string & subText)
{
    setSoftwareVersion(std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(subminor) +  (subText.empty()? "" : (" " + subText))  );
}

void WebServer::setUsingCSRFToken(bool value)
{
    usingCSRFToken = value;
}

bool WebServer::getUsingCSRFToken() const
{
    return usingCSRFToken;
}

sWebServerCallBack WebServer::getExtCallBackOnTimeOut() const
{
    return extCallBackOnTimeOut;
}

sWebServerCallBack WebServer::getExtCallBackOnInitFailed() const
{
    return extCallBackOnInitFailed;
}

sWebServerCallBack WebServer::getExtCallBackOnConnect() const
{
    return extCallBackOnConnect;
}

void WebServer::setExtCallBackOnTimeOut(const sWebServerCallBack &value)
{
    extCallBackOnTimeOut = value;
}

void WebServer::setExtCallBackOnInitFailed(const sWebServerCallBack &value)
{
    extCallBackOnInitFailed = value;
}

void WebServer::setExtCallBackOnConnect(const sWebServerCallBack &value)
{
    extCallBackOnConnect = value;
}

std::string WebServer::getResourcesLocalPath() const
{
    return resourcesLocalPath;
}

bool WebServer::setResourcesLocalPath(const std::string &value)
{
    if (access(value.c_str(), R_OK)) return false;
    resourcesLocalPath = value;
    return  true;
}

ResourcesFilter *WebServer::getResourceFilter() const
{
    return resourceFilter;
}

void WebServer::setResourceFilter(ResourcesFilter *value)
{
    resourceFilter = value;
}

bool WebServer::getUseFormattedJSONOutput() const
{
    return useFormattedJSONOutput;
}

void WebServer::setUseFormattedJSONOutput(bool value)
{
    useFormattedJSONOutput = value;
}

SessionsManager *WebServer::getSessionsManager()
{
    return &sessionsManager;
}

Authorization::IAuth_Domains *WebServer::getAuthenticator() const
{
    return authenticator;
}

MethodsManager *WebServer::getMethodManagers() const
{
    return methodManagers;
}

void WebServer::setMethodManagers(MethodsManager *value)
{
    methodManagers = value;
}

void WebServer::setAuthenticator(Authorization::IAuth_Domains *value)
{
    authenticator = value;
}

void WebServer::setObj(void *value)
{
    obj = value;

}
