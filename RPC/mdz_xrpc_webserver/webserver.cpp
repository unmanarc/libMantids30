#include "webserver.h"
#include <mdz_proto_http/httpv1_server.h>
#include <mdz_net_sockets/socket_tls.h>
#include "webclienthandler.h"

#include <stdexcept>

using namespace Mantids::RPC::Web;
using namespace Mantids::RPC;
using namespace Mantids;

WebServer::WebServer()
{
    rpcLog = nullptr;
    resourceFilter = nullptr;
    obj = nullptr;
    sessionsManager.startGC( SessionsManager::threadGC, &sessionsManager, "GC:WebSessions" );
    useFormattedJSONOutput = true;
    usingCSRFToken = true;
    useHTMLIEngine = true;
    authenticator = nullptr;
    methodManagers = nullptr;
    redirectOn404 = "";
}

WebServer::~WebServer()
{
    for (const auto & i : staticContentElements)
    {
        delete i.second;
    }
    for (const auto & i : memToBeFreed)
    {
        free(i);
    }

    if (resourceFilter)
        delete resourceFilter;
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

bool WebServer::_callbackOnConnect(void * obj, Network::Streams::StreamSocket * s, const char *cUserIP, bool isSecure)
{
    WebServer * webserver = ((WebServer *)obj);

    std::string tlsCN;
    if (s->isSecure())
    {
        Network::TLS::Socket_TLS * tlsSock = (Network::TLS::Socket_TLS *)s;
        tlsCN = tlsSock->getTLSPeerCN();
    }


    // Prepare the web services handler.
    WebClientHandler webHandler(webserver,s);

    webHandler.setRPCLog(webserver->getRPCLog());
    webHandler.setAppName(webserver->getAppName());
    webHandler.setIsSecure(isSecure);
    webHandler.setUserIP(cUserIP);
    webHandler.setRemoteTLSCN(tlsCN);
    webHandler.setMethodsManager(webserver->getMethodManagers());
    webHandler.setAuthenticators(webserver->getAuthenticator());
    webHandler.setSessionsManagger(webserver->getSessionsManager());
    webHandler.setUseFormattedJSONOutput(webserver->getUseFormattedJSONOutput());
    webHandler.setUsingCSRFToken(webserver->getUsingCSRFToken());
    webHandler.setResourceFilter(webserver->getResourceFilter());
    webHandler.setDocumentRootPath(webserver->getDocumentRootPath());
    webHandler.setWebServerName(webserver->getWebServerName());
    webHandler.setSoftwareVersion(webserver->getSoftwareVersion());
    webHandler.setUseHTMLIEngine(webserver->getUseHTMLIEngine());
    webHandler.setStaticContentElements(webserver->getStaticContentElements());
    webHandler.setRedirectOn404(webserver->getRedirectOn404());

    if (webserver->getExtCallBackOnConnect().call(obj,s,cUserIP,isSecure))
    {
        // Handle the webservice.
        Memory::Streams::Parsing::ParseErrorMSG err;
        webHandler.parseObject(&err);
    }
    return true;
}

bool WebServer::_callbackOnInitFailed(void * obj, Network::Streams::StreamSocket * s, const char * cUserIP, bool isSecure)
{
    WebServer * webserver = ((WebServer *)obj);
    webserver->getExtCallBackOnInitFailed().call(obj,s,cUserIP,isSecure);
    return true;
}

void WebServer::_callbackOnTimeOut(void * obj, Network::Streams::StreamSocket *s, const char * cUserIP, bool isSecure)
{
    WebServer * webserver = ((WebServer *)obj);
    if (webserver->getExtCallBackOnInitFailed().call(obj,s,cUserIP,isSecure))
    {
        s->writeString("HTTP/1.1 503 Service Temporarily Unavailable\r\n");
        s->writeString("Content-Type: text/html; charset=UTF-8\r\n");
        s->writeString("Connection: close\r\n");
        s->writeString("\r\n");
        s->writeString("<center><h1>503 Service Temporarily Unavailable</h1></center><hr>\r\n");
    }
}

std::string WebServer::getRedirectOn404() const
{
    return redirectOn404;
}

void WebServer::setRedirectOn404(const std::string & newRedirectOn404)
{
    redirectOn404 = newRedirectOn404;
}

std::map<std::string, Mantids::Memory::Containers::B_MEM *> WebServer::getStaticContentElements()
{
    std::lock_guard<std::mutex> lck (mutexInternalContent);
    return staticContentElements;
}

Application::Logs::RPCLog *WebServer::getRPCLog() const
{
    return rpcLog;
}

void WebServer::setRPCLog(Application::Logs::RPCLog *value)
{
    rpcLog = value;
}

std::string WebServer::getAppName() const
{
    if (!methodManagers) return "";
    return methodManagers->getAppName();
}

bool WebServer::getUseHTMLIEngine() const
{
    return useHTMLIEngine;
}

void WebServer::setUseHTMLIEngine(bool value)
{
    useHTMLIEngine = value;
}

void WebServer::addInternalContentElement(const std::string &path, const std::string &content)
{
    std::lock_guard<std::mutex> lck (mutexInternalContent);

    // TODO: update.... (when no http clients running)

    if (staticContentElements.find(path) == staticContentElements.end())
    {
        char * xmem = (char *)malloc(content.size()+1);
        xmem[content.size()]=0;
        memcpy(xmem,content.c_str(),content.size());
        staticContentElements[path] = new Mantids::Memory::Containers::B_MEM(xmem,content.size());
        memToBeFreed.push_back(xmem);
    }
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

std::string WebServer::getDocumentRootPath() const
{
    return documentRootPath;
}

bool WebServer::setDocumentRootPath(const std::string &value, const bool &autoloadResourceFilter)
{
    if (access(value.c_str(), R_OK)) return false;
    documentRootPath = value;

    if (autoloadResourceFilter)
    {
        std::string resourceFilterPath = documentRootPath + "/resources.conf";
        if (!access(resourceFilterPath.c_str(), R_OK))
        {
            ResourcesFilter * rf = new ResourcesFilter;
            if (rf->loadFile(resourceFilterPath))
            {
                if (resourceFilter)
                    delete resourceFilter; // Remove the previous RF.
                resourceFilter = rf;
            }
            else
                delete rf;
        }
    }

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

Mantids::Authentication::Domains *WebServer::getAuthenticator() const
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

void WebServer::setAuthenticator(Mantids::Authentication::Domains *value)
{
    authenticator = value;
}

void WebServer::setObj(void *value)
{
    obj = value;

}
