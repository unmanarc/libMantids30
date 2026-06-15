#include "api_restful_endpoints.h"
#include <Mantids30/Helpers/json.h>
#include <Mantids30/Protocol_HTTP/rsp_status.h>
#include <Mantids30/Threads/lock_shared.h>

using namespace Mantids30;
using namespace Mantids30::Network::Protocols;
using namespace API::RESTful;

bool Endpoints::addEndpoint(const HTTPMethod &httpMethodType, const std::string &endpointPath, const Security &securityOptions, const std::set<std::string>& requiredScopes, void *context,
                            APIEndpointFunctionType endpointDefinition)
{
    RESTfulAPIEndpointFullDefinition def;
    def.endpointDefinition = endpointDefinition;
    def.context = context;
    def.security.requireJWTHeaderAuthentication = (securityOptions & Endpoints::Security::REQUIRE_JWT_HEADER_AUTH);
    def.security.requireJWTCookieAuthentication = (securityOptions & Endpoints::Security::REQUIRE_JWT_COOKIE_AUTH);
    def.security.requiredScopes = requiredScopes;
    return addEndpoint(httpMethodType, endpointPath, def);
}

bool Endpoints::addEndpoint(const HTTPMethod &httpMethodType, const std::string &endpointPath, const RESTfulAPIEndpointFullDefinition &apiEndpointFullDefinition)
{
    switch (httpMethodType)
    {
    case HTTPMethod::GET:
        m_endpointsGET[endpointPath] = apiEndpointFullDefinition;
        break;
    case HTTPMethod::POST:
        m_endpointsPOST[endpointPath] = apiEndpointFullDefinition;
        break;
    case HTTPMethod::PUT:
        m_endpointsPUT[endpointPath] = apiEndpointFullDefinition;
        break;
    case HTTPMethod::DELETE:
        m_endpointsDELETE[endpointPath] = apiEndpointFullDefinition;
        break;
    case HTTPMethod::PATCH:
        m_endpointsPATCH[endpointPath] = apiEndpointFullDefinition;
        break;
    default:
        return false;
    }

    return true;
}

Sessions::ClientDetails Endpoints::extractClientDetails(const RequestParameters &inputParameters)
{
    Mantids30::Sessions::ClientDetails clientDetails;
    clientDetails.ipAddress = inputParameters.clientRequest->networkClientInfo.REMOTE_ADDR;
    clientDetails.tlsCommonName = inputParameters.clientRequest->networkClientInfo.tlsCommonName;
    clientDetails.userAgent = inputParameters.clientRequest->userAgent;
    return clientDetails;
}

Endpoints::HandleResult Endpoints::handleEndpoint(const HTTPMethod &httpMethodType, const std::string &endpointPath, RESTful::RequestParameters &inputParameters,
                                                const std::set<std::string> &currentScopes, bool isAdmin, const SecurityParameters &securityParameters, APIReturn *apiResponse)
{
    RESTfulAPIEndpointFullDefinition endpointFullDefinition;
    std::map<std::string, RESTfulAPIEndpointFullDefinition>::iterator it = m_endpointsGET.end();

    switch (httpMethodType)
    {
    case HTTPMethod::GET:
        it = m_endpointsGET.find(endpointPath);
        if (it != m_endpointsGET.end())
        {
            endpointFullDefinition = it->second;
        }
        break;
    case HTTPMethod::POST:
        it = m_endpointsPOST.find(endpointPath);
        if (it != m_endpointsPOST.end())
        {
            endpointFullDefinition = it->second;
        }
        break;
    case HTTPMethod::PUT:
        it = m_endpointsPUT.find(endpointPath);
        if (it != m_endpointsPUT.end())
        {
            endpointFullDefinition = it->second;
        }
        break;
    case HTTPMethod::DELETE:
        it = m_endpointsDELETE.find(endpointPath);
        if (it != m_endpointsDELETE.end())
        {
            endpointFullDefinition = it->second;
        }
        break;
    case HTTPMethod::PATCH:
        it = m_endpointsPATCH.find(endpointPath);
        if (it != m_endpointsPATCH.end())
        {
            endpointFullDefinition = it->second;
        }
        break;
    default:
        if (apiResponse != nullptr)
        {
            apiResponse->setError(HTTP::Status::S_400_BAD_REQUEST, "invalid_invokation", "Invalid Method Mode");
        }
        return HandleResult::INVALID_METHOD_MODE;
    }

    if (endpointFullDefinition.endpointDefinition == nullptr)
    {
        if (apiResponse != nullptr)
        {
            apiResponse->setError(HTTP::Status::S_404_NOT_FOUND, "invalid_invokation", "Resource not found");
        }
        return HandleResult::RESOURCE_NOT_FOUND;
    }

    if (endpointFullDefinition.security.requireJWTHeaderAuthentication && !securityParameters.haveJWTAuthHeader)
    {
        if (apiResponse != nullptr)
        {
            apiResponse->setError(HTTP::Status::S_403_FORBIDDEN, "invalid_invokation", "JWT Authentication Header Required");
        }
        return HandleResult::AUTHENTICATION_REQUIRED;
    }

    if (endpointFullDefinition.security.requireJWTCookieAuthentication && !securityParameters.haveJWTAuthCookie)
    {
        if (apiResponse != nullptr)
        {
            apiResponse->setError(HTTP::Status::S_403_FORBIDDEN, "invalid_invokation", "JWT Authentication Cookie Required");
        }
        return HandleResult::AUTHENTICATION_REQUIRED;
    }

    //authMode.haveGenericCSRFToken = !m_clientRequest.getCookie("GenCSRFToken").empty() &&  == m_clientRequest.getCookie("GenCSRFToken");
    /*if (method.security.requireGenericAntiCSRFToken)
    {
        auto cookies = inputParameters.clientRequest->getAllCookies();
        auto it = cookies.find("GenCSRFToken");
        if (    it == cookies.end() || // Parameter does not exist..
                it->second != securityParameters.genCSRFToken || // Parameter exist, but differs from genCSRFToken
                it->second.empty() // Parameter exist, but empty
            )
        {
            if (apiResponse != nullptr)
            {
                apiResponse->setError( HTTP::Status::S_403_FORBIDDEN, "Generic CSRF Token Required");
            }
            return AUTHENTICATION_REQUIRED;
        }
    }*/

    if (!isAdmin)
    {
        for (const std::string &attr : endpointFullDefinition.security.requiredScopes)
        {
            if (currentScopes.find(attr) == currentScopes.end())
            {
                if (apiResponse != nullptr)
                {
                    apiResponse->setError(HTTP::Status::S_401_UNAUTHORIZED, "invalid_invokation", "Invalid Scope");
                }
                return HandleResult::INVALID_SCOPE;
            }
        }
    }

    std::shared_ptr<Mantids30::Memory::Streams::StreamableJSON> jsonStreamerContent = inputParameters.clientRequest->getJSONStreamerContent();

    inputParameters.inputJSON = &inputParameters.emptyJSON;

    if (jsonStreamerContent != nullptr)
    {
        inputParameters.inputJSON = jsonStreamerContent->getValue();
    }
    else
    {
        if (inputParameters.clientRequest->requestLine.getHTTPMethod() == "GET")
        {
            if (!inputParameters.clientRequest->requestLine.getRequestGETVarsRawString().empty())
            {
                // Bad parsing... (should be JSON or empty)
                apiResponse->setError(HTTP::Status::S_400_BAD_REQUEST, "invalid_invokation", "Bad Input JSON Parsing during GET");
                return HandleResult::INTERNAL_ERROR;
            }
            else
            {
                // No problem, empty.
            }
        }
        else
        {
            if (inputParameters.clientRequest->content.getContainerType() == HTTP::Content::ContainerType::JSON)
            {
                // Bad parsing... (should be parsed as JSON...)
                apiResponse->setError(HTTP::Status::S_400_BAD_REQUEST, "invalid_invokation", "Bad Input JSON Parsing during POST");
                return HandleResult::INTERNAL_ERROR;
            }
        }
    }
    /*
    if (method.security.requireJWTCookieHash)
    {
        // Test for the hash from the cookie to be the same of the post...
        std::string inputHash = JSON_ASSTRING((*inputParameters.inputJSON), "AuthTokenHash", "empty");

        auto it = inputParameters.cookies.find("AuthTokenHash");
        if (    it == inputParameters.cookies.end() || // Parameter does not exist..
            it->second != inputHash || // Parameter exist, but differs from inputHash
            it->second.empty() // Parameter exist, but empty
            )
        {
            if (apiResponse != nullptr)
            {
                apiResponse->setError( HTTP::Status::S_403_FORBIDDEN, "JWT CSRF Token Hash Required");
            }
            return AUTHENTICATION_REQUIRED;
        }
    }*/

    if (endpointFullDefinition.endpointDefinition != nullptr && apiResponse != nullptr)
    {
        Mantids30::Sessions::ClientDetails clientDetails = extractClientDetails(inputParameters);

        *apiResponse = endpointFullDefinition.endpointDefinition(endpointFullDefinition.context, // Context
                                                                 inputParameters,                // Parameters from the RESTful request in JSON format
                                                                 clientDetails);
        return HandleResult::SUCCESS;
    }

    if (apiResponse != nullptr)
    {
        apiResponse->setError(HTTP::Status::S_500_INTERNAL_SERVER_ERROR, "invalid_invokation", "Internal error");
    }

    return HandleResult::INTERNAL_ERROR;
}

Endpoints::HandleResult Endpoints::handleEndpoint(const std::string &httpMethodType, const std::string &endpointPath, RequestParameters &inputParameters, const std::set<std::string> &currentScopes,
                                                bool isAdmin, const SecurityParameters &securityParameters, APIReturn *payloadOut)
{
    HTTPMethod mode;

    if (httpMethodType == "GET")
    {
        mode = HTTPMethod::GET;
    }
    else if (httpMethodType == "POST")
    {
        mode = HTTPMethod::POST;
    }
    else if (httpMethodType == "PUT")
    {
        mode = HTTPMethod::PUT;
    }
    else if (httpMethodType == "DELETE")
    {
        mode = HTTPMethod::DELETE;
    }
    else if (httpMethodType == "PATCH")
    {
        mode = HTTPMethod::PATCH;
    }
    else
    {
        if (payloadOut != nullptr)
        {
            payloadOut->setError(HTTP::Status::S_400_BAD_REQUEST, "invalid_invokation", "Invalid method mode string");
        }
        return HandleResult::INVALID_METHOD_MODE;
    }

    return handleEndpoint(mode, endpointPath, inputParameters, currentScopes, isAdmin, securityParameters, payloadOut);
}
