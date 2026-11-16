#pragma once

#include "session.h"
#include <Mantids30/Protocol/HTTP/httpv1_server.h>
#include <Mantids30/Core/Threads/safe_mapitem.h>

namespace Mantids30::Server::WebRESTFul::WebSocket {

class WebSocketConnection : public Mantids30::Core::Threads::Safe::MapItem
{
public:
    Mantids30::Protocol::HTTP::HTTPv1_Server *webSocketHTTPServer = nullptr; ///< Holds all the information from the initial client request
    Mantids30::Server::WebRESTFul::Sessions::SessionInfo *sessionInfo = nullptr;
    std::set<std::string> subscribedTopics;
    std::mutex subscribedTopicsMutex;
};

} // namespace Mantids30::Server::WebRESTFul::WebSocket
