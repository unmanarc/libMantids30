#pragma once

#include "sessionsmanager.h"
#include <Mantids30/Server_WebCore/resourcesfilter.h>

#include <Mantids30/API_Monolith/endpointshandler.h>
#include <Mantids30/Memory/b_mem.h>
#include <Mantids30/Net_Sockets/acceptor_multithreaded.h>
#include <Mantids30/Net_Sockets/acceptor_poolthreaded.h>
#include <Mantids30/Net_Sockets/socket_stream.h>
#include <Mantids30/Program_Logs/rpclog.h>
#include <Mantids30/Server_WebCore/apienginecore.h>
#include <memory>

namespace Mantids30 { namespace Network { namespace Servers { namespace WebMonolith {

class Engine : public Web::APIEngineCore
{
public:
    Engine();

    // Seteables (before starting the acceptor, non-thread safe):
    std::map<uint32_t,API::Monolith::Endpoints *> endpointsHandlerByAPIVersion;
    // Here you can set the web session manager internals (eg. session timeout)
    WebSessionsManager sessionsManager;

protected:
    std::shared_ptr<Web::APIClientHandler> createNewAPIClientHandler(APIEngineCore * webServer, std::shared_ptr<Sockets::Socket_Stream> s ) override;
};

}}}}
