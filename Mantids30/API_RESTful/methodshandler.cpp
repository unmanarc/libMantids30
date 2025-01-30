#include "methodshandler.h"
#include <Mantids30/Helpers/json.h>
#include <Mantids30/Protocol_HTTP/rsp_status.h>
#include <Mantids30/Threads/lock_shared.h>

using namespace Mantids30;
using namespace API::RESTful;

MethodsHandler::MethodsHandler()
{

}

bool MethodsHandler::addResource(const MethodMode &mode, const std::string &resourceName, MethodType method, void *context, const uint32_t & SecurityOptions, const std::set<std::string> requiredPermissions)
{
    RESTfulAPIDefinition def;
    def.method = method;
    def.context = context;
    def.security.requireJWTHeaderAuthentication = SecurityOptions & MethodsHandler::SecurityOptions::REQUIRE_JWT_HEADER_AUTH;
    def.security.requireJWTCookieAuthentication = SecurityOptions & MethodsHandler::SecurityOptions::REQUIRE_JWT_COOKIE_AUTH;
    //def.security.requireJWTCookieHash = SecurityOptions & MethodsHandler::SecurityOptions::REQUIRE_JWT_COOKIE_HASH;
    //def.security.requireGenericAntiCSRFToken = SecurityOptions & MethodsHandler::SecurityOptions::REQUIRE_GENERIC_ANTICSRF_TOKEN;
    def.security.requiredPermissions = requiredPermissions;
    return addResource(mode, resourceName, def);
}

bool MethodsHandler::addResource(const MethodMode &mode, const std::string &resourceName, const RESTfulAPIDefinition &method)
{
    Threads::Sync::Lock_RW lock(m_methodsMutex);

    switch (mode)
    {
    case GET:
        m_methodsGET[resourceName] = method;
        break;
    case POST:
        m_methodsPOST[resourceName] = method;
        break;
    case PUT:
        m_methodsPUT[resourceName] = method;
        break;
    case DELETE:
        m_methodsDELETE[resourceName] = method;
        break;
    default:
        return false;
    }

    return true;
}

Sessions::ClientDetails MethodsHandler::extractClientDetails(const RequestParameters &inputParameters)
{
    Mantids30::Sessions::ClientDetails clientDetails;
    clientDetails.ipAddress = inputParameters.clientRequest->networkClientInfo.REMOTE_ADDR;
    clientDetails.tlsCommonName = inputParameters.clientRequest->networkClientInfo.tlsCommonName;
    clientDetails.userAgent = inputParameters.clientRequest->userAgent;
    return clientDetails;
}


MethodsHandler::ErrorCodes MethodsHandler::invokeResource(const MethodMode & mode,
                                                          const std::string & resourceName,
                                                          RESTful::RequestParameters &inputParameters,
                                                          const std::set<std::string> &currentPermissions,
                                                          const SecurityParameters & securityParameters,
                                                          APIReturn *apiResponse)
{    Threads::Sync::Lock_RD lock(m_methodsMutex);

    RESTfulAPIDefinition method;
    auto it = m_methodsGET.end();

    switch (mode)
    {
    case GET:
        it = m_methodsGET.find(resourceName);
        if (it != m_methodsGET.end())
        {
            method = it->second;
        }
        break;
    case POST:
        it = m_methodsPOST.find(resourceName);
        if (it != m_methodsPOST.end())
        {
            method = it->second;
        }
        break;
    case PUT:
        it = m_methodsPUT.find(resourceName);
        if (it != m_methodsPUT.end())
        {
            method = it->second;
        }
        break;
    case DELETE:
        it = m_methodsDELETE.find(resourceName);
        if (it != m_methodsDELETE.end())
        {
            method = it->second;
        }
        break;
    default:
        if (apiResponse != nullptr)
        {
            apiResponse->setFullStatus(false, Network::Protocols::HTTP::Status::S_400_BAD_REQUEST, "Invalid method mode");
        }
        return INVALID_METHOD_MODE;
    }

    if (it == m_methodsGET.end())
    {
        if (apiResponse != nullptr)
        {
            apiResponse->setFullStatus(false, Network::Protocols::HTTP::Status::S_404_NOT_FOUND, "Resource not found");
        }
        return RESOURCE_NOT_FOUND;
    }

    if (method.security.requireJWTHeaderAuthentication && !securityParameters.haveJWTAuthHeader)
    {
        if (apiResponse != nullptr)
        {
            apiResponse->setFullStatus(false, Network::Protocols::HTTP::Status::S_403_FORBIDDEN, "JWT Authentication Header Required");
        }
        return AUTHENTICATION_REQUIRED;
    }

    if (method.security.requireJWTCookieAuthentication && !securityParameters.haveJWTAuthCookie)
    {
        if (apiResponse != nullptr)
        {
            apiResponse->setFullStatus(false, Network::Protocols::HTTP::Status::S_403_FORBIDDEN, "JWT Authentication Cookie Required");
        }
        return AUTHENTICATION_REQUIRED;
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
                apiResponse->setFullStatus(false, Network::Protocols::HTTP::Status::S_403_FORBIDDEN, "Generic CSRF Token Required");
            }
            return AUTHENTICATION_REQUIRED;
        }
    }*/

    for (const auto &attr : method.security.requiredPermissions)
    {
        if (currentPermissions.find(attr) == currentPermissions.end())
        {
            if (apiResponse != nullptr)
            {
                apiResponse->setFullStatus(false, Network::Protocols::HTTP::Status::S_401_UNAUTHORIZED, "Insufficient permissions");
            }
            return INSUFFICIENT_PERMISSIONS;
        }
    }

    if (inputParameters.clientRequest->getJSONStreamerContent() != nullptr)
    {
        if (!inputParameters.clientRequest->getJSONStreamerContent()->getString().empty())
        {
            if ((inputParameters.inputJSON = inputParameters.clientRequest->getJSONStreamerContent()->processValue()) == nullptr)
            {
                // Bad parsing...
                apiResponse->setFullStatus(false, Network::Protocols::HTTP::Status::S_400_BAD_REQUEST, "Bad Input JSON Parsing");
                inputParameters.inputJSON = &inputParameters.emptyJSON;
                return INTERNAL_ERROR;
            }
            else
            {
                // Parsed...
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
                apiResponse->setFullStatus(false, Network::Protocols::HTTP::Status::S_403_FORBIDDEN, "JWT CSRF Token Hash Required");
            }
            return AUTHENTICATION_REQUIRED;
        }
    }*/

    if (method.method != nullptr && apiResponse != nullptr)
    {
        Mantids30::Sessions::ClientDetails clientDetails = extractClientDetails(inputParameters);
        method.method(method.context, // Context
                      *apiResponse, // The API return object
                      *(apiResponse->body->getValue()), // JSON Response data pointer (JSON format)
                      inputParameters, // Parameters from the RESTful request in JSON format
                      *(inputParameters.inputJSON), // Input data
                      *(inputParameters.jwtToken), // JWT Token
                      clientDetails);
        return SUCCESS;
    }

    if (apiResponse != nullptr)
    {
        apiResponse->setFullStatus(false, Network::Protocols::HTTP::Status::S_500_INTERNAL_SERVER_ERROR, "Internal error");
    }

    return INTERNAL_ERROR;
}

MethodsHandler::ErrorCodes MethodsHandler::invokeResource(const std::string &modeStr, const std::string &resourceName, RequestParameters &inputParameters, const std::set<std::string> &currentPermissions, const SecurityParameters & securityParameters, APIReturn *payloadOut)
{
    MethodMode mode;

    if (modeStr == "GET")
    {
        mode = GET;
    }
    else if (modeStr == "POST")
    {
        mode = POST;
    }
    else if (modeStr == "PUT")
    {
        mode = PUT;
    }
    else if (modeStr == "DELETE")
    {
        mode = DELETE;
    }
    else
    {
        if (payloadOut != nullptr)
        {
            *payloadOut->body = "Invalid method mode string";
            payloadOut->code = Network::Protocols::HTTP::Status::S_400_BAD_REQUEST;
        }
        return INVALID_METHOD_MODE;
    }

    return invokeResource(mode, resourceName, inputParameters, currentPermissions, securityParameters, payloadOut);

}

