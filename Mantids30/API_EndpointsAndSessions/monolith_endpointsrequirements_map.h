#pragma once
#include <map>
#include <string>
#include <set>
#include "session.h"

namespace Mantids30 {
namespace API {
namespace Monolith {

/**
 * @brief EndpointsRequirements_Map manages and validates access requirements for API endpoints
 *
 * This class provides functionality to define and validate required application scopes and roles
 * for different API endpoints. It acts as a central registry for endpoint access control requirements
 * and provides validation methods to check if a session has the necessary permissions.
 *
 * The class uses multimap structures to store multiple scope and role requirements per endpoint,
 * allowing for flexible access control policies where endpoints may require multiple permissions.
 */
class EndpointsRequirements_Map
{
public:
    /**
     * @brief Default constructor
     *
     * Initializes an empty requirements map with no endpoint access rules defined.
     */
    EndpointsRequirements_Map() = default;

    /**
     * @brief Add required application scopes for a specific endpoint
     *
     * Registers the set of application scopes that are required to access the specified endpoint.
     * Multiple scopes can be required for a single endpoint.
     *
     * @param endpointName The name/identifier of the endpoint
     * @param applicationScopes Set of required application scopes for this endpoint
     */
    void addEndpointRequiredScopes(const std::string &endpointName, const std::set<std::string> &applicationScopes);

    /**
     * @brief Add required application roles for a specific endpoint
     *
     * Registers the set of application roles that are required to access the specified endpoint.
     * Multiple roles can be required for a single endpoint.
     *
     * @param endpointName The name/identifier of the endpoint
     * @param applicationRoles Set of required application roles for this endpoint
     */
    void addEndpointRequiredRoles(const std::string &endpointName, const std::set<std::string> &applicationRoles);

    /**
     * @brief Validate if a session has the required permissions for an endpoint
     *
     * Checks if the provided authentication session has all the required scopes and roles
     * to access the specified endpoint. This method also returns the remaining scopes and roles
     * that were not satisfied, which can be useful for partial access scenarios.
     *
     * @param authSession Shared pointer to the authentication session to validate
     * @param endpointName The name/identifier of the endpoint to validate access for
     * @param rolesLeft Set that will be populated with unsatisfied roles (output parameter)
     * @param scopesLeft Set that will be populated with unsatisfied scopes (output parameter)
     * @return true if all required scopes and roles are satisfied, false otherwise
     */
    bool validateEndpoint(std::shared_ptr<Sessions::Session> authSession, const std::string &endpointName, std::set<std::string> &rolesLeft, std::set<std::string> &scopesLeft);

private:
    /**
     * @brief Retrieve all required scopes for a specific endpoint
     *
     * Helper method to fetch all application scopes required for the given endpoint.
     *
     * @param endpointName The name/identifier of the endpoint
     * @return Set of required scopes for the endpoint
     */
    std::set<std::string> getEndpointRequiredScopes(const std::string & endpointName);

    /**
     * @brief Retrieve all required roles for a specific endpoint
     *
     * Helper method to fetch all application roles required for the given endpoint.
     *
     * @param endpointName The name/identifier of the endpoint
     * @return Set of required roles for the endpoint
     */
    std::set<std::string> getEndpointRequiredRoles(const std::string &endpointName);

    // Endpoint -> App Scope mapping
    // Stores multiple scope requirements per endpoint (using multimap to allow multiple scopes per endpoint)
    std::multimap<std::string,std::string> m_endpointRequiredScopes;

    // Endpoint -> App Role mapping
    // Stores multiple role requirements per endpoint (using multimap to allow multiple roles per endpoint)
    std::multimap<std::string,std::string> m_endpointRequiredRoles;
};

}}}
