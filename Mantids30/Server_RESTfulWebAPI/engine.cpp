#include "engine.h"
#include <Mantids30/API_RESTful/methodshandler.h>
#include <Mantids30/Helpers/json.h>
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
    jwtRevokeDef.obj = this;

    handler->addResource(API::RESTful::MethodsHandler::POST, "revokeJWT", jwtRevokeDef);
    m_methodsHandler[0] = handler;
}

Engine::~Engine()
{
}

void Engine::revokeJWT(API::RESTful::APIReturn &response,
                                          void *context,
                                          Mantids30::Auth::ClientDetails &authClientDetails,
                                          const json &inputData,
                                          json &responseData,
                                          const DataFormat::JWT::Token &authToken,
                                          const API::RESTful::RequestParameters &requestParams)
{
    // TODO: pasar a parametros POST...
    std::string jwtSignature = Helpers::Encoders::decodeFromBase64(JSON_ASSTRING(requestParams.pathParameters, "signature",""),true);
    time_t expirationTime = JSON_ASUINT64(requestParams.pathParameters, "expiration",0);
    ((Engine *)context)->m_jwtValidator->m_revocation.addToRevocationList( jwtSignature, expirationTime );
    responseData = (json)true;
}

Web::APIClientHandler *Engine::createNewAPIClientHandler(APIEngineCore * webServer, Network::Sockets::Socket_Stream_Base * s)
{
    auto clientHandler = new RESTful::ClientHandler(webServer,s);
    clientHandler->m_jwtValidator = m_jwtValidator;
    clientHandler->m_jwtSigner = m_jwtSigner;
    clientHandler->m_methodsHandler = m_methodsHandler;
    return clientHandler;
}
