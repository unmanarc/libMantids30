#pragma once

#include "json/value.h"
#include <Mantids30/DataFormat_JWT/jwt.h>
#include <Mantids30/Protocol_HTTP/httpv1_server.h>
#include <Mantids30/Protocol_HTTP/websocket_eventtype.h>
#include <Mantids30/Threads/safe_map.h>
#include <memory>
#include <mutex>
#include "session.h"

namespace Mantids30::API::WebSocket {

// Forward declaration
struct WebSocketEndpointFullDefinition;
class WebSocketConnection;

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
    const WebSocketEndpointFullDefinition * currentWebSocketEndpoint = nullptr;               ///< Holds the current web socket endpoint
};

// Callback function types for WebSocket events
using WebSocketEventFunctionType = void (*)(void *context,
                                            std::shared_ptr<Memory::Containers::B_Chunks> content,
                                            const Json::Value & jsonContent,
                                            const WebSocket::WebSocketParameters &parameters,
                                            Mantids30::Sessions::ClientDetails &authClientDetails);

//TODO: Max subscription topics per connection... Max web socket connections per user. create the API to join or left a topic
class WebSocketConnection : public Threads::Safe::MapItem
{
public:
    Mantids30::Network::Protocols::HTTP::HTTPv1_Server * webSocketHTTPServer = nullptr; ///< Holds all the information from the initial client request
    Sessions::SessionInfo sessionInfo;
    std::set<std::string> subscribedTopics;
    std::mutex subscribedTopicsMutex;
};

/**
* @struct WebSocketEndpointFullDefinition
*
* @brief Struct to define the WebSocket endpoint handlers, security, and object pointer.
*/
struct WebSocketEndpointFullDefinition
{
    WebSocketEndpointFullDefinition() {
        connectionsByIdMap = std::make_shared<Threads::Safe::Map<std::string>>();
    }

    struct Security
    {
        bool requireJWTHeaderAuthentication = true;
        bool requireJWTCookieAuthentication = true;
        bool requireSession = true;
        std::set<std::string> requiredScopes;
    };

    WebSocketEventFunctionType sessionStartHandler = nullptr;
    WebSocketEventFunctionType binaryMessageReceivedHandler = nullptr;
    WebSocketEventFunctionType textMessageReceivedHandler = nullptr;
    WebSocketEventFunctionType sessionEndHandler = nullptr;

    bool sendJSONToConnectionID(const std::string & sessionId, const Json::Value & v)
    {
        API::WebSocket::WebSocketConnection * connection = (API::WebSocket::WebSocketConnection *)connectionsByIdMap->openElement(sessionId);
        if (connection)
        {
            connection->webSocketHTTPServer->sendWebSocketText(v.toStyledString());
            connectionsByIdMap->releaseElement(sessionId);
            return true;
        }
        return false;
    }

    bool sendJSONToUser(const std::string & userId, const Json::Value & v)
    {
        auto keys = connectionsByIdMap->getKeys();

        for ( const auto & sessionId : keys )
        {
            API::WebSocket::WebSocketConnection * connection = (API::WebSocket::WebSocketConnection *)connectionsByIdMap->openElement(sessionId);
            if (connection)
            {
                if (connection->sessionInfo.authSession->getUser() == userId)
                {
                    connection->webSocketHTTPServer->sendWebSocketText(v.toStyledString());
                    connectionsByIdMap->releaseElement(sessionId);
                    return true;
                }
                else
                {
                    connectionsByIdMap->releaseElement(sessionId);
                }
            }
        }

        return false;
    }

    size_t sendJSONToSubscriptionTopic(const std::string & topicId, const Json::Value & v)
    {
        size_t i = 0;
        auto keys = connectionsByIdMap->getKeys();

        for ( const auto & sessionId : keys )
        {
            API::WebSocket::WebSocketConnection * connection = (API::WebSocket::WebSocketConnection *)connectionsByIdMap->openElement(sessionId);
            if (connection)
            {
                {
                    std::unique_lock<std::mutex> lock(connection->subscribedTopicsMutex);
                    if (connection->subscribedTopics.find(topicId) != connection->subscribedTopics.end())
                    {
                        connection->webSocketHTTPServer->sendWebSocketText(v.toStyledString());
                        i++;
                    }
                }
                connectionsByIdMap->releaseElement(sessionId);
            }
        }

        return i;
    }

    bool joinToSubscriptionTopic(const std::string & sessionId, const std::string & topicId)
    {
        API::WebSocket::WebSocketConnection * connection = (API::WebSocket::WebSocketConnection *)connectionsByIdMap->openElement(sessionId);
        if (connection)
        {
            {
                std::unique_lock<std::mutex> lock(connection->subscribedTopicsMutex);
                connection->subscribedTopics.insert(topicId);
            }
            connectionsByIdMap->releaseElement(sessionId);
            return true;
        }
        return false;
    }


    bool leftSubscriptionTopic(const std::string & sessionId, const std::string & topicId)
    {
        API::WebSocket::WebSocketConnection * connection = (API::WebSocket::WebSocketConnection *)connectionsByIdMap->openElement(sessionId);
        if (connection)
        {
            {
                std::unique_lock<std::mutex> lock(connection->subscribedTopicsMutex);
                connection->subscribedTopics.erase(topicId);
            }
            connectionsByIdMap->releaseElement(sessionId);
            return true;
        }
        return false;
    }

    std::shared_ptr<Threads::Safe::Map<std::string>> connectionsByIdMap;

    Security security;
    void *context = nullptr;
};


/**
* @class Endpoints
*
* @brief Handles the mapping of WebSocket URIs to corresponding handlers,
* manages security requirements, and invokes the appropriate callbacks.
*/
class Endpoints
{
public:
    /**
     * @enum ErrorCodes
     *
     * @brief Enumeration for possible error codes.
     */
    enum ErrorCodes
    {
        SUCCESS = 0,
        INVALID_EVENT_TYPE = -1,
        ENDPOINT_NOT_FOUND = -2,
        AUTHENTICATION_REQUIRED = -3,
        INVALID_SCOPE = -4,
        INTERNAL_ERROR = -5
    };

    enum SecurityOptions
    {
        NO_AUTH = 0,
        REQUIRE_JWT_HEADER_AUTH = 1,
        REQUIRE_JWT_COOKIE_AUTH = 2,
        REQUIRE_SESSION = 4,
    };

    struct SecurityParameters
    {
        bool haveJWTAuthHeader = false;
        bool haveJWTAuthCookie = false;
        // Only for monolith:
        bool haveSession = false;
    };

    /**
     * @brief Default constructor for Endpoints.
     */
    Endpoints() = default;

    /**
     * @brief Add a new WebSocket endpoint with full definition struct.
     *
     * @param endpointPath The WebSocket URI path.
     * @param endpointDefinition The WebSocketEndpointFullDefinition struct.
     * @return Returns true if the endpoint was added successfully, false otherwise.
     */
    bool addEndpoint(const std::string &endpointPath, const WebSocketEndpointFullDefinition &endpointDefinition);

    /**
     * @brief checkEndpoint
     * @param endpointPath
     * @param currentScopes
     * @param isAdmin
     * @param securityParameters
     * @return
     */
    Endpoints::ErrorCodes checkEndpoint(const std::string &endpointPath, const std::set<std::string> &currentScopes, bool isAdmin, const SecurityParameters &securityParameters);

    /**
     * @brief Handle WebSocket event and return the error code.
     *
     * @param eventType The type of WebSocket event (SESSION_START, MESSAGE_RECEIVED, SESSION_END).
     * @param parameters The WebSocket parameters.
     * @return The error code indicating the result of event handling.
     */
    ErrorCodes handleEvent(const Network::Protocols::WebSocket::EventType &eventType, std::shared_ptr<Memory::Containers::B_Chunks> content, WebSocket::WebSocketParameters &parameters);

    /**
     * @brief setTranslateTextMessagesToJSON Don´t use, because the webcore will replace with the configuration.
     * @param newTranslateTextMessagesToJSON
     */
    void setTranslateTextMessagesToJSON(bool newTranslateTextMessagesToJSON);

    const WebSocketEndpointFullDefinition* getWebSocketEndpointByURI(const std::string & uri) const;


private:

    bool m_translateTextMessagesToJSON = false;
    std::map<std::string, WebSocketEndpointFullDefinition> m_endpoints; ///< Map of WebSocket endpoints by URI path.
    std::map<std::string, WebSocketEndpointFullDefinition>::iterator it;

    Sessions::ClientDetails extractClientDetails(const WebSocketParameters &parameters);

    bool invokeHandler(const WebSocketEndpointFullDefinition &endpointDef, const Network::Protocols::WebSocket::EventType &eventType, std::shared_ptr<Memory::Containers::B_Chunks> content,
                            const WebSocketParameters &parameters);

};

}
