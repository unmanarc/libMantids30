#pragma once

#include "sessionsmanager.h"
#include <Mantids30/Server_WebCore/resourcesfilter.h>

#include <Mantids30/API_EndpointsAndSessions/api_monolith_endpoints.h>
#include <Mantids30/Memory/b_mem.h>
#include <Mantids30/Net_Sockets/socket_stream.h>
#include <Mantids30/Program_Logs/rpclog.h>
#include <Mantids30/Server_WebCore/apiserver_core.h>
#include <memory>

namespace Mantids30::Network::Servers::WebMonolith {

class Engine : public Web::APIServerCore
{
public:
    Engine();

    // Seteables (before starting the acceptor, non-thread safe):
    // (After first initialization should not be modified)
    std::map<uint32_t,API::Monolith::Endpoints *> endpointsHandlerByAPIVersion;
    // Here you can set the web session manager internals (eg. session timeout)
    WebSessionsManager sessionsManager;

protected:
    std::shared_ptr<Web::APIServer_ClientHandler> createNewAPIServer_ClientHandler(APIServerCore * webServer, std::shared_ptr<Sockets::Socket_Stream> s ) override;
};

}
