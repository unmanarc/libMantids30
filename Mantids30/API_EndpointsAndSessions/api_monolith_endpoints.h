#pragma once

#include <map>
#include <set>

#include "endpoints_options.h"
#include "endpoints_requirements_map.h"
#include <Mantids30/Helpers/json.h>
#include <Mantids30/Threads/mutex_shared.h>
#include <memory>
#include <string>

namespace Mantids30::API::Monolith {

class Endpoints : public Endpoints_Options
{
public:
    // Enumerations for endpoint validation and return codes
    enum class ValidationResult : std::uint8_t
    {
        VALIDATION_OK = 0x0,               // Endpoint is valid
        VALIDATION_ENDPOINTNOTFOUND = 0x1, // Endpoint not found
        VALIDATION_NOTAUTHORIZED = 0x2     // Not authorized to access the endpoint
    };

    enum class StatusCode : std::int16_t
    {
        ENDPOINT_RET_CODE_SUCCESS = 0,              // Success return code
        ENDPOINT_RET_CODE_UNAUTHENTICATED = -9994,  // User is unauthenticated
        ENDPOINT_RET_CODE_INVALIDLOCALAUTH = -9995, // Local authentication failed
        ENDPOINT_RET_CODE_TIMEDOUT = -9996,         // Operation timed out
        ENDPOINT_RET_CODE_INVALIDAUTH = -9997,      // Authentication failed
        ENDPOINT_RET_CODE_SERVERMEMORYFULL = -9998, // Server memory is full
        ENDPOINT_RET_CODE_NOTFOUND = -9999          // Endpoint not found
    };


    struct MonolithAPIEndpointFunction
    {
        /**
         * @brief Function pointer to the API endpoint.
         */
        json (*endpoint)(void *context, std::shared_ptr<Mantids30::Sessions::Session> session, const json &parameters);

        /**
         * @brief Context object to pass to the function pointer.
         */
        void *context;
    };

    /**
     * @brief Constructor for Endpoints class
     * 
     * Initializes a new instance of Endpoints.
     */
    Endpoints() = default;

    //////////////////////////////////////////////////

    struct EndpointDefinition
    {
        MonolithAPIEndpointFunction endpointFunction; // API endpoint definition
        std::string endpointName;                     // Name of the endpoint
        std::set<std::string> reqScopes;              // Required scopes for the endpoint
        std::set<std::string> reqRoles;               // Required roles for the endpoint
        bool doUsageUpdateLastSessionActivity = true; // Flag to update last session activity on usage
        bool isActiveSessionRequired = true;          // Flag to check if an active session is required
    };

    /**
     * @brief Add a new API endpoint
     * 
     * Adds a new endpoint definition to the endpoints handler.
     * 
     * @param endpointDefinition EndpointDefinition struct containing details about the endpoint
     * @return bool True on success, false on failure
     */
    bool addEndpoint(const EndpointDefinition &endpointDefinition);

    /**
     * @brief Invoke an API endpoint
     * 
     * Invokes a specified endpoint with given parameters and returns the result.
     * 
     * @param session Session object for authentication and context
     * @param endpointName Name of the endpoint to invoke
     * @param payload JSON payload containing parameters for the endpoint
     * @param payloadOut Pointer to store the output JSON from the endpoint
     * @return int Return code indicating success or failure
     */
    [[nodiscard]] StatusCode invoke(const std::shared_ptr<Sessions::Session> &session, const std::string &endpointName, const json &payload, json *payloadOut);

    /**
     * @brief Validate endpoint requirements
     * 
     * Validates if the given endpoint can be accessed based on session scopes and roles.
     * 
     * @param session Session object for authentication and context
     * @param endpointName Name of the endpoint to validate
     * @param reasons Pointer to store reasons for validation failure, if any
     * @return ValidationResult Validation result code
     */
    [[nodiscard]] ValidationResult validateEndpointRequirements(const std::shared_ptr<Mantids30::Sessions::Session> &session, const std::string &endpointName, json *reasons);

    /**
     * @brief Get endpoints requirements map
     * 
     * Returns the EndpointsRequirements_Map object containing endpoint scopes and roles.
     * 
     * @return EndpointsRequirements_Map* Pointer to the endpoints requirements map
     */
    [[nodiscard]] EndpointsRequirements_Map *endpointsRequirements();

    /**
     * @brief Check if a endpoint requires an active session
     * 
     * Checks if the specified endpoint requires an active user session.
     * 
     * @param endpointName Name of the endpoint to check
     * @return bool True if active session is required, false otherwise
     */
    [[nodiscard]] bool doesAPIEndpointRequireActiveSession(const std::string &endpointName);

    /**
     * @brief Set CORS/Options configuration for a specific endpoint or globally
     * @param endpointName The endpoint name. If empty, sets global configuration.
     * @param config The OptionsHandlerConfig containing CORS settings.
     */
    void setEndpointOptions(const std::string &endpointName, const OptionsHandlerConfig &config);

    /**
     * @brief Get the OptionsHandlerConfig for a specific endpoint
     * @param endpointName The endpoint name to look up.
     * @return Pointer to the config, or nullptr if OPTIONS is not enabled.
     */
    [[nodiscard]] const OptionsHandlerConfig *getEndpointOptionsConfig(const std::string &endpointName) const;

private:
    /////////////////////////////////
    // Endpoints:

    // endpoint name -> endpoint.
    std::map<std::string, MonolithAPIEndpointFunction> m_endpoints;

    // endpoint name -> bool (RequireActiveSession).
    std::map<std::string, bool> m_endpointRequireActiveSession;

    // endpoint name -> bool (Update last activity on usage).
    std::map<std::string, bool> m_endpointUpdateSessionLastActivityOnUsage;

    //std::string m_applicationName;
    EndpointsRequirements_Map m_endpointsScopes;
};

} // namespace Mantids30::API::Monolith
