#include "engine.h"
#include "clienthandler.h"

using namespace Mantids29::Network::Servers::RESTful;
using namespace Mantids29;

Engine::Engine()
{
}

Engine::~Engine()
{
}

Network::Servers::Web::APIClientHandler *Engine::createNewAPIClientHandler(APIEngineCore * webServer, Network::Sockets::Socket_Stream_Base * s)
{
    // TODO: create method for key revocation
    auto clientHandler = new Mantids29::Network::Servers::RESTful::ClientHandler(webServer,s);
    clientHandler->m_jwtEngine = m_jwtEngine;
    clientHandler->m_methodsHandler = m_methodsHandler;
    return clientHandler;
}
