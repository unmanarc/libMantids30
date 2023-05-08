#pragma once

#include <Mantids29/Protocol_HTTP/httpv1_base.h>
#include <map>
#include <memory>
#include <set>
#include <list>
#include <Mantids29/Helpers/json.h>
#include <Mantids29/Threads/mutex_shared.h>
#include <Mantids29/DataFormat_JWT/jwt.h>
#include <Mantids29/Protocol_HTTP/rsp_status.h>
#include <Mantids29/Protocol_HTTP/httpv1_server.h>
#include <Mantids29/Memory/streamablejson.h>

namespace Mantids29 { namespace API { namespace RESTful {

// Struct to hold HTTP request parameters
struct InputParameters
{
    Mantids29::Network::Protocols::HTTP::HTTPv1_Base::Request * clientRequest = nullptr; ///< Holds all the information from the client request
    Json::Value pathParameters;     ///< Holds parameters from the URL path

    DataFormat::JWT::Token emptyToken; ///< Holds a default empty token
    DataFormat::JWT::Token * jwtToken = &emptyToken;    ///< Holds JWT token data, if present and validated the pointer will be changed.

    std::shared_ptr<DataFormat::JWT> jwtValidator; ///< Holds the JWT Validator
    std::shared_ptr<DataFormat::JWT> jwtSigner; ///< Holds the JWT Signer
};

/**
 * @brief Represents the response of an API endpoint.
 */
struct APIReturn {
    /**
     * @brief Default constructor for APIReturn.
     */
    APIReturn() {
    }

    /**
     * @brief Parameterized constructor for APIReturn.
     * @param body The body of the response.
     */
    APIReturn(const json & body) {
        // Initialize the body member variable with the provided json object.
        *this->body = body;
    }    

    /**
     * @brief Assignment operator for APIReturn.
     * @param body The body of the response.
     */
    APIReturn& operator=(const json& body) {
        *this->body = body;
        return *this;
    }

    Network::Protocols::HTTP::Status::eRetCode code = Network::Protocols::HTTP::Status::S_200_OK; ///< HTTP status code of the response.
    std::shared_ptr<Memory::Streams::StreamableJSON> body = std::make_shared<Memory::Streams::StreamableJSON>();
};


/**
 * @struct RESTfulAPIDefinition
 *
 * @brief Struct to define the RESTful API method, security, and object pointer.
 */
struct RESTfulAPIDefinition
{
    struct Security {
        bool requireUserAuthentication = true;
        std::set<std::string> requiredAttributes;
    };

    APIReturn (*method)(void * obj, const RESTful::InputParameters &inputParameters ) = nullptr;
    Security security;

    void * obj = nullptr;
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

    /**
     * @brief Default constructor for MethodsHandler.
     */
    MethodsHandler();

    /**
     * @brief Add a new resource to the MethodsHandler.
     *
     * @param mode The RESTful method mode (GET, POST, PUT, DELETE).
     * @param resourceName The name of the resource.
     * @param method The function pointer to the method.
     * @param obj The object pointer for the method.
     * @param requireUserAuthentication If true, user authentication is required.
     * @param requiredAttributes The set of required attributes for the resource.
     * @return Returns true if the resource was added successfully, false otherwise.
     */
    bool addResource(const MethodMode & mode, const std::string & resourceName,
                     APIReturn (*method)(void * obj, const RESTful::InputParameters &inputParameters ),
                     void * obj,
                     bool requireUserAuthentication,
                     const std::set<std::string> requiredAttributes
                     );

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
     * @param currentAttributes The set of current attributes for the user.
     * @param authenticated If true, the user is authenticated.
     * @param[out] payloadOut The output payload after invoking the method.
     * @return The error code indicating the result of the method invocation.
     */
    ErrorCodes invokeResource(const MethodMode & mode, const std::string & resourceName, const RESTful::InputParameters &inputParameters, const std::set<std::string> &currentAttributes, bool authenticated, APIReturn *payloadOut);

    /**
     * @brief Invoke a resource with a string representation of the method mode and return the error code.
     *
     * @param modeStr The string representation of the RESTful method mode (e.g. "GET", "POST", "PUT", "DELETE").
     * @param resourceName The name of the resource.
     * @param inputParameters The input parameters for the method.
     * @param currentAttributes The set of current attributes for the user.
     * @param authenticated If true, the user is authenticated.
     * @param[out] payloadOut The output payload after invoking the method.
     * @return The error code indicating the result of the method invocation.
     */
    ErrorCodes invokeResource(const std::string & modeStr, const std::string & resourceName, const RESTful::InputParameters &inputParameters, const std::set<std::string> &currentAttributes, bool authenticated, APIReturn *payloadOut);

private:
    std::map<std::string, RESTfulAPIDefinition> m_methodsGET;    ///< Map of GET resources.
    std::map<std::string, RESTfulAPIDefinition> m_methodsPOST;   ///< Map of POST resources.
    std::map<std::string, RESTfulAPIDefinition> m_methodsPUT;    ///< Map of PUT resources.
    std::map<std::string, RESTfulAPIDefinition> m_methodsDELETE; ///< Map of DELETE resources.

    Threads::Sync::Mutex_Shared m_methodsMutex; ///< Mutex for protecting access to the maps of resources.
};

}}}

