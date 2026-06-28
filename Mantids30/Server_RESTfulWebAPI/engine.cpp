#include "engine.h"
#include <Mantids30/API_EndpointsAndSessions/api_restful_endpoints.h>
#include <Mantids30/API_EndpointsAndSessions/api_websocket_endpoint.h>

#include <Mantids30/Helpers/encoders.h>
#include <Mantids30/Helpers/json.h>
#include <memory>

#include <Mantids30/API_EndpointsAndSessions/security.h>
#include "clienthandler.h"

using namespace Mantids30;
using namespace Network::Servers;
using namespace Network::Protocol;
using namespace Network::Servers::RESTful;
using namespace API::RESTful;

Engine::Engine()
{
    std::shared_ptr<Endpoints> handler = std::make_shared<Endpoints>();

    // This method handler will be used for IAM notification of revoked token...
    RESTfulAPIEndpointFullDefinition jwtRevokeDef;
    jwtRevokeDef.endpointDefinition = &revokeJWT;
    jwtRevokeDef.security.requiredScopes = {"IAM"};
    jwtRevokeDef.context = this;
    handler->addEndpoint(HTTP::Method::POST, "jwt/revoke", jwtRevokeDef);
    handler->addEndpoint(HTTP::Method::PUT, "websockets/topics/subscribe", API::Security::Requirements::JWT_COOKIE_AUTH, {"WSTOPICS"}, this, subscribeToTopic);
    handler->addEndpoint(HTTP::Method::DELETE, "websockets/topics/unsubscribe", API::Security::Requirements::JWT_COOKIE_AUTH, {"WSTOPICS"}, this, unsubscribeFromTopic);

    endpointsHandler[0] = handler;
}

API::APIReturn Engine::revokeJWT(void *context,                                        // Context pointer
                                 const API::RESTful::RequestContext &request,       // Parameters from the RESTful request
                                 Mantids30::Sessions::ClientDetails &authClientDetails // Client authentication details
)
{
    std::string jwtSignature = Helpers::Encoders::decodeFromBase64(Helpers::JSON::ASSTRING(*request.inputJSON, "signature", ""), true);
    time_t expirationTime = Helpers::JSON::ASUINT64(*request.inputJSON, "expiration", 0);
    ((Engine *) context)->config.jwtValidator->m_revocation.addToRevocationList(jwtSignature, expirationTime);

    return {};
}

API::APIReturn Engine::subscribeToTopic(void *context, const API::RESTful::RequestContext &request, Sessions::ClientDetails &authClientDetails)
{
    // Extraer parámetros de la solicitud
    const std::string uri = Helpers::JSON::ASSTRING(*request.inputJSON, "uri", "");
    const std::string webSocketSessionId = Helpers::JSON::ASSTRING(*request.inputJSON, "webSocketSessionId", "");
    const std::string topicId = Helpers::JSON::ASSTRING(*request.inputJSON, "topicId", "");

    // Validar que se proporcionaron los parámetros necesarios
    if (uri.empty() || webSocketSessionId.empty() || topicId.empty())
    {
        return API::APIReturn(HTTP::Status::Code::S_400_BAD_REQUEST, "missing_parameters", "Required parameters (uri, webSocketSessionId, topicId) are missing");
    }

    // Obtener el endpoint WebSocket
    const API::WebSocket::Endpoint *endpoint = ((Engine *) context)->m_websocketEndpoints->getWebSocketEndpointByURI(uri);
    if (!endpoint)
    {
        return API::APIReturn(HTTP::Status::Code::S_404_NOT_FOUND, "invalid_uri", "WebSocket endpoint not found");
    }

    // Intentar unirse al tema de suscripción
    if (!endpoint->joinTopicSubscription(webSocketSessionId, topicId))
    {
        return API::APIReturn(HTTP::Status::Code::S_429_TOO_MANY_REQUESTS, "limits_reached", "Connection limit reached for this topic");
    }

    // Suscripción exitosa
    return {};
}

API::APIReturn Engine::unsubscribeFromTopic(void *context, const API::RESTful::RequestContext &request, Sessions::ClientDetails &authClientDetails)
{
    // Extraer parámetros de la solicitud
    const std::string uri = Helpers::JSON::ASSTRING(*request.inputJSON, "uri", "");
    const std::string webSocketSessionId = Helpers::JSON::ASSTRING(*request.inputJSON, "webSocketSessionId", "");
    const std::string topicId = Helpers::JSON::ASSTRING(*request.inputJSON, "topicId", "");

    // Validar que se proporcionaron los parámetros necesarios
    if (uri.empty() || webSocketSessionId.empty() || topicId.empty())
    {
        return API::APIReturn(HTTP::Status::Code::S_400_BAD_REQUEST, "missing_parameters", "Required parameters (uri, webSocketSessionId, topicId) are missing");
    }

    // Obtener el endpoint WebSocket
    const API::WebSocket::Endpoint *endpoint = ((Engine *) context)->m_websocketEndpoints->getWebSocketEndpointByURI(uri);
    if (!endpoint)
    {
        return API::APIReturn(HTTP::Status::Code::S_404_NOT_FOUND, "invalid_uri", "WebSocket endpoint not found");
    }

    // Intentar dejar el tema de suscripción
    if (!endpoint->leaveTopicSubscription(webSocketSessionId, topicId))
    {
        return API::APIReturn(HTTP::Status::Code::S_404_NOT_FOUND, "not_subscribed", "Client is not subscribed to this topic");
    }

    // Desuscripción exitosa
    return {};
}

std::shared_ptr<Web::APIServer_ClientHandler> Engine::createNewAPIServer_ClientHandler(APIServerCore *webServer, const std::shared_ptr<Sockets::Socket_Stream> &s)
{
    std::shared_ptr<RESTful::ClientHandler> clientHandler = std::make_shared<RESTful::ClientHandler>(webServer, s);
    clientHandler->m_endpointsHandler = endpointsHandler;
    clientHandler->jwtAccessTokenName = jwtAccessTokenName;
    return clientHandler;
}
