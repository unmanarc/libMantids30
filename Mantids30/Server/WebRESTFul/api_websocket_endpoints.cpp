#include "api_websocket_endpoints.h"
#include <Mantids30/Protocol/HTTP/websocket_eventtype.h>

#include <Mantids30/DataFormat/JSON/json.h>
#include <Mantids30/Protocol/HTTP/rsp_status.h>

using namespace Mantids30;
using namespace Mantids30::Protocol;
using namespace Mantids30::Server::WebRESTFul::WebSocket;

bool Endpoints::addEndpoint(const std::string &endpointPath, const Mantids30::Server::WebRESTFul::WebSocket::Endpoint &endpointDefinition)
{
    m_endpoints[endpointPath] = endpointDefinition;
    m_endpoints[endpointPath].config = &(config);
    return true;
}

Mantids30::Server::WebRESTFul::Sessions::ClientDetails Endpoints::extractClientDetails(const WebSocketParameters &parameters)
{
    Mantids30::Server::WebRESTFul::Sessions::ClientDetails clientDetails;
    if (parameters.clientRequest)
    {
        clientDetails.ipAddress = parameters.clientRequest->networkClientInfo.REMOTE_ADDR;
        clientDetails.tlsCommonName = parameters.clientRequest->networkClientInfo.tlsCommonName;
        clientDetails.userAgent = parameters.clientRequest->userAgent;
    }
    return clientDetails;
}

bool Endpoints::invokeHandler(const Mantids30::Server::WebRESTFul::WebSocket::Endpoint &endpointDef, const Mantids30::Protocol::HTTP::WebSocket::EventType &eventType, const std::shared_ptr<Mantids30::Core::Memory::Containers::B_Chunks> &content,
                              const WebSocketParameters &parameters)
{
    WebSocketEventFunctionType handler = nullptr;

    switch (eventType)
    {
    case Mantids30::Protocol::HTTP::WebSocket::EventType::SESSION_START:
        handler = endpointDef.sessionStartHandler;
        break;
    case Mantids30::Protocol::HTTP::WebSocket::EventType::RECEIVED_MESSAGE_TEXT:
        handler = endpointDef.textMessageReceivedHandler;
        break;
    case Mantids30::Protocol::HTTP::WebSocket::EventType::RECEIVED_MESSAGE_BINARY:
        handler = endpointDef.binaryMessageReceivedHandler;
        break;
    case Mantids30::Protocol::HTTP::WebSocket::EventType::SESSION_END:
        handler = endpointDef.sessionEndHandler;
        break;
    default:
        return false; // Empty return for invalid event type (do nothing)
    }

    if (handler != nullptr)
    {
        Mantids30::Server::WebRESTFul::Sessions::ClientDetails clientDetails = extractClientDetails(parameters);

        boost::json::value jsonContent;
        Mantids30::DataFormat::JSON::Reader charReader;
        std::vector<char> buf = content->copyToBuffer();
        if (config->translateWebSocketTextMessagesToJSON
            && (eventType == Mantids30::Protocol::HTTP::WebSocket::EventType::RECEIVED_MESSAGE_TEXT || eventType == Mantids30::Protocol::HTTP::WebSocket::EventType::SESSION_END))
        {
            if (!charReader.parse(std::string(buf.data(), buf.size()), jsonContent))
            {
                // Failed to translate...
            }
        }

        handler(endpointDef.context, content, jsonContent, parameters, clientDetails);
    }
    return true;
}

Endpoints::HandleResult Endpoints::checkEndpoint(const std::string &endpointPath, const std::set<std::string> &currentScopes, bool isAdmin, const SecurityParameters &securityParameters)
{
    it = m_endpoints.find(endpointPath);
    if (it == m_endpoints.end())
    {
        return HandleResult::ENDPOINT_NOT_FOUND;
    }

    const Mantids30::Server::WebRESTFul::WebSocket::Endpoint &endpointDef = it->second;

    // Security checks
    if (endpointDef.security.requireJWTHeaderAuthentication && !securityParameters.haveJWTAuthHeader)
    {
        return HandleResult::AUTHENTICATION_REQUIRED;
    }

    if (endpointDef.security.requireJWTCookieAuthentication && !securityParameters.haveJWTAuthCookie)
    {
        return HandleResult::AUTHENTICATION_REQUIRED;
    }

    if (endpointDef.security.requireSession && !securityParameters.haveSession)
    {
        return HandleResult::AUTHENTICATION_REQUIRED;
    }

    if (!isAdmin)
    {
        for (const std::string &scope : endpointDef.security.requiredScopes)
        {
            if (currentScopes.find(scope) == currentScopes.end())
            {
                return HandleResult::INVALID_SCOPE;
            }
        }
    }

    return HandleResult::SUCCESS;
}

const Endpoint* Endpoints::getWebSocketEndpointByURI(std::string_view uri) const
{
    auto it = m_endpoints.find(uri);
    return it != m_endpoints.end() ? &it->second : nullptr;
}

Endpoints::HandleResult Endpoints::handleEvent(const Mantids30::Protocol::HTTP::WebSocket::EventType &eventType, const std::shared_ptr<Mantids30::Core::Memory::Containers::B_Chunks> &content,
                                               Mantids30::Server::WebRESTFul::WebSocket::WebSocketParameters &parameters)
{
    if (it == m_endpoints.end())
    {
        return HandleResult::ENDPOINT_NOT_FOUND;
    }

    const Mantids30::Server::WebRESTFul::WebSocket::Endpoint &endpointDef = it->second;

    if (invokeHandler(endpointDef, eventType, content, parameters))
    {
        return HandleResult::SUCCESS;
    }
    else
    {
        return HandleResult::INVALID_EVENT_TYPE;
    }
}
