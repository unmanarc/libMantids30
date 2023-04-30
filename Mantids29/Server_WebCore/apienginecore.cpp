#include "apienginecore.h"
#include "apiclienthandler.h"

#include <Mantids29/Protocol_HTTP/httpv1_server.h>
#include <Mantids29/Net_Sockets/socket_tls.h>

#include <memory>
#include <stdexcept>

using namespace Mantids29::Network::Servers::Web;
using namespace Mantids29;

APIEngineCore::APIEngineCore()
{
    m_multiThreadedAcceptor = std::make_shared<Network::Sockets::Acceptors::MultiThreaded>();
    m_poolThreadedAcceptor = std::make_shared<Network::Sockets::Acceptors::PoolThreaded>();
}

APIEngineCore::~APIEngineCore()
{
    for (const auto & i : m_memToBeFreed)
    {
        free(i);
    }

    if (m_resourceFilter)
        delete m_resourceFilter;
}

void APIEngineCore::acceptMultiThreaded(const std::shared_ptr<Network::Sockets::Socket_Stream_Base> & listenerSocket, const uint32_t &maxConcurrentConnections)
{
    m_multiThreadedAcceptor->setAcceptorSocket(listenerSocket);
    m_multiThreadedAcceptor->setCallbackOnConnect(_onConnect,!m_obj?this:m_obj);
    m_multiThreadedAcceptor->setCallbackOnInitFail(_onInitFailed,!m_obj?this:m_obj);
    m_multiThreadedAcceptor->setCallbackOnTimedOut(_onTimeOut,!m_obj?this:m_obj);
    m_multiThreadedAcceptor->setMaxConcurrentClients(maxConcurrentConnections);
    m_multiThreadedAcceptor->startThreaded(m_multiThreadedAcceptor);
}

void APIEngineCore::acceptPoolThreaded(const std::shared_ptr<Network::Sockets::Socket_Stream_Base> & listenerSocket, const uint32_t &threadCount, const uint32_t &threadMaxQueuedElements)
{
    m_poolThreadedAcceptor->setAcceptorSocket(listenerSocket);
    m_poolThreadedAcceptor->setCallbackOnConnect(_onConnect,!m_obj?this:m_obj);
    m_poolThreadedAcceptor->setCallbackOnInitFail(_onInitFailed,!m_obj?this:m_obj);
    m_poolThreadedAcceptor->setCallbackOnTimedOut(_onTimeOut,!m_obj?this:m_obj);
    m_poolThreadedAcceptor->setThreadsCount(threadCount);
    m_poolThreadedAcceptor->setTaskQueues(threadMaxQueuedElements);
    m_poolThreadedAcceptor->start(m_poolThreadedAcceptor);
}

bool APIEngineCore::_onConnect(void * obj, Network::Sockets::Socket_Stream_Base * s, const char *cUserIP, bool isSecure)
{
    APIEngineCore * webserver = ((APIEngineCore *)obj);

    std::string tlsCN;
    if (s->isSecure())
    {
        Network::Sockets::Socket_TLS * tlsSock = (Network::Sockets::Socket_TLS *)s;
        tlsCN = tlsSock->getTLSPeerCN();
    }

    // Prepare the web services handler.
    auto webHandler = webserver->createNewAPIClientHandler(webserver,s);

    webHandler->setRPCLog(webserver->m_rpcLog);
    webHandler->setAppName(webserver->m_applicationName);
    webHandler->setSecure(isSecure);
    webHandler->setUserIP(cUserIP);
    webHandler->setRemoteTLSCN(tlsCN);
    webHandler->setUseFormattedJSONOutput(webserver->m_useFormattedJSONOutput);
    webHandler->setResourcesFilter(webserver->m_resourceFilter);
    webHandler->setDocumentRootPath(webserver->getDocumentRootPath());
    webHandler->setWebServerName(webserver->m_webServerName);
    webHandler->setSoftwareVersion(webserver->getSoftwareVersion());
    webHandler->setUseHTMLIEngine(webserver->m_useHTMLIEngine);
    webHandler->setStaticContentElements(webserver->getStaticContentElements());
    webHandler->setRedirectPathOn404(webserver->m_redirectOn404);

    if (webserver->m_callbacks.m_onConnect.call(obj,s,cUserIP,isSecure))
    {
        // Handle the webservice.
        Memory::Streams::Parser::ErrorMSG err;
        webHandler->parseObject(&err);
    }

    delete webHandler;
    return true;
}

bool APIEngineCore::_onInitFailed(void * obj, Network::Sockets::Socket_Stream_Base * s, const char * cUserIP, bool isSecure)
{
    APIEngineCore * webserver = ((APIEngineCore *)obj);
    webserver->m_callbacks.m_onInitFailed.call(obj,s,cUserIP,isSecure);
    return true;
}

void APIEngineCore::_onTimeOut(void * obj, Network::Sockets::Socket_Stream_Base *s, const char * cUserIP, bool isSecure)
{
    APIEngineCore * webserver = ((APIEngineCore *)obj);
    if (webserver->m_callbacks.m_onTimeOut.call(obj,s,cUserIP,isSecure))
    {
        s->writeString("HTTP/1.1 503 Service Temporarily Unavailable\r\n");
        s->writeString("Content-Type: text/html; charset=UTF-8\r\n");
        s->writeString("Connection: close\r\n");
        s->writeString("\r\n");
        s->writeString("<center><h1>503 Service Temporarily Unavailable</h1></center><hr>\r\n");
    }
}

std::map<std::string, std::shared_ptr<Mantids29::Memory::Containers::B_MEM>> APIEngineCore::getStaticContentElements()
{
    std::lock_guard<std::mutex> lck (m_internalContentMutex);
    return m_staticContentElements;
}

void APIEngineCore::addStaticContentElement(const std::string &path, const std::string &content)
{
    std::lock_guard<std::mutex> lck (m_internalContentMutex);

    // TODO: update.... (when no http clients running)

    if (m_staticContentElements.find(path) == m_staticContentElements.end())
    {
        char * xmem = (char *)malloc(content.size()+1);
        xmem[content.size()]=0;
        memcpy(xmem,content.c_str(),content.size());
        m_staticContentElements[path] = std::make_shared<Mantids29::Memory::Containers::B_MEM>(xmem,content.size());
        m_memToBeFreed.push_back(xmem);
    }
}

std::string APIEngineCore::getSoftwareVersion() const
{
    return m_softwareVersion;
}

void APIEngineCore::setSoftwareVersion(const std::string &value)
{
    m_softwareVersion = value;
}

void APIEngineCore::setSoftwareVersion(const uint32_t major, const uint32_t minor, const uint32_t subminor, const std::string & subText)
{
    setSoftwareVersion(std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(subminor) +  (subText.empty()? "" : (" " + subText))  );
}

std::string APIEngineCore::getDocumentRootPath() const
{
    return m_documentRootPath;
}

bool APIEngineCore::setDocumentRootPath(const std::string &value, const bool &autoloadResourcesFilter)
{
    if (access(value.c_str(), R_OK)) return false;
    m_documentRootPath = value;

    if (autoloadResourcesFilter)
    {
        std::string resourceFilterPath = m_documentRootPath + "/resources.conf";
        if (!access(resourceFilterPath.c_str(), R_OK))
        {
            API::Web::ResourcesFilter * rf = new API::Web::ResourcesFilter;
            if (rf->loadFiltersFromFile(resourceFilterPath))
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

