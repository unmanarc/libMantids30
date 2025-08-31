#pragma once

#include <map>

#include <Mantids30/Helpers/json.h>
#include <Mantids30/Threads/mutex_shared.h>
#include <memory>
#include <string>
#include "methodsrequirements_map.h"

namespace Mantids30 { namespace API { namespace Monolith {

class MethodsHandler
{
public:
    // Enumerations for method validation and return codes
    enum eMethodValidationCodes
    {
        VALIDATION_OK = 0x0,                  // Method is valid
        VALIDATION_METHODNOTFOUND = 0x1,      // Method not found
        VALIDATION_NOTAUTHORIZED = 0x2       // Not authorized to access the method
    };

    enum StatusCodess {
        METHOD_RET_CODE_SUCCESS = 0,            // Success return code
        METHOD_RET_CODE_UNAUTHENTICATED = -9994,// User is unauthenticated
        METHOD_RET_CODE_INVALIDLOCALAUTH = -9995,// Local authentication failed
        METHOD_RET_CODE_TIMEDOUT = -9996,       // Operation timed out
        METHOD_RET_CODE_INVALIDAUTH = -9997,    // Authentication failed
        METHOD_RET_CODE_SERVERMEMORYFULL = -9998,// Server memory is full
        METHOD_RET_CODE_METHODNOTFOUND = -9999  // Method not found
    };

    struct MonolithAPIMethod
    {
        /**
         * @brief Function pointer to the API method.
         */
        json (*method)(void * context, std::shared_ptr<Mantids30::Sessions::Session> session, const json & parameters);

        /**
         * @brief Context object to pass to the function pointer.
         */
        void * context;
    };

    /**
     * @brief Constructor for MethodsHandler class
     * 
     * Initializes a new instance of MethodsHandler.
     */
    MethodsHandler() = default;

    //////////////////////////////////////////////////

    struct MethodDefinition
    {
        MonolithAPIMethod method;               // API method definition
        std::string methodName;                 // Name of the method
        std::set<std::string> reqScopes;        // Required scopes for the method
        std::set<std::string> reqRoles;         // Required roles for the method
        bool doUsageUpdateLastSessionActivity = true;    // Flag to update last session activity on usage
        bool isActiveSessionRequired = true;     // Flag to check if an active session is required
    };

    /**
     * @brief Add a new API method
     * 
     * Adds a new method definition to the methods handler.
     * 
     * @param methodDefinition MethodDefinition struct containing details about the method
     * @return bool True on success, false on failure
     */
    bool addMethod(const MethodDefinition & methodDefinition);

    /**
     * @brief Invoke an API method
     * 
     * Invokes a specified method with given parameters and returns the result.
     * 
     * @param session Session object for authentication and context
     * @param methodName Name of the method to invoke
     * @param payload JSON payload containing parameters for the method
     * @param payloadOut Pointer to store the output JSON from the method
     * @return int Return code indicating success or failure
     */
    int invoke(std::shared_ptr<Sessions::Session> session, const std::string &methodName, const json &payload, json *payloadOut);

    /**
     * @brief Validate method requirements
     * 
     * Validates if the given method can be accessed based on session scopes and roles.
     * 
     * @param session Session object for authentication and context
     * @param methodName Name of the method to validate
     * @param reasons Pointer to store reasons for validation failure, if any
     * @return eMethodValidationCodes Validation result code
     */
    eMethodValidationCodes validateMethodRequirements(std::shared_ptr<Mantids30::Sessions::Session> session, const std::string & methodName, json * reasons);

    /**
     * @brief Get methods requirements map
     * 
     * Returns the MethodsRequirements_Map object containing method scopes and roles.
     * 
     * @return MethodsRequirements_Map* Pointer to the methods requirements map
     */
    MethodsRequirements_Map * methodsRequirements();

    /**
     * @brief Check if a method requires an active session
     * 
     * Checks if the specified method requires an active user session.
     * 
     * @param methodName Name of the method to check
     * @return bool True if active session is required, false otherwise
     */
    bool doesMethodRequireActiveSession(const std::string & methodName);

   // std::string getApplicationName() const;

private:
    //std::set<std::string> getApplicationScopes(const std::set<std::string> & reqScopes);

    /////////////////////////////////
    // Methods:

    // method name -> method.
    std::map<std::string,MonolithAPIMethod> m_methods;

    // method name -> bool (RequireActiveSession).
    std::map<std::string,bool> m_methodRequireActiveSession;

    // method name -> bool (Update last activity on usage).
    std::map<std::string,bool> m_methodUpdateSessionLastActivityOnUsage;

    //std::string m_applicationName;
    MethodsRequirements_Map m_methodsScopes;

    // lock for methods manipulation...
    Threads::Sync::Mutex_Shared m_methodsMutex;
};

}}}



