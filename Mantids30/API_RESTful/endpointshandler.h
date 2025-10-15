#pragma once

#include <Mantids30/DataFormat_JWT/jwt.h>
#include <Mantids30/Helpers/json.h>
#include <Mantids30/Memory/streamablejson.h>
#include <Mantids30/Protocol_HTTP/api_return.h>
#include <Mantids30/Protocol_HTTP/httpv1_base.h>
#include <Mantids30/Protocol_HTTP/httpv1_server.h>
#include <Mantids30/Protocol_HTTP/rsp_status.h>
#include <Mantids30/Sessions/session.h>
#include <Mantids30/Threads/mutex_shared.h>
#include <cstdint>
#include <map>
#include <memory>
#include <set>

namespace Mantids30 {
namespace API {
namespace RESTful {

// Struct to hold HTTP request parameters
struct RequestParameters
{
    Mantids30::Network::Protocols::HTTP::HTTPv1_Base::Request *clientRequest = nullptr; ///< Holds all the information from the client request
    Json::Value pathParameters;                                                         ///< Holds parameters from the URL path
    Json::Value emptyJSON;
    Json::Value *inputJSON = &emptyJSON; ///< Holds the input JSON that came from the request body.

    DataFormat::JWT::Token emptyToken;              ///< Holds a default empty token
    DataFormat::JWT::Token *jwtToken = &emptyToken; ///< Holds JWT token data, if present and validated the pointer will be changed.

    std::shared_ptr<DataFormat::JWT> jwtValidator; ///< Holds the JWT Validator
    std::shared_ptr<DataFormat::JWT> jwtSigner;    ///< Holds the JWT Signer
    //std::multimap<std::string, std::string> cookies;
};

using APIEndpointFunctionType = APIReturn (*)(void *context,                                        // Context pointer
                            const RESTful::RequestParameters &request,            // Parameters from the RESTful request
                            Mantids30::Sessions::ClientDetails &authClientDetails // Client authentication details
);

/**
 * @struct RESTfulAPIDefinition
 *
 * @brief Struct to define the RESTful API method, security, and object pointer.
 */
struct RESTfulAPIEndpointFullDefinition
{
    struct Security
    {
        bool requireJWTHeaderAuthentication = true;
        bool requireJWTCookieAuthentication = true;
        std::set<std::string> requiredScopes;
        //bool requireGenericAntiCSRFToken = true;
        //bool requireJWTCookieHash = true;
    };
    APIEndpointFunctionType endpointDefinition = nullptr;
    Security security;
    void *context = nullptr;
};

/**
 * @class Endpoints
 *
 * @brief Handles the mapping of RESTful API resources to corresponding methods,
 * manages security requirements, and invokes the appropriate methods.
 */
class Endpoints
{
public:
    /**
     * @enum HTTPMethodType
     *
     * @brief Enumeration for different RESTful method modes.
     */
    enum HTTPMethodType
    {
        GET = 0,
        POST = 1,
        PUT = 2,
        DELETE = 3,
        PATCH = 4
    };

    static std::string HTTPMethodTypeToString(HTTPMethodType mode)
    {
        switch (mode)
        {
        case GET:
            return "GET";
        case POST:
            return "POST";
        case PUT:
            return "PUT";
        case DELETE:
            return "DELETE";
        case PATCH:
            return "PATCH";
        default:
            return "POST";
        }
    }

    static HTTPMethodType stringToHTTPMethodType(const std::string &str)
    {
        if (str == "GET")
        {
            return GET;
        }
        else if (str == "POST")
        {
            return POST;
        }
        else if (str == "PUT")
        {
            return PUT;
        }
        else if (str == "DELETE")
        {
            return DELETE;
        }
        else if (str == "PATCH")
        {
            return PATCH;
        }
        else
        {
            return POST; // default: POST
        }
    }

    /**
     * @enum ErrorCodes
     *
     * @brief Enumeration for possible error codes.
     */
    enum ErrorCodes
    {
        SUCCESS = 0,
        INVALID_METHOD_MODE = -1,
        RESOURCE_NOT_FOUND = -2,
        AUTHENTICATION_REQUIRED = -3,
        INVALID_SCOPE = -4,
        INTERNAL_ERROR = -5
    };

    enum SecurityOptions
    {
        NO_AUTH = 0,
        REQUIRE_JWT_HEADER_AUTH = 1,
        REQUIRE_JWT_COOKIE_AUTH = 2 //,
        //REQUIRE_GENERIC_ANTICSRF_TOKEN=4//,
        //REQUIRE_JWT_COOKIE_HASH=8
    };

    struct SecurityParameters
    {
        Json::Value toJSON() const
        {
            Json::Value securityJSON;
            securityJSON["haveJWTAuthHeader"] = haveJWTAuthHeader;
            securityJSON["haveJWTAuthCookie"] = haveJWTAuthCookie;
            return securityJSON;
        }

        void setJSON(const Json::Value &securityJSON)
        {
            haveJWTAuthHeader = JSON_ASBOOL(securityJSON, "haveJWTAuthHeader", false);
            haveJWTAuthCookie = JSON_ASBOOL(securityJSON, "haveJWTAuthCookie", false);
        }

        bool haveJWTAuthHeader = false;
        bool haveJWTAuthCookie = false;
    };

    /**
     * @brief Default constructor for Endpoints.
     */
    Endpoints() = default;

    /**
     * @brief Add a new resource to the Endpoints.
     *
     * @param httpMethodType The RESTful method httpMethodType (GET, POST, PUT, DELETE).
     * @param endpointPath The name of the resource.
     * @param endpointDefinition The function pointer to the endpoint definition.
     * @param context The object pointer for the method.
     * @param requireJWTHeaderAuthentication If true, user authentication is required.
     * @param requiredScopes The set of required scopes for the resource.
     * @return Returns true if the resource was added successfully, false otherwise.
     */
    bool addEndpoint(const HTTPMethodType &httpMethodType, const std::string &endpointPath, APIEndpointFunctionType endpointDefinition, void *context, const uint32_t &SecurityOptions, const std::set<std::string> requiredScopes);

    /**
     * @brief Add a new resource to the Endpoints with RESTfulAPIDefinition struct.
     *
     * @param httpMethodType The RESTful apiEndpointFullDefinition httpMethodType (GET, POST, PUT, DELETE).
     * @param endpointPath The name of the resource.
     * @param apiEndpointFullDefinition The RESTfulAPIDefinition struct containing apiEndpointFullDefinition, security, and object pointer.
     * @return Returns true if the resource was added successfully, false otherwise.
     */
    bool addEndpoint(const HTTPMethodType &httpMethodType, const std::string &endpointPath, const RESTfulAPIEndpointFullDefinition &apiEndpointFullDefinition);

    /**
     * @brief Invoke a resource and return the error code.
     *
     * @param httpMethodType The RESTful method httpMethodType (GET, POST, PUT, DELETE).
     * @param endpointPath The name of the resource.
     * @param inputParameters The input parameters for the method.
     * @param currentScopes The set of current scopes for the user.
     * @param authenticated If true, the user is authenticated.
     * @param[out] payloadOut The output payload after invoking the method.
     * @return The error code indicating the result of the method invocation.
     */
    ErrorCodes invokeEndpoint(const HTTPMethodType &httpMethodType, const std::string &endpointPath, RESTful::RequestParameters &inputParameters, const std::set<std::string> &currentScopes, bool isAdmin,
                              const SecurityParameters &securityParameters, APIReturn *payloadOut);

    /**
     * @brief Invoke a resource with a string representation of the method mode and return the error code.
     *
     * @param httpMethodType The string representation of the RESTful method mode (e.g. "GET", "POST", "PUT", "DELETE").
     * @param endpointPath The name of the resource.
     * @param inputParameters The input parameters for the method.
     * @param currentScopes The set of current scopes for the user.
     * @param authenticated If true, the user is authenticated.
     * @param[out] payloadOut The output payload after invoking the method.
     * @return The error code indicating the result of the method invocation.
     */
    ErrorCodes invokeEndpoint(const std::string &httpMethodType, const std::string &endpointPath, RESTful::RequestParameters &inputParameters, const std::set<std::string> &currentScopes, bool isAdmin,
                              const SecurityParameters &securityParameters, APIReturn *payloadOut);

private:
    std::map<std::string, RESTfulAPIEndpointFullDefinition> m_endpointsPATCH;    ///< Map of PATCH endpoints.
    std::map<std::string, RESTfulAPIEndpointFullDefinition> m_endpointsGET;    ///< Map of GET endpoints.
    std::map<std::string, RESTfulAPIEndpointFullDefinition> m_endpointsPOST;   ///< Map of POST endpoints.
    std::map<std::string, RESTfulAPIEndpointFullDefinition> m_endpointsPUT;    ///< Map of PUT endpoints.
    std::map<std::string, RESTfulAPIEndpointFullDefinition> m_endpointsDELETE; ///< Map of DELETE endpoints.

    Threads::Sync::Mutex_Shared m_endpointsMutex; ///< Mutex for protecting access to the maps of endpoints.

    Sessions::ClientDetails extractClientDetails(const RequestParameters &inputParameters);
};

} // namespace RESTful
} // namespace API
} // namespace Mantids30
