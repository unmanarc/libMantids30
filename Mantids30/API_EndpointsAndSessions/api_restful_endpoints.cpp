#include "api_restful_endpoints.h"
#include <Mantids30/Protocol_HTTP/methods.h>
#include <Mantids30/Helpers/json.h>
#include <Mantids30/Protocol_HTTP/rsp_status.h>
#include <shared_mutex>
#include <mutex>
#include "security.h"

using namespace Mantids30;
using namespace Mantids30::Network::Protocol;
using namespace API::RESTful;

bool Endpoints::addEndpoint(const HTTP::Method &httpMethodType, const std::string &endpointPath, const API::Security::Requirements &securityRequirements, const std::set<std::string> &requiredScopes,
                            void *context, APIEndpointFunctionType endpointDefinition)
{
    RESTfulAPIEndpointFullDefinition def;
    def.endpointDefinition = endpointDefinition;
    def.context = context;
    def.security.set(securityRequirements, requiredScopes);
    return addEndpoint(httpMethodType, endpointPath, def);
}

bool Endpoints::addEndpoint(const HTTP::Method &httpMethodType, const std::string &endpointPath, const RESTfulAPIEndpointFullDefinition &apiEndpointFullDefinition)
{
    switch (httpMethodType)
    {
    case HTTP::Method::GET:
        m_endpointsGET[endpointPath] = apiEndpointFullDefinition;
        break;
    case HTTP::Method::POST:
        m_endpointsPOST[endpointPath] = apiEndpointFullDefinition;
        break;
    case HTTP::Method::PUT:
        m_endpointsPUT[endpointPath] = apiEndpointFullDefinition;
        break;
    case HTTP::Method::DELETE:
        m_endpointsDELETE[endpointPath] = apiEndpointFullDefinition;
        break;
    case HTTP::Method::PATCH:
        m_endpointsPATCH[endpointPath] = apiEndpointFullDefinition;
        break;
    default:
        return false;
    }

    return true;
}

Sessions::ClientDetails Endpoints::extractClientDetails(const RequestContext &requestContext)
{
    Mantids30::Sessions::ClientDetails clientDetails;
    clientDetails.ipAddress = requestContext.clientRequest->networkClientInfo.REMOTE_ADDR;
    clientDetails.tlsCommonName = requestContext.clientRequest->networkClientInfo.tlsCommonName;
    clientDetails.userAgent = requestContext.clientRequest->userAgent;
    return clientDetails;
}

Endpoints::HandleResult Endpoints::handleEndpoint(const HTTP::Method &httpMethodType, const std::string &endpointPath, RESTful::RequestContext &requestContext,
                                                  const std::set<std::string> &currentScopes, bool isAdmin, const API::Security::ReceivedAuth &securityParameters, APIReturn *apiResponse)
{
    RESTfulAPIEndpointFullDefinition endpointFullDefinition;
    std::map<std::string, RESTfulAPIEndpointFullDefinition>::iterator it = m_endpointsGET.end();

    switch (httpMethodType)
    {
    case HTTP::Method::GET:
        it = m_endpointsGET.find(endpointPath);
        if (it != m_endpointsGET.end())
        {
            endpointFullDefinition = it->second;
        }
        break;
    case HTTP::Method::POST:
        it = m_endpointsPOST.find(endpointPath);
        if (it != m_endpointsPOST.end())
        {
            endpointFullDefinition = it->second;
        }
        break;
    case HTTP::Method::PUT:
        it = m_endpointsPUT.find(endpointPath);
        if (it != m_endpointsPUT.end())
        {
            endpointFullDefinition = it->second;
        }
        break;
    case HTTP::Method::DELETE:
        it = m_endpointsDELETE.find(endpointPath);
        if (it != m_endpointsDELETE.end())
        {
            endpointFullDefinition = it->second;
        }
        break;
    case HTTP::Method::PATCH:
        it = m_endpointsPATCH.find(endpointPath);
        if (it != m_endpointsPATCH.end())
        {
            endpointFullDefinition = it->second;
        }
        break;
    default:
        if (apiResponse != nullptr)
        {
            apiResponse->setError(HTTP::Status::Code::S_400_BAD_REQUEST, "invalid_invokation", "Invalid Method Mode");
        }
        return HandleResult::INVALID_METHOD_MODE;
    }

    if (endpointFullDefinition.endpointDefinition == nullptr)
    {
        if (apiResponse != nullptr)
        {
            apiResponse->setError(HTTP::Status::Code::S_404_NOT_FOUND, "invalid_invokation", "Resource not found");
        }
        return HandleResult::RESOURCE_NOT_FOUND;
    }

    if (endpointFullDefinition.security.requireJWTHeaderAuthentication && !securityParameters.hasVerifiedJWTAuthorizationHeader)
    {
        if (apiResponse != nullptr)
        {
            apiResponse->setError(HTTP::Status::Code::S_403_FORBIDDEN, "invalid_invokation", "JWT Authentication Header Required");
        }
        return HandleResult::AUTHENTICATION_REQUIRED;
    }

    if (endpointFullDefinition.security.requireJWTCookieAuthentication && !securityParameters.hasVerifiedJWTAccessTokenCookie)
    {
        if (apiResponse != nullptr)
        {
            apiResponse->setError(HTTP::Status::Code::S_403_FORBIDDEN, "invalid_invokation", "JWT Authentication Cookie Required");
        }
        return HandleResult::AUTHENTICATION_REQUIRED;
    }

    if (!isAdmin)
    {
        for (const std::string &attr : endpointFullDefinition.security.requiredScopes)
        {
            if (currentScopes.find(attr) == currentScopes.end())
            {
                if (apiResponse != nullptr)
                {
                    apiResponse->setError(HTTP::Status::Code::S_401_UNAUTHORIZED, "invalid_invokation", "Invalid Scope");
                }
                return HandleResult::INVALID_SCOPE;
            }
        }
    }

    std::shared_ptr<Mantids30::Memory::Streams::StreamableJSON> jsonStreamerContent = requestContext.clientRequest->getJSONStreamerContent();

    requestContext.inputJSON = &requestContext.emptyJSON;

    if (jsonStreamerContent != nullptr)
    {
        requestContext.inputJSON = jsonStreamerContent->getValue();
    }
    else
    {
        if (requestContext.clientRequest->requestLine.getHTTPMethod() == "GET")
        {
            if (!requestContext.clientRequest->requestLine.getRequestGETVarsRawString().empty())
            {
                // Bad parsing... (should be JSON or empty)
                apiResponse->setError(HTTP::Status::Code::S_400_BAD_REQUEST, "invalid_invokation", "Bad Input JSON Parsing during GET");
                return HandleResult::INTERNAL_ERROR;
            }
            else
            {
                // No problem, empty.
            }
        }
        else
        {
            if (requestContext.clientRequest->content.getContainerType() == HTTP::Content::ContainerType::JSON)
            {
                // Bad parsing... (should be parsed as JSON...)
                apiResponse->setError(HTTP::Status::Code::S_400_BAD_REQUEST, "invalid_invokation", "Bad Input JSON Parsing during POST");
                return HandleResult::INTERNAL_ERROR;
            }
        }
    }

    if (endpointFullDefinition.endpointDefinition != nullptr && apiResponse != nullptr)
    {
        Mantids30::Sessions::ClientDetails clientDetails = extractClientDetails(requestContext);

        *apiResponse = endpointFullDefinition.endpointDefinition(endpointFullDefinition.context, // Context
                                                                 requestContext,                // Parameters from the RESTful request in JSON format
                                                                 clientDetails);
        return HandleResult::SUCCESS;
    }

    if (apiResponse != nullptr)
    {
        apiResponse->setError(HTTP::Status::Code::S_500_INTERNAL_SERVER_ERROR, "invalid_invokation", "Internal error");
    }

    return HandleResult::INTERNAL_ERROR;
}

Endpoints::HandleResult Endpoints::handleEndpoint(const std::string &httpMethodType, const std::string &endpointPath, RequestContext &inputParameters, const std::set<std::string> &currentScopes,
                                                  bool isAdmin, const API::Security::ReceivedAuth &securityParameters, APIReturn *payloadOut)
{
    HTTP::Method mode;

    mode = HTTP::stringToMethod(httpMethodType);

    if (mode == HTTP::Method::UNKNOWN)
    {
        if (payloadOut != nullptr)
        {
            payloadOut->setError(HTTP::Status::Code::S_400_BAD_REQUEST, "invalid_invokation", "Invalid method mode string");
        }
        return HandleResult::INVALID_METHOD_MODE;
    }

    return handleEndpoint(mode, endpointPath, inputParameters, currentScopes, isAdmin, securityParameters, payloadOut);
}
