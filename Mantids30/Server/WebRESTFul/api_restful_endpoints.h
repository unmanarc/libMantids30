#pragma once

#include "endpoints_options.h"

#include "security.h"
#include "session.h"
#include <Mantids30/DataFormat/JWT/jwt.h>
#include <Mantids30/DataFormat/JSON/json.h>
#include <Mantids30/DataFormat/JSON/streamable_json.h>
#include <Mantids30/Protocol/HTTP/api_return.h>
#include <Mantids30/Protocol/HTTP/httpv1_base.h>
#include <Mantids30/Protocol/HTTP/httpv1_server.h>
#include <Mantids30/Protocol/HTTP/methods.h>
#include <Mantids30/Protocol/HTTP/rsp_status.h>

#include <cstdint>
#include <map>
#include <memory>
#include <set>

namespace Mantids30::Server::WebRESTFul::RESTful {

// Struct to hold HTTP request parameters
struct RequestContext
{
    Mantids30::Protocol::HTTP::HTTPv1_Base::Request *clientRequest = nullptr; ///< Holds all the information from the client request
    boost::json::value emptyJSON;
    boost::json::value *inputJSON = &emptyJSON; ///< Holds the input JSON that came from the request body.
    boost::json::value *explicitInputJSON = nullptr; ///< Embedded call arguments, independent of the HTTP request.

    Mantids30::DataFormat::JWT::Token emptyToken;              ///< Holds a default empty token
    Mantids30::DataFormat::JWT::Token *jwtToken = &emptyToken; ///< Holds JWT token data, if present and validated the pointer will be changed.

    std::shared_ptr<Mantids30::DataFormat::JWT> jwtValidator; ///< Holds the JWT Validator
    std::shared_ptr<Mantids30::DataFormat::JWT> jwtSigner;    ///< Holds the JWT Signer
};

using APIEndpointFunctionType = Mantids30::Protocol::HTTP::APIReturn (*)(void *context,                                        // Context pointer
                                              const Mantids30::Server::WebRESTFul::RESTful::RequestContext &request,            // Parameters from the RESTful request
                                              Mantids30::Server::WebRESTFul::Sessions::ClientDetails &authClientDetails // Client authentication details
);

/**
 * @struct RESTfulAPIDefinition
 *
 * @brief Struct to define the RESTful API method, security, and object pointer.
 */
struct RESTfulAPIEndpointFullDefinition
{
    APIEndpointFunctionType endpointDefinition = nullptr;
    Mantids30::Server::WebRESTFul::Security::Configuration security;
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
    bool addEndpoint(const Mantids30::Protocol::HTTP::Method &httpMethodType, const std::string &endpointPath, const Mantids30::Server::WebRESTFul::Security::Requirements &securityRequirements,
                     const std::set<std::string> &requiredScopes, void *context, APIEndpointFunctionType endpointDefinition);

    /**
     * @brief Add a new resource to the Endpoints with RESTfulAPIDefinition struct.
     *
     * @param httpMethodType The RESTful apiEndpointFullDefinition httpMethodType (GET, POST, PUT, DELETE).
     * @param endpointPath The name of the resource.
     * @param apiEndpointFullDefinition The RESTfulAPIDefinition struct containing apiEndpointFullDefinition, security, and object pointer.
     * @return Returns true if the resource was added successfully, false otherwise.
     */
    bool addEndpoint(const Mantids30::Protocol::HTTP::Method &httpMethodType, const std::string &endpointPath, const RESTfulAPIEndpointFullDefinition &apiEndpointFullDefinition);

    /**
     * @brief Invoke a resource and return the error code.
     *
     * @param httpMethodType The RESTful method httpMethodType (GET, POST, PUT, DELETE).
     * @param endpointPath The name of the resource.
     * @param requestContext The API Request Context (Paramters, JWT, etc)
     * @param currentScopes The set of current scopes for the user.
     * @param authenticated If true, the user is authenticated.
     * @param[out] payloadOut The output payload after invoking the method.
     * @return The error code indicating the result of the method invocation.
     */
    [[nodiscard]] HandleResult handleEndpoint(const Mantids30::Protocol::HTTP::Method &httpMethodType, const std::string &endpointPath, Mantids30::Server::WebRESTFul::RESTful::RequestContext &requestContext,
                                              const std::set<std::string> &currentScopes, bool isAdmin, const Mantids30::Server::WebRESTFul::Security::ReceivedAuth &securityParameters, Mantids30::Protocol::HTTP::APIReturn *apiResponse);

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
    [[nodiscard]] HandleResult handleEndpoint(const std::string &httpMethodType, const std::string &endpointPath, Mantids30::Server::WebRESTFul::RESTful::RequestContext &inputParameters,
                                              const std::set<std::string> &currentScopes, bool isAdmin, const Mantids30::Server::WebRESTFul::Security::ReceivedAuth &securityParameters, Mantids30::Protocol::HTTP::APIReturn *payloadOut);

private:
    std::map<std::string, RESTfulAPIEndpointFullDefinition> m_endpointsPATCH;  ///< Map of PATCH endpoints.
    std::map<std::string, RESTfulAPIEndpointFullDefinition> m_endpointsGET;    ///< Map of GET endpoints.
    std::map<std::string, RESTfulAPIEndpointFullDefinition> m_endpointsPOST;   ///< Map of POST endpoints.
    std::map<std::string, RESTfulAPIEndpointFullDefinition> m_endpointsPUT;    ///< Map of PUT endpoints.
    std::map<std::string, RESTfulAPIEndpointFullDefinition> m_endpointsDELETE; ///< Map of DELETE endpoints.

    Mantids30::Server::WebRESTFul::Sessions::ClientDetails extractClientDetails(const RequestContext &requestContext);
};

} // namespace Mantids30::Server::WebRESTFul::RESTful
