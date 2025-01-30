#include "engine.h"
#include <Mantids30/API_RESTful/methodshandler.h>
#include <Mantids30/Helpers/json.h>
#include <Mantids30/Helpers/encoders.h>
#include <memory>

#include "clienthandler.h"

using namespace Mantids30;
using namespace Network::Servers;
using namespace Network::Servers::RESTful;


Engine::Engine()
{
    // This method handler will be used for IAM inter-process communication (like token revokations)
    std::shared_ptr<API::RESTful::MethodsHandler> handler = std::make_shared<API::RESTful::MethodsHandler>();

    API::RESTful::RESTfulAPIDefinition jwtRevokeDef;
    jwtRevokeDef.method = &revokeJWT;
    jwtRevokeDef.security.requiredPermissions = {"IAM"};
    jwtRevokeDef.context = this;

    handler->addResource(API::RESTful::MethodsHandler::POST, "revokeJWT", jwtRevokeDef);
    methodsHandler[0] = handler;

}

Engine::~Engine()
{
}

void Engine::revokeJWT(
    void *context,                                          // Context pointer
    API::APIReturn &response,                               // The API return object
    json &responseData,                                     // Response data (JSON format)
    const API::RESTful::RequestParameters &requestParams,   // Parameters from the RESTful request
    const json &inputData,                                  // Input data (JSON format)
    const DataFormat::JWT::Token &authToken,                // Authentication token (JWT)
    Mantids30::Sessions::ClientDetails &authClientDetails   // Client authentication details
    )
{
    // TODO: pasar a parametros POST...
    std::string jwtSignature = Helpers::Encoders::decodeFromBase64(JSON_ASSTRING(requestParams.pathParameters, "signature",""),true);
    time_t expirationTime = JSON_ASUINT64(requestParams.pathParameters, "expiration",0);
    ((Engine *)context)->config.jwtValidator->m_revocation.addToRevocationList( jwtSignature, expirationTime );
    responseData = (json)true;
}

std::shared_ptr<Web::APIClientHandler> Engine::createNewAPIClientHandler(APIEngineCore * webServer, std::shared_ptr<Sockets::Socket_Stream_Base> s)
{
    auto clientHandler = std::make_shared<RESTful::ClientHandler>(webServer,s);
    clientHandler->m_methodsHandler = methodsHandler;
    return clientHandler;
}
