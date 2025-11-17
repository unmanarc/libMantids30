#include "api_websocket_endpoints.h"
#include "Mantids30/Protocol_HTTP/websocket_eventtype.h"

#include "json/value.h"
#include <Mantids30/Helpers/json.h>
#include <Mantids30/Protocol_HTTP/rsp_status.h>
#include <Mantids30/Threads/lock_shared.h>

using namespace Mantids30;
using namespace Mantids30::Network::Protocols;
using namespace API::WebSocket;

bool Endpoints::addEndpoint(const std::string &endpointPath, const WebSocket::Endpoint &endpointDefinition)
{
    m_endpoints[endpointPath] = endpointDefinition;
    m_endpoints[endpointPath].config = &(config);
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

bool Endpoints::invokeHandler(const WebSocket::Endpoint &endpointDef, const Network::Protocols::WebSocket::EventType &eventType, std::shared_ptr<Memory::Containers::B_Chunks> content,
                              const WebSocketParameters &parameters)
{
    WebSocketEventFunctionType handler = nullptr;

    switch (eventType)
    {
    case Network::Protocols::WebSocket::SESSION_START:
        handler = endpointDef.sessionStartHandler;
        break;
    case Network::Protocols::WebSocket::RECEIVED_MESSAGE_TEXT:
        handler = endpointDef.textMessageReceivedHandler;
        break;
    case Network::Protocols::WebSocket::RECEIVED_MESSAGE_BINARY:
        handler = endpointDef.binaryMessageReceivedHandler;
        break;
    case Network::Protocols::WebSocket::SESSION_END:
        handler = endpointDef.sessionEndHandler;
        break;
    default:
        return false; // Empty return for invalid event type (do nothing)
    }

    if (handler != nullptr)
    {
        Mantids30::Sessions::ClientDetails clientDetails = extractClientDetails(parameters);

        Json::Value jsonContent;
        Json::CharReaderBuilder reader;
        std::unique_ptr<Json::CharReader> charReader(reader.newCharReader()); // create a unique_ptr to manage the JsonCpp char reader
        Json::Value header;
        std::string errs;
        std::vector<char> buf = content->copyToBuffer();
        if (config->translateWebSocketTextMessagesToJSON && (eventType == Network::Protocols::WebSocket::RECEIVED_MESSAGE_TEXT || eventType == Network::Protocols::WebSocket::SESSION_END))
        {
            if (!charReader->parse(buf.data(), buf.data() + buf.size(), &jsonContent, &errs))
            {
                // Failed to translate...
            }
        }

        handler(endpointDef.context, content, jsonContent, parameters, clientDetails);
    }
    return true;
}

Endpoints::ErrorCodes Endpoints::checkEndpoint(const std::string &endpointPath, const std::set<std::string> &currentScopes, bool isAdmin, const SecurityParameters &securityParameters)
{
    it = m_endpoints.find(endpointPath);
    if (it == m_endpoints.end())
    {
        return ENDPOINT_NOT_FOUND;
    }

    const WebSocket::Endpoint &endpointDef = it->second;

    // Security checks
    if (endpointDef.security.requireJWTHeaderAuthentication && !securityParameters.haveJWTAuthHeader)
    {
        return AUTHENTICATION_REQUIRED;
    }

    if (endpointDef.security.requireJWTCookieAuthentication && !securityParameters.haveJWTAuthCookie)
    {
        return AUTHENTICATION_REQUIRED;
    }

    if (endpointDef.security.requireSession && !securityParameters.haveSession)
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

const Endpoint *Endpoints::getWebSocketEndpointByURI(const std::string &uri) const
{
    auto it = m_endpoints.find(uri);
    if (it != m_endpoints.end())
    {
        return &(it->second);
    }
    return nullptr;
}

Endpoints::ErrorCodes Endpoints::handleEvent(const Network::Protocols::WebSocket::EventType &eventType, std::shared_ptr<Memory::Containers::B_Chunks> content,
                                             WebSocket::WebSocketParameters &parameters)
{
    if (it == m_endpoints.end())
    {
        return ENDPOINT_NOT_FOUND;
    }

    const WebSocket::Endpoint &endpointDef = it->second;

    if (invokeHandler(endpointDef, eventType, content, parameters))
        return SUCCESS;
    else
        return INVALID_EVENT_TYPE;
}
