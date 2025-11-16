#pragma once
#include <Mantids30/DataFormat_JWT/jwt.h>
#include <Mantids30/Helpers/json.h>
#include <Mantids30/Memory/streamablejson.h>
#include <Mantids30/Protocol_HTTP/api_return.h>
#include <Mantids30/Protocol_HTTP/httpv1_base.h>
#include <Mantids30/Protocol_HTTP/httpv1_server.h>
#include <Mantids30/Protocol_HTTP/rsp_status.h>
#include <Mantids30/Protocol_HTTP/websocket_eventtype.h>
#include <Mantids30/Sessions/session.h>
#include <Mantids30/Threads/mutex_shared.h>
#include <cstdint>
#include <map>
#include <memory>
#include <set>
#include <string>

namespace Mantids30 {
namespace API {
namespace WebSocket {

// Struct to hold WebSocket request parameters
struct WebSocketParameters
{
    Mantids30::Network::Protocols::HTTP::HTTPv1_Base::Request *clientRequest = nullptr; ///< Holds all the information from the initial client request
    DataFormat::JWT::Token emptyToken;                                                  ///< Holds a default empty token
    DataFormat::JWT::Token *jwtToken = &emptyToken;                                     ///< Holds JWT token data, if present and validated the pointer will be changed.
    std::shared_ptr<DataFormat::JWT> jwtValidator;                                      ///< Holds the JWT Validator
    std::shared_ptr<DataFormat::JWT> jwtSigner;                                         ///< Holds the JWT Signer
};

// Callback function types for WebSocket events
using WebSocketEventFunctionType = APIReturn (*)(void *context, std::shared_ptr<Memory::Containers::B_Chunks> content, const WebSocket::WebSocketParameters &parameters,
                                                 Mantids30::Sessions::ClientDetails &authClientDetails);
/**
* @struct WebSocketEndpointFullDefinition
*
* @brief Struct to define the WebSocket endpoint handlers, security, and object pointer.
*/
struct WebSocketEndpointFullDefinition
{
    struct Security
    {
        bool requireJWTHeaderAuthentication = true;
        bool requireJWTCookieAuthentication = true;
        std::set<std::string> requiredScopes;
    };

    WebSocketEventFunctionType sessionStartHandler = nullptr;
    WebSocketEventFunctionType messageReceivedHandler = nullptr;
    WebSocketEventFunctionType sessionEndHandler = nullptr;
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
        REQUIRE_JWT_COOKIE_AUTH = 2
    };

    struct SecurityParameters
    {
        Json::Value toJSON() const
        {
            Json::Value securityJSON;
            securityJSON["haveJWTAuthHeader"] = haveJWTAuthHeader;
            securityJSON["haveJWTAuthCookie"] = haveJWTAuthCookie;
            return securityJSON;
        }

        void setJSON(const Json::Value &securityJSON)
        {
            haveJWTAuthHeader = JSON_ASBOOL(securityJSON, "haveJWTAuthHeader", false);
            haveJWTAuthCookie = JSON_ASBOOL(securityJSON, "haveJWTAuthCookie", false);
        }

        bool haveJWTAuthHeader = false;
        bool haveJWTAuthCookie = false;
    };

    /**
     * @brief Default constructor for Endpoints.
     */
    Endpoints() = default;
    /**
     * @brief Add a new WebSocket endpoint.
     *
     * @param endpointPath The WebSocket URI path.
     * @param securityOptions Security options for the endpoint.
     * @param requiredScopes The set of required scopes for the endpoint.
     * @param context The object pointer for the handlers.
     * @param sessionStartHandler Handler for session start events.
     * @param messageReceivedHandler Handler for message received events.
     * @param sessionEndHandler Handler for session end events.
     * @return Returns true if the endpoint was added successfully, false otherwise.
     */
    bool addEndpoint(const std::string &endpointPath, const uint32_t &securityOptions, const std::set<std::string> requiredScopes, void *context, WebSocketEventFunctionType sessionStartHandler,
                     WebSocketEventFunctionType messageReceivedHandler, WebSocketEventFunctionType sessionEndHandler);

    /**
     * @brief Add a new WebSocket endpoint with full definition struct.
     *
     * @param endpointPath The WebSocket URI path.
     * @param endpointDefinition The WebSocketEndpointFullDefinition struct.
     * @return Returns true if the endpoint was added successfully, false otherwise.
     */
    bool addEndpoint(const std::string &endpointPath, const WebSocketEndpointFullDefinition &endpointDefinition);

    Endpoints::ErrorCodes checkEndpoint(const std::string &endpointPath, const std::set<std::string> &currentScopes, bool isAdmin, const SecurityParameters &securityParameters);

    /**
     * @brief Handle WebSocket event and return the error code.
     *
     * @param eventType The type of WebSocket event (SESSION_START, MESSAGE_RECEIVED, SESSION_END).
     * @param parameters The WebSocket parameters.
     * @return The error code indicating the result of event handling.
     */
    ErrorCodes handleEvent(const Network::Protocols::WebSocket::EventType &eventType, std::shared_ptr<Memory::Containers::B_Chunks> content, WebSocket::WebSocketParameters &parameters);


private:
    std::map<std::string, WebSocketEndpointFullDefinition> m_endpoints; ///< Map of WebSocket endpoints by URI path.
    std::map<std::string, WebSocketEndpointFullDefinition>::iterator it;

    Sessions::ClientDetails extractClientDetails(const WebSocketParameters &parameters);

    void invokeHandler(const WebSocketEndpointFullDefinition &endpointDef, const Network::Protocols::WebSocket::EventType &eventType, std::shared_ptr<Memory::Containers::B_Chunks> content,
                            const WebSocketParameters &parameters);
};

} // namespace WebSocket
} // namespace API
} // namespace Mantids30
