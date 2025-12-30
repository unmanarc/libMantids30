#pragma once

#include "api_websocket_endpoint.h"

namespace Mantids30::Network::Servers::Web {
class APIEngineCore;
}

namespace Mantids30::API::WebSocket {

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
     * @param endpointDefinition The WebSocket::Endpoint struct.
     * @return Returns true if the endpoint was added successfully, false otherwise.
     */
    bool addEndpoint(const std::string &endpointPath, const WebSocket::Endpoint &endpointDefinition);

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
     * @brief getWebSocketEndpointByURI
     * @param uri
     * @return
     */
    const WebSocket::Endpoint *getWebSocketEndpointByURI(const std::string &uri) const;

private:
    Config *config = nullptr;

    std::map<std::string, WebSocket::Endpoint> m_endpoints; ///< Map of WebSocket endpoints by URI path.
    std::map<std::string, WebSocket::Endpoint>::iterator it;

    Sessions::ClientDetails extractClientDetails(const WebSocketParameters &parameters);

    bool invokeHandler(const WebSocket::Endpoint &endpointDef, const Network::Protocols::WebSocket::EventType &eventType, std::shared_ptr<Memory::Containers::B_Chunks> content,
                       const WebSocketParameters &parameters);

    friend class Mantids30::Network::Servers::Web::APIEngineCore;
};

} // namespace Mantids30::API::WebSocket
