#pragma once

#include "endpoints_options.h"

#include "session.h"
#include "security.h"
#include <Mantids30/DataFormat_JWT/jwt.h>
#include <Mantids30/Protocol_HTTP/methods.h>
#include <Mantids30/Helpers/json.h>
#include <Mantids30/Memory/streamable_json.h>
#include <Mantids30/Protocol_HTTP/api_return.h>
#include <Mantids30/Protocol_HTTP/httpv1_base.h>
#include <Mantids30/Protocol_HTTP/httpv1_server.h>
#include <Mantids30/Protocol_HTTP/rsp_status.h>
#include <Mantids30/Threads/mutex_shared.h>
#include <cstdint>
#include <map>
#include <memory>
#include <set>

namespace Mantids30::API::RESTful {

// Struct to hold HTTP request parameters
struct RequestParameters
{
    Mantids30::Network::Protocol::HTTP::HTTPv1_Base::Request *clientRequest = nullptr; ///< Holds all the information from the client request
    Json::Value emptyJSON;
    Json::Value *inputJSON = &emptyJSON; ///< Holds the input JSON that came from the request body.

    DataFormat::JWT::Token emptyToken;              ///< Holds a default empty token
    DataFormat::JWT::Token *jwtToken = &emptyToken; ///< Holds JWT token data, if present and validated the pointer will be changed.

    std::shared_ptr<DataFormat::JWT> jwtValidator; ///< Holds the JWT Validator
    std::shared_ptr<DataFormat::JWT> jwtSigner;    ///< Holds the JWT Signer
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
    APIEndpointFunctionType endpointDefinition = nullptr;
    API::Security::Configuration security;
    void *context = nullptr;
};

/**
 * @class Endpoints
 *
 * @brief Handles the mapping of RESTful API resources to corresponding methods,
 * manages security requirements, and invokes the appropriate methods.
 */
class Endpoints : public Endpoints_Options
{
public:


    /**
     * @enum HandleResult
     *
     * @brief Enumeration for possible error codes.
     */
    enum class HandleResult : std::int8_t
    {
        SUCCESS = 0,
        INVALID_METHOD_MODE = -1,
        RESOURCE_NOT_FOUND = -2,
        AUTHENTICATION_REQUIRED = -3,
        INVALID_SCOPE = -4,
        INTERNAL_ERROR = -5
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
     * @param requireJWTHeaderAuthentication If true, user authentication is required.
     * @param requiredScopes The set of required scopes for the resource.
     * @param context The object pointer for the method.
     * @param endpointDefinition The function pointer to the endpoint definition.
     * @return Returns true if the resource was added successfully, false otherwise.
     */
    bool addEndpoint(const Network::Protocol::HTTP::Method &httpMethodType, const std::string &endpointPath, const API::Security::Requirements &securityRequirements, const std::set<std::string> &requiredScopes, void *context,
                     APIEndpointFunctionType endpointDefinition);

    /**
     * @brief Add a new resource to the Endpoints with RESTfulAPIDefinition struct.
     *
     * @param httpMethodType The RESTful apiEndpointFullDefinition httpMethodType (GET, POST, PUT, DELETE).
     * @param endpointPath The name of the resource.
     * @param apiEndpointFullDefinition The RESTfulAPIDefinition struct containing apiEndpointFullDefinition, security, and object pointer.
     * @return Returns true if the resource was added successfully, false otherwise.
     */
    bool addEndpoint(const Network::Protocol::HTTP::Method &httpMethodType, const std::string &endpointPath, const RESTfulAPIEndpointFullDefinition &apiEndpointFullDefinition);

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
    [[nodiscard]] HandleResult handleEndpoint(const Network::Protocol::HTTP::Method &httpMethodType, const std::string &endpointPath, RESTful::RequestParameters &inputParameters,
                                              const std::set<std::string> &currentScopes, bool isAdmin, const API::Security::ReceivedAuth &securityParameters, APIReturn *apiResponse);

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
    [[nodiscard]] HandleResult handleEndpoint(const std::string &httpMethodType, const std::string &endpointPath, RESTful::RequestParameters &inputParameters,
                                              const std::set<std::string> &currentScopes, bool isAdmin, const API::Security::ReceivedAuth &securityParameters, APIReturn *payloadOut);

private:
    std::map<std::string, RESTfulAPIEndpointFullDefinition> m_endpointsPATCH;  ///< Map of PATCH endpoints.
    std::map<std::string, RESTfulAPIEndpointFullDefinition> m_endpointsGET;    ///< Map of GET endpoints.
    std::map<std::string, RESTfulAPIEndpointFullDefinition> m_endpointsPOST;   ///< Map of POST endpoints.
    std::map<std::string, RESTfulAPIEndpointFullDefinition> m_endpointsPUT;    ///< Map of PUT endpoints.
    std::map<std::string, RESTfulAPIEndpointFullDefinition> m_endpointsDELETE; ///< Map of DELETE endpoints.

    Sessions::ClientDetails extractClientDetails(const RequestParameters &inputParameters);
};

} // namespace Mantids30::API::RESTful
