#pragma once

#include <Mantids30/Protocol_HTTP/api_return.h>
#include <Mantids30/Sessions/session.h>
#include <Mantids30/DataFormat_JWT/jwt.h>
#include <Mantids30/Helpers/json.h>
#include <Mantids30/Memory/streamablejson.h>
#include <Mantids30/Protocol_HTTP/httpv1_base.h>
#include <Mantids30/Protocol_HTTP/httpv1_server.h>
#include <Mantids30/Protocol_HTTP/rsp_status.h>
#include <Mantids30/Threads/mutex_shared.h>
#include <cstdint>
#include <map>
#include <memory>
#include <set>

namespace Mantids30 { namespace API { namespace RESTful {

// Struct to hold HTTP request parameters
struct RequestParameters
{
    Mantids30::Network::Protocols::HTTP::HTTPv1_Base::Request * clientRequest = nullptr; ///< Holds all the information from the client request
    Json::Value pathParameters;     ///< Holds parameters from the URL path
    Json::Value emptyJSON;
    Json::Value * inputJSON = &emptyJSON;     ///< Holds the input JSON that came from the request body.

    DataFormat::JWT::Token emptyToken; ///< Holds a default empty token
    DataFormat::JWT::Token * jwtToken = &emptyToken;    ///< Holds JWT token data, if present and validated the pointer will be changed.

    std::shared_ptr<DataFormat::JWT> jwtValidator; ///< Holds the JWT Validator
    std::shared_ptr<DataFormat::JWT> jwtSigner; ///< Holds the JWT Signer
    //std::multimap<std::string, std::string> cookies;
};


using MethodType = void (*)(
    void *context,                                         // Context pointer
    APIReturn &response,                                   // The API return object
    const RESTful::RequestParameters &request,       // Parameters from the RESTful request
    Mantids30::Sessions::ClientDetails &authClientDetails  // Client authentication details
    );

/**
 * @struct RESTfulAPIDefinition
 *
 * @brief Struct to define the RESTful API method, security, and object pointer.
 */
struct RESTfulAPIDefinition
{
    struct Security {
        bool requireJWTHeaderAuthentication = true;
        bool requireJWTCookieAuthentication = true;
        std::set<std::string> requiredPermissions;
        //bool requireGenericAntiCSRFToken = true;
        //bool requireJWTCookieHash = true;
    };
    MethodType method = nullptr;
    Security security;
    void * context = nullptr;
};

/**
 * @class MethodsHandler
 *
 * @brief Handles the mapping of RESTful API resources to corresponding methods,
 * manages security requirements, and invokes the appropriate methods.
 */
class MethodsHandler
{
public:
    /**
     * @enum MethodMode
     *
     * @brief Enumeration for different RESTful method modes.
     */
    enum MethodMode {
        GET=0,
        POST=1,
        PUT=2,
        DELETE=3
    };

    /**
     * @enum ErrorCodes
     *
     * @brief Enumeration for possible error codes.
     */
    enum ErrorCodes {
        SUCCESS = 0,
        INVALID_METHOD_MODE = -1,
        RESOURCE_NOT_FOUND = -2,
        AUTHENTICATION_REQUIRED = -3,
        INSUFFICIENT_PERMISSIONS = -4,
        INTERNAL_ERROR = -5
    };


    enum SecurityOptions {
        NO_AUTH=0,
        REQUIRE_JWT_HEADER_AUTH=1,
        REQUIRE_JWT_COOKIE_AUTH=2//,
        //REQUIRE_GENERIC_ANTICSRF_TOKEN=4//,
        //REQUIRE_JWT_COOKIE_HASH=8
    };

    struct SecurityParameters
    {
        bool haveJWTAuthHeader = false;
        bool haveJWTAuthCookie = false;
        //std::string genCSRFToken;
    };

    /**
     * @brief Default constructor for MethodsHandler.
     */
    MethodsHandler() = default;

    /**
     * @brief Add a new resource to the MethodsHandler.
     *
     * @param mode The RESTful method mode (GET, POST, PUT, DELETE).
     * @param resourceName The name of the resource.
     * @param method The function pointer to the method.
     * @param context The object pointer for the method.
     * @param requireJWTHeaderAuthentication If true, user authentication is required.
     * @param requiredPermissions The set of required permissions for the resource.
     * @return Returns true if the resource was added successfully, false otherwise.
     */
    bool addResource(const MethodMode &mode,
                     const std::string &resourceName,
                     MethodType method,
                     void *context,
                     const uint32_t &SecurityOptions,
                     const std::set<std::string> requiredPermissions);

    /**
     * @brief Add a new resource to the MethodsHandler with RESTfulAPIDefinition struct.
     *
     * @param mode The RESTful method mode (GET, POST, PUT, DELETE).
     * @param resourceName The name of the resource.
     * @param method The RESTfulAPIDefinition struct containing method, security, and object pointer.
     * @return Returns true if the resource was added successfully, false otherwise.
     */
    bool addResource(const MethodMode & mode, const std::string & resourceName, const RESTfulAPIDefinition & method);

    /**
     * @brief Invoke a resource and return the error code.
     *
     * @param mode The RESTful method mode (GET, POST, PUT, DELETE).
     * @param resourceName The name of the resource.
     * @param inputParameters The input parameters for the method.
     * @param currentPermissions The set of current permissions for the user.
     * @param authenticated If true, the user is authenticated.
     * @param[out] payloadOut The output payload after invoking the method.
     * @return The error code indicating the result of the method invocation.
     */
    ErrorCodes invokeResource(const MethodMode & mode, const std::string & resourceName, RESTful::RequestParameters &inputParameters, const std::set<std::string> &currentPermissions, const SecurityParameters & securityParameters, APIReturn *payloadOut);

    /**
     * @brief Invoke a resource with a string representation of the method mode and return the error code.
     *
     * @param modeStr The string representation of the RESTful method mode (e.g. "GET", "POST", "PUT", "DELETE").
     * @param resourceName The name of the resource.
     * @param inputParameters The input parameters for the method.
     * @param currentPermissions The set of current permissions for the user.
     * @param authenticated If true, the user is authenticated.
     * @param[out] payloadOut The output payload after invoking the method.
     * @return The error code indicating the result of the method invocation.
     */
    ErrorCodes invokeResource(const std::string & modeStr, const std::string & resourceName, RESTful::RequestParameters &inputParameters, const std::set<std::string> &currentPermissions, const SecurityParameters & securityParameters, APIReturn *payloadOut);

private:
    std::map<std::string, RESTfulAPIDefinition> m_methodsGET;    ///< Map of GET resources.
    std::map<std::string, RESTfulAPIDefinition> m_methodsPOST;   ///< Map of POST resources.
    std::map<std::string, RESTfulAPIDefinition> m_methodsPUT;    ///< Map of PUT resources.
    std::map<std::string, RESTfulAPIDefinition> m_methodsDELETE; ///< Map of DELETE resources.

    Threads::Sync::Mutex_Shared m_methodsMutex; ///< Mutex for protecting access to the maps of resources.


    Sessions::ClientDetails extractClientDetails(const RequestParameters &inputParameters);
};

}}}

