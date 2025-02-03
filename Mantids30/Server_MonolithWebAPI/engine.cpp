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

std::shared_ptr<Network::Servers::Web::APIClientHandler> Engine::createNewAPIClientHandler(APIEngineCore *webServer, std::shared_ptr<Sockets::Socket_Stream_Base> s)
{
    auto webHandler = std::make_shared<ClientHandler>(webServer,s);

    webHandler->m_methodsHandlerByAPIVersion = methodsHandlerByAPIVersion;
    webHandler->m_sessionsManager = &sessionsManager;

    // Other parameters are set by the APIEngineCore.
    return webHandler;
}
