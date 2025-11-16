#include "api_websocket_endpoints.h"

#include <Mantids30/Helpers/json.h>
#include <Mantids30/Protocol_HTTP/rsp_status.h>
#include <Mantids30/Threads/lock_shared.h>

using namespace Mantids30;
using namespace Mantids30::Network::Protocols;
using namespace API::WebSocket;

bool Endpoints::addEndpoint(const std::string &endpointPath, const uint32_t &securityOptions, const std::set<std::string> requiredScopes, void *context, WebSocketEventFunctionType sessionStartHandler,
                            WebSocketEventFunctionType messageReceivedHandler, WebSocketEventFunctionType sessionEndHandler)
{
    WebSocketEndpointFullDefinition def;
    def.sessionStartHandler = sessionStartHandler;
    def.messageReceivedHandler = messageReceivedHandler;
    def.sessionEndHandler = sessionEndHandler;
    def.context = context;
    def.security.requireJWTHeaderAuthentication = securityOptions & Endpoints::SecurityOptions::REQUIRE_JWT_HEADER_AUTH;
    def.security.requireJWTCookieAuthentication = securityOptions & Endpoints::SecurityOptions::REQUIRE_JWT_COOKIE_AUTH;
    def.security.requiredScopes = requiredScopes;
    return addEndpoint(endpointPath, def);
}

bool Endpoints::addEndpoint(const std::string &endpointPath, const WebSocketEndpointFullDefinition &endpointDefinition)
{
    m_endpoints[endpointPath] = endpointDefinition;
    return true;
}

Sessions::ClientDetails Endpoints::extractClientDetails(const WebSocketParameters &parameters)
{
    Mantids30::Sessions::ClientDetails clientDetails;
    if (parameters.clientRequest)
    {
        clientDetails.ipAddress = parameters.clientRequest->networkClientInfo.REMOTE_ADDR;
        clientDetails.tlsCommonName = parameters.clientRequest->networkClientInfo.tlsCommonName;
        clientDetails.userAgent = parameters.clientRequest->userAgent;
    }
    return clientDetails;
}


void Endpoints::invokeHandler(const WebSocketEndpointFullDefinition &endpointDef, const Network::Protocols::WebSocket::EventType &eventType, std::shared_ptr<Memory::Containers::B_Chunks> content,
    const WebSocketParameters &parameters)
{
    WebSocketEventFunctionType handler = nullptr;

    switch (eventType)
    {
    case Network::Protocols::WebSocket::SESSION_START:
        handler = endpointDef.sessionStartHandler;
        break;
    case Network::Protocols::WebSocket::MESSAGE_RECEIVED:
        handler = endpointDef.messageReceivedHandler;
        break;
    case Network::Protocols::WebSocket::SESSION_END:
        handler = endpointDef.sessionEndHandler;
        break;
    default:
        return; // Empty return for invalid event type (do nothing)
    }

    if (handler != nullptr)
    {
        Mantids30::Sessions::ClientDetails clientDetails = extractClientDetails(parameters);
        handler(endpointDef.context, content, parameters, clientDetails);
    }
}

Endpoints::ErrorCodes Endpoints::checkEndpoint(const std::string &endpointPath, const std::set<std::string> &currentScopes, bool isAdmin, const SecurityParameters &securityParameters)
{
    it = m_endpoints.find(endpointPath);
    if (it == m_endpoints.end())
    {
        return ENDPOINT_NOT_FOUND;
    }

    const WebSocketEndpointFullDefinition &endpointDef = it->second;

    // Security checks
    if (endpointDef.security.requireJWTHeaderAuthentication && !securityParameters.haveJWTAuthHeader)
    {
        return AUTHENTICATION_REQUIRED;
    }

    if (endpointDef.security.requireJWTCookieAuthentication && !securityParameters.haveJWTAuthCookie)
    {
        return AUTHENTICATION_REQUIRED;
    }

    if (!isAdmin)
    {
        for (const auto &scope : endpointDef.security.requiredScopes)
        {
            if (currentScopes.find(scope) == currentScopes.end())
            {
                return INVALID_SCOPE;
            }
        }
    }

    return SUCCESS;
}

Endpoints::ErrorCodes Endpoints::handleEvent(const Network::Protocols::WebSocket::EventType &eventType, std::shared_ptr<Memory::Containers::B_Chunks> content,
                                             WebSocket::WebSocketParameters &parameters)
{
    if (it == m_endpoints.end())
    {
        return ENDPOINT_NOT_FOUND;
    }

    const WebSocketEndpointFullDefinition &endpointDef = it->second;

    // Check if handler exists for this event type
    bool handlerExists = false;
    switch (eventType)
    {
    case Network::Protocols::WebSocket::SESSION_START:
        handlerExists = (endpointDef.sessionStartHandler != nullptr);
        break;
    case Network::Protocols::WebSocket::MESSAGE_RECEIVED:
        handlerExists = (endpointDef.messageReceivedHandler != nullptr);
        break;
    case Network::Protocols::WebSocket::SESSION_END:
        handlerExists = (endpointDef.sessionEndHandler != nullptr);
        break;
    }

    if (!handlerExists)
    {
        return INVALID_EVENT_TYPE;
    }

    invokeHandler(endpointDef, eventType, content, parameters);
    return SUCCESS;
}
