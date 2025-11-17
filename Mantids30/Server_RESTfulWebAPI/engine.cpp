#include "engine.h"
#include <Mantids30/API_EndpointsAndSessions/api_restful_endpoints.h>
#include <Mantids30/API_EndpointsAndSessions/api_websocket_endpoint.h>

#include <Mantids30/Helpers/encoders.h>
#include <Mantids30/Helpers/json.h>
#include <memory>

#include "clienthandler.h"

using namespace Mantids30;
using namespace Network::Servers;
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
    handler->addEndpoint(Endpoints::POST, "jwt/revoke", jwtRevokeDef);
    handler->addEndpoint(Endpoints::PUT, "websockets/topics/subscribe", Endpoints::REQUIRE_JWT_COOKIE_AUTH, {"WSTOPICS"}, this, subscribeToTopic);
    handler->addEndpoint(Endpoints::DELETE, "websockets/topics/unsubscribe", Endpoints::REQUIRE_JWT_COOKIE_AUTH, {"WSTOPICS"}, this, unsubscribeFromTopic);

    endpointsHandler[0] = handler;
}

Engine::~Engine() {}

API::APIReturn Engine::revokeJWT(void *context,                                        // Context pointer
                                 const API::RESTful::RequestParameters &request,       // Parameters from the RESTful request
                                 Mantids30::Sessions::ClientDetails &authClientDetails // Client authentication details
)
{
    std::string jwtSignature = Helpers::Encoders::decodeFromBase64(JSON_ASSTRING(*request.inputJSON, "signature", ""), true);
    time_t expirationTime = JSON_ASUINT64(*request.inputJSON, "expiration", 0);
    ((Engine *) context)->config.jwtValidator->m_revocation.addToRevocationList(jwtSignature, expirationTime);

    return API::APIReturn();
}

API::APIReturn Engine::subscribeToTopic(void *context, const API::RESTful::RequestParameters &request, Sessions::ClientDetails &authClientDetails)
{
    // Extraer parámetros de la solicitud
    const std::string uri = JSON_ASSTRING(*request.inputJSON, "uri", "");
    const std::string webSocketSessionId = JSON_ASSTRING(*request.inputJSON, "webSocketSessionId", "");
    const std::string topicId = JSON_ASSTRING(*request.inputJSON, "topicId", "");

    // Validar que se proporcionaron los parámetros necesarios
    if (uri.empty() || webSocketSessionId.empty() || topicId.empty())
    {
        return API::APIReturn(Protocols::HTTP::Status::S_400_BAD_REQUEST, "missing_parameters", "Required parameters (uri, webSocketSessionId, topicId) are missing");
    }

    // Obtener el endpoint WebSocket
    const API::WebSocket::Endpoint *endpoint = ((Engine *) context)->m_websocketEndpoints->getWebSocketEndpointByURI(uri);
    if (!endpoint)
    {
        return API::APIReturn(Protocols::HTTP::Status::S_404_NOT_FOUND, "invalid_uri", "WebSocket endpoint not found");
    }

    // Intentar unirse al tema de suscripción
    if (!endpoint->joinTopicSubscription(webSocketSessionId, topicId))
    {
        return API::APIReturn(Protocols::HTTP::Status::S_429_TOO_MANY_REQUESTS, "limits_reached", "Connection limit reached for this topic");
    }

    // Suscripción exitosa
    return API::APIReturn();
}

API::APIReturn Engine::unsubscribeFromTopic(void *context, const API::RESTful::RequestParameters &request, Sessions::ClientDetails &authClientDetails)
{
    // Extraer parámetros de la solicitud
    const std::string uri = JSON_ASSTRING(*request.inputJSON, "uri", "");
    const std::string webSocketSessionId = JSON_ASSTRING(*request.inputJSON, "webSocketSessionId", "");
    const std::string topicId = JSON_ASSTRING(*request.inputJSON, "topicId", "");

    // Validar que se proporcionaron los parámetros necesarios
    if (uri.empty() || webSocketSessionId.empty() || topicId.empty())
    {
        return API::APIReturn(Protocols::HTTP::Status::S_400_BAD_REQUEST,
                              "missing_parameters",
                              "Required parameters (uri, webSocketSessionId, topicId) are missing");
    }

    // Obtener el endpoint WebSocket
    const API::WebSocket::Endpoint *endpoint = ((Engine *) context)->m_websocketEndpoints->getWebSocketEndpointByURI(uri);
    if (!endpoint)
    {
        return API::APIReturn(Protocols::HTTP::Status::S_404_NOT_FOUND,
                              "invalid_uri",
                              "WebSocket endpoint not found");
    }

    // Intentar dejar el tema de suscripción
    if (!endpoint->leaveTopicSubscription(webSocketSessionId, topicId))
    {
        return API::APIReturn(Protocols::HTTP::Status::S_404_NOT_FOUND,
                              "not_subscribed",
                              "Client is not subscribed to this topic");
    }

    // Desuscripción exitosa
    return API::APIReturn();
}

std::shared_ptr<Web::APIClientHandler> Engine::createNewAPIClientHandler(APIEngineCore *webServer, std::shared_ptr<Sockets::Socket_Stream> s)
{
    auto clientHandler = std::make_shared<RESTful::ClientHandler>(webServer, s);
    clientHandler->m_endpointsHandler = endpointsHandler;
    return clientHandler;
}
