#include "engine.h"
#include "clienthandler.h"

#include <Mantids30/Net_Sockets/socket_tls.h>
#include <Mantids30/Protocol_HTTP/httpv1_server.h>
#include <memory>

using namespace Mantids30::Network::Servers::WebMonolith;
using namespace Mantids30;

Engine::Engine()
{
    sessionsManager.startGarbageCollector(WebSessionsManager::threadGC, &sessionsManager, "GC:WebSessions");
}

std::shared_ptr<Network::Servers::Web::APIServer_ClientHandler> Engine::createNewAPIServer_ClientHandler(APIServerCore *webServer, std::shared_ptr<Sockets::Socket_Stream> s)
{
    std::shared_ptr<ClientHandler> webHandler = std::make_shared<ClientHandler>(webServer, s);

    webHandler->m_endpointsHandlerByAPIVersion = endpointsHandlerByAPIVersion;
    webHandler->m_sessionsManager = &sessionsManager;

    // Other parameters are set by the APIServerCore.
    return webHandler;
}
