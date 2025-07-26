#include "methodshandler.h"
#include <Mantids30/Helpers/json.h>
#include <Mantids30/Threads/lock_shared.h>
#include <memory>

using namespace Mantids30::API::Monolith;
using namespace Mantids30;

bool MethodsHandler::addMethod(const MethodDefinition &methodDefinition)
{
    // Locks the methods mutex to ensure thread-safe modification of methods map
    Threads::Sync::Lock_RW lock(m_methodsMutex);
    
    // Checks if method with given name does not already exist in methods map
    if (m_methods.find(methodDefinition.methodName) == m_methods.end() )
    {
        // Adds method definition to the methods map using methodName as key
        m_methods[methodDefinition.methodName] = methodDefinition.method;

        // Updates required permissions and roles for the new method
        m_methodsPermissions.addMethodRequiredPermissions(methodDefinition.methodName,methodDefinition.reqPermissions);
        m_methodsPermissions.addMethodRequiredRoles(methodDefinition.methodName,methodDefinition.reqRoles);

        // Sets whether an active session is required for this method and whether to update last activity on usage
        m_methodRequireActiveSession[methodDefinition.methodName] = methodDefinition.isActiveSessionRequired;
        m_methodUpdateSessionLastActivityOnUsage[methodDefinition.methodName] = methodDefinition.doUsageUpdateLastSessionActivity;

        return true; // Method added successfully
    }
    return false; // Method with given name already exists, cannot add
}

int MethodsHandler::invoke(std::shared_ptr<Mantids30::Sessions::Session> session, const std::string & methodName, const json & payload,  json * payloadOut)
{
    // Locks the methods mutex in read mode to ensure thread-safe access to methods map
    Threads::Sync::Lock_RD lock(m_methodsMutex);

    // Checks if method with given name exists in methods map
    if (m_methods.find(methodName) == m_methods.end())
        return METHOD_RET_CODE_METHODNOTFOUND; // Method not found, return error code
    else
    {
        // Invokes the specified method and stores result in payloadOut
        *payloadOut = m_methods[methodName].method(m_methods[methodName].context, session, payload);

        // If configured, updates the last activity time for the session associated with this invocation
        if ( m_methodUpdateSessionLastActivityOnUsage[methodName] )
        {
            session->updateLastActivity();
        }

        return METHOD_RET_CODE_SUCCESS; // Method invoked successfully, return success code
    }
}

MethodsHandler::eMethodValidationCodes MethodsHandler::validateMethodRequirements(std::shared_ptr<Mantids30::Sessions::Session> session, const std::string & methodName, json * reasons)
{
    std::set<std::string> permissionsLeft, rolesLeft;
    
    // Locks the methods mutex in read mode to ensure thread-safe access to methods map
    Threads::Sync::Lock_RD lock(m_methodsMutex);

    // Checks if method with given name exists in methods map
    if (m_methods.find(methodName) == m_methods.end())
        return VALIDATION_METHODNOTFOUND; // Method not found, return validation code indicating this

    // Checks if an active session is required for the specified method and validates it
    if (m_methodRequireActiveSession[methodName])
    {
        if (!session)
            return VALIDATION_NOTAUTHORIZED; // No session provided but one is required, return unauthorized code
    }

    // Validates whether the session meets the roles and permissions requirements for the method
    if (m_methodsPermissions.validateMethod(session,methodName,rolesLeft,permissionsLeft))
    {
        return VALIDATION_OK; // All requirements met, return validation code indicating success
    }
    else
    {
        // Fills reasons with any missing roles and permissions required by the method
        (*reasons)["rolesLeft"] = Helpers::setToJSON(rolesLeft);
        (*reasons)["permissionsLeft"] = Helpers::setToJSON(permissionsLeft);

        return VALIDATION_NOTAUTHORIZED; // Requirements not met, return unauthorized code
    }

}

MethodsRequirements_Map *MethodsHandler::methodsRequirements()
{
    // Returns a pointer to the methods requirements map for external usage or modification
    return &m_methodsPermissions;
}

bool MethodsHandler::doesMethodRequireActiveSession(const std::string &methodName)
{
    // Locks the methods mutex in read mode to ensure thread-safe access to methods map
    Threads::Sync::Lock_RD lock(m_methodsMutex);

    // Returns whether the specified method requires an active session or not based on its definition
    return m_methodRequireActiveSession[methodName];
}
