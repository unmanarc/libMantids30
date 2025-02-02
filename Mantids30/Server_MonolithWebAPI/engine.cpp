#include "engine.h"
#include "clienthandler.h"

#include <Mantids30/Protocol_HTTP/httpv1_server.h>
#include <Mantids30/Net_Sockets/socket_tls.h>
#include <memory>

using namespace Mantids30::Network::Servers::WebMonolith;
using namespace Mantids30;

Engine::Engine()
{
    sessionsManager.startGarbageCollector( WebSessionsManager::threadGC, &sessionsManager, "GC:WebSessions" );
}

Engine::~Engine()
{
}

void Engine::checkEngineStatus()
{
    // Well, it can be empty...
/*    if (m_methodsHandlerByAPIVersion.empty())
        throw std::runtime_error("The WebMonolith Engine was not properly initialized with any API");*/
}

std::shared_ptr<Network::Servers::Web::APIClientHandler> Engine::createNewAPIClientHandler(APIEngineCore *webServer, std::shared_ptr<Sockets::Socket_Stream_Base> s)
{
    auto webHandler = std::make_shared<ClientHandler>(webServer,s);

    webHandler->m_methodsHandlerByAPIVersion = methodsHandlerByAPIVersion;
    webHandler->m_sessionsManager = &sessionsManager;

    // Other parameters are set by the APIEngineCore.
    return webHandler;
}
/*
bool Engine::_onConnect(std::shared_ptr<void> context, std::shared_ptr<Sockets::Socket_Stream_Base> s)
{
    Engine * webserver = ((Engine *)context);

    std::string tlsCN;
    if (s->isSecure())
    {
        Network::Sockets::Socket_TLS * tlsSock = (Network::Sockets::Socket_TLS *)s;
        tlsCN = tlsSock->getTLSPeerCN();
    }

    // Prepare the web services handler.

    webHandler.jwtValidator = jwtValidator;
    webHandler.jwtSigner = jwtSigner;

    //webHandler.setRPCLog(webserver->rpcLog);
    //webHandler.setAppName(webserver->getApplicationName());
    webHandler.setClientInfoVars( cUserIP, isSecure, tlsCN );
   // webHandler.setAuthenticators(webserver->m_authenticator);
    webHandler.setUseFormattedJSONOutput(webserver->m_useFormattedJSONOutput);
   // webHandler.setUsingCSRFToken(webserver->m_useCSRFToken);
    webHandler.setResourcesFilter(webserver->m_resourceFilter);
    webHandler.setDocumentRootPath(webserver->getDocumentRootPath());
    webHandler.setWebServerName(webserver->m_webServerName);
    webHandler.setSoftwareVersion(webserver->getSoftwareVersion());
    webHandler.setUseHTMLIEngine(webserver->useHTMLIEngine);
    webHandler.setStaticContentElements(webserver->getStaticContentElements());
    webHandler.setRedirectPathOn404(webserver->redirectPathOn404);

    if (webserver->m_callbacks.m_onConnect.call(context,s))
    {
        // Handle the webservice.
        Memory::Streams::Parser::ErrorMSG err;
        webHandler.parseObject(&err);
    }
    return true;
}*/
/*
bool Engine::_onInitFailed(std::shared_ptr<void> context, std::shared_ptr<Sockets::Socket_Stream_Base> s)
{
    Engine * webserver = ((Engine *)context);
    webserver->m_callbacks.m_onInitFailed.call(context,s);
    return true;
}

void Engine::_onTimeOut(std::shared_ptr<void> context, std::shared_ptr<Sockets::Socket_Stream_Base> s)
{
    Engine * webserver = ((Engine *)context);
    if (webserver->m_callbacks.m_onTimeOut.call(context,s))
    {
        s->writeString("HTTP/1.1 503 Service Temporarily Unavailable\r\n");
        s->writeString("Content-Type: text/html; charset=UTF-8\r\n");
        s->writeString("Connection: close\r\n");
        s->writeString("\r\n");
        s->writeString("<center><h1>503 Service Temporarily Unavailable</h1></center><hr>\r\n");
    }
}
*/


/*
std::string Engine::getApplicationName() const
{
    if (!m_methodsHandler) return "";
    return m_methodsHandler->getApplicationName();
}
*/

/*
void Engine::addStaticContentElement(const std::string &path, const std::string &content)
{
    std::lock_guard<std::mutex> lck (internalContentMutex);

    // TODO: update.... (when no http clients running)
    if (staticContentElements.find(path) == staticContentElements.end())
    {
        char * xmem = (char *)malloc(content.size()+1);
        xmem[content.size()]=0;
        memcpy(xmem,content.c_str(),content.size());
        staticContentElements[path] = std::make_shared<Mantids30::Memory::Containers::B_MEM>(xmem,content.size());
        memToBeFreed.push_back(xmem);
    }
}*/
/*
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

bool Engine::setDocumentRootPath(const std::string &value, const bool &autoloadResourcesFilter)
{
    if (access(value.c_str(), R_OK)) return false;
    m_documentRootPath = value;

    if (autoloadResourcesFilter)
    {
        std::string resourceFilterPath = m_documentRootPath + "/resources.conf";
        if (!access(resourceFilterPath.c_str(), R_OK))
        {
            API::Monolith::ResourcesFilter * rf = new API::Monolith::ResourcesFilter;
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

*/
