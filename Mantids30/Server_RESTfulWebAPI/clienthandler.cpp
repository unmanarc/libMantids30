#include "clienthandler.h"
#include <Mantids30/API_EndpointsAndSessions/security.h>

#include <Mantids30/API_EndpointsAndSessions/api_restful_endpoints.h>
#include <Mantids30/Helpers/encoders.h>
#include <Mantids30/Program_Logs/loglevels.h>
#include <Mantids30/Protocol_HTTP/rsp_cookies.h>
#include <Mantids30/Protocol_HTTP/rsp_status.h>

#include <json/config.h>
#include <json/json.h>
#include <json/value.h>

#include <memory>
#include <string>

using namespace Mantids30::Network::Servers::RESTful;
using namespace Mantids30::Program::Logs;
using namespace Mantids30::Network;
using namespace Mantids30::Network::Protocol;
using namespace Mantids30::DataFormat;
using namespace Mantids30::Memory;
using namespace Mantids30;
using namespace std;

ClientHandler::ClientHandler(void *parent, const std::shared_ptr<Memory::Streams::StreamableObject> &sock)
    : Servers::Web::APIServer_ClientHandler(parent, sock)
{}

API::APIReturn ClientHandler::handleAPIRequest(const string &baseApiUrl, const uint32_t &apiVersion, const string &httpMethodMode, const string &endpointName, const Json::Value &postParameters)
{
    API::APIReturn apiReturn;
    set<string> currentScopes;
    bool isAdmin = false;
    bool authenticated = false;
    API::RESTful::RequestContext requestContext;

    requestContext.jwtSigner = this->config->jwtSigner;
    requestContext.jwtValidator = this->config->jwtValidator;
    requestContext.clientRequest = &clientRequest;

    if (isSessionActive())
    {
        isAdmin = jwtToken.isAdmin();
        currentScopes = jwtToken.getAllScopes();
        requestContext.jwtToken = &jwtToken;
    }

    if (m_endpointsHandler.find(apiVersion) == m_endpointsHandler.end())
    {
        log(LogLevel::WARN, "restAPI", 2048, "API Version Not Found / Resource Not found {ver=%u, mode=%s, endpoint=%s}", apiVersion, httpMethodMode.c_str(), endpointName.c_str());
        apiReturn.setError(HTTP::Status::Code::S_404_NOT_FOUND, "invalid_api_request", "Resource Not Found");
        return apiReturn;
    }

    json x;
    API::Security::ReceivedAuth securityParameters;
    securityParameters.hasVerifiedJWTAccessTokenCookie = m_isAccessTokenCookieJWTVerified;
    securityParameters.hasVerifiedJWTAuthorizationHeader = m_isAuthorizationHeaderJWTVerified;

    API::RESTful::Endpoints::HandleResult result = m_endpointsHandler[apiVersion]->handleEndpoint(httpMethodMode, endpointName, requestContext, currentScopes, isAdmin, securityParameters, &apiReturn);

    switch (result)
    {
    case API::RESTful::Endpoints::HandleResult::SUCCESS:
        log(LogLevel::DEBUG, "restAPI", 2048, "API REST Endpoint Executed {ver=%u, mode=%s, endpoint=%s}", apiVersion, httpMethodMode.c_str(), endpointName.c_str());
        break;
    case API::RESTful::Endpoints::HandleResult::INVALID_METHOD_MODE:
        log(LogLevel::WARN, "restAPI", 2048, "API REST Invalid Endpoint Mode {ver=%u, mode=%s, endpoint=%s}", apiVersion, httpMethodMode.c_str(), endpointName.c_str());
        break;
    case API::RESTful::Endpoints::HandleResult::RESOURCE_NOT_FOUND:
        log(LogLevel::WARN, "restAPI", 2048, "API REST Endpoint Not Found {ver=%u, mode=%s, endpoint=%s}", apiVersion, httpMethodMode.c_str(), endpointName.c_str());
        break;
    case API::RESTful::Endpoints::HandleResult::AUTHENTICATION_REQUIRED:
        log(LogLevel::WARN, "restAPI", 2048, "API REST Authentication Not Provided {ver=%u, mode=%s, endpoint=%s}", apiVersion, httpMethodMode.c_str(), endpointName.c_str());
        break;
    case API::RESTful::Endpoints::HandleResult::INVALID_SCOPE:
        log(LogLevel::WARN, "restAPI", 2048, "API REST Invalid Scope {ver=%u, mode=%s, endpoint=%s}", apiVersion, httpMethodMode.c_str(), endpointName.c_str());
        break;
    case API::RESTful::Endpoints::HandleResult::INTERNAL_ERROR:
        log(LogLevel::WARN, "restAPI", 2048, "API REST Internal Error {ver=%u, mode=%s, endpoint=%s}", apiVersion, httpMethodMode.c_str(), endpointName.c_str());
        break;
    default:
        log(LogLevel::ERR, "restAPI", 2048, "API REST Unknown Error {ver=%u, mode=%s, endpoint=%s}", apiVersion, httpMethodMode.c_str(), endpointName.c_str());
        apiReturn.setError(Protocol::HTTP::Status::Code::S_500_INTERNAL_SERVER_ERROR, "invalid_api_request", "Unknown Error");
        break;
    }

    return apiReturn;
}

API::APIReturn ClientHandler::handleOptionsRequest(const std::string &baseApiUrl, const uint32_t &apiVersion, const std::string &endpointName)
{
    API::APIReturn apiReturn;
    if (m_endpointsHandler.find(apiVersion) == m_endpointsHandler.end())
    {
        log(LogLevel::WARN, "restAPI", 2048, "API Version Not Found / Resource Not found {ver=%u, mode=OPTIONS, endpoint=%s}", apiVersion, endpointName.c_str());
        apiReturn.setError(HTTP::Status::Code::S_404_NOT_FOUND, "invalid_options_request", "Resource Not Found");
        return apiReturn;
    }

    if (m_endpointsHandler[apiVersion]->isOptionsEnabled())
    {
        // No specific handler registered, but CORS is enabled.
        // Respond with CORS headers using buildCORSOptionsResponse.
        std::string origin;

        origin = clientRequest.getOrigin();

        // Look for per-endpoint config first, then fall back to global
        const API::OptionsHandlerConfig *useConfig = m_endpointsHandler[apiVersion]->getGlobalOptionsConfig();
        const API::OptionsHandlerConfig *endpointConfig = m_endpointsHandler[apiVersion]->getOptionsConfigOnEndpoint(endpointName);
        if (endpointConfig != nullptr)
        {
            useConfig = endpointConfig;
        }
        return m_endpointsHandler[apiVersion]->buildCORSOptionsResponse(*useConfig, origin);
    }

    return apiReturn;
}
