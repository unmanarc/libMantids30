#include "api_restful_endpoints.h"
#include <Mantids30/Protocol/HTTP/methods.h>
#include <Mantids30/DataFormat/JSON/json.h>
#include <Mantids30/Protocol/HTTP/rsp_status.h>
#include "security.h"

using namespace Mantids30;
using namespace Mantids30::Protocol;
using namespace Mantids30::Server::WebRESTFul::RESTful;

bool Endpoints::addEndpoint(const Mantids30::Protocol::HTTP::Method &httpMethodType, const std::string &endpointPath, const Mantids30::Server::WebRESTFul::Security::Requirements &securityRequirements, const std::set<std::string> &requiredScopes,
                            void *context, APIEndpointFunctionType endpointDefinition)
{
    RESTfulAPIEndpointFullDefinition def;
    def.endpointDefinition = endpointDefinition;
    def.context = context;
    def.security.set(securityRequirements, requiredScopes);
    return addEndpoint(httpMethodType, endpointPath, def);
}

bool Endpoints::addEndpoint(const Mantids30::Protocol::HTTP::Method &httpMethodType, const std::string &endpointPath, const RESTfulAPIEndpointFullDefinition &apiEndpointFullDefinition)
{
    switch (httpMethodType)
    {
    case Mantids30::Protocol::HTTP::Method::GET:
        m_endpointsGET[endpointPath] = apiEndpointFullDefinition;
        break;
    case Mantids30::Protocol::HTTP::Method::POST:
        m_endpointsPOST[endpointPath] = apiEndpointFullDefinition;
        break;
    case Mantids30::Protocol::HTTP::Method::PUT:
        m_endpointsPUT[endpointPath] = apiEndpointFullDefinition;
        break;
    case Mantids30::Protocol::HTTP::Method::DELETE:
        m_endpointsDELETE[endpointPath] = apiEndpointFullDefinition;
        break;
    case Mantids30::Protocol::HTTP::Method::PATCH:
        m_endpointsPATCH[endpointPath] = apiEndpointFullDefinition;
        break;
    default:
        return false;
    }

    return true;
}

Mantids30::Server::WebRESTFul::Sessions::ClientDetails Endpoints::extractClientDetails(const RequestContext &requestContext)
{
    Mantids30::Server::WebRESTFul::Sessions::ClientDetails clientDetails;
    clientDetails.ipAddress = requestContext.clientRequest->networkClientInfo.REMOTE_ADDR;
    clientDetails.tlsCommonName = requestContext.clientRequest->networkClientInfo.tlsCommonName;
    clientDetails.userAgent = requestContext.clientRequest->userAgent;
    return clientDetails;
}

Endpoints::HandleResult Endpoints::handleEndpoint(const Mantids30::Protocol::HTTP::Method &httpMethodType, const std::string &endpointPath, Mantids30::Server::WebRESTFul::RESTful::RequestContext &requestContext,
                                                  const std::set<std::string> &currentScopes, bool isAdmin, const Mantids30::Server::WebRESTFul::Security::ReceivedAuth &securityParameters, Mantids30::Protocol::HTTP::APIReturn *apiResponse)
{
    RESTfulAPIEndpointFullDefinition endpointFullDefinition;
    std::map<std::string, RESTfulAPIEndpointFullDefinition>::iterator it = m_endpointsGET.end();

    switch (httpMethodType)
    {
    case Mantids30::Protocol::HTTP::Method::GET:
        it = m_endpointsGET.find(endpointPath);
        if (it != m_endpointsGET.end())
        {
            endpointFullDefinition = it->second;
        }
        break;
    case Mantids30::Protocol::HTTP::Method::POST:
        it = m_endpointsPOST.find(endpointPath);
        if (it != m_endpointsPOST.end())
        {
            endpointFullDefinition = it->second;
        }
        break;
    case Mantids30::Protocol::HTTP::Method::PUT:
        it = m_endpointsPUT.find(endpointPath);
        if (it != m_endpointsPUT.end())
        {
            endpointFullDefinition = it->second;
        }
        break;
    case Mantids30::Protocol::HTTP::Method::DELETE:
        it = m_endpointsDELETE.find(endpointPath);
        if (it != m_endpointsDELETE.end())
        {
            endpointFullDefinition = it->second;
        }
        break;
    case Mantids30::Protocol::HTTP::Method::PATCH:
        it = m_endpointsPATCH.find(endpointPath);
        if (it != m_endpointsPATCH.end())
        {
            endpointFullDefinition = it->second;
        }
        break;
    default:
        if (apiResponse != nullptr)
        {
            apiResponse->setError(Mantids30::Protocol::HTTP::Status::Code::S_400_BAD_REQUEST, "invalid_invokation", "Invalid Method Mode");
        }
        return HandleResult::INVALID_METHOD_MODE;
    }

    if (endpointFullDefinition.endpointDefinition == nullptr)
    {
        if (apiResponse != nullptr)
        {
            apiResponse->setError(Mantids30::Protocol::HTTP::Status::Code::S_404_NOT_FOUND, "invalid_invokation", "Resource not found");
        }
        return HandleResult::RESOURCE_NOT_FOUND;
    }

    if (endpointFullDefinition.security.requireJWTHeaderAuthentication && !securityParameters.hasVerifiedJWTAuthorizationHeader)
    {
        if (apiResponse != nullptr)
        {
            apiResponse->setError(Mantids30::Protocol::HTTP::Status::Code::S_403_FORBIDDEN, "invalid_invokation", "JWT Authentication Header Required");
        }
        return HandleResult::AUTHENTICATION_REQUIRED;
    }

    if (endpointFullDefinition.security.requireJWTCookieAuthentication && !securityParameters.hasVerifiedJWTAccessTokenCookie)
    {
        if (apiResponse != nullptr)
        {
            apiResponse->setError(Mantids30::Protocol::HTTP::Status::Code::S_403_FORBIDDEN, "invalid_invokation", "JWT Authentication Cookie Required");
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
                    apiResponse->setError(Mantids30::Protocol::HTTP::Status::Code::S_401_UNAUTHORIZED, "invalid_invokation", "Invalid Scope");
                }
                return HandleResult::INVALID_SCOPE;
            }
        }
    }

    const auto jsonStreamerContent = requestContext.explicitInputJSON != nullptr
                                         ? nullptr : requestContext.clientRequest->getJSONStreamerContent();
    requestContext.inputJSON = &requestContext.emptyJSON;

    if (requestContext.explicitInputJSON != nullptr)
    {
        requestContext.inputJSON = requestContext.explicitInputJSON;
    }
    else if (jsonStreamerContent != nullptr)
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
                apiResponse->setError(Mantids30::Protocol::HTTP::Status::Code::S_400_BAD_REQUEST, "invalid_invokation", "Bad Input JSON Parsing during GET");
                return HandleResult::INTERNAL_ERROR;
            }
            else
            {
                // No problem, empty.
            }
        }
        else
        {
            if (requestContext.clientRequest->content.getContainerType() == Mantids30::Protocol::HTTP::Content::ContainerType::JSON)
            {
                // Bad parsing... (should be parsed as JSON...)
                apiResponse->setError(Mantids30::Protocol::HTTP::Status::Code::S_400_BAD_REQUEST, "invalid_invokation", "Bad Input JSON Parsing during POST");
                return HandleResult::INTERNAL_ERROR;
            }
        }
    }

    if (endpointFullDefinition.endpointDefinition != nullptr && apiResponse != nullptr)
    {
        Mantids30::Server::WebRESTFul::Sessions::ClientDetails clientDetails = extractClientDetails(requestContext);

        *apiResponse = endpointFullDefinition.endpointDefinition(endpointFullDefinition.context, // Context
                                                                 requestContext,                // Parameters from the RESTful request in JSON format
                                                                 clientDetails);
        return HandleResult::SUCCESS;
    }

    if (apiResponse != nullptr)
    {
        apiResponse->setError(Mantids30::Protocol::HTTP::Status::Code::S_500_INTERNAL_SERVER_ERROR, "invalid_invokation", "Internal error");
    }

    return HandleResult::INTERNAL_ERROR;
}

Endpoints::HandleResult Endpoints::handleEndpoint(const std::string &httpMethodType, const std::string &endpointPath, RequestContext &inputParameters, const std::set<std::string> &currentScopes,
                                                  bool isAdmin, const Mantids30::Server::WebRESTFul::Security::ReceivedAuth &securityParameters, Mantids30::Protocol::HTTP::APIReturn *payloadOut)
{
    Mantids30::Protocol::HTTP::Method mode;

    mode = HTTP::stringToMethod(httpMethodType);

    if (mode == Mantids30::Protocol::HTTP::Method::UNKNOWN)
    {
        if (payloadOut != nullptr)
        {
            payloadOut->setError(Mantids30::Protocol::HTTP::Status::Code::S_400_BAD_REQUEST, "invalid_invokation", "Invalid method mode string");
        }
        return HandleResult::INVALID_METHOD_MODE;
    }

    return handleEndpoint(mode, endpointPath, inputParameters, currentScopes, isAdmin, securityParameters, payloadOut);
}
