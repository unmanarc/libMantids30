#include "clienthandler.h"
#include <Mantids30/Protocol_HTTP/rsp_status.h>
#include "json/value.h"
#include <Mantids30/API_EndpointsAndSessions/api_restful_endpoints.h>
#include <Mantids30/Program_Logs/loglevels.h>
#include <Mantids30/Protocol_HTTP/rsp_cookies.h>
#include <Mantids30/Helpers/encoders.h>

#include <json/config.h>
#include <memory>
#include <string>

#include <string>
#include <json/json.h>

using namespace Mantids30::Network::Servers::RESTful;
using namespace Mantids30::Program::Logs;
using namespace Mantids30::Network;
using namespace Mantids30::Network::Protocols;
using namespace Mantids30::DataFormat;
using namespace Mantids30::Memory;
using namespace Mantids30;
using namespace std;


ClientHandler::ClientHandler(void *parent, std::shared_ptr<Memory::Streams::StreamableObject> sock) : Servers::Web::APIClientHandler(parent,sock)
{
}

API::APIReturn ClientHandler::handleAPIRequest(const string & baseApiUrl,
                                     const uint32_t & apiVersion,
                                     const string &httpMethodMode,
                                     const string &endpointName,
                                     const Json::Value & postParameters
                                               )
{
    API::APIReturn apiReturn;
    set<string> currentScopes;
    bool isAdmin = false;
    bool authenticated =false;
    API::RESTful::RequestParameters inputParameters;

    inputParameters.jwtSigner       = this->config->jwtSigner;
    inputParameters.jwtValidator    = this->config->jwtValidator;
    inputParameters.clientRequest   = &clientRequest;

    if ( isSessionActive() )
    {
        isAdmin = jwtToken.isAdmin();
        currentScopes = jwtToken.getAllScopes();
        inputParameters.jwtToken = &jwtToken;
    }

    if (m_endpointsHandler.find(apiVersion) == m_endpointsHandler.end())
    {
        log(eLogLevels::LEVEL_WARN, "restAPI", 2048, "API Version Not Found / Resource Not found {ver=%u, mode=%s, endpoint=%s}", apiVersion, httpMethodMode.c_str(), endpointName.c_str());
        apiReturn.setError( HTTP::Status::S_404_NOT_FOUND,"invalid_api_request","Resource Not Found");
        return apiReturn;
    }


    json x;
    API::RESTful::Endpoints::SecurityParameters securityParameters;
    securityParameters.haveJWTAuthCookie = m_JWTCookieTokenVerified;
    securityParameters.haveJWTAuthHeader = m_JWTHeaderTokenVerified;

    API::RESTful::Endpoints::ErrorCodes result = m_endpointsHandler[apiVersion]->handleEndpoint( httpMethodMode,
                                                                                                    endpointName,
                                                                                                    inputParameters,
                                                                                                    currentScopes,
                                                                                                    isAdmin,
                                                                                                    securityParameters,
                                                                                                    &apiReturn
                                                                                                   );

    switch (result)
    {
    case API::RESTful::Endpoints::SUCCESS:
        log(eLogLevels::LEVEL_DEBUG, "restAPI", 2048, "API REST Endpoint Executed {ver=%u, mode=%s, endpoint=%s}", apiVersion, httpMethodMode.c_str(), endpointName.c_str());
        break;
    case API::RESTful::Endpoints::INVALID_METHOD_MODE:
        log(eLogLevels::LEVEL_WARN, "restAPI", 2048, "API REST Invalid Endpoint Mode {ver=%u, mode=%s, endpoint=%s}", apiVersion, httpMethodMode.c_str(), endpointName.c_str());
        break;
    case API::RESTful::Endpoints::RESOURCE_NOT_FOUND:
        log(eLogLevels::LEVEL_WARN, "restAPI", 2048, "API REST Endpoint Not Found {ver=%u, mode=%s, endpoint=%s}", apiVersion, httpMethodMode.c_str(), endpointName.c_str());
        break;
    case API::RESTful::Endpoints::AUTHENTICATION_REQUIRED:
        log(eLogLevels::LEVEL_WARN, "restAPI", 2048, "API REST Authentication Not Provided {ver=%u, mode=%s, endpoint=%s}", apiVersion, httpMethodMode.c_str(), endpointName.c_str());
        break;
    case API::RESTful::Endpoints::INVALID_SCOPE:
        log(eLogLevels::LEVEL_WARN, "restAPI", 2048, "API REST Invalid Scope {ver=%u, mode=%s, endpoint=%s}", apiVersion, httpMethodMode.c_str(), endpointName.c_str());
        break;
    case API::RESTful::Endpoints::INTERNAL_ERROR:
        log(eLogLevels::LEVEL_WARN, "restAPI", 2048, "API REST Internal Error {ver=%u, mode=%s, endpoint=%s}", apiVersion, httpMethodMode.c_str(), endpointName.c_str());
        break;
    default:
        log(eLogLevels::LEVEL_ERR, "restAPI", 2048, "API REST Unknown Error {ver=%u, mode=%s, endpoint=%s}", apiVersion, httpMethodMode.c_str(), endpointName.c_str());
        apiReturn.setError( Protocols::HTTP::Status::S_500_INTERNAL_SERVER_ERROR,"invalid_api_request","Unknown Error");
        break;
    }

    return apiReturn;
}

