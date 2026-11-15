#pragma once

#include "session.h"
#include <Mantids30/Protocols/HTTP/httpv1_server.h>
#include <Mantids30/Core/Threads/safe_mapitem.h>

namespace Mantids30::Servers::RESTfulWebServer::WebSocket {

class WebSocketConnection : public Mantids30::Core::Threads::Safe::MapItem
{
public:
    Mantids30::Protocols::HTTP::HTTPv1_Server *webSocketHTTPServer = nullptr; ///< Holds all the information from the initial client request
    Mantids30::Servers::RESTfulWebServer::Sessions::SessionInfo *sessionInfo = nullptr;
    std::set<std::string> subscribedTopics;
    std::mutex subscribedTopicsMutex;
};

} // namespace Mantids30::Servers::RESTfulWebServer::WebSocket
