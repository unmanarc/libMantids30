#pragma once

#include "api_websocket_config.h"
#include <Mantids30/Protocol_HTTP/httpv1_server.h>
#include <Mantids30/Threads/safe_map.h>
#include "session.h"

#include <Mantids30/DataFormat_JWT/jwt.h>
#include <Mantids30/Protocol_HTTP/websocket_eventtype.h>

namespace Mantids30::Network::Servers::Web {
    class APIClientHandler;
}

namespace Mantids30::API::WebSocket {

// Forward declaration
class Endpoint;

// Struct to hold WebSocket request parameters
struct WebSocketParameters
{
    Mantids30::Network::Protocols::HTTP::HTTPv1_Base::Request *clientRequest = nullptr; ///< Holds all the information from the initial client request
    DataFormat::JWT::Token emptyToken;                                                  ///< Holds a default empty token
    DataFormat::JWT::Token *jwtToken = &emptyToken;                                     ///< Holds JWT token data, if present and validated the pointer will be changed. (for restful servers only)
    std::shared_ptr<DataFormat::JWT> jwtValidator;                                      ///< Holds the JWT Validator
    std::shared_ptr<DataFormat::JWT> jwtSigner;                                         ///< Holds the JWT Signer
    std::shared_ptr<Mantids30::Sessions::Session> session;                              ///< Holds the current session (for monolith servers)
    std::string webSocketSessionId;                                                     ///< Holds the current session id
    const Endpoint *currentWebSocketEndpoint = nullptr;                                 ///< Holds the current web socket endpoint
};

// Callback function types for WebSocket events
using WebSocketEventFunctionType = void (*)(void *context, std::shared_ptr<Memory::Containers::B_Chunks> content, const Json::Value &jsonContent, const WebSocket::WebSocketParameters &parameters,
                                            Mantids30::Sessions::ClientDetails &authClientDetails);

class Endpoint
{
public:
    // Constructor
    Endpoint();

    // Security Internal Class
    struct Security
    {
        bool requireJWTHeaderAuthentication = true;
        bool requireJWTCookieAuthentication = true;
        bool requireSession = true;
        std::set<std::string> requiredScopes;
    };

    // Event Managers...
    WebSocketEventFunctionType sessionStartHandler = nullptr;
    WebSocketEventFunctionType binaryMessageReceivedHandler = nullptr;
    WebSocketEventFunctionType textMessageReceivedHandler = nullptr;
    WebSocketEventFunctionType sessionEndHandler = nullptr;

    // Public Methods:
    size_t getActiveUserConnectionsCount(const std::string &userId) const;
    bool sendJSONToConnectionID(const std::string &sessionId, const Json::Value &v) const;
    bool sendJSONToUser(const std::string &userId, const Json::Value &v) const;
    size_t sendJSONToSubscriptionTopic(const std::string &topicId, const Json::Value &v) const;

    // User Functions:
    bool joinTopicSubscription(const std::string &sessionId, const std::string &topicId) const;
    bool leaveTopicSubscription(const std::string &sessionId, const std::string &topicId) const;

private:
    // Private Members:
    std::shared_ptr<Threads::Safe::Map<std::string>> connectionsByIdMap;
    Security security;
    Config **config = nullptr; // autofilled
    void *context = nullptr;
    friend class Mantids30::Network::Servers::Web::APIClientHandler;
    friend class Endpoints;
};

} // namespace Mantids30::API::WebSocket
