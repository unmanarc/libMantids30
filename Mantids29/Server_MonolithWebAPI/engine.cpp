#include "engine.h"
#include "clienthandler.h"

#include <Mantids29/Protocol_HTTP/httpv1_server.h>
#include <Mantids29/Net_Sockets/socket_tls.h>

#include <memory>
#include <stdexcept>

using namespace Mantids29::Network::Servers::WebMonolith;
using namespace Mantids29;

Engine::Engine()
{
    m_multiThreadedAcceptor = std::make_shared<Network::Sockets::Acceptors::MultiThreaded>();
    m_poolThreadedAcceptor = std::make_shared<Network::Sockets::Acceptors::PoolThreaded>();

    m_rpcLog = nullptr;
    m_resourceFilter = nullptr;
    m_obj = nullptr;
    m_sessionsManager.startGarbageCollector( SessionsManager::threadGC, &m_sessionsManager, "GC:WebSessions" );
    m_useFormattedJSONOutput = true;
    m_useCSRFToken = true;
    m_useHTMLIEngine = true;
    m_authenticator = nullptr;
    m_methodsHandler = nullptr;
    m_redirectOn404 = "";
}

Engine::~Engine()
{
    for (const auto & i : m_staticContentElements)
    {
        delete i.second;
    }
    for (const auto & i : m_memToBeFreed)
    {
        free(i);
    }

    if (m_resourceFilter)
        delete m_resourceFilter;
}

void Engine::acceptMultiThreaded(const std::shared_ptr<Network::Sockets::Socket_Stream_Base> & listenerSocket, const uint32_t &maxConcurrentConnections)
{
    if (!m_methodsHandler) throw std::runtime_error("Don't Accept Monolith Web before setting some methodsmanager");
    if (!m_authenticator) throw std::runtime_error("Don't Accept Monolith Web before setting some authenticator");

    m_multiThreadedAcceptor->setAcceptorSocket(listenerSocket);
    m_multiThreadedAcceptor->setCallbackOnConnect(_onConnect,!m_obj?this:m_obj);
    m_multiThreadedAcceptor->setCallbackOnInitFail(_onInitFailed,!m_obj?this:m_obj);
    m_multiThreadedAcceptor->setCallbackOnTimedOut(_onTimeOut,!m_obj?this:m_obj);
    m_multiThreadedAcceptor->setMaxConcurrentClients(maxConcurrentConnections);
    m_multiThreadedAcceptor->startThreaded(m_multiThreadedAcceptor);
}

void Engine::acceptPoolThreaded(const std::shared_ptr<Network::Sockets::Socket_Stream_Base> & listenerSocket, const uint32_t &threadCount, const uint32_t &threadMaxQueuedElements)
{
    if (!m_methodsHandler) throw std::runtime_error("Don't Accept Monolith Web before setting some methodsmanager");
    if (!m_authenticator) throw std::runtime_error("Don't Accept Monolith Web before setting some authenticator");

    m_poolThreadedAcceptor->setAcceptorSocket(listenerSocket);
    m_poolThreadedAcceptor->setCallbackOnConnect(_onConnect,!m_obj?this:m_obj);
    m_poolThreadedAcceptor->setCallbackOnInitFail(_onInitFailed,!m_obj?this:m_obj);
    m_poolThreadedAcceptor->setCallbackOnTimedOut(_onTimeOut,!m_obj?this:m_obj);
    m_poolThreadedAcceptor->setThreadsCount(threadCount);
    m_poolThreadedAcceptor->setTaskQueues(threadMaxQueuedElements);
    m_poolThreadedAcceptor->start(m_poolThreadedAcceptor);
}

bool Engine::_onConnect(void * obj, Network::Sockets::Socket_Stream_Base * s, const char *cUserIP, bool isSecure)
{
    Engine * webserver = ((Engine *)obj);

    std::string tlsCN;
    if (s->isSecure())
    {
        Network::Sockets::Socket_TLS * tlsSock = (Network::Sockets::Socket_TLS *)s;
        tlsCN = tlsSock->getTLSPeerCN();
    }

    // Prepare the web services handler.
    ClientHandler webHandler(webserver,s);

    webHandler.setRPCLog(webserver->m_rpcLog);
    webHandler.setAppName(webserver->getApplicationName());
    webHandler.setSecure(isSecure);
    webHandler.setUserIP(cUserIP);
    webHandler.setRemoteTLSCN(tlsCN);
    webHandler.setMethodsHandler(webserver->m_methodsHandler);
    webHandler.setAuthenticators(webserver->m_authenticator);
    webHandler.setSessionsManagger(&(webserver->m_sessionsManager));
    webHandler.setUseFormattedJSONOutput(webserver->m_useFormattedJSONOutput);
    webHandler.setUsingCSRFToken(webserver->m_useCSRFToken);
    webHandler.setResourceFilter(webserver->m_resourceFilter);
    webHandler.setDocumentRootPath(webserver->getDocumentRootPath());
    webHandler.setWebServerName(webserver->getWebServerName());
    webHandler.setSoftwareVersion(webserver->getSoftwareVersion());
    webHandler.setUseHTMLIEngine(webserver->getUseHTMLIEngine());
    webHandler.setStaticContentElements(webserver->getStaticContentElements());
    webHandler.setRedirectPathOn404(webserver->m_redirectOn404);

    if (webserver->m_callbacks.m_onConnect.call(obj,s,cUserIP,isSecure))
    {
        // Handle the webservice.
        Memory::Streams::Parser::ErrorMSG err;
        webHandler.parseObject(&err);
    }
    return true;
}

bool Engine::_onInitFailed(void * obj, Network::Sockets::Socket_Stream_Base * s, const char * cUserIP, bool isSecure)
{
    Engine * webserver = ((Engine *)obj);
    webserver->m_callbacks.m_onInitFailed.call(obj,s,cUserIP,isSecure);
    return true;
}

void Engine::_onTimeOut(void * obj, Network::Sockets::Socket_Stream_Base *s, const char * cUserIP, bool isSecure)
{
    Engine * webserver = ((Engine *)obj);
    if (webserver->m_callbacks.m_onTimeOut.call(obj,s,cUserIP,isSecure))
    {
        s->writeString("HTTP/1.1 503 Service Temporarily Unavailable\r\n");
        s->writeString("Content-Type: text/html; charset=UTF-8\r\n");
        s->writeString("Connection: close\r\n");
        s->writeString("\r\n");
        s->writeString("<center><h1>503 Service Temporarily Unavailable</h1></center><hr>\r\n");
    }
}

std::map<std::string, Mantids29::Memory::Containers::B_MEM *> Engine::getStaticContentElements()
{
    std::lock_guard<std::mutex> lck (m_internalContentMutex);
    return m_staticContentElements;
}


std::string Engine::getApplicationName() const
{
    if (!m_methodsHandler) return "";
    return m_methodsHandler->getApplicationName();
}

void Engine::addInternalContentElement(const std::string &path, const std::string &content)
{
    std::lock_guard<std::mutex> lck (m_internalContentMutex);

    // TODO: update.... (when no http clients running)

    if (m_staticContentElements.find(path) == m_staticContentElements.end())
    {
        char * xmem = (char *)malloc(content.size()+1);
        xmem[content.size()]=0;
        memcpy(xmem,content.c_str(),content.size());
        m_staticContentElements[path] = new Mantids29::Memory::Containers::B_MEM(xmem,content.size());
        m_memToBeFreed.push_back(xmem);
    }
}

std::string Engine::getSoftwareVersion() const
{
    return m_softwareVersion;
}

void Engine::setSoftwareVersion(const std::string &value)
{
    m_softwareVersion = value;
}

void Engine::setSoftwareVersion(const uint32_t major, const uint32_t minor, const uint32_t subminor, const std::string & subText)
{
    setSoftwareVersion(std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(subminor) +  (subText.empty()? "" : (" " + subText))  );
}

std::string Engine::getDocumentRootPath() const
{
    return m_documentRootPath;
}

bool Engine::setDocumentRootPath(const std::string &value, const bool &autoloadResourceFilter)
{
    if (access(value.c_str(), R_OK)) return false;
    m_documentRootPath = value;

    if (autoloadResourceFilter)
    {
        std::string resourceFilterPath = m_documentRootPath + "/resources.conf";
        if (!access(resourceFilterPath.c_str(), R_OK))
        {
            ResourcesFilter * rf = new ResourcesFilter;
            if (rf->loadFile(resourceFilterPath))
            {
                if (m_resourceFilter)
                    delete m_resourceFilter; // Remove the previous RF.
                m_resourceFilter = rf;
            }
            else
                delete rf;
        }
    }

    return  true;
}

