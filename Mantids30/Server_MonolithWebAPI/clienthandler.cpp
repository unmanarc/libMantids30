#include "clienthandler.h"
#include <Mantids30/API_EndpointsAndSessions/api_options_handler.h>

using namespace Mantids30::Network::Servers::WebMonolith;
using namespace Mantids30::Program::Logs;
using namespace Mantids30::Network;
using namespace Mantids30::Network::Protocol;
using namespace Mantids30::Memory;
using namespace Mantids30;
using namespace std;

ClientHandler::ClientHandler(void *parent, const std::shared_ptr<StreamableObject> &sock)
    : Servers::Web::APIServer_ClientHandler(parent, sock)
{}

API::APIReturn ClientHandler::handleAPIRequest(const std::string &baseApiUrl, const uint32_t &apiVersion, const std::string &httpMethodMode, const std::string &endpointName,
                                               const Json::Value &postParameters)
{
    API::APIReturn apiReturn;
    //json jPayloadIn;
    Mantids30::Helpers::JSONReader2 reader;

    std::map<uint32_t, API::Monolith::Endpoints *>::iterator it = m_endpointsHandlerByAPIVersion.find(apiVersion);
    if (it == m_endpointsHandlerByAPIVersion.end())
    {
        // Key does not exist
        log(LogLevel::ERR, "monolithAPI", 2048, "API version %lu does not exist {endpoint=%s}", apiVersion, endpointName.c_str());
        // Endpoint not available for this null session..
        apiReturn.setError(HTTP::Status::Code::S_404_NOT_FOUND, "invalid_api_handling", "Endpoint Not Found");
        return apiReturn;
    }

    API::Monolith::Endpoints *endpointsHandler = m_endpointsHandlerByAPIVersion[apiVersion];

    // TODO: upgrade Token (2fa) y/o  token acompañante.
    if (endpointsHandler->doesAPIEndpointRequireActiveSession(endpointName) && !m_currentWebSession)
    {
        log(LogLevel::ERR, "monolithAPI", 2048, "This endpoint requires full authentication / session {endpoint=%s}", endpointName.c_str());
        // Endpoint not available for this null session..
        apiReturn.setError(HTTP::Status::Code::S_404_NOT_FOUND, "invalid_api_handling", "Endpoint Not Found");
        return apiReturn;
    }

    json reasons;

    // Validate that the endpoint requirements are satisfied.
    API::Monolith::Endpoints::ValidationResult i = endpointsHandler->validateEndpointRequirements(currentSessionInfo.authSession, endpointName, &reasons);

    switch (i)
    {
    case API::Monolith::Endpoints::ValidationResult::SUCCESS:
    {
        log(LogLevel::INFO, "monolithAPI", 2048, "Executing Web Endpoint {endpoint=%s}", endpointName.c_str());
        log(LogLevel::DEBUG, "monolithAPI", 8192, "Executing Web Endpoint - debugging parameters {endpoint=%s,params=%s}", endpointName.c_str(),
            Mantids30::Helpers::jsonToString(postParameters).c_str());

        chrono::time_point<chrono::high_resolution_clock, chrono::duration<double>> start = chrono::high_resolution_clock::now();
        chrono::time_point<chrono::high_resolution_clock, chrono::duration<double>> finish = chrono::high_resolution_clock::now();
        chrono::duration<double, milli> elapsed = finish - start;

        switch (endpointsHandler->invoke(currentSessionInfo.authSession, endpointName, postParameters, apiReturn.responseJSON()))
        {
        case API::Monolith::Endpoints::StatusCode::SUCCESS:

            finish = chrono::high_resolution_clock::now();
            elapsed = finish - start;
            log(LogLevel::INFO, "monolithAPI", 2048, "Web Endpoint executed OK {endpoint=%s, elapsedMS=%f}", endpointName.c_str(), elapsed.count());
            log(LogLevel::DEBUG, "monolithAPI", 8192, "Web Endpoint executed OK - debugging parameters {endpoint=%s,params=%s}", endpointName.c_str(),
                Mantids30::Helpers::jsonToString(*(apiReturn.responseJSON())).c_str());
            break;
        case API::Monolith::Endpoints::StatusCode::NOTFOUND:
            log(LogLevel::ERR, "monolithAPI", 2048, "Web Endpoint not found {endpoint=%s}", endpointName.c_str());
            apiReturn.setError(HTTP::Status::Code::S_404_NOT_FOUND, "invalid_api_handling", "Endpoint not found.");
            break;
        default:
            log(LogLevel::ERR, "monolithAPI", 2048, "Unknown error during web endpoint execution {endpoint=%s}", endpointName.c_str());
            apiReturn.setError(HTTP::Status::Code::S_401_UNAUTHORIZED, "invalid_api_handling", "Endpoint unauthorized.");
            break;
        }
    }
    break;
    case API::Monolith::Endpoints::ValidationResult::NOTAUTHORIZED:
    {
        // Endpoint unauthorized.
        log(LogLevel::ERR, "monolithAPI", 8192, "Not authorized to execute endpoint {endpoint=%s,reasons=%s}", endpointName.c_str(), Mantids30::Helpers::jsonToString(reasons).c_str());
        apiReturn.setError(HTTP::Status::Code::S_401_UNAUTHORIZED, "invalid_api_handling", "Endpoint unauthorized.");
        apiReturn.setReasons(reasons);
    }
    break;
    case API::Monolith::Endpoints::ValidationResult::ENDPOINTNOTFOUND:
    default:
    {
        log(LogLevel::ERR, "monolithAPI", 2048, "Endpoint not found {endpoint=%s}", endpointName.c_str());
        // Endpoint not found.
        apiReturn.setError(HTTP::Status::Code::S_404_NOT_FOUND, "invalid_api_handling", "Endpoint not found.");
    }
    break;
    }

    return apiReturn;
}

API::APIReturn ClientHandler::handleOptionsRequest(const std::string &baseApiUrl, const uint32_t &apiVersion, const std::string &endpointName)
{
    API::APIReturn apiReturn;

    std::map<uint32_t, API::Monolith::Endpoints *>::const_iterator it = m_endpointsHandlerByAPIVersion.find(apiVersion);
    if (it == m_endpointsHandlerByAPIVersion.end())
    {
        // Key does not exist
        log(LogLevel::ERR, "monolithAPI", 2048, "API version %lu does not exist {endpoint=%s}", apiVersion, endpointName.c_str());
        // Endpoint not available for this null session..
        apiReturn.setError(HTTP::Status::Code::S_404_NOT_FOUND, "invalid_api_handling", "Endpoint not found.");
        return apiReturn;
    }

    API::Monolith::Endpoints *endpointsHandler = m_endpointsHandlerByAPIVersion[apiVersion];

    if (endpointsHandler->isOptionsEnabled())
    {
        // No specific handler registered, but CORS is enabled.
        // Respond with CORS headers using buildCORSOptionsResponse.
        std::string origin;

        origin = clientRequest.getOrigin();

        // Look for per-endpoint config first, then fall back to global
        const API::OptionsHandlerConfig *useConfig = endpointsHandler->getGlobalOptionsConfig();
        const API::OptionsHandlerConfig *endpointConfig = endpointsHandler->getOptionsConfigOnEndpoint(endpointName);
        if (endpointConfig != nullptr)
        {
            useConfig = endpointConfig;
        }
        return endpointsHandler->buildCORSOptionsResponse(*useConfig, origin);
    }

    return apiReturn;
}

json ClientHandler::handleAPIInfo(const string &baseApiUrl)
{
    json x;
    // Retrieve software information...
    /*
    {
        "softwareName": "MyWebServer",
        "version": "1.3.5",
        "description": "Monolithic web server for handling user sessions",
        "endpoint" : "v1",
        "buildDate": "2024-10-12"
    }*/

    // Obtener la fecha de compilación
    const char *buildDate = __DATE__; // Macro estándar para fecha de compilación
    const char *buildTime = __TIME__; // Macro estándar para hora de compilación

    x["version"] = config->softwareVersion;
    x["softwareName"] = config->softwareName;
    x["description"] = config->softwareDescription;
    x["endpoint"] = baseApiUrl;
    x["buildDate"] = string(buildDate) + " " + buildTime;

    return x;
}

/*
    Mitigation: Ensure that sensitive endpoints like logout include this headers in their responses:
                Cache-Control: no-store
                Pragma: no-cache

    Using a custom header like X-Session-ID for logout is generally safe, but should be implemented together with:

        - Mandatory HTTPS.
        - Cookies configured with SameSite and Secure flags.
        - Origin or Referer validation on the backend.
        - XSS protection measures in the application.
*/
bool ClientHandler::validateSessionAntiCSRFMechanism()
{
    string sessionIdFromHeader = clientRequest.getHeaderOption("X-HalfSession-ID");
    string sessionIdFromCookie = clientRequest.getCookie(CURRENT_SESSIONID_COOKIENAME);

    // Validate session header == cookie, in theory, JS does not allow header manipulation via CSRF
    if (sessionIdFromHeader == RPCLog::truncateSessionId(sessionIdFromCookie))
    {
        return true;
    }

    string referer = clientRequest.getHeaderOption("Referer");
    log(LogLevel::SECURITY_ALERT, "monolithAPI", 2048, "Invalid CSRF Validation {path=%s, referer=%s}", clientRequest.getURI().c_str(), referer.c_str());

    return false;
}
