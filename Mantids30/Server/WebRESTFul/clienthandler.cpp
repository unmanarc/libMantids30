#include "clienthandler.h"
#include "security.h"

#include "api_restful_endpoints.h"
#include <Mantids30/Core/Helpers/encoders.h>
#include <Mantids30/Runtime/Logs/loglevels.h>
#include <Mantids30/Protocol/HTTP/rsp_cookies.h>
#include <Mantids30/Protocol/HTTP/rsp_status.h>


#include <memory>
#include <string>

using namespace Mantids30::Server::WebRESTFul;
using namespace Mantids30::Runtime::Logs;
using namespace Mantids30::Protocol;
using namespace Mantids30::DataFormat;
using namespace Mantids30::Core::Memory;
using namespace Mantids30;
using namespace std;

ClientHandler::ClientHandler(void *parent, const std::shared_ptr<Mantids30::Core::Memory::Streams::StreamableObject> &sock)
    : Mantids30::Server::WebRESTFul::APIServer_ClientHandler(parent, sock)
{}

Mantids30::Protocol::HTTP::APIReturn ClientHandler::handleAPIRequest(const string &baseApiUrl, const uint32_t &apiVersion, const string &httpMethodMode, const string &endpointName, const boost::json::value &postParameters)
{
    Mantids30::Protocol::HTTP::APIReturn apiReturn;
    set<string> currentScopes;
    bool isAdmin = false;
    bool authenticated = false;
    Mantids30::Server::WebRESTFul::RESTful::RequestContext requestContext;

    requestContext.jwtSigner = this->config->jwtSigner;
    requestContext.jwtValidator = this->config->jwtValidator;
    requestContext.clientRequest = &clientRequest;
    boost::json::value embeddedParameters;
    if (m_isEmbeddedAPICall)
    {
        embeddedParameters = postParameters;
        requestContext.explicitInputJSON = &embeddedParameters;
    }

    if (isSessionActive())
    {
        isAdmin = jwtToken.isAdmin();
        currentScopes = jwtToken.getAllScopes();
        requestContext.jwtToken = &jwtToken;
    }

    if (m_endpointsHandler.find(apiVersion) == m_endpointsHandler.end())
    {
        log(LogLevel::WARNING, "restAPI", 2048, "API Version Not Found / Resource Not found {ver=%u, mode=%s, endpoint=%s}", apiVersion, httpMethodMode.c_str(), endpointName.c_str());
        apiReturn.setError(Mantids30::Protocol::HTTP::Status::Code::S_404_NOT_FOUND, "invalid_api_request", "Resource Not Found");
        return apiReturn;
    }

    boost::json::value x;
    Mantids30::Server::WebRESTFul::Security::ReceivedAuth securityParameters;
    securityParameters.hasVerifiedJWTAccessTokenCookie = m_isAccessTokenCookieJWTVerified;
    securityParameters.hasVerifiedJWTAuthorizationHeader = m_isAuthorizationHeaderJWTVerified;

    Mantids30::Server::WebRESTFul::RESTful::Endpoints::HandleResult result = m_endpointsHandler[apiVersion]->handleEndpoint(httpMethodMode, endpointName, requestContext, currentScopes, isAdmin, securityParameters, &apiReturn);

    switch (result)
    {
    case Mantids30::Server::WebRESTFul::RESTful::Endpoints::HandleResult::SUCCESS:
        log(LogLevel::DEBUG, "restAPI", 2048, "API REST Endpoint Executed {ver=%u, mode=%s, endpoint=%s}", apiVersion, httpMethodMode.c_str(), endpointName.c_str());
        break;
    case Mantids30::Server::WebRESTFul::RESTful::Endpoints::HandleResult::INVALID_METHOD_MODE:
        log(LogLevel::WARNING, "restAPI", 2048, "API REST Invalid Endpoint Mode {ver=%u, mode=%s, endpoint=%s}", apiVersion, httpMethodMode.c_str(), endpointName.c_str());
        break;
    case Mantids30::Server::WebRESTFul::RESTful::Endpoints::HandleResult::RESOURCE_NOT_FOUND:
        log(LogLevel::WARNING, "restAPI", 2048, "API REST Endpoint Not Found {ver=%u, mode=%s, endpoint=%s}", apiVersion, httpMethodMode.c_str(), endpointName.c_str());
        break;
    case Mantids30::Server::WebRESTFul::RESTful::Endpoints::HandleResult::AUTHENTICATION_REQUIRED:
        log(LogLevel::SECURITY_ALERT, "restAPI", 2048, "API REST Authentication Not Provided {ver=%u, mode=%s, endpoint=%s}", apiVersion, httpMethodMode.c_str(), endpointName.c_str());
        break;
    case Mantids30::Server::WebRESTFul::RESTful::Endpoints::HandleResult::INVALID_SCOPE:
        log(LogLevel::SECURITY_ALERT, "restAPI", 2048, "API REST Invalid Scope {ver=%u, mode=%s, endpoint=%s}", apiVersion, httpMethodMode.c_str(), endpointName.c_str());
        break;
    case Mantids30::Server::WebRESTFul::RESTful::Endpoints::HandleResult::INTERNAL_ERROR:
        log(LogLevel::WARNING, "restAPI", 2048, "API REST Internal Error {ver=%u, mode=%s, endpoint=%s}", apiVersion, httpMethodMode.c_str(), endpointName.c_str());
        break;
    default:
        log(LogLevel::ERROR, "restAPI", 2048, "API REST Unknown Error {ver=%u, mode=%s, endpoint=%s}", apiVersion, httpMethodMode.c_str(), endpointName.c_str());
        apiReturn.setError(Mantids30::Protocol::HTTP::Status::Code::S_500_INTERNAL_SERVER_ERROR, "invalid_api_request", "Unknown Error");
        break;
    }

    return apiReturn;
}

Mantids30::Protocol::HTTP::APIReturn ClientHandler::handleOptionsRequest(const std::string &baseApiUrl, const uint32_t &apiVersion, const std::string &endpointName)
{
    Mantids30::Protocol::HTTP::APIReturn apiReturn;
    if (m_endpointsHandler.find(apiVersion) == m_endpointsHandler.end())
    {
        log(LogLevel::WARNING, "restAPI", 2048, "API Version Not Found / Resource Not found {ver=%u, mode=OPTIONS, endpoint=%s}", apiVersion, endpointName.c_str());
        apiReturn.setError(Mantids30::Protocol::HTTP::Status::Code::S_404_NOT_FOUND, "invalid_options_request", "Resource Not Found");
        return apiReturn;
    }

    if (m_endpointsHandler[apiVersion]->isOptionsEnabled())
    {
        // No specific handler registered, but CORS is enabled.
        // Respond with CORS headers using buildCORSOptionsResponse.
        std::string origin;

        origin = clientRequest.getOrigin();

        // Look for per-endpoint config first, then fall back to global
        const Mantids30::Server::WebRESTFul::OptionsHandlerConfig *useConfig = m_endpointsHandler[apiVersion]->getGlobalOptionsConfig();
        const Mantids30::Server::WebRESTFul::OptionsHandlerConfig *endpointConfig = m_endpointsHandler[apiVersion]->getOptionsConfigOnEndpoint(endpointName);
        if (endpointConfig != nullptr)
        {
            useConfig = endpointConfig;
        }
        return m_endpointsHandler[apiVersion]->buildCORSOptionsResponse(*useConfig, origin);
    }

    return apiReturn;
}
