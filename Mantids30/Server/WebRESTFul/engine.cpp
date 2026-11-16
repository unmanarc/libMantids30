#include "engine.h"
#include "api_restful_endpoints.h"
#include "api_websocket_endpoint.h"

#include <Mantids30/Core/Helpers/encoders.h>
#include <Mantids30/DataFormat/JSON/json.h>
#include <memory>

#include "security.h"
#include "clienthandler.h"

using namespace Mantids30;
using namespace Mantids30::Protocol;
using namespace Mantids30::Server::WebRESTFul;
using namespace Mantids30::Server::WebRESTFul::RESTful;

Engine::Engine()
{
    std::shared_ptr<Endpoints> handler = std::make_shared<Endpoints>();

    // This method handler will be used for IAM notification of revoked token...
    RESTfulAPIEndpointFullDefinition jwtRevokeDef;
    jwtRevokeDef.endpointDefinition = &revokeJWT;
    jwtRevokeDef.security.requiredScopes = {"IAM"};
    jwtRevokeDef.context = this;
    handler->addEndpoint(Mantids30::Protocol::HTTP::Method::POST, "jwt/revoke", jwtRevokeDef);
    handler->addEndpoint(Mantids30::Protocol::HTTP::Method::PUT, "websockets/topics/subscribe", Mantids30::Server::WebRESTFul::Security::Requirements::JWT_COOKIE_AUTH, {"WSTOPICS"}, this, subscribeToTopic);
    handler->addEndpoint(Mantids30::Protocol::HTTP::Method::DELETE, "websockets/topics/unsubscribe", Mantids30::Server::WebRESTFul::Security::Requirements::JWT_COOKIE_AUTH, {"WSTOPICS"}, this, unsubscribeFromTopic);

    endpointsHandler[0] = handler;
}

Mantids30::Protocol::HTTP::APIReturn Engine::revokeJWT(void *context,                                        // Context pointer
                                 const Mantids30::Server::WebRESTFul::RESTful::RequestContext &request,       // Parameters from the RESTful request
                                 Mantids30::Server::WebRESTFul::Sessions::ClientDetails &authClientDetails // Client authentication details
)
{
    std::string jwtSignature = Mantids30::Core::Helpers::Encoders::decodeFromBase64(Mantids30::DataFormat::JSON::ASSTRING(*request.inputJSON, "signature", ""), true);
    time_t expirationTime = Mantids30::DataFormat::JSON::ASINT64(*request.inputJSON, "expiration", 0);
    (static_cast<Engine *>(context))->config.jwtValidator->m_revocation.addToRevocationList(jwtSignature, expirationTime);

    return {};
}

Mantids30::Protocol::HTTP::APIReturn Engine::subscribeToTopic(void *context, const Mantids30::Server::WebRESTFul::RESTful::RequestContext &request, Mantids30::Server::WebRESTFul::Sessions::ClientDetails &authClientDetails)
{
    // Extraer parámetros de la solicitud
    const std::string_view uri = Mantids30::DataFormat::JSON::ASSTRING(*request.inputJSON, "uri", "");
    const std::string_view webSocketSessionId = Mantids30::DataFormat::JSON::ASSTRING(*request.inputJSON, "webSocketSessionId", "");
    const std::string_view topicId = Mantids30::DataFormat::JSON::ASSTRING(*request.inputJSON, "topicId", "");

    // Validar que se proporcionaron los parámetros necesarios
    if (uri.empty() || webSocketSessionId.empty() || topicId.empty())
    {
        return {Mantids30::Protocol::HTTP::Status::Code::S_400_BAD_REQUEST, "missing_parameters", "Required parameters (uri, webSocketSessionId, topicId) are missing"};
    }

    // Obtener el endpoint WebSocket
    const Mantids30::Server::WebRESTFul::WebSocket::Endpoint *endpoint = (static_cast<Engine *>(context))->m_websocketEndpoints->getWebSocketEndpointByURI(uri);
    if (!endpoint)
    {
        return {Mantids30::Protocol::HTTP::Status::Code::S_404_NOT_FOUND, "invalid_uri", "WebSocket endpoint not found"};
    }

    // Intentar unirse al tema de suscripción
    if (!endpoint->joinTopicSubscription(webSocketSessionId, topicId))
    {
        return {Mantids30::Protocol::HTTP::Status::Code::S_429_TOO_MANY_REQUESTS, "limits_reached", "Connection limit reached for this topic"};
    }

    // Suscripción exitosa
    return {};
}

Mantids30::Protocol::HTTP::APIReturn Engine::unsubscribeFromTopic(void *context, const Mantids30::Server::WebRESTFul::RESTful::RequestContext &request, Mantids30::Server::WebRESTFul::Sessions::ClientDetails &authClientDetails)
{
    // Extraer parámetros de la solicitud
    const std::string_view uri = Mantids30::DataFormat::JSON::ASSTRING(*request.inputJSON, "uri", "");
    const std::string_view webSocketSessionId = Mantids30::DataFormat::JSON::ASSTRING(*request.inputJSON, "webSocketSessionId", "");
    const std::string_view topicId = Mantids30::DataFormat::JSON::ASSTRING(*request.inputJSON, "topicId", "");

    // Validar que se proporcionaron los parámetros necesarios
    if (uri.empty() || webSocketSessionId.empty() || topicId.empty())
    {
        return {Mantids30::Protocol::HTTP::Status::Code::S_400_BAD_REQUEST, "missing_parameters", "Required parameters (uri, webSocketSessionId, topicId) are missing"};
    }

    // Obtener el endpoint WebSocket
    const Mantids30::Server::WebRESTFul::WebSocket::Endpoint *endpoint = static_cast<Engine *>(context)->m_websocketEndpoints->getWebSocketEndpointByURI(uri);
    if (!endpoint)
    {
        return {Mantids30::Protocol::HTTP::Status::Code::S_404_NOT_FOUND, "invalid_uri", "WebSocket endpoint not found"};
    }

    // Intentar dejar el tema de suscripción
    if (!endpoint->leaveTopicSubscription(webSocketSessionId, topicId))
    {
        return {Mantids30::Protocol::HTTP::Status::Code::S_404_NOT_FOUND, "not_subscribed", "Client is not subscribed to this topic"};
    }

    // Desuscripción exitosa
    return {};
}

std::shared_ptr<Mantids30::Server::WebRESTFul::APIServer_ClientHandler> Engine::createNewAPIServer_ClientHandler(APIServerCore *webServer, const std::shared_ptr<Mantids30::Network::Sockets::Socket_Stream> &s)
{
    std::shared_ptr<Mantids30::Server::WebRESTFul::ClientHandler> clientHandler = std::make_shared<Mantids30::Server::WebRESTFul::ClientHandler>(webServer, s);
    clientHandler->m_endpointsHandler = endpointsHandler;
    clientHandler->jwtAccessTokenName = jwtAccessTokenName;
    return clientHandler;
}
