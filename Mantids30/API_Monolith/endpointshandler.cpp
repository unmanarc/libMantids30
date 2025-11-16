#include "endpointshandler.h"
#include <Mantids30/Helpers/json.h>
#include <Mantids30/Threads/lock_shared.h>
#include <memory>

using namespace Mantids30::API::Monolith;
using namespace Mantids30;

bool Endpoints::addEndpoint(const EndpointDefinition &endpointDefinition)
{
    // Checks if endpoint with given name does not already exist in endpoints map
    if (m_endpoints.find(endpointDefinition.endpointName) == m_endpoints.end() )
    {
        // Adds endpoint definition to the endpoints map using endpointName as key
        m_endpoints[endpointDefinition.endpointName] = endpointDefinition.endpointFunction;

        // Updates required scopes and roles for the new endpoint
        m_endpointsScopes.addEndpointRequiredScopes(endpointDefinition.endpointName,endpointDefinition.reqScopes);
        m_endpointsScopes.addEndpointRequiredRoles(endpointDefinition.endpointName,endpointDefinition.reqRoles);

        // Sets whether an active session is required for this endpoint and whether to update last activity on usage
        m_endpointRequireActiveSession[endpointDefinition.endpointName] = endpointDefinition.isActiveSessionRequired;
        m_endpointUpdateSessionLastActivityOnUsage[endpointDefinition.endpointName] = endpointDefinition.doUsageUpdateLastSessionActivity;

        return true; // Endpoint added successfully
    }
    return false; // Endpoint with given name already exists, cannot add
}

int Endpoints::invoke(std::shared_ptr<Mantids30::Sessions::Session> session, const std::string & endpointName, const json & payload,  json * payloadOut)
{
    // Checks if endpoint with given name exists in endpoints map
    if (m_endpoints.find(endpointName) == m_endpoints.end())
        return ENDPOINT_RET_CODE_NOTFOUND; // Endpoint not found, return error code
    else
    {
        // Invokes the specified endpoint and stores result in payloadOut
        *payloadOut = m_endpoints[endpointName].endpoint(m_endpoints[endpointName].context, session, payload);

        // If configured, updates the last activity time for the session associated with this invocation
        if ( m_endpointUpdateSessionLastActivityOnUsage[endpointName] )
        {
            session->updateLastActivity();
        }

        return ENDPOINT_RET_CODE_SUCCESS; // Endpoint invoked successfully, return success code
    }
}

Endpoints::eEndpointValidationCodes Endpoints::validateEndpointRequirements(std::shared_ptr<Mantids30::Sessions::Session> session, const std::string & endpointName, json * reasons)
{
    std::set<std::string> scopesLeft, rolesLeft;
    
    // Checks if endpoint with given name exists in endpoints map
    if (m_endpoints.find(endpointName) == m_endpoints.end())
        return VALIDATION_ENDPOINTNOTFOUND; // Endpoint not found, return validation code indicating this

    // Checks if an active session is required for the specified endpoint and validates it
    if (m_endpointRequireActiveSession[endpointName])
    {
        if (!session)
            return VALIDATION_NOTAUTHORIZED; // No session provided but one is required, return unauthorized code
    }

    // Validates whether the session meets the roles and scopes requirements for the endpoint
    if (m_endpointsScopes.validateEndpoint(session,endpointName,rolesLeft,scopesLeft))
    {
        return VALIDATION_OK; // All requirements met, return validation code indicating success
    }
    else
    {
        // Fills reasons with any missing roles and scopes required by the endpoint
        (*reasons)["rolesLeft"] = Helpers::setToJSON(rolesLeft);
        (*reasons)["scopesLeft"] = Helpers::setToJSON(scopesLeft);

        return VALIDATION_NOTAUTHORIZED; // Requirements not met, return unauthorized code
    }

}

EndpointsRequirements_Map *Endpoints::endpointsRequirements()
{
    // Returns a pointer to the endpoints requirements map for external usage or modification
    return &m_endpointsScopes;
}

bool Endpoints::doesAPIEndpointRequireActiveSession(const std::string &endpointName)
{
    // Returns whether the specified endpoint requires an active session or not based on its definition
    return m_endpointRequireActiveSession[endpointName];
}
