#include "engine.h"
#include <Mantids30/Servers/RESTfulWebServer/api_restful_endpoints.h>
#include <Mantids30/Servers/RESTfulWebServer/api_websocket_endpoint.h>

#include <Mantids30/Core/Helpers/encoders.h>
#include <Mantids30/DataFormats/JSON/json.h>
#include <memory>

#include <Mantids30/Servers/RESTfulWebServer/security.h>
#include "clienthandler.h"

using namespace Mantids30;
using namespace Network::Servers;
using namespace Mantids30::Protocols;
using namespace Mantids30::Servers::RESTfulWebServer;
using namespace Mantids30::Servers::RESTfulWebServer::RESTful;

Engine::Engine()
{
    std::shared_ptr<Endpoints> handler = std::make_shared<Endpoints>();

    // This method handler will be used for IAM notification of revoked token...
    RESTfulAPIEndpointFullDefinition jwtRevokeDef;
    jwtRevokeDef.endpointDefinition = &revokeJWT;
    jwtRevokeDef.security.requiredScopes = {"IAM"};
    jwtRevokeDef.context = this;
    handler->addEndpoint(Mantids30::Protocols::HTTP::Method::POST, "jwt/revoke", jwtRevokeDef);
    handler->addEndpoint(Mantids30::Protocols::HTTP::Method::PUT, "websockets/topics/subscribe", Mantids30::Servers::RESTfulWebServer::Security::Requirements::JWT_COOKIE_AUTH, {"WSTOPICS"}, this, subscribeToTopic);
    handler->addEndpoint(Mantids30::Protocols::HTTP::Method::DELETE, "websockets/topics/unsubscribe", Mantids30::Servers::RESTfulWebServer::Security::Requirements::JWT_COOKIE_AUTH, {"WSTOPICS"}, this, unsubscribeFromTopic);

    endpointsHandler[0] = handler;
}

Mantids30::Protocols::HTTP::APIReturn Engine::revokeJWT(void *context,                                        // Context pointer
                                 const Mantids30::Servers::RESTfulWebServer::RESTful::RequestContext &request,       // Parameters from the RESTful request
                                 Mantids30::Servers::RESTfulWebServer::Sessions::ClientDetails &authClientDetails // Client authentication details
)
{
    std::string jwtSignature = Mantids30::Core::Helpers::Encoders::decodeFromBase64(Mantids30::DataFormats::JSON::ASSTRING(*request.inputJSON, "signature", ""), true);
    time_t expirationTime = Mantids30::DataFormats::JSON::ASINT64(*request.inputJSON, "expiration", 0);
    (static_cast<Engine *>(context))->config.jwtValidator->m_revocation.addToRevocationList(jwtSignature, expirationTime);

    return {};
}

Mantids30::Protocols::HTTP::APIReturn Engine::subscribeToTopic(void *context, const Mantids30::Servers::RESTfulWebServer::RESTful::RequestContext &request, Mantids30::Servers::RESTfulWebServer::Sessions::ClientDetails &authClientDetails)
{
    // Extraer parámetros de la solicitud
    const std::string_view uri = Mantids30::DataFormats::JSON::ASSTRING(*request.inputJSON, "uri", "");
    const std::string_view webSocketSessionId = Mantids30::DataFormats::JSON::ASSTRING(*request.inputJSON, "webSocketSessionId", "");
    const std::string_view topicId = Mantids30::DataFormats::JSON::ASSTRING(*request.inputJSON, "topicId", "");

    // Validar que se proporcionaron los parámetros necesarios
    if (uri.empty() || webSocketSessionId.empty() || topicId.empty())
    {
        return {Mantids30::Protocols::HTTP::Status::Code::S_400_BAD_REQUEST, "missing_parameters", "Required parameters (uri, webSocketSessionId, topicId) are missing"};
    }

    // Obtener el endpoint WebSocket
    const Mantids30::Servers::RESTfulWebServer::WebSocket::Endpoint *endpoint = (static_cast<Engine *>(context))->m_websocketEndpoints->getWebSocketEndpointByURI(uri);
    if (!endpoint)
    {
        return {Mantids30::Protocols::HTTP::Status::Code::S_404_NOT_FOUND, "invalid_uri", "WebSocket endpoint not found"};
    }

    // Intentar unirse al tema de suscripción
    if (!endpoint->joinTopicSubscription(webSocketSessionId, topicId))
    {
        return {Mantids30::Protocols::HTTP::Status::Code::S_429_TOO_MANY_REQUESTS, "limits_reached", "Connection limit reached for this topic"};
    }

    // Suscripción exitosa
    return {};
}

Mantids30::Protocols::HTTP::APIReturn Engine::unsubscribeFromTopic(void *context, const Mantids30::Servers::RESTfulWebServer::RESTful::RequestContext &request, Mantids30::Servers::RESTfulWebServer::Sessions::ClientDetails &authClientDetails)
{
    // Extraer parámetros de la solicitud
    const std::string_view uri = Mantids30::DataFormats::JSON::ASSTRING(*request.inputJSON, "uri", "");
    const std::string_view webSocketSessionId = Mantids30::DataFormats::JSON::ASSTRING(*request.inputJSON, "webSocketSessionId", "");
    const std::string_view topicId = Mantids30::DataFormats::JSON::ASSTRING(*request.inputJSON, "topicId", "");

    // Validar que se proporcionaron los parámetros necesarios
    if (uri.empty() || webSocketSessionId.empty() || topicId.empty())
    {
        return {Mantids30::Protocols::HTTP::Status::Code::S_400_BAD_REQUEST, "missing_parameters", "Required parameters (uri, webSocketSessionId, topicId) are missing"};
    }

    // Obtener el endpoint WebSocket
    const Mantids30::Servers::RESTfulWebServer::WebSocket::Endpoint *endpoint = ((Engine *) context)->m_websocketEndpoints->getWebSocketEndpointByURI(uri);
    if (!endpoint)
    {
        return {Mantids30::Protocols::HTTP::Status::Code::S_404_NOT_FOUND, "invalid_uri", "WebSocket endpoint not found"};
    }

    // Intentar dejar el tema de suscripción
    if (!endpoint->leaveTopicSubscription(webSocketSessionId, topicId))
    {
        return {Mantids30::Protocols::HTTP::Status::Code::S_404_NOT_FOUND, "not_subscribed", "Client is not subscribed to this topic"};
    }

    // Desuscripción exitosa
    return {};
}

std::shared_ptr<Mantids30::Servers::RESTfulWebServer::APIServer_ClientHandler> Engine::createNewAPIServer_ClientHandler(APIServerCore *webServer, const std::shared_ptr<Mantids30::Network::Sockets::Socket_Stream> &s)
{
    std::shared_ptr<Mantids30::Servers::RESTfulWebServer::ClientHandler> clientHandler = std::make_shared<Mantids30::Servers::RESTfulWebServer::ClientHandler>(webServer, s);
    clientHandler->m_endpointsHandler = endpointsHandler;
    clientHandler->jwtAccessTokenName = jwtAccessTokenName;
    return clientHandler;
}
