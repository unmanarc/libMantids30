#pragma once

#include <cstddef>

namespace Mantids30::API::WebSocket {

struct Config
{
    /**
     * @brief translateWebSocketTextMessagesToJSON Translate WebSocket Messages To JSON
     */
    bool translateWebSocketTextMessagesToJSON = true;

    /**
     * @brief sendWebSocketSessionIDAtConnection Send WebSocket Session ID Message At Connection Start
     */
    bool sendWebSocketSessionIDAtConnection = true;

    /**
     * @brief maxSubscriptionTopicsPerConnection Prevent user flooding the subscription
     */
    size_t maxSubscriptionTopicsPerConnection = 64;

    /**
     * @brief maxConnectionsPerUserPerEndpoint Maximum WebSocket Connections Per User Per Endpoint
     */
    size_t maxConnectionsPerUserPerEndpoint = 16;
};

} // namespace Mantids30::API::WebSocket
