#pragma once

#include "session.h"
#include <Mantids30/Protocol_HTTP/httpv1_server.h>
#include <Mantids30/Threads/safe_mapitem.h>

namespace Mantids30::API::WebSocket {

class WebSocketConnection : public Threads::Safe::MapItem
{
public:
    Mantids30::Network::Protocols::HTTP::HTTPv1_Server *webSocketHTTPServer = nullptr; ///< Holds all the information from the initial client request
    Sessions::SessionInfo *sessionInfo = nullptr;
    std::set<std::string> subscribedTopics;
    std::mutex subscribedTopicsMutex;
};

} // namespace Mantids30::API::WebSocket
